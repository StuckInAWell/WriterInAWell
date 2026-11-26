#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <lvgl.h>
#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <WiFi.h>
#include <WebServer.h>
#include <esp_heap_caps.h>
#include <esp_sleep.h>
#include <esp_system.h>
#include <Preferences.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iterator>
#include <string>
#include <vector>

#include "ble_keyboard.h"

namespace {
constexpr int SCREEN_W = 480;
constexpr int SCREEN_H = 320;
constexpr int SD_CS = 41;
constexpr size_t MAX_DOCUMENT_PSRAM = 512 * 1024;
constexpr size_t MAX_DOCUMENT_FALLBACK = 32 * 1024;
// Editor text sizes, smallest first. Terminess ships native bitmap strikes at
// 12/14/16/18/…; odd sizes render smaller, mismatched ink (13 px produced an
// 8 px-tall digit while 12 px produced a 12 px one), so the ladder stays on
// even cuts. 10 px reuses the smallest available strike as the dense option;
// 14 px is the default and the middle of the range.
constexpr int EDITOR_FONT_SIZE_COUNT = 4;
constexpr uint8_t EDITOR_FONT_SIZE_DEFAULT = 2;
constexpr uint16_t EDITOR_FONT_PIXELS[EDITOR_FONT_SIZE_COUNT] = {10, 12, 14, 16};
// The smallest cut has a 12 px line box and therefore the most bands:
// (231 - 2 borders - 6 top pad) / 12 = 18. Every fixed band table is sized
// for that worst case; the live count is editorVisibleLines.
constexpr int MAX_VISIBLE_LINES = 18;
constexpr int EDITOR_CANVAS_H = 229;
constexpr int EDITOR_TEXT_TOP = 6;
// Spaces a Tab keypress inserts. Two matches the project's own house indent;
// hard tabs in an opened file are left untouched (there are no tab stops in
// the label renderer, so they would not line up anyway).
constexpr size_t EDITOR_TAB_SPACES = 2;

// --- Document language -------------------------------------------------------
// Every buffer carries a language: the highlighter, the auto-indent rules and
// the auto-paired brackets all key off it. A file takes its language from its
// extension when it is opened; a new buffer uses the language last picked in
// Settings, so "Untitled" saves with the extension it was started in.
enum class DocLang : uint8_t {
  Markdown = 0,
  PlainText,
  Html,
  Css,
  Python,
  Cpp,
  JavaScript,
  Json,
  Shell,
  Yaml,
};

struct LangEntry {
  const char *label;  // Settings pill and picker row
  const char *ext;    // extension a new buffer of this language saves as
  const char *hint;   // dim extension list in the picker
};

constexpr LangEntry LANG_ENTRIES[] = {
    {"Markdown", ".md", ".md .markdown"},
    {"Plain Text", ".txt", ".txt .log .ini .conf .toml"},
    {"HTML", ".html", ".html .htm .xml .php"},
    {"CSS", ".css", ".css"},
    {"Python", ".py", ".py .pyi"},
    {"C/C++", ".cpp", ".c .h .cpp .hpp .cc .hh"},
    {"JavaScript", ".js", ".js .mjs .ts .tsx"},
    {"JSON", ".json", ".json"},
    {"Shell", ".sh", ".sh .bash .zsh"},
    {"YAML", ".yml", ".yml .yaml"},
};
constexpr size_t LANG_COUNT = sizeof(LANG_ENTRIES) / sizeof(LANG_ENTRIES[0]);
static_assert(LANG_COUNT <= 255, "DocLang is stored in one NVS byte");

const char *langLabel(DocLang lang) {
  const size_t index = static_cast<size_t>(lang);
  return index < LANG_COUNT ? LANG_ENTRIES[index].label : "Plain Text";
}

bool nameEndsWith(const std::string &name, const char *suffix) {
  const size_t n = strlen(suffix);
  if (name.size() < n) return false;
  for (size_t i = 0; i < n; ++i) {
    if (tolower(static_cast<unsigned char>(name[name.size() - n + i])) !=
        suffix[i]) return false;
  }
  return true;
}

// Extension -> language. False means the editor has no grammar for the name,
// which is also what keeps unrelated files out of the browser list.
bool docLangForName(const std::string &name, DocLang &lang) {
  struct Ext {
    const char *ext;
    DocLang lang;
  };
  static const Ext kExts[] = {
      {".md", DocLang::Markdown},     {".markdown", DocLang::Markdown},
      {".txt", DocLang::PlainText},   {".log", DocLang::PlainText},
      {".ini", DocLang::PlainText},   {".conf", DocLang::PlainText},
      {".toml", DocLang::PlainText},  {".html", DocLang::Html},
      {".htm", DocLang::Html},        {".xml", DocLang::Html},
      {".php", DocLang::Html},        {".css", DocLang::Css},
      {".py", DocLang::Python},       {".pyi", DocLang::Python},
      {".c", DocLang::Cpp},           {".h", DocLang::Cpp},
      {".cpp", DocLang::Cpp},         {".hpp", DocLang::Cpp},
      {".cc", DocLang::Cpp},          {".hh", DocLang::Cpp},
      {".cxx", DocLang::Cpp},         {".hxx", DocLang::Cpp},
      {".js", DocLang::JavaScript},   {".mjs", DocLang::JavaScript},
      {".ts", DocLang::JavaScript},   {".tsx", DocLang::JavaScript},
      {".jsx", DocLang::JavaScript},  {".json", DocLang::Json},
      {".sh", DocLang::Shell},        {".bash", DocLang::Shell},
      {".zsh", DocLang::Shell},       {".yml", DocLang::Yaml},
      {".yaml", DocLang::Yaml},
  };
  for (const Ext &entry : kExts) {
    if (nameEndsWith(name, entry.ext)) {
      lang = entry.lang;
      return true;
    }
  }
  return false;
}

// A name the browser may list; anything else on the card stays out of it.
bool isEditableFileName(const String &name) {
  DocLang ignored = DocLang::PlainText;
  return docLangForName(std::string(name.c_str(), name.length()), ignored);
}

// Unknown extensions still open (from recent files, or an upload) as plain
// text rather than being refused.
DocLang docLangOrPlain(const std::string &name) {
  DocLang lang = DocLang::PlainText;
  docLangForName(name, lang);
  return lang;
}

bool psramAvailable = false;

template <typename T>
class DocumentAllocator {
public:
  using value_type = T;
  using is_always_equal = std::true_type;

  DocumentAllocator() noexcept = default;
  template <typename U>
  DocumentAllocator(const DocumentAllocator<U> &) noexcept {}

  T *allocate(size_t count) {
    const uint32_t caps = psramAvailable
        ? (MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
        : MALLOC_CAP_8BIT;
    void *memory = heap_caps_malloc(count * sizeof(T), caps);
    if (!memory) abort();
    return static_cast<T *>(memory);
  }

  void deallocate(T *memory, size_t) noexcept {
    heap_caps_free(memory);
  }

  template <typename U>
  struct rebind { using other = DocumentAllocator<U>; };
};

template <typename T, typename U>
bool operator==(const DocumentAllocator<T> &, const DocumentAllocator<U> &) {
  return true;
}

template <typename T, typename U>
bool operator!=(const DocumentAllocator<T> &, const DocumentAllocator<U> &) {
  return false;
}

using DocumentString =
    std::basic_string<char, std::char_traits<char>, DocumentAllocator<char>>;

class WriterDisplay final : public lgfx::LGFX_Device {
  lgfx::Panel_ST7796 panel;
  lgfx::Bus_Parallel8 bus;
  lgfx::Light_PWM backlight;
  lgfx::Touch_FT5x06 touch;

public:
  WriterDisplay() {
    auto busCfg = bus.config();
    busCfg.freq_write = 40000000;
    busCfg.pin_wr = 47;
    busCfg.pin_rd = -1;
    busCfg.pin_rs = 0;
    busCfg.pin_d0 = 9;
    busCfg.pin_d1 = 46;
    busCfg.pin_d2 = 3;
    busCfg.pin_d3 = 8;
    busCfg.pin_d4 = 18;
    busCfg.pin_d5 = 17;
    busCfg.pin_d6 = 16;
    busCfg.pin_d7 = 15;
    bus.config(busCfg);
    panel.setBus(&bus);

    auto panelCfg = panel.config();
    panelCfg.pin_cs = -1;
    panelCfg.pin_rst = 4;
    panelCfg.panel_width = 320;
    panelCfg.panel_height = 480;
    panelCfg.offset_x = 0;
    panelCfg.offset_y = 0;
    panelCfg.readable = false;
    panelCfg.invert = true;
    panelCfg.rgb_order = false;
    panel.config(panelCfg);

    auto lightCfg = backlight.config();
    lightCfg.pin_bl = 45;
    lightCfg.invert = false;
    lightCfg.freq = 44100;
    lightCfg.pwm_channel = 7;
    backlight.config(lightCfg);
    panel.setLight(&backlight);

    auto touchCfg = touch.config();
    touchCfg.x_min = 0;
    touchCfg.x_max = 319;
    touchCfg.y_min = 0;
    touchCfg.y_max = 479;
    touchCfg.pin_int = 7;
    touchCfg.bus_shared = true;
    touchCfg.offset_rotation = 0;
    touchCfg.i2c_port = 1;
    touchCfg.i2c_addr = 0x38;
    touchCfg.pin_sda = 6;
    touchCfg.pin_scl = 5;
    touchCfg.freq = 400000;
    touch.config(touchCfg);
    panel.setTouch(&touch);
    setPanel(&panel);
  }
};

WriterDisplay display;
lv_disp_draw_buf_t drawBuffer;
lv_color_t *lvBuffer = nullptr;
lv_obj_t *lineLabels[MAX_VISIBLE_LINES] = {};
// Line-number gutter labels; all-null means the gutter is off.
lv_obj_t *gutterLabels[MAX_VISIBLE_LINES] = {};
// Last paint state of each band. updateEditorLines() runs on every keypress and
// every lv_obj_set_style_*() call invalidates the object, so the renderer
// compares against this table and only writes what changed. A caret move then
// repaints two rows instead of the whole page, and a keystroke stops
// reallocating 18 label buffers.
struct BandPaint {
  uint32_t ink;        // base ink under the inline spans
  uint32_t bg;         // band background
  uint32_t gutterInk;  // gutter text colour
  const lv_font_t *font;
};
constexpr uint32_t PAINT_UNSET = 0xFFFFFFFFu;
BandPaint bandPaint[MAX_VISIBLE_LINES];
// Cached caret block geometry, so a render that does not move the caret does
// not invalidate the block (or the canvas) at all.
int cursorBlockX = -1;
int cursorBlockY = -1;
int cursorBlockW = -1;
int cursorBlockH = -1;
bool cursorBlockShown = false;
// The caret blinks in two looks of the same cell: a solid light-blue block
// with the character reversed out of it, then a 1 px outline of that same
// cell so the character underneath stays readable. The glyph state below
// describes the character currently lent to the solid phase; end of line (or
// a hard tab) has none, and the solid phase is then a plain block.
constexpr uint32_t CURSOR_BLUE = 0x89DCEB;
bool cursorSolidPhase = true;
bool cursorGlyphHasText = false;
int cursorGlyphW = -1;
uint32_t cursorGlyphInk = PAINT_UNSET;

// Forget the per-band paint state. Called whenever the screen is rebuilt,
// because the labels (and every style on them) are new objects then.
void resetEditorPaint() {
  for (int i = 0; i < MAX_VISIBLE_LINES; ++i) {
    bandPaint[i].ink = PAINT_UNSET;
    bandPaint[i].bg = PAINT_UNSET;
    bandPaint[i].gutterInk = PAINT_UNSET;
    bandPaint[i].font = nullptr;
  }
  cursorBlockX = cursorBlockY = cursorBlockW = cursorBlockH = -1;
  cursorBlockShown = false;
  cursorSolidPhase = true;
  cursorGlyphHasText = false;
  cursorGlyphW = -1;
  cursorGlyphInk = PAINT_UNSET;
}
lv_obj_t *modeLabel = nullptr;
lv_obj_t *statusLabel = nullptr;
lv_obj_t *footerLabel = nullptr;
lv_obj_t *hintLabel = nullptr;
// Keycap chips + captions in the editor's bottom hint row. updateStatusBar()
// toggles them per mode; the flex row skips hidden children, so it reflows.
lv_obj_t *hintRow = nullptr;
lv_obj_t *hintKeyI = nullptr;
lv_obj_t *hintWordInput = nullptr;
lv_obj_t *hintKeyV = nullptr;
lv_obj_t *hintWordVisual = nullptr;
lv_obj_t *hintKeyEsc = nullptr;
lv_obj_t *hintWordNormal = nullptr;
lv_obj_t *hintKeyColon = nullptr;
lv_obj_t *hintWordCommands = nullptr;
lv_obj_t *hintKeyQuestion = nullptr;
lv_obj_t *hintWordHelp = nullptr;
// Menu-shortcut chips right-aligned in the same hint strip: same keycap
// shape, blue outline instead of flat grey, so the two app-menu
// destinations stand apart from the mode-switch keys. No captions — the
// command names say where they lead.
lv_obj_t *hintKeyMenu = nullptr;
lv_obj_t *hintKeySettings = nullptr;
lv_obj_t *documentLabel = nullptr;
lv_obj_t *pathLabel = nullptr;
lv_obj_t *radioLabel = nullptr;
lv_obj_t *powerLabel = nullptr;
lv_obj_t *settingsRadioLabel = nullptr;
lv_obj_t *brightnessStatusLabel = nullptr;
lv_obj_t *brightnessSlider = nullptr;
lv_obj_t *smoothSwitch = nullptr;
lv_obj_t *markdownSwitch = nullptr;
lv_obj_t *autoSaveSwitch = nullptr;
lv_obj_t *dimSwitch = nullptr;
lv_obj_t *lineNumberSwitch = nullptr;
lv_obj_t *wordWrapSwitch = nullptr;
lv_obj_t *pairingStatusLabel = nullptr;
lv_obj_t *menuRadioLabel = nullptr;
// Wi-Fi screen value labels, updated in place while typing so a keystroke does
// not rebuild the whole screen.
lv_obj_t *wifiNameValueLabel = nullptr;
lv_obj_t *wifiPasswordValueLabel = nullptr;
lv_obj_t *browserPanel = nullptr;
lv_obj_t *cursorBlock = nullptr;
// Single-character label drawn above the solid caret block in the band's own
// background colour: that is the reversed-out character of the solid phase.
lv_obj_t *cursorGlyph = nullptr;
// Zen mode's two-second "esc to exit" toast; hidden by loop() after the hint.
lv_obj_t *zenHintLabel = nullptr;
uint32_t zenHintHideMs = 0;
// Two-finger tap exits Zen. The latch swallows the rest of the gesture so the
// rebuilt screen never receives a stray press from the same fingers.
bool twoFingerLatch = false;
uint32_t twoFingerLatchUntilMs = 0;
bool zenExitRequested = false;
bool zenEntryRequested = false;

void zenHintClicked(lv_event_t *) { zenExitRequested = true; }
WebServer transferServer(80);
File uploadFile;

DocumentString document = "# Untitled\n\n";
std::string fileName = "Untitled.md";
std::string currentPath;
std::string commandLine;
DocumentString yankBuffer;
DocumentString undoBuffer;
// Redo mirrors undo: one whole-document snapshot reserved at boot. The flags
// exist because a snapshot of an empty document is legitimately empty, so
// checking buffer.size() would wrongly disable undo/redo after deleting all.
DocumentString redoBuffer;
bool undoAvailable = false;
bool redoAvailable = false;
// Line-start index: byte offset of the first character of every document line,
// plus the trailing empty line after a final '\n'. Rebuilt lazily once per
// document change, so the per-render line math is a binary search instead of
// up to 18 full prefix scans. uint32_t is enough for the 512 KiB document cap.
// PSRAM-backed like the document itself.
std::vector<uint32_t, DocumentAllocator<uint32_t>> lineStarts;
bool lineIndexDirty = true;
// Content stamp: bumped on every document change so caches keyed on "the
// document has not moved since" (syntax state, rendered bands) can invalidate.
uint32_t documentEditStamp = 1;
// Last "/" pattern, kept in RAM for n/N repeat (bounded like commandLine).
std::string lastSearch;
// Word-count cache for the footer: invalidated by edits and recomputed at most
// twice a second, so typing never rescans a 512 KiB draft per keystroke.
size_t documentWordCache = 0;
uint32_t documentWordCacheMs = 0;
bool documentWordCacheValid = false;
// Word count when the current document was opened; the footer shows the net
// gain since then as a session count.
size_t sessionWordBase = 0;
std::string statusMessage = "Ready — press i to write";
size_t documentLimit = MAX_DOCUMENT_FALLBACK;
// Hotspot SSID. IEEE 802.11 caps an SSID at 32 bytes; the extra byte is the NUL.
char wifiSsid[33] = "WriterInAWell";
// Hotspot passphrase. WPA2 accepts 8..63 chars; seeded from the eFuse MAC on
// first boot, then editable and persisted.
char wifiApPassword[64] = "";
String uploadDestination;
size_t cursor = 0;
size_t visualAnchor = 0;
size_t viewportStart = 0;
// Wrap mode: column within the viewportStart line where the top band begins
// (always 0 or a soft-wrap boundary). No-wrap mode keeps it at 0.
size_t viewportWrapCol = 0;
// No-wrap mode: columns scrolled out to the left. Wrap mode keeps it at 0.
size_t viewportScrollCol = 0;
size_t browserSelection = 0;
// Visible browser rows and the first match index they show. An arrow key that
// stays inside the window retints two rows instead of rebuilding the screen.
lv_obj_t *browserRowButtons[10] = {};
size_t browserRowFirst = SIZE_MAX;
size_t menuSelection = 0;
// The four menu tiles, so an arrow-key move retints two tiles instead of
// rebuilding the screen (four tiles, their labels, the hint row and a full
// repaint) on every key.
lv_obj_t *menuTileButtons[4] = {};
size_t pairSelection = 0;
size_t pairListRenderedCount = SIZE_MAX;
uint32_t lastEditMs = 0;
uint32_t lastInputMs = 0;
uint32_t lastRadioUpdateMs = 0;
uint32_t lastTouchMs = 0;
uint32_t lastCursorBlinkMs = 0;
uint16_t lastTouchX = 0;
uint16_t lastTouchY = 0;
bool touchWasDown = false;
bool cardAvailable = false;
bool dirty = false;
bool insertMode = false;
bool visualMode = false;
bool commandMode = false;
bool searchMode = false;
bool backlightDimmed = false;
// Screen visibility: the main menu is the boot screen. The editor's start
// screen (a LazyVim-style dashboard) shows whenever no document is open.
bool editorVisible = false;
bool pairingVisible = false;
bool settingsVisible = false;
bool menuVisible = true;
bool startVisible = false;
bool helpVisible = false;
bool typingVisible = false;
bool typingSettingsVisible = false;
bool previewVisible = false;
bool wifiScreenVisible = false;
// Version history (:history): a simple list screen, newest first.
bool historyVisible = false;
size_t historySelection = 0;
std::vector<String> historyFiles;
// Where a sub-screen's BACK button (and Esc) returns to. The main menu is the
// root, so a screen opened from it must not silently drop into the editor.
enum class BackTarget : uint8_t { Menu, Editor, Start };
BackTarget backTarget = BackTarget::Menu;
// True once a document is created or opened. False makes "Text Editor" open
// the start screen instead of an empty buffer, matching Neovim's dashboard.
bool hasDocument = false;
bool previewTruncated = false;  // block table hit its cap mid-document
int previewScrollY = 0;
int previewTotalHeight = 0;     // sum of block heights, clamps scrolling
size_t previewBlockCount = 0;

// Rendered preview (:p): one PreviewBlock per source line, in a PSRAM table
// allocated once at boot so :p never allocates from a key handler. 16384
// blocks x 8 B = 128 KiB covers full-length docs unless every line is tiny;
// past the cap the view stops and the header says (TRUNCATED).
constexpr size_t PREVIEW_MAX_BLOCKS = 16384;
constexpr int PREVIEW_VIEW_H = 240;      // matches the preview panel height
constexpr size_t PREVIEW_LINE_CAP = 200; // ~3 wrapped lines per block max
constexpr int PREVIEW_POOL_LABELS = 18; // 240 px / 19 px min block + margin
enum PreviewBlockType : uint8_t {
  PB_BLANK, PB_PARA, PB_HEAD, PB_QUOTE, PB_CODE, PB_LIST
};
struct PreviewBlock {
  uint32_t offset;  // document offset of the source line
  uint16_t height;  // rendered slot height in px, spacing included
  uint8_t type;
};
PreviewBlock *previewBlocks = nullptr;
lv_obj_t *previewPool[PREVIEW_POOL_LABELS] = {};
bool pairingReturnsToSettings = false;
bool directoryReady = false;
bool transferEnabled = false;
// Which Wi-Fi field the keyboard is editing; None otherwise.
enum class WifiField : uint8_t { None, Name, Password };
WifiField wifiEditingField = WifiField::None;
bool uploadFailed = false;
bool uiNeedsRender = true;
bool undoGroupActive = false;
bool showingTouchCoords = false;
uint8_t brightnessValue = 170;
constexpr uint8_t BRIGHTNESS_MIN = 24;
constexpr uint8_t BRIGHTNESS_MAX = 255;
constexpr uint8_t BRIGHTNESS_PRESETS[] = {64, 112, 170, 230};
bool editorSmoothText = false;
bool settingsDimIdle = true;
bool editorHighlight = true;
bool editorAutoSave = true;
bool editorLineNumbers = false;
bool editorWordWrap = true;
// Phase 2 additions, all opt-in so existing behavior is unchanged.
bool editorWordCount = false;        // footer shows words instead of line:col
bool editorRelativeNumbers = false;  // gutter: current line absolute, rest delta
bool editorTypewriter = false;       // keep the caret's band vertically centred
bool editorZenMode = false;          // hide header/status/hints, expand the page
bool editorSearchWrap = false;       // "n"/"N" wrap around the document
bool bootIntoEditor = false;         // resume the last file at power-on
bool editorListContinue = true;      // Enter continues Markdown list markers
bool editorProseTypo = false;        // curly quotes, em dash, ellipsis in prose
// Phase 4: version snapshots and idle power. All opt-in.
bool snapshotsEnabled = false;       // keep a copy of the previous save
uint8_t snapshotRetention = 3;       // versions kept per file: 3, 5 or 10
uint8_t screenOffMinutes = 0;        // 0 = never, else 2/5/10/15
bool lightSleepEnabled = false;      // timer-wake light sleep after long idle
bool screenOff = false;              // backlight currently at zero
// Waking from a blank screen: the tap that restores the backlight must not
// press anything, so the backlight ramps up over WAKE_FADE_MS (with the panel
// still dark at the start) and touches are swallowed until the ramp ends.
constexpr uint32_t WAKE_FADE_MS = 1000;
bool wakeFadeActive = false;
bool wakeGestureLatch = false;  // waking finger still down after the ramp
uint32_t wakeFadeStartMs = 0;
// Last BLE keyboard address, loaded at boot for one auto-reconnect attempt.
char bootKeyboardAddress[20] = {};
bool keyboardWasConnected = false;
// --- Display lock -----------------------------------------------------------
// The lock screen is a UI gate, not disk encryption: files on the SD card are
// still readable with a card reader. The password itself is never stored, only
// a salted 64-bit FNV-1a hash in NVS, which keeps someone who dumps the flash
// from reading the literal password.
enum class LockMode : uint8_t { Unlock, VerifyOld, EnterNew, ConfirmNew };
constexpr size_t LOCK_MIN_PASSWORD = 4;
// Fixed buffers: the login field, the keypad and the hash loop all agree on
// this cap, so no path can grow a password beyond what NVS and the UI expect.
constexpr size_t LOCK_MAX_PASSWORD = 24;
constexpr size_t LOCK_KEY_COUNT = 40;  // 4 rows of 10 keys
bool lockEnabled = false;       // a password hash is stored in NVS
bool lockActive = false;        // the login screen owns keyboard/touch input
bool lockReturnToSettings = false;  // cancel/success returns to Settings
bool lockVerifyForRemove = false;   // VerifyOld is removing, not changing
bool lockShift = false;         // one-shot SHIFT state for the touch keypad
bool lockSymbols = false;       // SYM layout instead of letters
char lockEntry[LOCK_MAX_PASSWORD + 1] = {};
char lockPending[LOCK_MAX_PASSWORD + 1] = {};  // first entry of a new password
uint32_t lockSalt = 0;
uint32_t lockHashLo = 0;
uint32_t lockHashHi = 0;
LockMode lockMode = LockMode::Unlock;
// Where a successful Unlock lands. Manual locks from the editor return to the
// page; boot, sleep and Settings lock-now return to the menu.
enum class LockReturn : uint8_t { Menu, Editor };
LockReturn lockReturnTarget = LockReturn::Menu;
lv_obj_t *lockFieldLabel = nullptr;
lv_obj_t *lockHintLabel = nullptr;
lv_obj_t *lockMessageLabel = nullptr;
lv_obj_t *lockKeys[LOCK_KEY_COUNT] = {};
lv_obj_t *lockKeyLabels[LOCK_KEY_COUNT] = {};
// Snapshots live in a hidden subfolder so the browser, the Wi-Fi page and
// recoverSaveBackups() (root-only) never list or touch them.
constexpr const char *SNAPSHOT_DIR = "/WriterInAWell/.wiaw-versions";
// Index into EDITOR_FONT_PIXELS, restored from NVS ("esize"); 14 px default.
uint8_t editorFontSizeIndex = EDITOR_FONT_SIZE_DEFAULT;
// Live editor geometry, refreshed from the selected font in
// refreshEditorMetrics(). Fixed arrays use MAX_VISIBLE_LINES instead.
int editorLineHeight = 14;
int editorVisibleLines = 15;
// Language of the current buffer, and the default for the next new one.
DocLang editorFileType = DocLang::Markdown;
// Settings is a short section list plus popup panels (the File Type picker
// pattern): None shows the list, anything else is the open panel. A popup
// opened from another popup (File Type from Files & Data) remembers its parent
// in settingsReturn so Esc walks back one level instead of closing everything.
enum class SettingsSection : uint8_t {
  None, Display, Editor, Writing, Files, Power, Keyboard, Security, FileType
};
SettingsSection settingsPopup = SettingsSection::None;
SettingsSection settingsReturn = SettingsSection::None;
uint8_t settingsSectionSelection = 0;
uint8_t settingsPopupSelection = 0;
// Switch widgets in the open popup, indexed by row, so a keyboard-driven
// toggle can sync the control without rebuilding the screen mid-event.
constexpr size_t SETTINGS_POPUP_SWITCH_MAX = 8;
lv_obj_t *settingsPopupSwitches[SETTINGS_POPUP_SWITCH_MAX] = {};
// The popup layer, so a selection move can rebuild just it instead of the
// whole settings screen underneath.
lv_obj_t *settingsPopupPanel = nullptr;
size_t settingsPopupSwitchCount = 0;
Preferences devicePrefs;
bool prefsDirty = false;
char pendingOperator = 0;
std::vector<String> markdownFiles;
// Recently opened documents, newest first, persisted in NVS under "recent" so
// the start screen can offer them after a reboot. Entries are full SD paths.
constexpr size_t RECENT_MAX = 5;
std::vector<std::string> recentFiles;
// Cursor offset saved with each recent path, so reopening a draft resumes
// where writing stopped. Aligned 1:1 with recentFiles.
std::vector<size_t> recentCursors;
size_t recentSelection = 0;
// Browser list as displayed: browserMatches holds indices into markdownFiles
// (every index while the filter is empty). Indices rather than copied names
// keep a large SD directory from doubling the heap. `f` arms filter mode so
// the next keystrokes narrow the list.
bool browserFilterMode = false;
std::string browserFilter;
std::vector<size_t> browserMatches;

const String &browserFileName(size_t index) {
  return markdownFiles[browserMatches[index]];
}

// --- Typing practice: a monkeytype-style word sprint -------------------------
// View-only like the help sheet (never touches the document, so the dirty
// guard does not apply). All state is static so the keystroke path never
// allocates; the recolor paragraph is rebuilt into one fixed buffer.
constexpr uint8_t TYPING_LENGTHS[] = {20, 0};  // 0 = fill the word area
constexpr size_t TYPING_LENGTH_COUNT = 2;
constexpr size_t TYPING_DEFAULT_LENGTH = 0;    // Short
const char *const TYPING_LENGTH_NAMES[] = {"Short", "Long"};
// Longest token is a 12-letter word inside a pair of brackets plus a
// sentence mark: 1 + 12 + 1 + 1 = 15 characters. Twenty bytes leave room for
// future decoration, and the typed slot adds eight mistyped extras so the
// red overflow run stays bounded.
constexpr size_t TYPING_MAX_WORD = 16;
constexpr size_t TYPING_EXTRA_CHARS = 8;
constexpr size_t TYPING_TYPED_CAP = TYPING_MAX_WORD + TYPING_EXTRA_CHARS;
constexpr size_t TYPING_WORD_STORE = TYPING_MAX_WORD + 4;
// Long is resolved at pick time: the paragraph is generated first, then
// words are added to the measure string until the next one would need a
// tenth band. The wrapped text itself is capped near 300 characters, so 200
// slots cover every complexity/symbol combination.
constexpr size_t TYPING_MAX_WORDS = 200;
constexpr size_t TYPING_TEXT_CAP = 2048;
// Word area geometry, shared by the layout and the Long fit check: the words
// start under the stats bar and run to the panel's bottom edge, which leaves
// nine 20 px Terminess bands.
constexpr int TYPING_PANEL_H = 240;
constexpr int TYPING_WORD_AREA_W = 452;
constexpr int TYPING_WORDS_Y = 44;
constexpr int TYPING_WORD_AREA_H = TYPING_PANEL_H - TYPING_WORDS_Y - 8;
// Stats bar: one 24 px live row, or two 18 px summary rows in a 40 px bar.
// Both sit above TYPING_WORDS_Y, so the word stream never moves.
constexpr int TYPING_STAT_BAR_W = 460;
constexpr int TYPING_STAT_LIVE_H = 24;
constexpr int TYPING_STAT_LIVE_CELL_H = TYPING_STAT_LIVE_H - 2;
constexpr int TYPING_STAT_SUMMARY_H = 40;
// Two rows inside 40 px minus the 1 px frame and 1 px row gap.
constexpr int TYPING_STAT_SUMMARY_CELL_H = (TYPING_STAT_SUMMARY_H - 3) / 2;
constexpr uint32_t TYPING_STAT_BG_BLUE = 0x1A2433;
constexpr uint32_t TYPING_STAT_BG_GREEN = 0x1B2821;
constexpr uint32_t TYPING_STAT_BG_SLATE = 0x1E1E2E;
constexpr uint32_t TYPING_STAT_BG_AMBER = 0x2A241A;
constexpr uint32_t TYPING_STAT_BLUE = 0x89B4FA;
constexpr uint32_t TYPING_STAT_GREEN = 0x96A57E;
constexpr uint32_t TYPING_STAT_SLATE = 0xCDD6F4;
constexpr uint32_t TYPING_STAT_AMBER = 0xFAB387;
constexpr uint32_t TYPING_STAT_CAPTION = 0x9399B2;
// Live wpm/elapsed repaint cadence; unchanged cells are not re-set, so a tick
// only touches the numbers that moved.
constexpr uint32_t TYPING_LIVE_TICK_MS = 250;
// 200 most common English words, lowercase, at most 6 letters each. Keep
// entries duplicate-free so the random draw stays uniform.
const char *const TYPING_WORDS[] = {
    "the", "be", "to", "of", "and", "a", "in", "that", "have", "it",
    "for", "not", "on", "with", "he", "as", "you", "do", "at", "this",
    "but", "his", "by", "from", "they", "we", "say", "her", "she", "or",
    "an", "will", "my", "one", "all", "would", "there", "their", "what", "so",
    "up", "out", "if", "about", "who", "get", "which", "go", "me", "when",
    "make", "can", "like", "time", "no", "just", "him", "know", "take", "people",
    "into", "year", "your", "good", "some", "could", "them", "see", "other", "than",
    "then", "now", "look", "only", "come", "its", "over", "think", "also", "back",
    "after", "use", "two", "how", "our", "work", "first", "well", "way", "even",
    "new", "want", "any", "these", "give", "day", "most", "us", "is", "are",
    "was", "were", "been", "has", "had", "did", "does", "being", "where", "why",
    "again", "off", "down", "same", "old", "right", "big", "high", "small", "large",
    "next", "early", "young", "few", "own", "under", "never", "always", "world", "hand",
    "part", "place", "case", "point", "fact", "week", "end", "side", "night", "home",
    "head", "book", "word", "line", "page", "read", "write", "study", "learn", "speak",
    "tell", "talk", "turn", "start", "stop", "move", "open", "close", "begin", "keep",
    "let", "put", "seem", "help", "show", "play", "run", "walk", "fast", "slow",
    "quick", "long", "short", "true", "real", "sure", "warm", "cold", "full", "free",
    "clear", "clean", "mind", "heart", "light", "dark", "water", "music", "paper", "money",
    "food", "drink", "room", "door", "table", "chair", "city", "town", "road", "friend",
};
constexpr size_t TYPING_WORD_POOL =
    sizeof(TYPING_WORDS) / sizeof(TYPING_WORDS[0]);

// Longer, lower-frequency words for the Hard complexity tier. All 7-12
// letters, lowercase, and duplicate-free. None contain '#', which the
// recolor span parser reserves as its escape character.
const char *const TYPING_HARD_WORDS[] = {
    "ability", "absence", "account", "achieve", "acquire", "address",
    "advance", "airport", "already", "ancient", "another", "anxiety",
    "apology", "arrange", "article", "attempt", "attract", "average",
    "balance", "barrier", "battery", "because", "bedroom", "believe",
    "benefit", "between", "bicycle", "brother", "cabinet",
    "apparent", "category", "computer", "continue", "decision", "distance",
    "document", "economic", "evidence", "exercise", "familiar", "function",
    "hospital", "identify", "language", "material", "movement", "personal",
    "positive", "practice",
    "beautiful", "challenge", "character", "community", "condition",
    "different", "education", "effective", "equipment", "establish",
    "financial", "knowledge", "necessary", "objective", "political",
    "potential", "practical", "president", "principle", "procedure",
    "professor", "recognize", "religious", "represent", "secretary",
    "situation", "statement", "structure", "technical", "treatment",
    "advantage", "agreement", "attention",
    "university", "technology", "experience", "particular", "population",
    "production", "protection", "successful", "throughout", "commercial",
    "difficulty", "employment", "friendship", "generation", "importance",
    "industrial", "management", "understand", "completely", "conclusion",
    "connection", "consistent", "continuous", "contribute", "definition",
    "information", "environment", "opportunity", "performance",
    "significant", "temperature", "traditional", "description",
    "manufacture", "consequence", "concentrate", "association",
    "professional", "contribution", "architecture", "circumstance",
};
constexpr size_t TYPING_HARD_POOL =
    sizeof(TYPING_HARD_WORDS) / sizeof(TYPING_HARD_WORDS[0]);

// Curated decoration sources. Contractions and compounds are already spelled
// the way English spells them, so the apostrophe or hyphen lands where it
// belongs instead of being sprinkled onto an arbitrary word. Acronyms are
// words that are genuinely upper case. All entries are duplicate-free and
// free of '#', which the recolor span parser reserves as its escape char.
const char *const TYPING_CONTRACTIONS[] = {
    "it's", "he's", "she's", "that's", "what's", "here's", "there's",
    "let's", "don't", "can't", "won't", "isn't", "aren't", "wasn't",
    "weren't", "didn't", "doesn't", "hasn't", "haven't", "couldn't",
    "wouldn't", "shouldn't", "we're", "you're", "they're", "we've",
    "you've", "they've", "we'll", "you'll", "they'll", "we'd", "you'd",
    "they'd",
};
constexpr size_t TYPING_CONTRACTION_POOL =
    sizeof(TYPING_CONTRACTIONS) / sizeof(TYPING_CONTRACTIONS[0]);
// Words a comma may legitimately follow mid-clause: introductory and
// conjunctive adverbs ("later,", "however,", "instead,"). Using a curated
// set instead of a random word keeps comma placement idiomatic.
const char *const TYPING_COMMA_WORDS[] = {
    "however", "also", "then", "later", "today", "perhaps", "instead",
    "finally", "usually", "often", "still", "meanwhile", "therefore",
    "otherwise", "next", "suddenly", "clearly",
};
constexpr size_t TYPING_COMMA_POOL =
    sizeof(TYPING_COMMA_WORDS) / sizeof(TYPING_COMMA_WORDS[0]);
const char *const TYPING_COMPOUNDS[] = {
    "well-known", "long-term", "short-term", "up-to-date", "first-hand",
    "part-time", "full-time", "high-level", "low-cost", "real-world",
    "self-made", "open-source", "two-way", "cross-check", "front-end",
    "back-end", "top-level", "end-user", "in-depth", "hands-on", "one-off",
    "day-to-day",
};
constexpr size_t TYPING_COMPOUND_POOL =
    sizeof(TYPING_COMPOUNDS) / sizeof(TYPING_COMPOUNDS[0]);
const char *const TYPING_ACRONYMS[] = {
    "API", "CPU", "USB", "SDK", "RAM", "SSD", "URL", "PDF", "HTML",
    "HTTP", "JSON", "TLS", "FTP", "SSH", "GPIO", "NVS",
};
constexpr size_t TYPING_ACRONYM_POOL =
    sizeof(TYPING_ACRONYMS) / sizeof(TYPING_ACRONYMS[0]);

// Word-complexity tiers and settings. Easy keeps the common bank to four
// letters, Normal uses the whole common bank, Hard draws from the longer
// bank. Max length caps the base word on top; when a combination leaves no
// candidate the draw falls back to the unfiltered pool.
enum TypingComplexity : uint8_t { TYPING_EASY, TYPING_NORMAL, TYPING_HARD };
constexpr size_t TYPING_COMPLEXITY_COUNT = 3;
const char *const TYPING_COMPLEXITY_NAMES[] = {"Easy", "Normal", "Hard"};
constexpr uint8_t TYPING_MAX_LENGTHS[] = {0, 8, 10, 12};  // 0 = any
constexpr size_t TYPING_MAX_LENGTH_COUNT = 4;
const char *const TYPING_MAX_LENGTH_NAMES[] = {"Any", "8", "10", "12"};

bool typingRunActive = false;   // false while the end-of-run summary shows
size_t typingLengthIndex = TYPING_DEFAULT_LENGTH;
size_t typingWordCount = 0;
// Generated word text. Decoration and sentence punctuation are baked into
// these fixed slots, so the keystroke path never formats or allocates.
char typingWords[TYPING_MAX_WORDS][TYPING_WORD_STORE] = {};
char typingTyped[TYPING_MAX_WORDS][TYPING_TYPED_CAP] = {};
uint8_t typingTypedLen[TYPING_MAX_WORDS] = {};
bool typingWordWrong[TYPING_MAX_WORDS] = {};
size_t typingCurrentWord = 0;
uint32_t typingStartMs = 0;      // arms on the first counted keystroke
uint32_t typingLastTickMs = 0;
uint32_t typingKeystrokes = 0;   // every counted keypress, errors included
uint32_t typingCorrectKeys = 0; // keystrokes that hit the target letter
size_t typingCorrectWords = 0;
uint16_t typingBestWpm[TYPING_LENGTH_COUNT] = {};
// Persisted practice rules (NVS: tcomp/tmax/tsym/tnum/tcap/tpunc).
uint8_t typingComplexity = TYPING_NORMAL;
uint8_t typingMaxLengthIndex = 0;  // Any
bool typingSymbols = false;
bool typingNumbers = false;
bool typingCapitalization = false;
bool typingPunctuation = false;
// Stats bar. The live and summary bars are built once per screen and swapped
// with the HIDDEN flag, so finishing a run never allocates LVGL objects.
lv_obj_t *typingLiveBar = nullptr;
lv_obj_t *typingSummaryBar = nullptr;
bool typingSummaryShown = false;
enum TypingLiveStat : size_t {
  TYPING_LIVE_WPM,
  TYPING_LIVE_ACC,
  TYPING_LIVE_TIME,
  TYPING_LIVE_PB,
  TYPING_LIVE_COUNT,
};
enum TypingSummaryStat : size_t {
  TYPING_SUM_WPM,
  TYPING_SUM_RAW,
  TYPING_SUM_ACC,
  TYPING_SUM_TIME,
  TYPING_SUM_WORDS,
  TYPING_SUM_BEST,
  TYPING_SUM_COUNT,
};
lv_obj_t *typingLiveValue[TYPING_LIVE_COUNT] = {};
lv_obj_t *typingSummaryValue[TYPING_SUM_COUNT] = {};
lv_obj_t *typingWordsLabel = nullptr;
lv_obj_t *typingStatusLabel = nullptr;
lv_obj_t *typingComplexityValue = nullptr;
lv_obj_t *typingMaxLengthValue = nullptr;
lv_obj_t *typingSymbolsSwitch = nullptr;
lv_obj_t *typingNumbersSwitch = nullptr;
lv_obj_t *typingCapitalSwitch = nullptr;
lv_obj_t *typingPunctuationSwitch = nullptr;
char typingText[TYPING_TEXT_CAP];
// Plain (no recolor codes) copy used to measure the Long run's wrapped
// height; the fit check bounds it to roughly 300 characters.
char typingMeasureText[512];
char typingStatusText[64];  // status sentence plus the Long word count

// Clause planner for the word stream: a run is a paragraph, so capitalization
// and punctuation need to know which token opens a sentence and which one
// closes it. Reset once per run, never touched per keystroke.
uint8_t typingClauseLeft = 0;      // words left in the current clause
bool typingClauseFinal = true;     // clause closes with . ? ! (else with ;)
bool typingSentenceStart = true;   // next token opens a sentence
const char *typingLastBase = nullptr;  // previous lexical word, for no-repeat


// Personal-best NVS keys for the Short/Long options, in TYPING_LENGTHS
// order. loadPrefs() migrates the retired tb10/tb15/tb25/tb50 records into
// them.
const char *typingBestKey(size_t index) {
  static const char *const keys[TYPING_LENGTH_COUNT] = {
      "tb20", "tblong"};
  return keys[index];
}

// Defined with the other widget helpers below; touchRead() needs it earlier.
void setWidgetHidden(lv_obj_t *obj, bool hidden);

void displayFlush(lv_disp_drv_t *driver, const lv_area_t *area,
                  lv_color_t *colors) {
  const uint32_t width = area->x2 - area->x1 + 1;
  const uint32_t height = area->y2 - area->y1 + 1;
  display.startWrite();
  display.setAddrWindow(area->x1, area->y1, width, height);
  display.writePixels(reinterpret_cast<lgfx::rgb565_t *>(&colors->full),
                      width * height);
  display.endWrite();
  lv_disp_flush_ready(driver);
}

// The FT6336U on this board reports touch coordinates shifted by a constant
// offset in both axes. Tuned empirically to (10, 10).
constexpr int32_t TOUCH_OFFSET_X = 10;
constexpr int32_t TOUCH_OFFSET_Y = 10;

// A touch on the blank screen is a wake gesture, not a button press. Restore
// the backlight at zero and let updateWakeFade() ramp it up; the panel keeps
// its pixels while dark, so the UI itself needs no rebuild.
void startWakeFade(uint32_t now) {
  screenOff = false;
  backlightDimmed = false;
  wakeFadeActive = true;
  wakeFadeStartMs = now;
  display.setBrightness(0);
  lastInputMs = now;
  uiNeedsRender = true;
}

// Ramp the backlight from zero to the configured level after a wake tap.
// brightnessValue is read live so a keyboard brightness change mid-ramp is
// followed instead of fighting the fade. 255 * 1000 ms stays inside uint32_t.
void updateWakeFade(uint32_t now) {
  if (!wakeFadeActive) return;
  const uint32_t elapsed = now - wakeFadeStartMs;
  if (elapsed >= WAKE_FADE_MS) {
    wakeFadeActive = false;
    display.setBrightness(brightnessValue);
    return;
  }
  display.setBrightness(
      static_cast<uint8_t>(brightnessValue * elapsed / WAKE_FADE_MS));
}

void touchRead(lv_indev_drv_t *, lv_indev_data_t *data) {
  // Ask for two points: the FT6336U reports them, and a two-finger tap is the
  // Zen-mode exit gesture. Single-touch behavior is unchanged.
  lgfx::touch_point_t points[2];
  const uint_fast8_t touchCount = display.getTouch(points, 2);
  const uint32_t now = millis();
  if (touchCount == 0) {
    twoFingerLatch = false;
    wakeGestureLatch = false;
    touchWasDown = false;
    data->state = LV_INDEV_STATE_REL;
    return;
  }
  if (screenOff) {
    // Blank screen: this touch only wakes the display. Swallow it (with any
    // second finger, so a blind gesture cannot flip Zen either) and let the
    // fade hide the buttons until the backlight is up.
    twoFingerLatch = false;
    startWakeFade(now);
    data->state = LV_INDEV_STATE_REL;
    return;
  }
  if (wakeFadeActive) {
    if (now - wakeFadeStartMs < WAKE_FADE_MS) {
      data->state = LV_INDEV_STATE_REL;
      return;
    }
    // The ramp finished but the finger may still be down; keep swallowing
    // until it lifts so holding the wake tap never becomes a press.
    wakeFadeActive = false;
    wakeGestureLatch = true;
  }
  if (wakeGestureLatch) {
    data->state = LV_INDEV_STATE_REL;
    return;
  }
  if (touchCount >= 2) {
    // Toggle once on the start of the gesture. A controller that keeps
    // reporting a second point must not flip Zen on every read.
    const bool gestureStarted = !twoFingerLatch;
    // Latch only long enough to swallow the rest of this gesture. A sticky
    // phantom point from the controller must never block the next tap.
    twoFingerLatch = true;
    twoFingerLatchUntilMs = millis() + 350;
    lastInputMs = millis();
    if (backlightDimmed) {
      display.setBrightness(brightnessValue);
      backlightDimmed = false;
    }
    // Two fingers toggle Zen in the editor: enter when not in Zen, exit when
    // in Zen. Other screens ignore the gesture.
    if (gestureStarted && editorVisible) {
      if (editorZenMode) zenExitRequested = true;
      else zenEntryRequested = true;
    }
    data->state = LV_INDEV_STATE_REL;
    return;
  }
  if (twoFingerLatch) {
    if (millis() < twoFingerLatchUntilMs) {
      // One finger lifted before the other; keep ignoring briefly.
      data->state = LV_INDEV_STATE_REL;
      return;
    }
    // The latch expired while a point is still reported: stop swallowing so
    // the helper button and normal taps stay usable.
    twoFingerLatch = false;
  }
  {
    // In Zen, a single tap brings the exit helper back for a few seconds; the
    // helper itself is a button, so it also exits if multi-touch is missed.
    if (editorZenMode && zenHintLabel) {
      setWidgetHidden(zenHintLabel, false);
      zenHintHideMs = millis() + 4000;
    }
    int32_t cx = static_cast<int32_t>(points[0].x) - TOUCH_OFFSET_X;
    int32_t cy = static_cast<int32_t>(points[0].y) - TOUCH_OFFSET_Y;
    if (cx < 0) cx = 0;
    if (cy < 0) cy = 0;
    // Only the movement of a held finger changes anything on screen: a rested
    // finger used to re-render the whole editor every indev poll (30 ms).
    const bool moved = !touchWasDown || cx != lastTouchX || cy != lastTouchY;
    touchWasDown = true;
    lastTouchX = static_cast<uint16_t>(cx);
    lastTouchY = static_cast<uint16_t>(cy);
    lastTouchMs = millis();
    // A tap counts as input: lift the idle dim and reset the idle timer,
    // exactly like a keypress does. A blank screen was handled above.
    lastInputMs = millis();
    if (backlightDimmed) {
      display.setBrightness(brightnessValue);
      backlightDimmed = false;
    }
    if (moved) uiNeedsRender = true;
    data->state = LV_INDEV_STATE_PR;
    data->point.x = static_cast<lv_coord_t>(cx);
    data->point.y = static_cast<lv_coord_t>(cy);
  }
}

void stylePanel(lv_obj_t *obj, lv_color_t color, lv_color_t border) {
  lv_obj_set_style_bg_color(obj, color, 0);
  lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(obj, border, 0);
  lv_obj_set_style_border_width(obj, 1, 0);
  lv_obj_set_style_radius(obj, 3, 0);
  lv_obj_set_style_pad_all(obj, 0, 0);
  lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

// UI icons drawn from the Space Mono Nerd Font (Font Awesome codepoints).
#define ICON_MENU "\xEF\x83\x89"       // U+F0C9 bars (hamburger)
#define ICON_SETTINGS "\xEF\x80\x93"   // U+F013 gear
#define ICON_BLUETOOTH "\xEF\x8A\x93"  // U+F293 bluetooth
#define ICON_FOLDER "\xEF\x81\xBB"     // U+F07B folder
#define ICON_SAVE "\xEF\x83\x87"       // U+F0C7 floppy disk
#define ICON_RETRY "\xEF\x80\x9E"      // U+F01E redo (circular arrow)
#define ICON_WIFI "\xEF\x87\xAB"       // U+F1EB wifi
#define ICON_KEYBOARD "\xF3\xB0\x8C\x8C"   // U+F030C keyboard (Material)

void setUiFont(lv_obj_t *obj) {
  lv_obj_set_style_text_font(obj, &elms_bold_12, 0);
}

// Keycap chip for the bottom hint row: the key letter on a darker grey,
// borderless rounded background in a lighter grey — the same padded-label
// trick the mode pill uses, so the letter reads as a keypress target.
lv_obj_t *makeHintKeycap(lv_obj_t *parent, const char *letter) {
  lv_obj_t *key = lv_label_create(parent);
  lv_label_set_text(key, letter);
  setUiFont(key);
  lv_obj_set_style_bg_color(key, lv_color_hex(0x313244), 0);
  lv_obj_set_style_bg_opa(key, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(key, 0, 0);
  lv_obj_set_style_radius(key, 2, 0);
  lv_obj_set_style_text_color(key, lv_color_hex(0xCDD6F4), 0);
  lv_obj_set_style_pad_left(key, 4, 0);
  lv_obj_set_style_pad_right(key, 4, 0);
  lv_obj_set_style_pad_top(key, 1, 0);
  lv_obj_set_style_pad_bottom(key, 1, 0);
  return key;
}

// Menu-shortcut variant of the keycap chip: identical geometry, but the
// app's link blue as an outline plus text color, so the ":menu" and
// ":settings" chips read as highlighted destinations rather than another
// mode-switch key. LVGL counts the border in the auto-size, so the 1 px
// vertical padding is dropped to keep the chip at the regular 16 px height
// (line 14 + 2 borders) — otherwise the outline grows 2 px taller and
// touches the status-panel border above and the strip edge below.
lv_obj_t *makeHintKeycapMenu(lv_obj_t *parent, const char *letter) {
  lv_obj_t *key = makeHintKeycap(parent, letter);
  lv_obj_set_style_text_color(key, lv_color_hex(0x89B4FA), 0);
  lv_obj_set_style_border_width(key, 1, 0);
  lv_obj_set_style_border_color(key, lv_color_hex(0x89B4FA), 0);
  lv_obj_set_style_pad_top(key, 0, 0);
  lv_obj_set_style_pad_bottom(key, 0, 0);
  return key;
}

// Muted caption beside a hint keycap. The label has no background, so any
// padding only affects spacing, never the look.
lv_obj_t *makeHintWord(lv_obj_t *parent, const char *text) {
  lv_obj_t *word = lv_label_create(parent);
  lv_label_set_text(word, text);
  setUiFont(word);
  lv_obj_set_style_text_color(word, lv_color_hex(0x7F849C), 0);
  return word;
}

// Bottom hint-row container shared by every screen: an invisible flex row
// for keycap chips, geometry matching the editor screen's inline hintRow.
// Styled transparent rather than lv_obj_remove_style_all(), which trips a
// deprecation warning inside this LVGL version's own header.
lv_obj_t *makeHintRow() {
  lv_obj_t *row = lv_obj_create(lv_scr_act());
  lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(row, 0, 0);
  lv_obj_set_style_pad_all(row, 0, 0);
  lv_obj_set_pos(row, 8, 299);
  lv_obj_set_size(row, SCREEN_W - 16, 21);
  lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_layout(row, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(row, 4, 0);
  return row;
}

// Keycap + caption pair, the hint row's basic rhythm. The word's trailing
// pad adds breathing room before the next chip group.
void addHintPair(lv_obj_t *row, const char *key, const char *word) {
  makeHintKeycap(row, key);
  lv_obj_t *caption = makeHintWord(row, word);
  lv_obj_set_style_pad_right(caption, 4, 0);
}

// Show or hide an object; LVGL marks flex parents dirty when HIDDEN
// toggles, so swapping the visible hint widgets reflows the row.
void setWidgetHidden(lv_obj_t *obj, bool hidden) {
  if (!obj) return;
  // add_flag/clear_flag invalidate unconditionally; the hint row is re-asserted
  // on every render, so skip the no-op case.
  if (hidden == lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN)) return;
  if (hidden) lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
  else lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
}

// lv_label_set_text() always invalidates the label and frees/reallocates its
// buffer, even when the text is identical (lv_label.c). The editor re-sets a
// whole screen of labels per keystroke, so compare first: unchanged bands then
// cost one strcmp and nothing else.
void labelSetTextIfChanged(lv_obj_t *label, const char *text) {
  if (!label || !text) return;
  const char *current = lv_label_get_text(label);
  if (current && strcmp(current, text) == 0) return;
  lv_label_set_text(label, text);
}

// Copy `len` bytes into a fixed buffer with a NUL, so a document slice can be
// handed to LVGL without constructing a std::string. Returns buf.
const char *copyBounded(char *buf, size_t cap, const char *src, size_t len) {
  const size_t n = std::min(len, cap - 1);
  if (n) memcpy(buf, src, n);
  buf[n] = '\0';
  return buf;
}

void setTitleFont(lv_obj_t *obj) {
  lv_obj_set_style_text_font(obj, &elms_bold_16, 0);
}

// The editor text font: anti-aliased Terminess, or the crisp 1-bpp variant
// when smoothing is off. Terminess is a pixel font: its line box equals the
// glyph cell, so both variants share each size's metrics. Every size has both
// variants, so the smoothing toggle keeps working at all four steps.
const lv_font_t *const EDITOR_FONTS_SMOOTH[EDITOR_FONT_SIZE_COUNT] = {
    &terminess_10, &terminess_12, &terminess_14, &terminess_16};
const lv_font_t *const EDITOR_FONTS_CRISP[EDITOR_FONT_SIZE_COUNT] = {
    &terminess_10_cr, &terminess_12_cr, &terminess_14_cr,
    &terminess_16_cr};

const lv_font_t *editorFont() {
  return editorSmoothText ? EDITOR_FONTS_SMOOTH[editorFontSizeIndex]
                          : EDITOR_FONTS_CRISP[editorFontSizeIndex];
}

// Monospace glyph advance width, re-measured whenever the font size changes;
// it drives the caret cell, wrap width and horizontal scroll window.
uint16_t editorGlyphWidth = 7;

// Zen mode hides the header, status bar and hint strip and grows the page to
// the full screen. These helpers keep the canvas geometry and the text origin
// in one place. Normal mode: a 36 px header at y 4 (ends 40), then the canvas
// at 41 so its bottom edge stays at 270 and the 221 px text area is preserved
// (229 - 2 borders - 6 top). At the 14 px Terminess default that is 15 rows
// of 14 px; the 10/12 px cuts fit 18 rows, the 16 px cut 13.
int editorCanvasY() { return editorZenMode ? 4 : 41; }
int editorCanvasH() { return editorZenMode ? SCREEN_H - 8 : EDITOR_CANVAS_H; }
int editorTextTop() { return editorZenMode ? 6 : EDITOR_TEXT_TOP; }

// Recompute the editor's live geometry from the selected font. The canvas is
// 231 px tall with a 1 px border and the first band starts at y=6, so the
// stack has 221 px to fill; Zen uses the full height instead. Called at boot,
// before every screen build and whenever the size changes.
void refreshEditorMetrics() {
  const lv_font_t *font = editorFont();
  editorLineHeight = font->line_height;
  const int textArea = editorCanvasH() - 2 - editorTextTop();
  editorVisibleLines = textArea / editorLineHeight;
  if (editorVisibleLines < 1) editorVisibleLines = 1;
  if (editorVisibleLines > MAX_VISIBLE_LINES)
    editorVisibleLines = MAX_VISIBLE_LINES;
  editorGlyphWidth = lv_font_get_glyph_width(font, '0', 0);
  if (editorGlyphWidth == 0) editorGlyphWidth = 7;
}

size_t lineStart(size_t offset) {
  // At offset 0 rfind must not search at all: rfind('\n', 0) still looks at
  // position 0, so a document that starts with an empty line would report the
  // first line as starting at 1. columnFor(0) then underflowed its length to
  // SIZE_MAX and scanned 4 GB past the buffer (cache/MMU fault).
  if (offset == 0) return 0;
  const size_t start = document.rfind('\n', offset - 1);
  return start == std::string::npos ? 0 : start + 1;
}

size_t lineEnd(size_t offset) {
  const size_t end = document.find('\n', offset);
  return end == std::string::npos ? document.size() : end;
}

// Any content change must call this: it drops the line index and dates the
// document so dependent caches (syntax state, painted bands) refresh.
void markDocumentChanged() {
  lineIndexDirty = true;
  ++documentEditStamp;
}

// Rebuild the line table in one linear pass. Called on the first line query
// after an edit, never once per query: a render asks for ~20 line positions.
void ensureLineIndex() {
  if (!lineIndexDirty) return;
  lineIndexDirty = false;
  const size_t size = document.size();
  lineStarts.clear();
  // ~1 start per 40 bytes is a normal Markdown line; the vector grows if the
  // document is denser, so this is only a starting capacity.
  lineStarts.reserve(size / 40 + 2);
  lineStarts.push_back(0);
  const char *const base = document.data();
  size_t pos = 0;
  while (pos < size) {
    const void *hit = memchr(base + pos, '\n', size - pos);
    if (!hit) break;
    pos = static_cast<size_t>(static_cast<const char *>(hit) - base) + 1;
    lineStarts.push_back(static_cast<uint32_t>(pos));
  }
}

// Zero-based index of the line holding `offset` (= count of '\n' before it).
size_t lineNumber(size_t offset) {
  ensureLineIndex();
  const auto it = std::upper_bound(lineStarts.begin(), lineStarts.end(),
                                   static_cast<uint32_t>(std::min(
                                       offset, document.size())));
  // upper_bound lands one past the last start <= offset, i.e. on the next
  // line, so stepping back gives the line that holds the offset.
  return static_cast<size_t>(it - lineStarts.begin()) - 1;
}

size_t offsetForLine(size_t targetLine) {
  ensureLineIndex();
  return targetLine < lineStarts.size()
             ? static_cast<size_t>(lineStarts[targetLine])
             : document.size();
}

// --- UTF-8 codepoint helpers -------------------------------------------------
// The document is a byte string, but the editor's column, wrap and caret math
// must count codepoints: a curly quote or em dash is three bytes and one cell.
// Movement and erasure always land on codepoint boundaries so the buffer can
// never be left with half a sequence.
bool utf8Continuation(unsigned char c) { return (c & 0xC0) == 0x80; }

// Next codepoint boundary at or after off+1, bounded by len.
size_t utf8Next(const char *s, size_t len, size_t off) {
  if (off >= len) return len;
  ++off;
  while (off < len && utf8Continuation(static_cast<unsigned char>(s[off]))) {
    ++off;
  }
  return off;
}

// Start of the codepoint before off.
size_t utf8Prev(const char *s, size_t off) {
  if (off == 0) return 0;
  --off;
  while (off > 0 && utf8Continuation(static_cast<unsigned char>(s[off]))) {
    --off;
  }
  return off;
}

// Codepoints in [s, s + len).
size_t utf8Columns(const char *s, size_t len) {
  size_t cols = 0;
  for (size_t i = 0; i < len; ++i) {
    if (!utf8Continuation(static_cast<unsigned char>(s[i]))) ++cols;
  }
  return cols;
}

// Bytes covering the first `cols` codepoints, clamped to len.
size_t utf8BytesForColumns(const char *s, size_t len, size_t cols) {
  size_t off = 0;
  while (off < len && cols > 0) {
    off = utf8Next(s, len, off);
    --cols;
  }
  return off;
}

size_t nextCursor(size_t off) {
  return utf8Next(document.data(), document.size(), std::min(off, document.size()));
}

size_t prevCursor(size_t off) {
  return utf8Prev(document.data(), std::min(off, document.size()));
}

// Clamp an arbitrary byte offset onto the nearest codepoint boundary. The end
// of the document is a valid insertion point, so it is preserved.
size_t clampCursor(size_t off) {
  if (off >= document.size()) return document.size();
  while (off > 0 && utf8Continuation(static_cast<unsigned char>(document[off]))) {
    --off;
  }
  return off;
}

size_t columnFor(size_t offset) {
  const size_t clamped = std::min(offset, document.size());
  // Belt and braces: a bad line start must never underflow the length into a
  // multi-gigabyte read (the crash this guards against was exactly that).
  const size_t start = std::min(lineStart(clamped), clamped);
  return utf8Columns(document.data() + start, clamped - start);
}

size_t lineOffsetForColumn(size_t line, size_t column) {
  const size_t start = offsetForLine(line);
  const size_t end = lineEnd(start);
  size_t off = start;
  size_t cols = column;
  while (off < end && cols > 0) {
    off = utf8Next(document.data(), end, off);
    --cols;
  }
  return off;
}

// Whitespace-separated word count for the footer. O(n), but only run when the
// footer is in Words mode and the caller's throttle allows it — never per
// keystroke on a large draft.
size_t countDocumentWords() {
  size_t words = 0;
  bool inWord = false;
  for (size_t i = 0; i < document.size(); ++i) {
    const unsigned char ch = static_cast<unsigned char>(document[i]);
    const bool space = ch == ' ' || ch == '\n' || ch == '\r' || ch == '\t' ||
                       ch == '\f' || ch == '\v';
    if (space) {
      inWord = false;
    } else if (!inWord) {
      inWord = true;
      ++words;
    }
  }
  return words;
}

void rememberUndo() {
  undoBuffer = document;
  undoAvailable = true;
  // A fresh edit breaks the undo/redo chain: the redo snapshot describes a
  // future that no longer follows from the new text.
  redoAvailable = false;
  redoBuffer.clear();
}

void undoEdit() {
  if (!undoAvailable) return;
  redoBuffer = document;
  redoAvailable = true;
  std::swap(document, undoBuffer);
  undoAvailable = false;
  markDocumentChanged();
  cursor = clampCursor(cursor);
  documentWordCacheValid = false;
  dirty = true;
  undoGroupActive = false;
  statusMessage = "Undo";
  uiNeedsRender = true;
}

void redoEdit() {
  if (!redoAvailable) return;
  undoBuffer = document;
  undoAvailable = true;
  std::swap(document, redoBuffer);
  redoAvailable = false;
  markDocumentChanged();
  cursor = clampCursor(cursor);
  documentWordCacheValid = false;
  dirty = true;
  undoGroupActive = false;
  statusMessage = "Redo";
  uiNeedsRender = true;
}

void enterInsertMode() {
  if (!undoGroupActive) rememberUndo();
  undoGroupActive = true;
  insertMode = true;
}

void leaveInsertMode() {
  insertMode = false;
  undoGroupActive = false;
}

void markEdited() {
  dirty = true;
  lastEditMs = millis();
  // The word count is deliberately NOT invalidated here: updateStatusBar()
  // recomputes it at most twice a second on its own, and forcing a full scan
  // per keystroke is exactly what that throttle exists to prevent.
  markDocumentChanged();
  uiNeedsRender = true;
}

void insertAtCursor(char c) {
  if (document.size() >= documentLimit) {
    statusMessage = "Document limit reached";
    uiNeedsRender = true;
    return;
  }
  if (!undoGroupActive) rememberUndo();
  // Every caller keeps the caret on a codepoint boundary, but a stale or
  // byte-stepped offset must never insert into the middle of a glyph.
  cursor = clampCursor(cursor);
  document.insert(document.begin() + cursor, c);
  ++cursor;
  markEdited();
}

void eraseRange(size_t first, size_t last) {
  if (first >= last || first >= document.size()) return;
  if (!undoGroupActive) rememberUndo();
  last = std::min(last, document.size());
  document.erase(first, last - first);
  // Insert mode keeps the caret in the gap between characters, where the
  // buffer end is a legal position; Normal/Visual park it ON a character
  // instead. One clamp for both strands the final glyph behind the caret
  // after any erase that consumed the buffer tail.
  cursor = std::min(first, insertMode ? document.size()
                                      : (document.empty() ? size_t(0)
                                                          : document.size() - 1));
  markEdited();
}

// --- Insert-mode helpers: indent, auto-pair, auto-indent ---------------------

// Tab inserts spaces up to the next tab stop, using EDITOR_TAB_SPACES (two by
// default) as the stop width. Hard tabs already in an opened file are left
// alone: the label renderer has no tab stops, so rewriting them would move
// text the user did not touch.
void insertTabIndent() {
  const size_t column = columnFor(cursor);
  size_t spaces = EDITOR_TAB_SPACES - (column % EDITOR_TAB_SPACES);
  if (spaces == 0) spaces = EDITOR_TAB_SPACES;
  if (document.size() + spaces >= documentLimit) {
    statusMessage = "Document limit reached";
    uiNeedsRender = true;
    return;
  }
  for (size_t i = 0; i < spaces; ++i) insertAtCursor(' ');
}

char closingPairFor(char open) {
  switch (open) {
  case '(': return ')';
  case '[': return ']';
  case '{': return '}';
  case '"': return '"';
  case '\'': return '\'';
  default: return '\0';
  }
}

// Quotes auto-pair in code, where they delimit strings, but not in Markdown or
// plain prose, where they are just punctuation.
bool autoPairsQuotes() {
  return editorFileType != DocLang::Markdown &&
         editorFileType != DocLang::PlainText;
}

void insertBytes(const char *s) {
  while (*s) insertAtCursor(*s++);
}

bool hasDocumentRoom(size_t bytes) {
  return document.size() + bytes <= documentLimit;
}

// Prose typography for Markdown and plain text. Returns true when the key was
// consumed, including the ASCII fallback when the document is nearly full.
// Multi-byte glyphs are only ever inserted whole, so the buffer stays valid.
bool insertProsePunctuation(char c) {
  if (c == '\'' || c == '"') {
    if (!hasDocumentRoom(3)) {
      insertAtCursor(c);
      return true;
    }
    bool opening = cursor == 0;
    if (cursor > 0) {
      const char prev = document[cursor - 1];
      opening = prev == ' ' || prev == '\n' || prev == '\t' ||
                prev == '(' || prev == '[' || prev == '{' ||
                prev == '-' || prev == '*' || prev == '_' ||
                prev == '/' || prev == '"' || prev == '\'';
    }
    if (c == '\'') {
      insertBytes(opening ? "\xE2\x80\x98" : "\xE2\x80\x99");
    } else {
      insertBytes(opening ? "\xE2\x80\x9C" : "\xE2\x80\x9D");
    }
    return true;
  }
  if (c == '-') {
    if (cursor > 0 && document[cursor - 1] == '-' &&
        !(cursor >= 2 && document[cursor - 2] == '-')) {
      // Do not turn a Markdown rule ("---" or a line of dashes) into a dash.
      const size_t start = lineStart(cursor);
      bool onlyRule = true;
      for (size_t i = start; i + 1 < cursor; ++i) {
        if (document[i] != ' ' && document[i] != '-') {
          onlyRule = false;
          break;
        }
      }
      if (!onlyRule && hasDocumentRoom(3)) {
        eraseRange(cursor - 1, cursor);
        insertBytes("\xE2\x80\x94");  // em dash
        return true;
      }
    }
    insertAtCursor('-');
    return true;
  }
  if (c == '.') {
    if (cursor >= 2 && document[cursor - 1] == '.' &&
        document[cursor - 2] == '.' &&
        !(cursor >= 3 && document[cursor - 3] == '.') &&
        hasDocumentRoom(3)) {
      eraseRange(cursor - 2, cursor);
      insertBytes("\xE2\x80\xA6");  // ellipsis
      return true;
    }
    insertAtCursor('.');
    return true;
  }
  return false;
}

// A typed character in Insert mode. An opener grows its closing partner and
// leaves the caret between them; typing a closer that is already next in the
// line steps over it instead of doubling the glyph. Prose typography runs
// first so its curly quotes do not enter the ASCII auto-pair path.
void insertTypedChar(char c) {
  if (editorProseTypo &&
      (editorFileType == DocLang::Markdown ||
       editorFileType == DocLang::PlainText) &&
      insertProsePunctuation(c)) {
    return;
  }
  const bool pairQuotes = autoPairsQuotes();
  const bool closer = c == ')' || c == ']' || c == '}' || c == '"' || c == '\'';
  if (closer && (pairQuotes || c == ')' || c == ']' || c == '}') &&
      cursor < document.size() && document[cursor] == c) {
    ++cursor;
    markEdited();
    return;
  }
  const char close = (pairQuotes || c == '(' || c == '[' || c == '{')
                         ? closingPairFor(c)
                         : '\0';
  if (close == '\0') {
    insertAtCursor(c);
    return;
  }
  if (document.size() + 2 >= documentLimit) {
    insertAtCursor(c);  // no room for the partner: insert the opener alone
    return;
  }
  insertAtCursor(c);
  insertAtCursor(close);
  --cursor;  // park between the pair
}

// One Markdown list marker on a line: bullet (-, *, +), optional task box, or
// an ordered "N." marker. markerStart is after the indent; markerEnd is after
// the marker and its single trailing space.
struct MarkdownListInfo {
  bool found = false;
  size_t markerStart = 0;
  size_t markerEnd = 0;
  char kind = 0;
  bool task = false;
  unsigned number = 0;
};

MarkdownListInfo parseMarkdownList(size_t start, size_t end) {
  MarkdownListInfo info;
  size_t i = start;
  while (i < end && document[i] == ' ') ++i;
  info.markerStart = i;
  if (i + 1 < end &&
      (document[i] == '-' || document[i] == '*' || document[i] == '+') &&
      document[i + 1] == ' ') {
    info.found = true;
    info.kind = document[i];
    i += 2;
    if (i + 2 < end && document[i] == '[' && document[i + 2] == ']' &&
        (document[i + 1] == ' ' || document[i + 1] == 'x' ||
         document[i + 1] == 'X')) {
      info.task = true;
      i += 3;
      if (i < end && document[i] == ' ') ++i;
    }
    info.markerEnd = i;
    return info;
  }
  const size_t digits = i;
  unsigned value = 0;
  while (i < end && document[i] >= '0' && document[i] <= '9' &&
         i - digits < 6) {
    value = value * 10 + unsigned(document[i] - '0');
    ++i;
  }
  if (i > digits && i + 1 < end && document[i] == '.' &&
      document[i + 1] == ' ') {
    info.found = true;
    info.kind = '.';
    info.number = value;
    info.markerEnd = i + 2;
  }
  return info;
}

// Enter in Insert mode: keep the current line's indentation, add one level
// after an opener or a Python colon, and — when the caret sits between a pair
// — push the closer down onto its own line.
void insertNewlineInEditor() {
  const size_t start = lineStart(cursor);
  size_t indent = 0;
  while (start + indent < cursor && document[start + indent] == ' ') ++indent;

  // Markdown list continuation: "Enter" after a bullet, task or numbered item
  // starts the next item with the same indent and marker. On an item with no
  // content, Enter exits the list instead of adding another bullet.
  if (editorFileType == DocLang::Markdown && editorListContinue) {
    const size_t end = lineEnd(start);
    const MarkdownListInfo info = parseMarkdownList(start, end);
    if (info.found && cursor >= info.markerEnd) {
      bool emptyContent = true;
      for (size_t k = info.markerEnd; k < end; ++k) {
        if (document[k] != ' ' && document[k] != '\t') {
          emptyContent = false;
          break;
        }
      }
      if (emptyContent) {
        // Remove the marker (and one indent level for a nested item) and
        // leave the caret on the now-plain line. This Enter is consumed by
        // leaving the list, so no extra newline is inserted.
        const size_t newIndent =
            indent >= EDITOR_TAB_SPACES ? indent - EDITOR_TAB_SPACES : 0;
        eraseRange(start + newIndent, end);
        cursor = start + newIndent;
        return;
      }
      insertAtCursor('\n');
      for (size_t k = 0; k < indent; ++k) insertAtCursor(' ');
      if (info.task) {
        const char *unit = "- [ ] ";
        for (const char *p = unit; *p; ++p) insertAtCursor(*p);
      } else if (info.kind == '.') {
        char next[16];
        snprintf(next, sizeof(next), "%u. ", info.number + 1);
        for (const char *p = next; *p; ++p) insertAtCursor(*p);
      } else {
        insertAtCursor(info.kind);
        insertAtCursor(' ');
      }
      return;
    }
  }

  char opener = '\0';
  bool opensBlock = false;
  if (cursor > start) {
    const char prev = document[cursor - 1];
    if (prev == '{' || prev == '[' || prev == '(') {
      opener = prev;
      opensBlock = true;
    } else if (editorFileType == DocLang::Python && prev == ':') {
      opensBlock = true;
    }
  }
  const size_t extra = opensBlock ? EDITOR_TAB_SPACES : 0;

  bool closeBelow = false;
  if (opener != '\0' && cursor < document.size()) {
    // Only when the closer is the line's last glyph: "{|}" splits, "{} x" does
    // not.
    closeBelow = document[cursor] == closingPairFor(opener) &&
                 cursor + 1 == lineEnd(cursor);
  }

  insertAtCursor('\n');
  for (size_t i = 0; i < indent + extra; ++i) insertAtCursor(' ');
  if (closeBelow) {
    const size_t middle = cursor;
    insertAtCursor('\n');
    for (size_t i = 0; i < indent; ++i) insertAtCursor(' ');
    cursor = middle;
  }
}

// Tab in a Markdown list indents the whole item by one 2-space step, the way
// Obsidian nests a sub-list. Outside a list it falls back to the usual
// next-tab-stop space insertion.
void insertTabInList() {
  if (editorFileType == DocLang::Markdown && editorListContinue) {
    const size_t start = lineStart(cursor);
    const size_t end = lineEnd(start);
    const MarkdownListInfo info = parseMarkdownList(start, end);
    if (info.found) {
      const size_t saved = cursor;
      cursor = start;
      for (size_t k = 0; k < EDITOR_TAB_SPACES; ++k) insertAtCursor(' ');
      cursor = std::min(saved + EDITOR_TAB_SPACES, document.size());
      return;
    }
  }
  insertTabIndent();
}

// Shift-Tab removes one indent step from a Markdown list item.
void outdentListLine() {
  if (editorFileType != DocLang::Markdown || !editorListContinue) return;
  const size_t start = lineStart(cursor);
  size_t spaces = 0;
  while (spaces < EDITOR_TAB_SPACES && start + spaces < document.size() &&
         document[start + spaces] == ' ') {
    ++spaces;
  }
  if (spaces == 0) return;
  eraseRange(start, start + spaces);
  cursor = cursor >= start + spaces ? cursor - spaces : start;
}

// Ctrl-Enter: flip a Markdown task box on the current line. The first "[ ]",
// "[x]" or "[X]" wins; the caret does not move, so the line stays in view.
void toggleTaskAtCursor() {
  if (editorFileType != DocLang::Markdown) {
    statusMessage = "Task boxes are Markdown";
    uiNeedsRender = true;
    return;
  }
  const size_t start = lineStart(cursor);
  const size_t end = lineEnd(start);
  for (size_t i = start; i + 2 < end; ++i) {
    if (document[i] == '[' && document[i + 2] == ']' &&
        (document[i + 1] == ' ' || document[i + 1] == 'x' ||
         document[i + 1] == 'X')) {
      if (!undoGroupActive) rememberUndo();
      const bool wasChecked = document[i + 1] != ' ';
      document[i + 1] = wasChecked ? ' ' : 'x';
      markEdited();
      statusMessage = wasChecked ? "Task unchecked" : "Task checked";
      uiNeedsRender = true;
      return;
    }
  }
  statusMessage = "No task box on this line";
  uiNeedsRender = true;
}

// Backspace in Insert mode: an empty auto-paired "()" collapses as one unit,
// so removing the opener never leaves a stray closer behind.
void backspaceInEditor() {
  if (cursor == 0) return;
  if (cursor < document.size()) {
    const char before = document[cursor - 1];
    const char close = closingPairFor(before);
    if (close != '\0' && close == document[cursor]) {
      const bool quotes = before == '"' || before == '\'';
      if (!quotes || autoPairsQuotes()) {
        eraseRange(cursor - 1, cursor + 1);
        return;
      }
    }
  }
  eraseRange(prevCursor(cursor), cursor);
}

// --- Recent documents (start screen) ---------------------------------------
// Persisted as one newline-joined NVS string. Each line is "<offset>\t<path>"
// where offset is the cursor position to restore; a line without a tab is the
// old path-only format and restores at offset 0. RECENT_MAX entries of ~50
// bytes stay far below the 4 KiB NVS string limit. Written when a document is
// opened, saved or quit — never per keystroke.
void saveRecentFiles() {
  std::string joined;
  for (size_t i = 0; i < recentFiles.size(); ++i) {
    if (!joined.empty()) joined.push_back('\n');
    const size_t savedCursor =
        i < recentCursors.size() ? recentCursors[i] : 0;
    char prefix[24];
    snprintf(prefix, sizeof(prefix), "%u\t", unsigned(savedCursor));
    joined += prefix;
    joined += recentFiles[i];
  }
  if (!devicePrefs.begin("wiaw", false)) return;
  devicePrefs.putString("recent", joined.c_str());
  devicePrefs.end();
}

void loadRecentFiles() {
  recentFiles.clear();
  recentCursors.clear();
  if (!devicePrefs.begin("wiaw", true)) return;
  const String stored = devicePrefs.getString("recent", "");
  devicePrefs.end();
  // Only paths this firmware wrote are accepted; anything else (truncated or
  // foreign NVS data) is dropped instead of shown as a broken entry.
  size_t start = 0;
  while (start < static_cast<size_t>(stored.length()) &&
         recentFiles.size() < RECENT_MAX) {
    int end = stored.indexOf('\n', start);
    if (end < 0) end = stored.length();
    if (end > static_cast<int>(start)) {
      std::string entry(stored.c_str() + start, end - start);
      size_t savedCursor = 0;
      const size_t tab = entry.find('\t');
      if (tab != std::string::npos) {
        savedCursor = static_cast<size_t>(
            strtoul(entry.c_str(), nullptr, 10));
        entry.erase(0, tab + 1);
      }
      if (entry.rfind("/WriterInAWell/", 0) == 0) {
        recentFiles.push_back(entry);
        recentCursors.push_back(savedCursor);
      }
    }
    start = static_cast<size_t>(end) + 1;
  }
}

// Cursor offset saved for a path, or 0 when it is not in the list yet.
size_t recentCursorFor(const std::string &path) {
  for (size_t i = 0; i < recentFiles.size() && i < recentCursors.size(); ++i) {
    if (recentFiles[i] == path) return recentCursors[i];
  }
  return 0;
}

// Update the current path's saved offset and persist. Called on save, quit and
// before replacing a clean buffer, so the next open resumes at the caret.
void rememberCurrentCursor() {
  if (currentPath.empty()) return;
  for (size_t i = 0; i < recentFiles.size() && i < recentCursors.size(); ++i) {
    if (recentFiles[i] == currentPath) {
      if (recentCursors[i] != cursor) {
        recentCursors[i] = cursor;
        saveRecentFiles();
      }
      return;
    }
  }
}

// Newest first, de-duplicated, capped at RECENT_MAX.
void pushRecent(const std::string &path, size_t savedCursor) {
  if (path.empty()) return;
  if (!recentFiles.empty() && recentFiles.front() == path) {
    if (recentCursors.empty()) recentCursors.push_back(savedCursor);
    else recentCursors.front() = savedCursor;
    saveRecentFiles();
    return;
  }
  for (size_t i = 0; i < recentFiles.size(); ++i) {
    if (recentFiles[i] == path) {
      recentFiles.erase(recentFiles.begin() + i);
      if (i < recentCursors.size()) {
        recentCursors.erase(recentCursors.begin() + i);
      }
      break;
    }
  }
  recentFiles.insert(recentFiles.begin(), path);
  recentCursors.insert(recentCursors.begin(), savedCursor);
  if (recentFiles.size() > RECENT_MAX) {
    recentFiles.resize(RECENT_MAX);
    recentCursors.resize(RECENT_MAX);
  }
  saveRecentFiles();
}

void loadDocument(const String &path) {
  if (!cardAvailable) {
    statusMessage = "Insert a microSD card to open files";
    return;
  }
  // Persist the outgoing buffer's caret before replacing the document, so
  // switching between two clean drafts resumes both correctly.
  rememberCurrentCursor();
  File file = SD.open(path, FILE_READ);
  if (!file || file.isDirectory()) {
    statusMessage = "Could not open file";
    if (file) file.close();
    return;
  }
  if (file.size() > documentLimit) {
    file.close();
    statusMessage = "File exceeds editing memory";
    uiNeedsRender = true;
    return;
  }
  document.clear();
  // Read in chunks: a byte-at-a-time loop cost one SD read call per byte, so
  // opening a 512 KiB draft meant half a million SPI transactions.
  char chunk[512];
  while (document.size() < documentLimit) {
    const size_t want = std::min(sizeof(chunk), documentLimit - document.size());
    const size_t got = file.read(reinterpret_cast<uint8_t *>(chunk), want);
    if (got == 0) break;
    for (size_t i = 0; i < got; ++i) {
      const char ch = chunk[i];
      // Normalise CRLF as it is read: the renderer treats '\r' as a glyph, and
      // HTML/CSS/Python sources routinely come from Windows machines.
      if (ch == '\n' && !document.empty() && document.back() == '\r') {
        document.back() = '\n';
        continue;
      }
      document.push_back(ch);
    }
  }
  file.close();
  markDocumentChanged();
  fileName = String(path.substring(path.lastIndexOf('/') + 1)).c_str();
  currentPath = path.c_str();
  // The extension picks the grammar; an unknown one opens as plain text.
  editorFileType = docLangOrPlain(fileName);
  hasDocument = true;
  // Resume the caret saved with this path; clamp for a file edited shorter
  // elsewhere. A path seen for the first time starts at 0.
  cursor = clampCursor(recentCursorFor(currentPath));
  pushRecent(currentPath, cursor);
  // Fresh view: the first render scrolls the restored caret back on screen.
  viewportStart = viewportWrapCol = viewportScrollCol = 0;
  dirty = false;
  undoBuffer.clear();
  undoAvailable = false;
  redoBuffer.clear();
  redoAvailable = false;
  sessionWordBase = countDocumentWords();
  documentWordCacheValid = false;
  visualMode = insertMode = commandMode = searchMode = false;
  pendingOperator = 0;
  undoGroupActive = false;
  editorVisible = true;
  statusMessage = "Opened " + fileName;
  lastInputMs = millis();
  uiNeedsRender = true;
}

// --- Version snapshots -------------------------------------------------------
// Each successful overwrite copies the outgoing file (already staged as .bak)
// into the hidden versions folder before the .bak is removed. Names are
// "<name>.vNNN.bak" so they sort and never collide with recoverSaveBackups(),
// which only looks at root-level ".bak" files whose stem is editable.

bool ensureSnapshotDir() {
  if (!cardAvailable) return false;
  if (SD.exists(SNAPSHOT_DIR)) return true;
  return SD.mkdir(SNAPSHOT_DIR);
}

// Parse the version number out of "<base>.vNNN.bak", or -1 if it is not one.
int snapshotNumber(const String &name, const String &base) {
  const String prefix = base + ".v";
  if (!name.startsWith(prefix)) return -1;
  if (!name.endsWith(".bak")) return -1;
  const int start = prefix.length();
  const int end = name.length() - 4;
  if (end <= start || end - start > 4) return -1;
  int value = 0;
  for (int i = start; i < end; ++i) {
    const char c = name[i];
    if (c < '0' || c > '9') return -1;
    value = value * 10 + (c - '0');
  }
  return value;
}

// Copy one text file with a fixed stack buffer; snapshots are only taken on a
// save, never on the typing path.
bool copySnapshotFile(const String &src, const String &dest) {
  File in = SD.open(src.c_str(), FILE_READ);
  if (!in) return false;
  File out = SD.open(dest.c_str(), FILE_WRITE);
  if (!out) {
    in.close();
    return false;
  }
  uint8_t buf[256];
  bool ok = true;
  while (in.available()) {
    const size_t n = in.read(buf, sizeof(buf));
    if (n == 0) break;
    if (out.write(buf, n) != n) {
      ok = false;
      break;
    }
  }
  out.flush();
  const bool writeOk = out.getWriteError() == 0;
  out.close();
  in.close();
  if (!ok || !writeOk) {
    SD.remove(dest.c_str());
    return false;
  }
  return true;
}

// Delete versions older than the retention window. The directory is reopened
// per deletion so removing an entry never invalidates the iterator.
void pruneSnapshots(const String &base, int highest) {
  if (snapshotRetention == 0) return;
  const int oldestKeep = highest - static_cast<int>(snapshotRetention) + 1;
  if (oldestKeep <= 1) return;
  bool removed = true;
  while (removed) {
    removed = false;
    File dir = SD.open(SNAPSHOT_DIR);
    if (!dir || !dir.isDirectory()) {
      if (dir) dir.close();
      return;
    }
    for (File entry = dir.openNextFile(); entry; entry = dir.openNextFile()) {
      String name = entry.name();
      const int slash = name.lastIndexOf('/');
      if (slash >= 0) name = name.substring(slash + 1);
      const int n = snapshotNumber(name, base);
      if (n >= 0 && n < oldestKeep) {
        entry.close();
        const String path = String(SNAPSHOT_DIR) + "/" + name;
        // If the card refuses the delete, stop rather than re-finding the
        // same entry forever.
        removed = SD.remove(path.c_str());
        break;
      }
      entry.close();
    }
    dir.close();
  }
}

void saveVersionSnapshot(const String &backupPath) {
  if (!snapshotsEnabled || snapshotRetention == 0) return;
  if (fileName.empty()) return;
  if (!ensureSnapshotDir()) return;
  const String base = fileName.c_str();
  File dir = SD.open(SNAPSHOT_DIR);
  if (!dir || !dir.isDirectory()) {
    if (dir) dir.close();
    return;
  }
  int highest = 0;
  for (File entry = dir.openNextFile(); entry; entry = dir.openNextFile()) {
    String name = entry.name();
    const int slash = name.lastIndexOf('/');
    if (slash >= 0) name = name.substring(slash + 1);
    const int n = snapshotNumber(name, base);
    if (n > highest) highest = n;
    entry.close();
  }
  dir.close();
  // 18-byte folder + 64-byte name + ".vNNN.bak" + NUL fits in 128.
  char dest[128];
  snprintf(dest, sizeof(dest), "%s/%s.v%03d.bak", SNAPSHOT_DIR, base.c_str(),
           highest + 1);
  if (!copySnapshotFile(backupPath, dest)) return;
  pruneSnapshots(base, highest + 1);
}

bool saveDocument() {
  if (!cardAvailable || !directoryReady) {
    statusMessage = "No SD card — draft in memory";
    uiNeedsRender = true;
    return false;
  }
  if (currentPath.empty()) currentPath = "/WriterInAWell/" + fileName;
  const String tempPath = String(currentPath.c_str()) + ".tmp";
  SD.remove(tempPath.c_str());
  File file = SD.open(tempPath.c_str(), FILE_WRITE);
  if (!file) {
    statusMessage = "Save failed";
    uiNeedsRender = true;
    return false;
  }
  file.print(document.c_str());
  file.flush();
  const bool complete = file.getWriteError() == 0;
  file.close();
  if (!complete) {
    SD.remove(tempPath.c_str());
    statusMessage = "Save failed — card write error";
    uiNeedsRender = true;
    return false;
  }
  const String backupPath = String(currentPath.c_str()) + ".bak";
  SD.remove(backupPath.c_str());
  const bool hadOriginal = SD.exists(currentPath.c_str());
  if (hadOriginal && !SD.rename(currentPath.c_str(), backupPath.c_str())) {
    SD.remove(tempPath.c_str());
    statusMessage = "Save failed — staging error";
    uiNeedsRender = true;
    return false;
  }
  if (!SD.rename(tempPath.c_str(), currentPath.c_str())) {
    if (hadOriginal) SD.rename(backupPath.c_str(), currentPath.c_str());
    SD.remove(tempPath.c_str());
    statusMessage = "Save failed — could not replace file";
    uiNeedsRender = true;
    return false;
  }
  if (hadOriginal) {
    // The outgoing copy is the snapshot source; the .bak itself is temporary.
    saveVersionSnapshot(backupPath);
    SD.remove(backupPath.c_str());
  }
  dirty = false;
  pushRecent(currentPath, cursor);
  statusMessage = "Saved " + fileName;
  uiNeedsRender = true;
  return true;
}

void scanMarkdownFiles() {
  markdownFiles.clear();
  if (!cardAvailable || !directoryReady) return;
  File dir = SD.open("/WriterInAWell");
  if (!dir || !dir.isDirectory()) {
    if (dir) dir.close();
    return;
  }
  for (File entry = dir.openNextFile(); entry; entry = dir.openNextFile()) {
    if (!entry.isDirectory()) {
      String name = entry.name();
      const int slash = name.lastIndexOf('/');
      if (slash >= 0) name = name.substring(slash + 1);
      if (isEditableFileName(name)) {
        markdownFiles.push_back(name);
      }
    }
    entry.close();
  }
  dir.close();
  std::sort(markdownFiles.begin(), markdownFiles.end(),
            [](const String &a, const String &b) { return a < b; });
}

String safeDocumentName(String name) {
  if (name.length() == 0 || name.length() > 64 ||
      name.indexOf('/') >= 0 || name.indexOf('\\') >= 0 ||
      name == "." || name == "..") return "";
  for (size_t i = 0; i < name.length(); ++i) {
    const char c = name[i];
    if (!(isalnum(static_cast<unsigned char>(c)) || c == ' ' || c == '-' ||
          c == '_' || c == '.')) return "";
  }
  DocLang ignored = DocLang::PlainText;
  if (!docLangForName(std::string(name.c_str(), name.length()), ignored)) {
    // A bare name takes the current language's extension, so ":w notes"
    // writes notes.md while Markdown is selected and notes.py under Python.
    if (name.indexOf('.') >= 0) return "";
    name += LANG_ENTRIES[static_cast<size_t>(editorFileType)].ext;
  }
  return name;
}

void handleTransferHome() {
  // Self-contained page: no external fonts, scripts, or images. The inline SVG
  // sprite uses currentColor so every glyph follows the device palette. The
  // palette and 1 px borders mirror the on-device UI (#11111B screen, #181825
  // header, #1E1E2E controls, #313244/#45475A borders, #96A57E accent).
  static const char page[] = R"WIAW(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>WriterInAWell · Documents</title>
<style>
  :root{
    --bg:#11111B; --panel:#181825; --card:#1E1E2E;
    --line:#313244; --line2:#45475A;
    --text:#CDD6F4; --text2:#BAC2DE; --muted:#9399B2;
    --green:#96A57E; --green2:#96A57E; --blue:#89B4FA;
    --yellow:#F9E2AF; --red:#F38BA8;
    --sans:system-ui,-apple-system,"Segoe UI",Roboto,"Helvetica Neue",Arial,sans-serif;
    --mono:ui-monospace,"SFMono-Regular",Menlo,Consolas,"Liberation Mono",monospace;
  }
  *{box-sizing:border-box}
  html,body{margin:0;padding:0}
  body{background:var(--bg);color:var(--text);font:14px/1.45 var(--sans);min-height:100vh}
  [hidden]{display:none!important}
  a{color:var(--green)}
  .wrap{max-width:980px;margin:0 auto;padding:20px 16px 48px}
  .appbar{display:flex;align-items:center;gap:12px;background:var(--panel);border:1px solid var(--line);border-radius:3px;padding:12px 16px;flex-wrap:wrap}
  .brand{display:flex;align-items:center;gap:10px;min-width:0;flex:1 1 180px}
  .brand > div{min-width:0}
  .logo{width:32px;height:32px;flex:0 0 auto;border:1px solid var(--line2);border-radius:3px;display:grid;place-items:center;color:var(--green);background:var(--card)}
  .brand h1{font:600 16px/1.2 var(--sans);margin:0;color:var(--green);letter-spacing:.2px;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
  .brand p{margin:2px 0 0;color:var(--muted);font-size:12px}
  .spacer{flex:1}
  .pill{display:inline-flex;align-items:center;gap:7px;border:1px solid var(--line2);border-radius:999px;padding:5px 10px;color:var(--text2);font:12px var(--mono);background:var(--card);white-space:nowrap}
  .dot{width:7px;height:7px;border-radius:50%;background:var(--green2);box-shadow:0 0 0 3px rgba(150,165,126,.18)}
  .panel{background:var(--bg);border:1px solid var(--line);border-radius:3px;margin-top:12px;overflow:hidden}
  .toolbar{display:flex;align-items:center;gap:10px;padding:10px 12px;border-bottom:1px solid var(--line);flex-wrap:wrap}
  .crumb{display:flex;align-items:center;gap:6px;color:var(--muted);font:12px var(--mono);min-width:0}
  .crumb b{color:var(--text);font-weight:600}
  .crumb .ficon{color:var(--yellow)}
  .grow{flex:1}
  .btn{display:inline-flex;align-items:center;gap:7px;background:var(--card);border:1px solid var(--line2);border-radius:3px;color:var(--text);padding:7px 11px;font:600 12px var(--sans);cursor:pointer;text-decoration:none;transition:background .12s,border-color .12s,color .12s;line-height:1}
  .btn:hover{background:#313244;border-color:var(--green)}
  .btn:focus-visible{outline:2px solid var(--green);outline-offset:2px}
  .btn.primary{color:var(--green);border-color:var(--green)}
  .btn.primary:hover{background:rgba(150,165,126,.10)}
  .btn:disabled{opacity:.55;cursor:default}
  .btn svg{width:15px;height:15px}
  .stats{display:flex;align-items:center;gap:18px;flex-wrap:wrap;padding:9px 12px;color:var(--muted);font:12px var(--mono)}
  .stats b{color:var(--text2);font-weight:600}
  .storagebar{height:3px;background:var(--card);border-top:1px solid var(--line);border-bottom:1px solid var(--line)}
  .storagebar i{display:block;height:100%;width:0;background:var(--green2);transition:width .3s}
  .list{width:100%}
  .lhead,.row{display:grid;grid-template-columns:minmax(0,1fr) 92px 180px 108px;align-items:center;gap:12px}
  .lhead{padding:9px 12px;border-bottom:1px solid var(--line);color:var(--muted);font:600 11px var(--sans);letter-spacing:.08em;text-transform:uppercase}
  .lhead .num{text-align:right}
  .row{padding:9px 12px;border-bottom:1px solid rgba(69,71,90,.6);font:13px var(--mono)}
  .row:hover{background:rgba(49,50,68,.55)}
  .row .meta{display:contents}
  .row .size{text-align:right;color:var(--text2);white-space:nowrap}
  .row .date{color:var(--text2);white-space:nowrap}
  .row .act{text-align:right;white-space:nowrap}
  .name{display:flex;align-items:center;gap:9px;min-width:0}
  .name .ico{width:16px;height:16px;flex:0 0 auto;color:var(--blue)}
  .name .lbl{overflow:hidden;text-overflow:ellipsis;white-space:nowrap;color:var(--text)}
  .name .ext{color:var(--green);font-size:11px;flex:0 0 auto}
  .dl{display:inline-flex;align-items:center;gap:6px;color:var(--green);text-decoration:none;border:1px solid var(--line2);border-radius:3px;padding:5px 9px;font:600 11px var(--sans);white-space:nowrap}
  .dl:hover{border-color:var(--green);background:rgba(150,165,126,.10)}
  .dl svg{width:13px;height:13px}
  .empty{padding:46px 16px;text-align:center;color:var(--muted)}
  .empty .big{color:var(--text2);font-size:15px;margin-bottom:4px}
  .upload{display:flex;align-items:center;gap:10px;flex-wrap:wrap;padding:12px;border-top:1px solid var(--line);background:rgba(24,24,37,.55)}
  .filepick{position:relative;overflow:hidden}
  .filepick input{position:absolute;inset:0;opacity:0;cursor:pointer}
  .filename{color:var(--muted);font:12px var(--mono);overflow:hidden;text-overflow:ellipsis;white-space:nowrap;max-width:100%;min-width:0}
  .foot{display:flex;justify-content:space-between;gap:12px;flex-wrap:wrap;color:var(--muted);font:11px var(--mono);padding:10px 12px;border-top:1px solid var(--line)}
  .toast{position:fixed;left:50%;bottom:22px;transform:translateX(-50%);background:var(--card);border:1px solid var(--line2);border-radius:3px;padding:10px 14px;color:var(--text);font:12px var(--sans);opacity:0;pointer-events:none;transition:opacity .18s;z-index:9;max-width:calc(100vw - 32px)}
  .toast.show{opacity:1}
  .toast.err{border-color:var(--red);color:var(--red)}
  .ico-sprite{position:absolute;width:0;height:0;overflow:hidden}
  @media (max-width:640px){
    .wrap{padding:12px 10px 40px}
    .brand p{display:none}
    .appbar{flex-wrap:wrap}
    .lhead{display:none}
    .row{grid-template-columns:minmax(0,1fr) auto;grid-template-areas:"name act" "meta meta";gap:6px 10px;padding:11px 12px;font-size:12px}
    .row .name{grid-area:name}
    .row .act{grid-area:act;justify-self:end}
    .row .meta{grid-area:meta;display:flex;align-items:center;gap:8px;flex-wrap:wrap}
    .row .size,.row .date{text-align:left;color:var(--muted);white-space:nowrap}
    .row .date::before{content:"·";margin-right:8px;color:var(--line2)}
    .dl span{display:none}
    .dl{padding:5px 7px}
    .stats{gap:12px}
  }
</style>
</head>
<body>
<svg class="ico-sprite" aria-hidden="true" xmlns="http://www.w3.org/2000/svg">
  <symbol id="i-book" viewBox="0 0 24 24"><path d="M4 19.5A2.5 2.5 0 0 1 6.5 17H20"/><path d="M6.5 2H20v20H6.5A2.5 2.5 0 0 1 4 19.5v-15A2.5 2.5 0 0 1 6.5 2z"/></symbol>
  <symbol id="i-folder" viewBox="0 0 24 24"><path d="M3 7a2 2 0 0 1 2-2h4l2 2h8a2 2 0 0 1 2 2v9a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2z"/></symbol>
  <symbol id="i-file" viewBox="0 0 24 24"><path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"/><path d="M14 2v6h6"/><path d="M8 13h8M8 17h5"/></symbol>
  <symbol id="i-download" viewBox="0 0 24 24"><path d="M12 3v12"/><path d="m7 10 5 5 5-5"/><path d="M4 21h16"/></symbol>
  <symbol id="i-upload" viewBox="0 0 24 24"><path d="M12 21V9"/><path d="m7 14 5-5 5 5"/><path d="M4 3h16"/></symbol>
  <symbol id="i-refresh" viewBox="0 0 24 24"><path d="M21 12a9 9 0 1 1-3-6.7"/><path d="M21 3v6h-6"/></symbol>
</svg>

<div class="wrap">
  <header class="appbar">
    <div class="brand">
      <span class="logo"><svg class="ico" width="17" height="17" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"><use href="#i-book"/></svg></span>
      <div><h1>WriterInAWell</h1><p>Document browser · microSD</p></div>
    </div>
    <div class="spacer"></div>
    <span class="pill" title="This page is served by the device"><span class="dot"></span><span id="host">192.168.4.1</span></span>
  </header>

  <section class="panel">
    <div class="toolbar">
      <div class="crumb">
        <svg class="ico ficon" width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"><use href="#i-folder"/></svg>
        <span>~</span><span>/</span><b>WriterInAWell</b>
      </div>
      <div class="grow"></div>
      <button class="btn" id="refresh" type="button" aria-label="Refresh file list"><svg class="ico" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><use href="#i-refresh"/></svg>REFRESH</button>
    </div>
    <div class="stats">
      <span><b id="count">0</b> documents</span>
      <span><b id="total">0 B</b> total</span>
      <span class="grow"></span>
      <span id="storage">—</span>
    </div>
    <div class="storagebar"><i id="bar"></i></div>

    <div class="list" id="list" hidden>
      <div class="lhead">
        <div>Name</div>
        <div class="num">Size</div>
        <div class="date" title="SD card timestamp; on-device files with no clock may show —">Modified</div>
        <div class="num"></div>
      </div>
      <div id="rows"></div>
    </div>
    <div class="empty" id="empty" hidden>
      <div class="big">No documents yet</div>
      <div>Upload a text file below to get started.</div>
    </div>

    <form class="upload" id="upform" action="/upload" method="post" enctype="multipart/form-data">
      <label class="btn primary filepick">
        <svg class="ico" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><use href="#i-upload"/></svg>
        <span>CHOOSE FILE</span>
        <input id="file" type="file" name="file" accept=".md,.markdown,.txt,.html,.htm,.css,.py,.c,.h,.cpp,.hpp,.js,.ts,.json,.sh,.yml,.yaml,.xml,.log,.ini,.conf,.toml" required>
      </label>
      <button class="btn" type="submit" id="upsubmit" disabled>UPLOAD</button>
      <span class="filename" id="fname">No file selected</span>
    </form>

    <div class="foot">
      <span>Text files · .md .txt .html .css .py .c .js .json .sh .yml</span>
      <span id="note">—</span>
    </div>
  </section>
</div>
<div class="toast" id="toast" role="status" aria-live="polite"></div>
<noscript><div class="wrap"><div class="empty">Enable JavaScript to browse documents. The upload form still works.</div></div></noscript>
<script>
(function(){
  var $ = function(id){ return document.getElementById(id); };
  $('host').textContent = location.host || '192.168.4.1';

  function fmtSize(n){
    n = Number(n) || 0;
    if (n < 1024) return n + ' B';
    if (n < 1048576) return (n/1024).toFixed(n < 10240 ? 1 : 0) + ' KB';
    if (n < 1073741824) return (n/1048576).toFixed(n < 10485760 ? 1 : 0) + ' MB';
    return (n/1073741824).toFixed(1) + ' GB';
  }
  function fmtDate(sec){
    // Below 2000-01-01 the FAT stamp is the 1980 epoch: the board has no RTC,
    // so treat it as unknown instead of showing a misleading date.
    if (!sec || sec < 946684800) return '—';
    var d = new Date(sec * 1000);
    if (isNaN(d.getTime())) return '—';
    var mobile = false;
    try { mobile = window.matchMedia('(max-width:640px)').matches; } catch (e) {}
    var opts = mobile
      ? {year:'numeric',month:'short',day:'2-digit'}
      : {year:'numeric',month:'short',day:'2-digit',hour:'2-digit',minute:'2-digit'};
    try {
      return d.toLocaleString(undefined, opts);
    } catch (e) {
      return d.toISOString().slice(0,10);
    }
  }
  function esc(s){
    return String(s).replace(/[&<>"']/g, function(c){
      return {'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c];
    });
  }
  var ICON_FILE = '<svg class="ico" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"><use href="#i-file"/></svg>';
  var ICON_DL = '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><use href="#i-download"/></svg>';

  function toast(msg, err){
    var t = $('toast');
    t.textContent = msg;
    t.className = 'toast show' + (err ? ' err' : '');
    clearTimeout(toast.timer);
    toast.timer = setTimeout(function(){ t.className = 'toast' + (err ? ' err' : ''); }, 2800);
  }

  async function load(){
    var r = await fetch('/api/files', {cache:'no-store'});
    if (!r.ok) throw new Error('HTTP ' + r.status);
    var data = await r.json();
    var files = data.files || [];
    var rows = $('rows');
    rows.innerHTML = files.map(function(f){
      var dot = f.name.lastIndexOf('.');
      var base = dot > 0 ? f.name.slice(0, dot) : f.name;
      var ext = dot > 0 ? f.name.slice(dot) : '';
      return '<div class="row">' +
        '<div class="name">' + ICON_FILE +
          '<span class="lbl">' + esc(base) + '</span>' +
          (ext ? '<span class="ext">' + esc(ext) + '</span>' : '') +
        '</div>' +
        '<div class="meta"><span class="size">' + fmtSize(f.size) + '</span>' +
          '<span class="date">' + fmtDate(f.modified) + '</span></div>' +
        '<div class="act"><a class="dl" href="/download?name=' + encodeURIComponent(f.name) + '" aria-label="Download ' + esc(f.name) + '">' + ICON_DL + '<span>DOWNLOAD</span></a></div>' +
      '</div>';
    }).join('');
    var total = 0;
    for (var i = 0; i < files.length; i++) total += Number(files[i].size) || 0;
    $('count').textContent = files.length;
    $('total').textContent = fmtSize(total);
    $('list').hidden = files.length === 0;
    $('empty').hidden = files.length !== 0;
    if (data.total) {
      var used = Number(data.used) || 0;
      var cap = Number(data.total) || 0;
      var pct = cap ? Math.min(100, Math.round(used * 100 / cap)) : 0;
      $('bar').style.width = pct + '%';
      $('storage').textContent = fmtSize(used) + ' of ' + fmtSize(cap) + ' used';
    } else {
      $('bar').style.width = '0%';
      $('storage').textContent = 'SD card';
    }
    $('note').textContent = 'Updated ' + new Date().toLocaleTimeString();
  }

  $('refresh').addEventListener('click', function(){
    load().catch(function(e){ toast('Refresh failed: ' + e.message, true); });
  });

  var input = $('file');
  input.addEventListener('change', function(){
    var has = input.files && input.files.length > 0;
    $('fname').textContent = has ? input.files[0].name : 'No file selected';
    $('upsubmit').disabled = !has;
  });

  $('upform').addEventListener('submit', async function(e){
    e.preventDefault();
    if (!input.files || !input.files.length) return;
    var btn = $('upsubmit');
    var old = btn.textContent;
    btn.disabled = true;
    btn.textContent = 'UPLOADING…';
    try {
      var fd = new FormData();
      fd.append('file', input.files[0]);
      var r = await fetch('/upload', {method:'POST', body:fd});
      var text = await r.text();
      if (!r.ok) throw new Error(text || ('HTTP ' + r.status));
      toast('Uploaded ' + input.files[0].name);
      $('upform').reset();
      $('fname').textContent = 'No file selected';
      await load();
    } catch (err) {
      toast('Upload failed: ' + err.message, true);
    } finally {
      btn.textContent = old;
      btn.disabled = !(input.files && input.files.length);
    }
  });

  window.addEventListener('focus', function(){ load().catch(function(){}); });
  load().catch(function(e){
    toast('Could not load documents: ' + e.message, true);
    $('list').hidden = true;
    $('empty').hidden = false;
    $('empty').innerHTML = '<div class="big">Could not load documents</div><div>Reload the page to try again.</div>';
  });
})();
</script>
</body>
</html>)WIAW";
  transferServer.send(200, "text/html; charset=utf-8", page);
}

void handleTransferFileList() {
  scanMarkdownFiles();
  String json;
  // Each entry is name (<=64) + size + timestamp; reserve once so the response
  // is assembled without repeated reallocations.
  json.reserve(192 + markdownFiles.size() * 128);
  json += "{\"files\":[";
  bool first = true;
  for (size_t i = 0; i < markdownFiles.size(); ++i) {
    // Only names the download/upload endpoints accept, so the browser never
    // lists a row that cannot be fetched. safeDocumentName also guarantees the
    // name has no quote or backslash, keeping the JSON valid.
    const String name = safeDocumentName(markdownFiles[i]);
    if (name.isEmpty()) continue;
    const String path = String("/WriterInAWell/") + name;
    File file = SD.open(path.c_str(), FILE_READ);
    unsigned long size = 0;
    long long modified = 0;
    if (file) {
      if (!file.isDirectory()) {
        size = static_cast<unsigned long>(file.size());
        modified = static_cast<long long>(file.getLastWrite());
      }
      file.close();
    }
    if (!first) json += ',';
    first = false;
    char num[24];
    json += "{\"name\":\"";
    json += name;
    json += "\",\"size\":";
    snprintf(num, sizeof(num), "%lu", size);
    json += num;
    json += ",\"modified\":";
    snprintf(num, sizeof(num), "%lld", modified);
    json += num;
    json += '}';
  }
  char num[24];
  json += "],\"used\":";
  snprintf(num, sizeof(num), "%llu",
           static_cast<unsigned long long>(SD.usedBytes()));
  json += num;
  json += ",\"total\":";
  snprintf(num, sizeof(num), "%llu",
           static_cast<unsigned long long>(SD.totalBytes()));
  json += num;
  json += '}';
  transferServer.send(200, "application/json", json);
}

void handleTransferDownload() {
  const String name = safeDocumentName(transferServer.arg("name"));
  if (!cardAvailable || name.isEmpty()) {
    transferServer.send(400, "text/plain", "Invalid filename");
    return;
  }
  const String path = String("/WriterInAWell/") + name;
  File file = SD.open(path.c_str(), FILE_READ);
  if (!file || file.isDirectory()) {
    if (file) file.close();
    transferServer.send(404, "text/plain", "File not found");
    return;
  }
  transferServer.sendHeader("Content-Disposition",
                            String("attachment; filename=\"") + name + "\"");
  transferServer.streamFile(file, "text/plain; charset=utf-8");
  file.close();
}

void handleTransferUploadData() {
  HTTPUpload &upload = transferServer.upload();
  const String tempPath = "/WriterInAWell/.incoming.tmp";
  switch (upload.status) {
  case UPLOAD_FILE_START:
    uploadFailed = !cardAvailable || !directoryReady;
    uploadDestination = safeDocumentName(upload.filename);
    uploadFailed |= uploadDestination.isEmpty();
    if (uploadFailed) break;
    SD.remove(tempPath.c_str());
    uploadFile = SD.open(tempPath.c_str(), FILE_WRITE);
    if (!uploadFile) uploadFailed = true;
    break;
  case UPLOAD_FILE_WRITE:
    if (!uploadFailed) {
      if (upload.totalSize + upload.currentSize > documentLimit ||
          uploadFile.write(upload.buf, upload.currentSize) != upload.currentSize) {
        uploadFailed = true;
      }
    }
    break;
  case UPLOAD_FILE_END:
    if (uploadFile) uploadFile.close();
    if (!uploadFailed) {
      const String destination = String("/WriterInAWell/") + uploadDestination;
      const String backup = destination + ".bak";
      SD.remove(backup.c_str());
      const bool hadOriginal = SD.exists(destination.c_str());
      if (hadOriginal && !SD.rename(destination.c_str(), backup.c_str())) {
        uploadFailed = true;
      }
      if (!uploadFailed && !SD.rename(tempPath.c_str(), destination.c_str())) {
        uploadFailed = true;
        if (hadOriginal) SD.rename(backup.c_str(), destination.c_str());
      }
      if (hadOriginal && !uploadFailed) SD.remove(backup.c_str());
      if (!uploadFailed && !dirty && currentPath == destination.c_str()) {
        loadDocument(destination);
      }
    }
    if (uploadFailed) SD.remove(tempPath.c_str());
    break;
  case UPLOAD_FILE_ABORTED:
    if (uploadFile) uploadFile.close();
    SD.remove(tempPath.c_str());
    uploadFailed = true;
    break;
  default:
    break;
  }
}

void handleTransferUploadComplete() {
  if (uploadFailed) transferServer.send(400, "text/plain", "Upload failed");
  else transferServer.send(201, "text/plain", "File saved to SD card");
  statusMessage = uploadFailed ? "WiFi upload failed" : "WiFi upload saved to SD";
  uiNeedsRender = true;
}

// Seed the passphrase from the eFuse MAC the first time the device boots (or
// when NVS has no value). WPA2 rejects anything shorter than 8 chars; this is
// 11. After the seed the value is user-editable and persisted.
void seedDefaultWifiPassword() {
  if (wifiApPassword[0] != '\0') return;
  snprintf(wifiApPassword, sizeof(wifiApPassword), "well-%06llX",
           static_cast<unsigned long long>(ESP.getEfuseMac() & 0xFFFFFF));
}

// Persist just the two Wi-Fi fields. savePrefs() rewrites every key, a
// noticeable NVS stall when leaving a text field; this keeps it short.
void saveWifiPrefs() {
  if (!devicePrefs.begin("wiaw", false)) return;
  devicePrefs.putString("ssid", wifiSsid);
  devicePrefs.putString("wpass", wifiApPassword);
  devicePrefs.end();
}

// The address a client should open: the live softAP address, not a guess.
void formatWifiUrl(char *out, size_t len) {
  const IPAddress ip = WiFi.softAPIP();
  snprintf(out, len, "http://%u.%u.%u.%u",
           unsigned(ip[0]), unsigned(ip[1]),
           unsigned(ip[2]), unsigned(ip[3]));
}

void startTransfer() {
  if (!cardAvailable || !directoryReady) {
    statusMessage = "WiFi sharing needs an SD card";
    uiNeedsRender = true;
    return;
  }
  if (transferEnabled) return;
  if (dirty && !saveDocument()) return;
  if (wifiSsid[0] == '\0') {
    statusMessage = "Wi-Fi name cannot be empty";
    uiNeedsRender = true;
    return;
  }
  const size_t passLen = strlen(wifiApPassword);
  if (passLen < 8 || passLen > 63) {
    statusMessage = "Password must be 8-63 characters";
    uiNeedsRender = true;
    return;
  }
  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP(wifiSsid, wifiApPassword, 1, false, 1)) {
    WiFi.mode(WIFI_OFF);
    statusMessage = "Could not start WiFi share";
    uiNeedsRender = true;
    return;
  }
  // WebServer keeps its route table across stop()/begin(), so register the
  // handlers once; re-registering on every toggle would leak handler nodes.
  static bool routesReady = false;
  if (!routesReady) {
    transferServer.on("/", HTTP_GET, handleTransferHome);
    transferServer.on("/api/files", HTTP_GET, handleTransferFileList);
    transferServer.on("/download", HTTP_GET, handleTransferDownload);
    transferServer.on("/upload", HTTP_POST, handleTransferUploadComplete,
                      handleTransferUploadData);
    transferServer.onNotFound([]() {
      transferServer.send(404, "text/plain", "Not found");
    });
    routesReady = true;
  }
  transferServer.begin();
  transferEnabled = true;
  char url[24];
  formatWifiUrl(url, sizeof(url));
  statusMessage = std::string("Share at ") + url;
  uiNeedsRender = true;
  Serial.printf("WriterInAWell WiFi: SSID %s, password %s, %s\n",
                wifiSsid, wifiApPassword, url);
}

void stopTransfer() {
  if (!transferEnabled) return;
  transferServer.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  transferEnabled = false;
  // Keep wifiApPassword so the field still shows it after sharing stops.
  statusMessage = "WiFi sharing off";
  uiNeedsRender = true;
}

// Mocha palette: peach, mauve, pink and yellow.
constexpr const char *C_HEADING = "FAB387";  // peach
constexpr const char *C_BOLD    = "CBA6F7";  // mauve
constexpr const char *C_ITALIC  = "F5C2E7";  // pink
constexpr const char *C_BULLET  = "F9E2AF";  // yellow
constexpr const char *C_CODE    = "F9E2AF";  // warm cream (inline code)
constexpr const char *C_QUOTE   = "CBA6F7";  // mauve (blockquote)
constexpr const char *C_TEXT    = "CDD6F4";  // default prose

// Heading ink by level, so "#", "##" and "###" read as a hierarchy instead of
// seven identical peach lines.
constexpr const char *C_H1 = "FAB387";  // peach
constexpr const char *C_H2 = "B5C39D";  // light moss
constexpr const char *C_H3 = "F5C2E7";  // pink
constexpr const char *C_H4 = "89B4FA";  // link blue (### and deeper)

// --- Syntax highlighting -----------------------------------------------------
//
// A visible band becomes one LVGL recolor string ("#RRGGBB text#") handed
// straight to that band's label, so a render costs work proportional to the 13
// visible bands rather than to the document. All of it runs in fixed scratch
// — one shared 1.25 KiB buffer plus one block-state byte — so no keystroke
// touches the heap.
//
// Recolor grammar (LVGL 8): "#RRGGBB text#" paints a run, "##" prints one
// literal '#', and a run may never cross a '\n' because the label parser
// restarts at every line. A band is one line by construction, so Markup always
// closes its run before the string is handed over.

// Token ink: the chrome's link blue, sage and peach, plus mauve for control
// flow and a cool grey for comments and punctuation.
constexpr const char *SX_COMMENT = "9399B2";
constexpr const char *SX_TEXT    = "CDD6F4";
constexpr const char *SX_STRING  = "B4D8CC";
constexpr const char *SX_KEYWORD = "CBA6F7";
constexpr const char *SX_NUMBER  = "F9E2AF";
constexpr const char *SX_TYPE    = "96A57E";
constexpr const char *SX_ATTR    = "89B4FA";
constexpr const char *SX_PUNCT   = "7F849C";
constexpr const char *SX_PREPROC = "FAB387";

// Multiline state carried from line to line. One byte per line is all the
// lexers need; the state at the viewport anchor is rebuilt from the document
// start (see syntaxStateAt).
constexpr uint8_t SYN_COMMENT  = 0x01;  // /* */ or <!-- -->
constexpr uint8_t SYN_JS       = 0x02;  // inside <script> in HTML
constexpr uint8_t SYN_CSS      = 0x04;  // inside <style> in HTML
constexpr uint8_t SYN_TRIPLE_D = 0x08;  // Python """ ... """
constexpr uint8_t SYN_FENCE    = 0x10;  // inside a ``` Markdown fence
constexpr uint8_t SYN_BRACE    = 0x20;  // inside a CSS { } block
constexpr uint8_t SYN_TRIPLE_S = 0x40;  // Python ''' ... '''

// Worst case per column is a literal '#' (2 bytes) or a colour change
// (1 + 1 + 6 + 1 = 9); closing a run and reopening it for a '#' costs 12.
// A band is at most editorViewportCols() = 89 glyphs at the 10 px cut, so
// 9 + 89 * 12 + 1 = 1078 bytes; 1280 leaves room for the closing '#' and
// always keeps three bytes free.
constexpr size_t MARKUP_CAPACITY = 1280;

class Markup {
 public:
  void reset() {
    len_ = 0;
    open_ = nullptr;
    scratch_[0] = '\0';
  }
  void color(const char *rgb) {
    if (open_ == rgb) return;
    if (open_) put('#');
    put('#');
    for (const char *p = rgb; *p; ++p) put(*p);
    put(' ');
    open_ = rgb;
  }
  void text(char c) {
    if (c != '#') {
      put(c);
      return;
    }
    if (!open_) {
      put('#');
      put('#');
      return;
    }
    // Inside a run a literal '#' has to close first: "#.. ## #RRGGBB ".
    const char *rgb = open_;
    open_ = nullptr;
    put('#');
    put('#');
    put('#');
    color(rgb);
  }
  const char *c_str() {
    if (open_) put('#');
    open_ = nullptr;
    scratch_[len_] = '\0';
    return scratch_;
  }

 private:
  // Three bytes always stay free so the closing '#' can never be dropped: an
  // unbalanced run makes LVGL eat the rest of the line.
  void put(char c) {
    if (len_ + 3 < MARKUP_CAPACITY) scratch_[len_++] = c;
  }
  char scratch_[MARKUP_CAPACITY] = {};
  size_t len_ = 0;
  const char *open_ = nullptr;
};

Markup markupScratch;

// The columns of one document line that a single band shows. count == 0 means
// the caller only wants the block state advanced, so nothing is written.
struct PaintWindow {
  size_t from;
  size_t count;
  Markup *out;

  void paint(char c, size_t index, const char *rgb) {
    if (!out || index < from || index >= from + count) return;
    out->color(rgb);
    out->text(c);
  }
};

bool identStart(char c) {
  return isalpha(static_cast<unsigned char>(c)) || c == '_';
}

bool identChar(char c) {
  return isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '$';
}

// Index just past the string that opens at `i`, honouring backslash escapes.
// An unterminated string stops at the line end: no lexer carries string state
// across lines, which is how an editor recovers from a stray quote.
size_t stringEnd(const char *s, size_t n, size_t i, char quote) {
  size_t j = i + 1;
  while (j < n) {
    if (s[j] == '\\' && j + 1 < n) {
      j += 2;
      continue;
    }
    if (s[j] == quote) return j + 1;
    ++j;
  }
  return n;
}

size_t findPair(const char *s, size_t n, size_t from, char a, char b) {
  for (size_t i = from; i + 1 < n; ++i) {
    if (s[i] == a && s[i + 1] == b) return i;
  }
  return n;
}

size_t findChar(const char *s, size_t n, size_t from, char c) {
  for (size_t i = from; i < n; ++i) {
    if (s[i] == c) return i;
  }
  return n;
}

size_t findSeq(const char *s, size_t n, size_t from, const char *needle,
               size_t length) {
  if (length == 0 || n < length) return n;
  for (size_t i = from; i + length <= n; ++i) {
    if (memcmp(s + i, needle, length) == 0) return i;
  }
  return n;
}

bool onlySpaceBefore(const char *s, size_t index) {
  for (size_t i = 0; i < index; ++i) {
    if (s[i] != ' ' && s[i] != '\t') return false;
  }
  return true;
}

char nextNonSpace(const char *s, size_t n, size_t from) {
  for (size_t i = from; i < n; ++i) {
    if (s[i] != ' ' && s[i] != '\t') return s[i];
  }
  return '\0';
}

// Space-wrapped keyword lookup: the probe carries a leading and trailing space,
// so "in" cannot match inside "inline".
bool inWordList(const char *word, size_t length, const char *list) {
  // 26 bytes cover the longest keyword in the tables below; a longer word is
  // simply not a keyword.
  char probe[26];
  if (length == 0 || length + 3 > sizeof(probe)) return false;
  probe[0] = ' ';
  memcpy(probe + 1, word, length);
  probe[length + 1] = ' ';
  probe[length + 2] = '\0';
  return strstr(list, probe) != nullptr;
}

bool isSyntaxPunct(char c) {
  return strchr("()[]{}<>;:,.+-*/%=&|!?^~@$\\", c) != nullptr;
}

const char *const kCppKeywords =
    " alignas alignof auto break case catch class const constexpr continue "
    "default delete do else enum explicit export extern false final for "
    "friend goto if inline mutable namespace new noexcept nullptr operator "
    "override private protected public return sizeof static static_assert "
    "static_cast dynamic_cast reinterpret_cast const_cast struct switch "
    "template this throw true try typedef typeid typename union using virtual "
    "volatile while";
const char *const kCppTypes =
    " bool char char16_t char32_t double float int long short signed size_t "
    "ssize_t uint8_t uint16_t uint32_t uint64_t int8_t int16_t int32_t int64_t "
    "unsigned void wchar_t";

const char *const kJsKeywords =
    " async await break case catch class const continue debugger default "
    "delete do else export extends false finally for from function get if "
    "import in instanceof let new null of return set static super switch this "
    "throw true try typeof undefined var void while with yield";
const char *const kJsTypes =
    " Array Boolean Date Error Map Number Object Promise Set String Symbol "
    "any boolean never number string unknown";
const char *const kJsonLiterals = " true false null ";

const char *const kPyKeywords =
    " and as assert async await break class continue def del elif else except "
    "False finally for from global if import in is lambda None nonlocal not or "
    "pass raise return True try while with yield match case";
const char *const kPyBuiltins =
    " print len range int str float list dict set tuple bool open enumerate "
    "sorted sum min max abs zip isinstance super self";

const char *const kShellKeywords =
    " if then elif else fi for while until do done case esac in function "
    "return exit local export readonly declare unset shift source alias trap "
    "break continue select time eval exec";

const char *const kYamlKeywords = " true false null yes no on off ";

// C, C++, JavaScript and JSON share one lexer; the flags pick out the pieces
// each grammar does not have.
void lexCLike(DocLang lang, const char *s, size_t n, PaintWindow &w,
              uint8_t &state) {
  const bool js = lang == DocLang::JavaScript;
  const bool json = lang == DocLang::Json;
  size_t i = 0;
  while (i < n) {
    if (state & SYN_COMMENT) {
      const size_t close = findPair(s, n, i, '*', '/');
      const size_t stop = close == n ? n : close + 2;
      for (; i < stop; ++i) w.paint(s[i], i, SX_COMMENT);
      if (close != n) state &= ~SYN_COMMENT;
      continue;
    }
    const char c = s[i];
    if (!json && c == '/' && i + 1 < n && s[i + 1] == '/') {
      for (; i < n; ++i) w.paint(s[i], i, SX_COMMENT);
      break;
    }
    if (!json && c == '/' && i + 1 < n && s[i + 1] == '*') {
      state |= SYN_COMMENT;
      w.paint('/', i, SX_COMMENT);
      w.paint('*', i + 1, SX_COMMENT);
      i += 2;
      continue;
    }
    if (!json && c == '#' && onlySpaceBefore(s, i)) {
      // A preprocessor directive owns the rest of its line, include path
      // included.
      for (; i < n; ++i) w.paint(s[i], i, SX_PREPROC);
      break;
    }
    if (c == '"' || c == '\'' || (c == '`' && js)) {
      const size_t stop = stringEnd(s, n, i, c);
      // In JSON a quoted word followed by ':' is a key, not a value.
      const char *rgb =
          (json && nextNonSpace(s, n, stop) == ':') ? SX_ATTR : SX_STRING;
      for (; i < stop; ++i) w.paint(s[i], i, rgb);
      continue;
    }
    if (isdigit(static_cast<unsigned char>(c)) &&
        (i == 0 || !identChar(s[i - 1]))) {
      size_t j = i + 1;
      while (j < n && (isalnum(static_cast<unsigned char>(s[j])) ||
                       s[j] == '.' || s[j] == '\'')) {
        ++j;
      }
      for (; i < j; ++i) w.paint(s[i], i, SX_NUMBER);
      continue;
    }
    if (identStart(c)) {
      size_t j = i + 1;
      while (j < n && identChar(s[j])) ++j;
      const char *rgb = SX_TEXT;
      if (inWordList(s + i, j - i, kCppKeywords) ||
          (js && (inWordList(s + i, j - i, kJsKeywords) ||
                  inWordList(s + i, j - i, kJsTypes))) ||
          (json && inWordList(s + i, j - i, kJsonLiterals))) {
        rgb = SX_KEYWORD;
      } else if (inWordList(s + i, j - i, kCppTypes)) {
        rgb = SX_TYPE;
      } else if (!js && !json && isupper(static_cast<unsigned char>(c))) {
        rgb = SX_TYPE;  // Type and constant names read as types
      } else if (nextNonSpace(s, n, j) == '(') {
        rgb = SX_ATTR;  // call site
      }
      for (; i < j; ++i) w.paint(s[i], i, rgb);
      continue;
    }
    w.paint(c, i, isSyntaxPunct(c) ? SX_PUNCT : SX_TEXT);
    ++i;
  }
}

void lexCss(const char *s, size_t n, PaintWindow &w, uint8_t &state) {
  size_t i = 0;
  while (i < n) {
    if (state & SYN_COMMENT) {
      const size_t close = findPair(s, n, i, '*', '/');
      const size_t stop = close == n ? n : close + 2;
      for (; i < stop; ++i) w.paint(s[i], i, SX_COMMENT);
      if (close != n) state &= ~SYN_COMMENT;
      continue;
    }
    const char c = s[i];
    if (c == '/' && i + 1 < n && s[i + 1] == '*') {
      state |= SYN_COMMENT;
      w.paint('/', i, SX_COMMENT);
      w.paint('*', i + 1, SX_COMMENT);
      i += 2;
      continue;
    }
    if (c == '"' || c == '\'') {
      const size_t stop = stringEnd(s, n, i, c);
      for (; i < stop; ++i) w.paint(s[i], i, SX_STRING);
      continue;
    }
    if (c == '#' && i + 1 < n &&
        isxdigit(static_cast<unsigned char>(s[i + 1]))) {
      size_t j = i + 1;
      while (j < n && isxdigit(static_cast<unsigned char>(s[j]))) ++j;
      for (; i < j; ++i) w.paint(s[i], i, SX_NUMBER);
      continue;
    }
    if (isdigit(static_cast<unsigned char>(c))) {
      size_t j = i + 1;
      while (j < n && (isalnum(static_cast<unsigned char>(s[j])) ||
                       s[j] == '.' || s[j] == '%')) {
        ++j;
      }
      for (; i < j; ++i) w.paint(s[i], i, SX_NUMBER);
      continue;
    }
    if (c == '@' && i + 1 < n && identStart(s[i + 1])) {
      size_t j = i + 1;
      while (j < n && identChar(s[j])) ++j;
      for (; i < j; ++i) w.paint(s[i], i, SX_KEYWORD);
      continue;
    }
    if (identStart(c) || c == '.' || c == '#') {
      size_t j = i + 1;
      while (j < n && (identChar(s[j]) || s[j] == '-')) ++j;
      // Selectors sit outside { }; inside one, a word followed by ':' is a
      // property and everything else is a value.
      const char *rgb;
      if (!(state & SYN_BRACE) || c == '.' || c == '#') {
        rgb = SX_TYPE;
      } else if (nextNonSpace(s, n, j) == ':') {
        rgb = SX_ATTR;
      } else {
        rgb = SX_TEXT;
      }
      for (; i < j; ++i) w.paint(s[i], i, rgb);
      continue;
    }
    if (c == '{') {
      state |= SYN_BRACE;
      w.paint(c, i, SX_PUNCT);
      ++i;
      continue;
    }
    if (c == '}') {
      state &= ~SYN_BRACE;
      w.paint(c, i, SX_PUNCT);
      ++i;
      continue;
    }
    w.paint(c, i, isSyntaxPunct(c) ? SX_PUNCT : SX_TEXT);
    ++i;
  }
}

bool tagNameIs(const char *s, size_t n, const char *name) {
  const size_t m = strlen(name);
  if (n != m) return false;
  for (size_t i = 0; i < m; ++i) {
    if (tolower(static_cast<unsigned char>(s[i])) != name[i]) return false;
  }
  return true;
}

void lexHtml(const char *s, size_t n, PaintWindow &w, uint8_t &state) {
  if (state & SYN_JS) {
    // A <script> body is JavaScript until its closing tag. The tag itself is
    // styled by the JavaScript pass, which is close enough at this size.
    uint8_t nested = 0;
    lexCLike(DocLang::JavaScript, s, n, w, nested);
    if (findSeq(s, n, 0, "</script", 8) != n) state &= ~SYN_JS;
    return;
  }
  if (state & SYN_CSS) {
    uint8_t nested = 0;
    lexCss(s, n, w, nested);
    if (findSeq(s, n, 0, "</style", 7) != n) state &= ~SYN_CSS;
    return;
  }
  size_t i = 0;
  while (i < n) {
    if (state & SYN_COMMENT) {
      const size_t close = findSeq(s, n, i, "-->", 3);
      const size_t stop = close == n ? n : close + 3;
      for (; i < stop; ++i) w.paint(s[i], i, SX_COMMENT);
      if (close != n) state &= ~SYN_COMMENT;
      continue;
    }
    if (s[i] == '<' && i + 3 < n && s[i + 1] == '!' && s[i + 2] == '-' &&
        s[i + 3] == '-') {
      state |= SYN_COMMENT;
      for (size_t k = 0; k < 4; ++k) w.paint(s[i + k], i + k, SX_COMMENT);
      i += 4;
      continue;
    }
    if (s[i] == '<') {
      const size_t close = findChar(s, n, i, '>');
      const size_t stop = close == n ? n : close + 1;
      size_t k = i + 1;
      w.paint('<', i, SX_PUNCT);
      const bool closing = (k < stop && s[k] == '/');
      if (closing) {
        w.paint('/', k, SX_PUNCT);
        ++k;
      }
      const size_t nameStart = k;
      while (k < stop && (isalnum(static_cast<unsigned char>(s[k])) ||
                          s[k] == '-' || s[k] == ':')) {
        ++k;
      }
      const size_t nameEnd = k;
      for (size_t p = nameStart; p < nameEnd; ++p) w.paint(s[p], p, SX_TYPE);
      // Attributes and their quoted values.
      while (k < stop) {
        const char c = s[k];
        if (c == '"' || c == '\'') {
          const size_t q = stringEnd(s, stop, k, c);
          for (; k < q; ++k) w.paint(s[k], k, SX_STRING);
          continue;
        }
        if (isalpha(static_cast<unsigned char>(c))) {
          size_t j = k;
          while (j < stop && (isalnum(static_cast<unsigned char>(s[j])) ||
                              s[j] == '-' || s[j] == ':')) {
            ++j;
          }
          for (; k < j; ++k) w.paint(s[k], k, SX_ATTR);
          continue;
        }
        w.paint(c, k, SX_PUNCT);
        ++k;
      }
      // A <script> or <style> body continues on the next line. A closing tag
      // later on the same line means the body never spans lines.
      if (!closing) {
        if (tagNameIs(s + nameStart, nameEnd - nameStart, "script") &&
            findSeq(s, n, stop, "</script", 8) == n) {
          state |= SYN_JS;
        } else if (tagNameIs(s + nameStart, nameEnd - nameStart, "style") &&
                   findSeq(s, n, stop, "</style", 7) == n) {
          state |= SYN_CSS;
        }
      }
      i = stop;
      continue;
    }
    w.paint(s[i], i, SX_TEXT);
    ++i;
  }
}

void lexPython(const char *s, size_t n, PaintWindow &w, uint8_t &state) {
  size_t i = 0;
  bool expectName = false;
  while (i < n) {
    if (state & (SYN_TRIPLE_D | SYN_TRIPLE_S)) {
      const char *delim = (state & SYN_TRIPLE_D) ? "\"\"\"" : "'''";
      const size_t close = findSeq(s, n, i, delim, 3);
      const size_t stop = close == n ? n : close + 3;
      for (; i < stop; ++i) w.paint(s[i], i, SX_STRING);
      if (close != n) state &= ~(SYN_TRIPLE_D | SYN_TRIPLE_S);
      continue;
    }
    const char c = s[i];
    if (c == '#') {
      for (; i < n; ++i) w.paint(s[i], i, SX_COMMENT);
      break;
    }
    if (c == '"' || c == '\'') {
      // Triple quotes open a block that can run past this line.
      if (i + 2 < n && s[i + 1] == c && s[i + 2] == c) {
        state |= (c == '"') ? SYN_TRIPLE_D : SYN_TRIPLE_S;
        const char *delim = (c == '"') ? "\"\"\"" : "'''";
        const size_t close = findSeq(s, n, i + 3, delim, 3);
        const size_t tripleStop = close == n ? n : close + 3;
        for (; i < tripleStop; ++i) w.paint(s[i], i, SX_STRING);
        if (close != n) state &= ~(SYN_TRIPLE_D | SYN_TRIPLE_S);
        continue;
      }
      const size_t stop = stringEnd(s, n, i, c);
      for (; i < stop; ++i) w.paint(s[i], i, SX_STRING);
      continue;
    }
    if (c == '@' && i + 1 < n && identStart(s[i + 1])) {
      size_t j = i + 1;
      while (j < n && identChar(s[j])) ++j;
      for (; i < j; ++i) w.paint(s[i], i, SX_ATTR);
      continue;
    }
    if (isdigit(static_cast<unsigned char>(c))) {
      size_t j = i + 1;
      while (j < n && (isalnum(static_cast<unsigned char>(s[j])) ||
                       s[j] == '.' || s[j] == '_')) {
        ++j;
      }
      for (; i < j; ++i) w.paint(s[i], i, SX_NUMBER);
      continue;
    }
    if (identStart(c)) {
      size_t j = i + 1;
      while (j < n && identChar(s[j])) ++j;
      const char *rgb;
      if (expectName) {
        rgb = SX_TYPE;  // the name after def/class
      } else if (inWordList(s + i, j - i, kPyKeywords)) {
        rgb = SX_KEYWORD;
      } else if (inWordList(s + i, j - i, kPyBuiltins)) {
        rgb = SX_ATTR;
      } else {
        rgb = SX_TEXT;
      }
      expectName = inWordList(s + i, j - i, " def class ");
      for (; i < j; ++i) w.paint(s[i], i, rgb);
      continue;
    }
    w.paint(c, i, isSyntaxPunct(c) ? SX_PUNCT : SX_TEXT);
    ++i;
  }
}

void lexShell(const char *s, size_t n, PaintWindow &w) {
  size_t i = 0;
  while (i < n) {
    const char c = s[i];
    if (c == '#' && (i == 0 || s[i - 1] == ' ' || s[i - 1] == '\t')) {
      for (; i < n; ++i) w.paint(s[i], i, SX_COMMENT);
      break;
    }
    if (c == '\'' || c == '"' || c == '`') {
      const size_t stop = stringEnd(s, n, i, c);
      for (; i < stop; ++i) w.paint(s[i], i, SX_STRING);
      continue;
    }
    if (c == '$' && i + 1 < n) {
      size_t j = i + 1;
      if (s[j] == '{') {
        j = findChar(s, n, j, '}');
        j = (j == n) ? n : j + 1;
      } else {
        while (j < n && identChar(s[j])) ++j;
      }
      for (; i < j; ++i) w.paint(s[i], i, SX_NUMBER);
      continue;
    }
    if (c == '-' && i + 1 < n &&
        isalpha(static_cast<unsigned char>(s[i + 1])) &&
        (i == 0 || s[i - 1] == ' ' || s[i - 1] == '\t')) {
      size_t j = i + 1;
      while (j < n && (identChar(s[j]) || s[j] == '-')) ++j;
      for (; i < j; ++i) w.paint(s[i], i, SX_PUNCT);
      continue;
    }
    if (identStart(c)) {
      size_t j = i + 1;
      while (j < n && identChar(s[j])) ++j;
      const char *rgb =
          inWordList(s + i, j - i, kShellKeywords) ? SX_KEYWORD : SX_TEXT;
      for (; i < j; ++i) w.paint(s[i], i, rgb);
      continue;
    }
    if (isdigit(static_cast<unsigned char>(c))) {
      size_t j = i + 1;
      while (j < n && (isalnum(static_cast<unsigned char>(s[j])) ||
                       s[j] == '.')) {
        ++j;
      }
      for (; i < j; ++i) w.paint(s[i], i, SX_NUMBER);
      continue;
    }
    w.paint(c, i, isSyntaxPunct(c) ? SX_PUNCT : SX_TEXT);
    ++i;
  }
}

void lexYaml(const char *s, size_t n, PaintWindow &w) {
  size_t i = 0;
  while (i < n) {
    const char c = s[i];
    if (c == '#' && (i == 0 || s[i - 1] == ' ' || s[i - 1] == '\t')) {
      for (; i < n; ++i) w.paint(s[i], i, SX_COMMENT);
      break;
    }
    if (c == '"' || c == '\'') {
      const size_t stop = stringEnd(s, n, i, c);
      for (; i < stop; ++i) w.paint(s[i], i, SX_STRING);
      continue;
    }
    if (c == '-' && (i + 1 == n || s[i + 1] == ' ') && onlySpaceBefore(s, i)) {
      w.paint(c, i, SX_PUNCT);
      ++i;
      continue;
    }
    if (identStart(c)) {
      size_t j = i + 1;
      while (j < n && (identChar(s[j]) || s[j] == '-' || s[j] == '.')) ++j;
      const char *rgb;
      if (inWordList(s + i, j - i, kYamlKeywords)) {
        rgb = SX_KEYWORD;
      } else if (nextNonSpace(s, n, j) == ':') {
        rgb = SX_ATTR;  // mapping key
      } else {
        rgb = SX_TEXT;
      }
      for (; i < j; ++i) w.paint(s[i], i, rgb);
      continue;
    }
    if (isdigit(static_cast<unsigned char>(c))) {
      size_t j = i + 1;
      while (j < n && (isalnum(static_cast<unsigned char>(s[j])) ||
                       s[j] == '.' || s[j] == ':' || s[j] == '-')) {
        ++j;
      }
      for (; i < j; ++i) w.paint(s[i], i, SX_NUMBER);
      continue;
    }
    w.paint(c, i, isSyntaxPunct(c) ? SX_PUNCT : SX_TEXT);
    ++i;
  }
}

// Markdown: the span grammar the editor shipped with, plus heading levels,
// links, task boxes, strikethrough, table pipes and real fence state. A span
// still only opens when a closer exists later on the same line, so the result
// is always balanced; the window decides which columns are written, which also
// lets a wrapped band pick up emphasis that started on the band above.
void lexMarkdown(const char *s, size_t n, PaintWindow &w, uint8_t &state) {
  size_t lead = 0;
  while (lead < n && s[lead] == ' ') ++lead;
  const bool fenceLine = lead + 2 < n && s[lead] == '`' &&
                         s[lead + 1] == '`' && s[lead + 2] == '`';
  if (state & SYN_FENCE) {
    if (fenceLine) state &= ~SYN_FENCE;
    for (size_t i = 0; i < n; ++i) w.paint(s[i], i, C_CODE);
    return;
  }
  if (fenceLine) {
    state |= SYN_FENCE;
    for (size_t i = 0; i < n; ++i) w.paint(s[i], i, SX_PUNCT);
    return;
  }

  // The ink every plain character of this line takes: quoted text stays
  // purple and fenced code keeps the code tint, exactly like the heading
  // levels above. docLineBaseColor() agrees, so the label base and the spans
  // never disagree about an unpainted column.
  const char *ink = (lead < n && s[lead] == '>') ? C_QUOTE : C_TEXT;

  auto range = [&](size_t from, size_t to, const char *rgb) {
    for (size_t k = from; k < to && k < n; ++k) w.paint(s[k], k, rgb);
  };

  // A horizontal rule: three or more of the same marker, nothing else.
  {
    char mark = 0;
    size_t count = 0;
    bool ok = true;
    for (size_t k = lead; k < n; ++k) {
      const char c = s[k];
      if (c == ' ' || c == '\t') continue;
      if (c != '-' && c != '*' && c != '_') {
        ok = false;
        break;
      }
      if (mark == 0) mark = c;
      else if (c != mark) {
        ok = false;
        break;
      }
      ++count;
    }
    if (ok && count >= 3) {
      for (size_t k = 0; k < n; ++k) w.paint(s[k], k, SX_PUNCT);
      return;
    }
  }

  // Indent first: every column has to reach the label, or the line's leading
  // spaces would vanish from the rendered text.
  for (size_t k = 0; k < lead; ++k) w.paint(s[k], k, ink);
  size_t i = lead;

  // Heading: the hashes are punctuation, the text takes the level's ink.
  if (i < n && s[i] == '#') {
    size_t level = 0;
    while (i + level < n && s[i + level] == '#' && level < 6) ++level;
    const char *rgb = level == 1 ? C_H1
                      : level == 2 ? C_H2
                      : level == 3 ? C_H3
                                   : C_H4;
    range(i, i + level, SX_PUNCT);
    for (size_t k = i + level; k < n; ++k) w.paint(s[k], k, rgb);
    return;
  }
  // Blockquote carets; the quoted text falls through to the span grammar.
  while (i < n && s[i] == '>') {
    w.paint(s[i], i, SX_PUNCT);
    ++i;
  }

  const bool tableRow = lead < n && s[lead] == '|';

  // Spaces between the carets and the first word.
  size_t textStart = i;
  while (textStart < n && (s[textStart] == ' ' || s[textStart] == '\t')) {
    ++textStart;
  }
  for (size_t k = i; k < textStart; ++k) w.paint(s[k], k, ink);
  size_t bulletLen = 0;
  if (textStart < n) {
    const char m = s[textStart];
    if ((m == '-' || m == '+' || m == '*') && textStart + 1 < n &&
        s[textStart + 1] == ' ') {
      bulletLen = 1;
    } else if (isdigit(static_cast<unsigned char>(m))) {
      size_t j = textStart;
      while (j < n && isdigit(static_cast<unsigned char>(s[j]))) ++j;
      if (j < n && (s[j] == '.' || s[j] == ')') && j + 1 < n &&
          s[j + 1] == ' ') {
        bulletLen = j - textStart + 1;
      }
    }
  }
  if (bulletLen) {
    range(textStart, textStart + bulletLen, C_BULLET);
    i = textStart + bulletLen;
    // Task box: "- [ ]" or "- [x]".
    if (i + 2 < n && s[i] == '[' && s[i + 2] == ']') {
      const char box = s[i + 1];
      const char *rgb = box == ' ' ? SX_PUNCT
                        : (box == 'x' || box == 'X') ? SX_TYPE
                                                     : ink;
      range(i, i + 3, rgb);
      i += 3;
    }
  } else {
    i = textStart;
  }

  while (i < n) {
    const char c = s[i];
    if (c == '\\' && i + 1 < n) {
      w.paint(c, i, ink);
      w.paint(s[i + 1], i + 1, ink);
      i += 2;
      continue;
    }
    // Bold **...** or __...__
    if ((c == '*' || c == '_') && i + 1 < n && s[i + 1] == c) {
      const char *mark = c == '*' ? "**" : "__";
      const size_t close = findSeq(s, n, i + 2, mark, 2);
      if (close != n && close > i + 2) {
        range(i, close + 2, C_BOLD);
        i = close + 2;
        continue;
      }
    }
    // Strikethrough ~~...~~
    if (c == '~' && i + 1 < n && s[i + 1] == '~') {
      const size_t close = findSeq(s, n, i + 2, "~~", 2);
      if (close != n && close > i + 2) {
        range(i, close + 2, SX_PUNCT);
        i = close + 2;
        continue;
      }
    }
    // Italic *...* or _..._; underscores only at a word boundary so
    // snake_case names stay plain.
    if (c == '*' || c == '_') {
      const bool boundary =
          c == '*' || i == 0 ||
          !isalnum(static_cast<unsigned char>(s[i - 1]));
      if (boundary) {
        const size_t close = findChar(s, n, i + 1, c);
        if (close != n && close > i + 1 &&
            !(close + 1 < n && s[close + 1] == c)) {
          range(i, close + 1, C_ITALIC);
          i = close + 1;
          continue;
        }
      }
    }
    // Inline code `...`
    if (c == '`') {
      const size_t close = findChar(s, n, i + 1, '`');
      if (close != n && close > i + 1) {
        range(i, close + 1, C_CODE);
        i = close + 1;
        continue;
      }
    }
    // Link [text](target): the text in link blue, the target dimmed.
    if (c == '[') {
      const size_t mid = findChar(s, n, i + 1, ']');
      if (mid != n && mid + 1 < n && s[mid + 1] == '(') {
        const size_t end = findChar(s, n, mid + 2, ')');
        if (end != n) {
          w.paint('[', i, SX_PUNCT);
          range(i + 1, mid, SX_ATTR);
          w.paint(']', mid, SX_PUNCT);
          range(mid + 1, end + 1, SX_COMMENT);
          i = end + 1;
          continue;
        }
      }
    }
    if (c == '|' && tableRow) {
      w.paint(c, i, SX_PUNCT);
      ++i;
      continue;
    }
    w.paint(c, i, ink);
    ++i;
  }
}

void lexLine(DocLang lang, const char *s, size_t n, size_t from, size_t count,
             uint8_t &state, Markup *out) {
  PaintWindow w{from, count, out};
  switch (lang) {
  case DocLang::Markdown: lexMarkdown(s, n, w, state); break;
  case DocLang::Html: lexHtml(s, n, w, state); break;
  case DocLang::Css: lexCss(s, n, w, state); break;
  case DocLang::Python: lexPython(s, n, w, state); break;
  case DocLang::Cpp:
  case DocLang::JavaScript:
  case DocLang::Json: lexCLike(lang, s, n, w, state); break;
  case DocLang::Shell: lexShell(s, n, w); break;
  case DocLang::Yaml: lexYaml(s, n, w); break;
  case DocLang::PlainText:
  default:
    for (size_t i = 0; i < n; ++i) w.paint(s[i], i, SX_TEXT);
    break;
  }
}

// Advances the block state across one whole line without rendering it.
uint8_t syntaxAdvance(DocLang lang, const char *s, size_t n, uint8_t state) {
  lexLine(lang, s, n, 0, 0, state, nullptr);
  return state;
}

// Recolored text for columns [from, from + count) of one line. The returned
// pointer is the shared scratch buffer; the label copies it immediately.
const char *syntaxRender(DocLang lang, const char *s, size_t n, size_t from,
                         size_t count, uint8_t state) {
  markupScratch.reset();
  lexLine(lang, s, n, from, count, state, &markupScratch);
  return markupScratch.c_str();
}

// How far above the viewport the state rebuild looks, in bytes. Past this the
// state is taken as closed: an unterminated comment 300 KiB up must not make
// every keystroke walk the whole file.
constexpr size_t SYNTAX_SCAN_BYTES = 192 * 1024;

bool mayOpenBlock(const char *s, size_t n) {
  for (size_t i = 0; i < n; ++i) {
    switch (s[i]) {
    case '/':
    case '*':
    case '"':
    case '\'':
    case '`':
    case '<':
      return true;
    default:
      break;
    }
  }
  return false;
}

// Block state at the first byte of `line`. A construct can open anywhere
// above, so the scan starts at the document start; lines with nothing that
// could open one are skipped without lexing.
// Cached scanner position: the block state at the start of `line`, valid for
// one document stamp and one grammar. Scrolling a line at a time walks forward
// from the last position instead of rescanning the whole prefix per keypress;
// only a jump or an edit resets it to the document start.
struct SyntaxMemo {
  uint32_t stamp;
  DocLang lang;
  size_t line;
  uint8_t state;
};
SyntaxMemo syntaxMemo = {0, DocLang::PlainText, 0, 0};
// Most lines one incremental step may walk. A scroll step is one line; a jump
// (page down, search hit) re-anchors from line 0 instead of walking.
constexpr size_t SYNTAX_MEMO_MAX_STEP = 256;

uint8_t syntaxStateAt(DocLang lang, size_t line) {
  if (lang == DocLang::PlainText || lang == DocLang::Json ||
      lang == DocLang::Yaml) {
    return 0;  // no construct outlives a line in these grammars
  }
  ensureLineIndex();
  // Clamp to the last real line so the walk always terminates.
  const size_t lastLine = lineStarts.empty() ? 0 : lineStarts.size() - 1;
  const size_t target = std::min(line, lastLine);

  if (syntaxMemo.stamp != documentEditStamp || syntaxMemo.lang != lang ||
      target < syntaxMemo.line ||
      target - syntaxMemo.line > SYNTAX_MEMO_MAX_STEP) {
    syntaxMemo.stamp = documentEditStamp;
    syntaxMemo.lang = lang;
    syntaxMemo.line = 0;
    syntaxMemo.state = 0;
  }
  const bool fromZero = syntaxMemo.line == 0;
  size_t ln = syntaxMemo.line;
  uint8_t state = syntaxMemo.state;
  // A walk from the document start is bounded by SYNTAX_SCAN_BYTES so an
  // unterminated comment far above cannot make a render read the whole file;
  // past the cap the state is taken as closed, exactly as before. Incremental
  // walks are bounded by SYNTAX_MEMO_MAX_STEP lines instead.
  while (ln < target) {
    const size_t pos = lineStarts[ln];
    if (fromZero && pos >= SYNTAX_SCAN_BYTES) {
      ln = target;  // pin the capped state so repeat renders are free
      break;
    }
    const size_t nl = document.find('\n', pos);
    const size_t stop = nl == std::string::npos ? document.size() : nl;
    if (state || mayOpenBlock(document.data() + pos, stop - pos)) {
      state = syntaxAdvance(lang, document.data() + pos, stop - pos, state);
    }
    ++ln;
  }
  syntaxMemo.line = ln;
  syntaxMemo.state = state;
  return state;
}

// Base ink under a line's inline spans, so every band of a wrapped heading,
// quote or fence keeps the line's colour.
const char *docLineBaseColor(DocLang lang, const char *s, size_t n,
                             uint8_t stateAtStart) {
  if (lang != DocLang::Markdown) return C_TEXT;
  if (stateAtStart & SYN_FENCE) return C_CODE;
  size_t i = 0;
  while (i < n && s[i] == ' ') ++i;
  if (i + 2 < n && s[i] == '`' && s[i + 1] == '`' && s[i + 2] == '`') {
    return C_CODE;
  }
  if (i < n && s[i] == '#') {
    size_t level = 0;
    while (i + level < n && s[i + level] == '#' && level < 6) ++level;
    if (level == 1) return C_H1;
    if (level == 2) return C_H2;
    if (level == 3) return C_H3;
    return C_H4;
  }
  if (i < n && s[i] == '>') return C_QUOTE;
  return C_TEXT;
}

// --- Soft wrap and the horizontal viewport -----------------------------------
//
// Word wrap is display-only: long document lines fold onto extra screen
// bands without inserting a '\n', and folded bands keep the gutter number
// of the line they started from. With wrap off, the viewport scrolls
// sideways instead of truncating. Both modes derive their column window
// from the same measured monospace advance, so a full band always fits its
// label.

// Columns of editor text that fit one screen band for the current gutter
// state. The 4 px matches the label's left pad (see the caret x math).
int editorViewportCols() {
  const int textW = editorLineNumbers ? (SCREEN_W - 28 - 34) : (SCREEN_W - 28);
  const int glyph = editorGlyphWidth ? editorGlyphWidth : 7;
  return (textW - 4) / glyph;
}

// End column of the soft-wrap segment that starts at `from` on a line of
// `len` columns. Greedy: break after the last space that keeps the segment
// inside `cols` columns; a word longer than the band breaks hard at the
// edge so the text always advances.
size_t wrapSegmentEnd(const char *line, size_t len, size_t from, int cols) {
  size_t byte = from;
  int col = 0;
  size_t lastSpace = from;  // byte index just after the last space
  while (byte < len && col < cols) {
    if (line[byte] == ' ') lastSpace = byte + 1;
    byte = utf8Next(line, len, byte);
    ++col;
  }
  if (byte >= len) return len;
  return lastSpace > from ? lastSpace : byte;
}

// Start byte of the soft-wrap segment holding codepoint column `colIn` on line
// `ln`. Byte and column indices are tracked separately now that a glyph can be
// several bytes.
size_t wrapSegmentStart(size_t ln, size_t colIn, int cols) {
  const size_t start = offsetForLine(ln);
  const size_t len = lineEnd(start) - start;
  const char *line = document.data() + start;
  size_t byte = 0;
  size_t col = 0;
  while (byte < len) {
    const size_t segEnd = wrapSegmentEnd(line, len, byte, cols);
    const size_t segCols = utf8Columns(line + byte, segEnd - byte);
    // A column exactly at a soft break belongs to the next segment, so
    // keep walking while the break sits strictly before the column.
    if (colIn < col + segCols || segEnd >= len) return byte;
    byte = segEnd;
    col += segCols;
  }
  return byte;
}

// Walk the wrap viewport anchor up by `steps` bands: each step lands on the
// previous soft break, crossing to the last segment of the line above when
// the anchor already sits at a line start.
void walkBackDisplayRows(int steps, int cols) {
  while (steps > 0) {
    if (viewportWrapCol > 0) {
      // viewportWrapCol is a byte offset; wrapSegmentStart takes a column.
      // Clamp it to this line first: a stale anchor from a longer previous
      // line must not make the column scan read past the document.
      const size_t lineStartOff = offsetForLine(viewportStart);
      const size_t lineLen = lineEnd(lineStartOff) - lineStartOff;
      const size_t anchor = std::min(viewportWrapCol, lineLen);
      const size_t colIn = anchor > 0
          ? utf8Columns(document.data() + lineStartOff, anchor - 1) : 0;
      viewportWrapCol = wrapSegmentStart(viewportStart, colIn, cols);
      --steps;
    } else if (viewportStart > 0) {
      --viewportStart;
      const size_t start = offsetForLine(viewportStart);
      const size_t len = lineEnd(start) - start;
      size_t col = 0;
      while (col < len) {
        viewportWrapCol = col;
        col = wrapSegmentEnd(document.data() + start, len, col, cols);
      }
      --steps;
    } else {
      break;
    }
  }
}

// One screen band of editor text. The render fills a fixed table of
// MAX_VISIBLE_LINES entries; the 480x320 geometry bounds it, so wrapping
// never allocates on the keystroke path.
struct EditorRow {
  size_t line;     // document line the band renders from
  size_t offset;   // byte offset of that line's start
  size_t lineLen;  // full line length in bytes
  size_t startCol; // first byte shown (wrap break, or the scroll window)
  size_t length;   // bytes shown on this band
};

// Fill `rows` with the bands from the viewport anchor and report which one
// holds the caret (`caretRow` stays editorVisibleLines when the caret sits
// below the view). Returns the number of filled bands.
int layoutEditorRows(EditorRow *rows, int cols, size_t cursorLine,
                     size_t cursorCol, size_t totalLines, size_t &caretRow,
                     size_t &caretColInRow) {
  const bool wrap = editorWordWrap;
  size_t ln = viewportStart;
  size_t col = wrap ? viewportWrapCol : 0;  // wrap: byte offset on the line
  size_t colCol = 0;                        // codepoint column of `col`
  int count = 0;
  while (count < editorVisibleLines && ln <= totalLines) {
    const size_t start = offsetForLine(ln);
    const size_t len = lineEnd(start) - start;
    // A wrap anchor can go stale past the end of a shrunken line; skip
    // forward rather than render a phantom empty band.
    if (wrap && col > 0 && col >= len) {
      ++ln;
      col = 0;
      colCol = 0;
      continue;
    }
    size_t rowStart = 0;  // bytes
    size_t rowLen = 0;    // bytes
    size_t rowCol = 0;    // codepoint column at the band start
    size_t rowCols = 0;   // codepoints shown
    if (wrap) {
      if (col == 0) colCol = 0;
      // Only the very first band can start mid-line (a restored anchor).
      if (count == 0 && col > 0) {
        colCol = utf8Columns(document.data() + start, col);
      }
      rowStart = col;
      const size_t segEnd =
          len ? wrapSegmentEnd(document.data() + start, len, col, cols) : 0;
      rowLen = segEnd - rowStart;
      rowCol = colCol;
      rowCols = utf8Columns(document.data() + start + rowStart, rowLen);
    } else {
      // No-wrap bands show one scroll window of the line. viewportScrollCol
      // is a codepoint column; convert it to the band's byte start.
      rowStart = utf8BytesForColumns(document.data() + start, len,
                                     viewportScrollCol);
      const size_t avail = len - rowStart;
      rowLen = utf8BytesForColumns(document.data() + start + rowStart, avail,
                                   size_t(cols));
      rowCol = viewportScrollCol;
      rowCols = utf8Columns(document.data() + start + rowStart, rowLen);
    }
    rows[count] = EditorRow{ln, start, len, rowStart, rowLen};
    if (ln == cursorLine) {
      if (wrap) {
        // The caret sits in the segment that starts at or before its
        // column; at a soft break that is the next band, unless the
        // break is also the line's end.
        const size_t lineCols = utf8Columns(document.data() + start, len);
        if (cursorCol >= rowCol &&
            (cursorCol < rowCol + rowCols || rowCol + rowCols >= lineCols)) {
          caretRow = size_t(count);
          caretColInRow = cursorCol - rowCol;
        }
      } else {
        caretRow = size_t(count);
        caretColInRow = cursorCol > rowCol ? cursorCol - rowCol : 0;
      }
    }
    if (wrap) {
      colCol += rowCols;
      col += rowLen;
      if (col >= len) {
        ++ln;
        col = 0;
        colCol = 0;
      }
    } else {
      ++ln;
    }
    ++count;
  }
  return count;
}

// Apply one caret blink phase. The block owns the geometry in both phases; the
// phase only swaps a filled cell for a 1 px outline of the same cell, so the
// caret never changes size or position. Style writes are guarded because a
// render calls this every keystroke while only a blink actually flips.
void setCursorPhase(bool solid) {
  if (!cursorBlock) return;
  if (cursorSolidPhase != solid) {
    cursorSolidPhase = solid;
    lv_obj_set_style_bg_opa(cursorBlock,
                            solid ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cursorBlock, solid ? 0 : 1, 0);
  }
  if (cursorGlyph) {
    // Only the solid phase draws the reversed glyph; in the outline phase the
    // band label's own character must show through the empty cell.
    const bool hide = !(solid && cursorGlyphHasText);
    if (hide != static_cast<bool>(
                    lv_obj_has_flag(cursorGlyph, LV_OBJ_FLAG_HIDDEN))) {
      if (hide) lv_obj_add_flag(cursorGlyph, LV_OBJ_FLAG_HIDDEN);
      else lv_obj_clear_flag(cursorGlyph, LV_OBJ_FLAG_HIDDEN);
    }
  }
}

void updateEditorLines() {
  // Normalize a stale or byte-stepped caret before any column math; the
  // renderer and the label measurement must never see half a glyph.
  cursor = clampCursor(cursor);
  const size_t cursorLine = lineNumber(cursor);
  const size_t cursorCol = columnFor(cursor);
  const size_t totalLines = lineNumber(document.size());
  // A delete can shrink the document below the old anchor.
  if (viewportStart > totalLines) viewportStart = totalLines;
  // The wrap anchor belongs to viewportStart's line; a shorter line can leave
  // it pointing past the text.
  if (editorWordWrap) {
    const size_t anchorStart = offsetForLine(viewportStart);
    const size_t anchorLen = lineEnd(anchorStart) - anchorStart;
    if (viewportWrapCol > anchorLen) viewportWrapCol = anchorLen;
  }
  // Wrap width and the horizontal scroll window share one measurement:
  // columns that truly fit the row label, so nothing renders past its edge.
  const int viewportCols = editorViewportCols();

  if (editorWordWrap) {
    if (editorTypewriter) {
      // Focus line: re-anchor on the caret's band and walk half a screen
      // back so the caret rides the middle band. Near the document start the
      // walk stops early and the caret simply sits higher.
      viewportStart = cursorLine;
      viewportWrapCol = wrapSegmentStart(cursorLine, cursorCol, viewportCols);
      walkBackDisplayRows(editorVisibleLines / 2, viewportCols);
    } else {
      // Wrap mode: keep the caret's band on screen. Moving up pins the
      // caret's band to the top row; the down case re-anchors after the
      // layout below finds the caret past the last band.
      const size_t caretSeg =
          wrapSegmentStart(cursorLine, cursorCol, viewportCols);
      if (cursorLine < viewportStart ||
          (cursorLine == viewportStart && caretSeg < viewportWrapCol)) {
        viewportStart = cursorLine;
        viewportWrapCol = caretSeg;
      }
    }
  } else {
    // No wrap: keep the caret's line inside the bands and follow it
    // horizontally instead. Once the caret reaches ~95% of the row width
    // the view jumps ~80% of the width, leaving the caret in the left
    // band; moving left of the window parks it ~15% from the edge.
    viewportWrapCol = 0;
    if (editorTypewriter) {
      const size_t target = size_t(editorVisibleLines / 2);
      viewportStart = cursorLine >= target ? cursorLine - target : 0;
    } else {
      if (cursorLine < viewportStart) viewportStart = cursorLine;
      if (cursorLine >= viewportStart + editorVisibleLines) {
        viewportStart = cursorLine - editorVisibleLines + 1;
      }
    }
    const size_t triggerCol = (size_t(viewportCols) * 95 + 99) / 100;
    const size_t stepCol = (size_t(viewportCols) * 80 + 50) / 100;
    const size_t restCol = (size_t(viewportCols) * 15 + 50) / 100;
    const long rel = static_cast<long>(cursorCol) -
                     static_cast<long>(viewportScrollCol);
    if (rel >= static_cast<long>(triggerCol)) {
      const long over = rel - static_cast<long>(triggerCol) + 1;
      viewportScrollCol += static_cast<size_t>(
          (over + static_cast<long>(stepCol) - 1) / static_cast<long>(stepCol) *
          static_cast<long>(stepCol));
    } else if (rel < 0) {
      viewportScrollCol = cursorCol > restCol ? cursorCol - restCol : 0;
    }
  }

  // One entry per screen band; the fixed 480x320 geometry bounds the
  // table so the render path stays allocation-free.
  EditorRow rows[MAX_VISIBLE_LINES];
  size_t caretRow = size_t(editorVisibleLines);  // sentinel: not in the bands
  size_t caretColInRow = 0;
  int rowCount = layoutEditorRows(rows, viewportCols, cursorLine, cursorCol,
                                  totalLines, caretRow, caretColInRow);
  if (editorWordWrap && caretRow == size_t(editorVisibleLines)) {
    // Caret below the view: anchor on its band and walk back so it lands
    // on the bottom band, mirroring the no-wrap line clamp above.
    viewportStart = cursorLine;
    viewportWrapCol = wrapSegmentStart(cursorLine, cursorCol, viewportCols);
    walkBackDisplayRows(editorVisibleLines - 1, viewportCols);
    rowCount = layoutEditorRows(rows, viewportCols, cursorLine, cursorCol,
                                totalLines, caretRow, caretColInRow);
  }

  // Language state at the top of the viewport. A block comment, fence,
  // <script> body or triple-quoted string can open above it, so the state is
  // rebuilt once per render (syntaxStateAt bounds the cost).
  const DocLang lang = editorFileType;
  // Plain Text is the "no styling" language: one ink, no recolor parsing.
  const bool highlight = editorHighlight && lang != DocLang::PlainText;
  uint8_t syntaxState = highlight ? syntaxStateAt(lang, viewportStart) : 0;
  // State at the start of the line being rendered; the bands of one wrapped
  // line all start from the same state.
  uint8_t lineState = syntaxState;
  const char *lineBase = C_TEXT;
  for (int row = 0; row < editorVisibleLines; ++row) {
    if (row >= rowCount) {
      labelSetTextIfChanged(lineLabels[row], "");
      // Reset the row's styling too. Clearing only the text left the old
      // cursor-line tint on bands that used to be wrapped continuation rows,
      // so an erased long line left a light block below the text.
      if (bandPaint[row].bg != 0x11111B) {
        lv_obj_set_style_bg_color(lineLabels[row], lv_color_hex(0x11111B), 0);
        lv_obj_set_style_bg_opa(lineLabels[row], LV_OPA_COVER, 0);
        bandPaint[row].bg = 0x11111B;
      }
      if (gutterLabels[row]) {
        labelSetTextIfChanged(gutterLabels[row], "");
        if (bandPaint[row].gutterInk != 0x7F849C) {
          lv_obj_set_style_text_color(gutterLabels[row],
                                      lv_color_hex(0x7F849C), 0);
          bandPaint[row].gutterInk = 0x7F849C;
        }
      }
      continue;
    }
    const EditorRow &band = rows[row];
    // Defensive clamps: a stale band after an edit must never let a string
    // constructor or a lexer read past the document.
    const size_t bandLineStart = std::min(band.offset, document.size());
    const size_t bandLineLen =
        std::min(band.lineLen, document.size() - bandLineStart);
    const size_t bandStart = std::min(band.startCol, bandLineLen);
    const size_t bandLen = std::min(band.length, bandLineLen - bandStart);
    // Line-level work runs once per document line, on its first rendered band;
    // a wrapped continuation band inherits the line's state and base ink.
    const bool firstOfLine = row == 0 || rows[row - 1].line != band.line;
    const char *lp = document.data() + bandLineStart;
    if (firstOfLine) {
      lineState = syntaxState;
      if (highlight) {
        // Advance across the whole line, not just this band, so a construct
        // that opens mid-line still colours the lines below it.
        syntaxState = syntaxAdvance(lang, lp, bandLineLen, syntaxState);
        lineBase = docLineBaseColor(lang, lp, bandLineLen, lineState);
      }
    }
    const size_t selectionStart = std::min(cursor, visualAnchor);
    const size_t selectionEnd = std::max(cursor, visualAnchor);
    const bool selected = visualMode && bandLineStart <= selectionEnd &&
                          bandLineStart + bandLineLen >= selectionStart;
    if (highlight) {
      // Label recolor mode must match the branch: the markup below relies on
      // '#RRGGBB' command parsing, and lv_label_set_recolor() early-returns
      // when unchanged, so this is free in steady state.
      lv_label_set_recolor(lineLabels[row], true);
      labelSetTextIfChanged(lineLabels[row],
          syntaxRender(lang, lp, bandLineLen, bandStart, bandLen,
                       lineState));
      // Base color under the inline spans; a band that starts inside a
      // construct gets the same ink as the band above it.
      const uint32_t ink = static_cast<uint32_t>(strtoul(lineBase, nullptr, 16));
      if (bandPaint[row].ink != ink) {
        lv_obj_set_style_text_color(lineLabels[row], lv_color_hex(ink), 0);
        bandPaint[row].ink = ink;
      }
    } else {
      // Plain view: no markup, no recolor spans, one ink color. Recolor
      // parsing must be off too: while it is on, LVGL eats a literal '#' as
      // the start of a '#RRGGBB' command and the glyph never renders.
      // The band is at most a few hundred bytes (the column window caps it),
      // so a stack copy replaces the old std::string scratch.
      char plain[512];
      lv_label_set_recolor(lineLabels[row], false);
      labelSetTextIfChanged(lineLabels[row],
          copyBounded(plain, sizeof(plain), lp + bandStart, bandLen));
      const uint32_t ink = static_cast<uint32_t>(strtoul(C_TEXT, nullptr, 16));
      if (bandPaint[row].ink != ink) {
        lv_obj_set_style_text_color(lineLabels[row], lv_color_hex(ink), 0);
        bandPaint[row].ink = ink;
      }
    }
    const lv_font_t *font = editorFont();
    if (bandPaint[row].font != font) {
      lv_obj_set_style_text_font(lineLabels[row], font, 0);
      bandPaint[row].font = font;
    }
    const uint32_t bg = selected ? 0x23304A
                        : band.line == cursorLine ? 0x181825
                                                  : 0x11111B;
    if (bandPaint[row].bg != bg) {
      lv_obj_set_style_bg_color(lineLabels[row], lv_color_hex(bg), 0);
      lv_obj_set_style_bg_opa(lineLabels[row], LV_OPA_COVER, 0);
      bandPaint[row].bg = bg;
    }
    if (gutterLabels[row]) {
      if (firstOfLine) {
        // 12 bytes covers any %u (10 digits + NUL): GCC can't bound the line
        // index, and -Wformat-truncation fires if the worst case could exceed
        // the buffer. The 512 KiB document cap tops out at ~6 digits anyway.
        char num[12];
        unsigned shown = unsigned(band.line + 1);
        // Relative mode: the caret's line keeps its absolute number, every
        // other line shows its distance from the caret (Vim hybrid style).
        if (editorRelativeNumbers && band.line != cursorLine) {
          shown = unsigned(band.line > cursorLine ? band.line - cursorLine
                                                  : cursorLine - band.line);
        }
        snprintf(num, sizeof(num), "%u", shown);
        labelSetTextIfChanged(gutterLabels[row], num);
      } else {
        // Wrapped continuation bands keep the gutter blank so the row
        // reads as a fold of the numbered line above.
        labelSetTextIfChanged(gutterLabels[row], "");
      }
      const uint32_t gutterInk =
          band.line == cursorLine ? 0xBAC2DE : 0x7F849C;
      if (bandPaint[row].gutterInk != gutterInk) {
        lv_obj_set_style_text_color(gutterLabels[row],
            lv_color_hex(gutterInk), 0);
        bandPaint[row].gutterInk = gutterInk;
      }
    }
  }

  // Position the live cursor block. Measure the rendered prefix with
  // lv_txt_get_width — the same per-glyph advance the label renderer sums —
  // and size the block to the exact cell width of the character under the
  // caret, so it always covers the glyph precisely.
  if (cursorBlock && caretRow < size_t(rowCount)) {
    const lv_font_t *font = editorFont();
    const EditorRow &band = rows[caretRow];
    // The caret's band is the wrap segment (or scroll window) that
    // contains it, so the prefix is measured from the band's own start.
    // Clamp defensively against a stale band after an edit.
    const size_t caretBandStart =
        std::min(band.offset + band.startCol, document.size());
    const size_t caretBandLen =
        std::min(band.length, document.size() - caretBandStart);
    // Bounded stack copy: one band is at most the column window's byte width
    // (a few hundred bytes) and LVGL's measuring helpers want a NUL. This used
    // to build a std::string on every render.
    char lineText[512];
    const char *text = copyBounded(lineText, sizeof(lineText),
                                   document.data() + caretBandStart,
                                   caretBandLen);
    const size_t textLen = strlen(text);
    // caretColInRow counts codepoints; lv_txt_get_width wants a byte count.
    const size_t prefixBytes =
        utf8BytesForColumns(text, textLen, caretColInRow);
    const lv_coord_t prefixW = lv_txt_get_width(
        text, static_cast<uint32_t>(prefixBytes), font, 0,
        LV_TEXT_FLAG_NONE);
    lv_coord_t cellW = editorGlyphWidth;
    const size_t nextBytes = utf8Next(text, textLen, prefixBytes);
    if (nextBytes > prefixBytes) {
      const lv_coord_t inclNext = lv_txt_get_width(
          text, static_cast<uint32_t>(nextBytes), font, 0,
          LV_TEXT_FLAG_NONE);
      cellW = inclNext - prefixW;
      if (cellW <= 0) cellW = editorGlyphWidth;
    }
    // Only touch geometry that changed: every setter invalidates the block,
    // and an unchanged caret should cost nothing.
    const int blockW = cellW;
    const int blockH = editorLineHeight - 2;
    if (blockW != cursorBlockW || blockH != cursorBlockH) {
      lv_obj_set_size(cursorBlock, blockW, blockH);
      cursorBlockW = blockW;
      cursorBlockH = blockH;
    }
    // Text origin shifts right when the line-number gutter is on.
    const int blockX = (editorLineNumbers ? 41 : 7) + 4 + prefixW;
    const int blockY =
        editorTextTop() + static_cast<int>(caretRow) * editorLineHeight;
    if (blockX != cursorBlockX || blockY != cursorBlockY) {
      lv_obj_set_pos(cursorBlock, blockX, blockY);
      cursorBlockX = blockX;
      cursorBlockY = blockY;
      if (cursorGlyph) lv_obj_set_pos(cursorGlyph, blockX, blockY);
    }
    if (!cursorBlockShown) {
      lv_obj_clear_flag(cursorBlock, LV_OBJ_FLAG_HIDDEN);
      cursorBlockShown = true;
    }
    // Reverse out the character under the caret for the solid phase. A hard
    // tab, another control byte or end of line has no glyph to lend, so the
    // solid phase is a plain block there.
    cursorGlyphHasText = false;
    const unsigned char lead =
        prefixBytes < textLen ? static_cast<unsigned char>(text[prefixBytes])
                              : 0u;
    if (cursorGlyph && lead >= 0x20 && lead != 0x7F &&
        nextBytes > prefixBytes) {
      // One codepoint is at most 4 bytes; the label needs a NUL.
      char glyph[5];
      const size_t glyphBytes = std::min<size_t>(nextBytes - prefixBytes, 4);
      memcpy(glyph, text + prefixBytes, glyphBytes);
      glyph[glyphBytes] = '\0';
      labelSetTextIfChanged(cursorGlyph, glyph);
      // Reverse video: the glyph takes the band's own background, so it reads
      // as the inverse of the cell (a selected band is tinted differently).
      const size_t selStart = std::min(cursor, visualAnchor);
      const size_t selEnd = std::max(cursor, visualAnchor);
      const bool caretBandSelected =
          visualMode && caretBandStart <= selEnd &&
          caretBandStart + caretBandLen >= selStart;
      const uint32_t invertInk = caretBandSelected ? 0x11111B : 0x181825;
      if (cursorGlyphInk != invertInk) {
        lv_obj_set_style_text_color(cursorGlyph, lv_color_hex(invertInk), 0);
        cursorGlyphInk = invertInk;
      }
      // Slack on the right so a wide glyph is never clipped by the label box.
      if (blockW + 4 != cursorGlyphW) {
        lv_obj_set_size(cursorGlyph, blockW + 4, editorLineHeight);
        cursorGlyphW = blockW + 4;
      }
      cursorGlyphHasText = true;
    }
    // Keep the block's look in step with the blink phase (a rebuild starts in
    // the solid phase, matching the styles stamped at creation).
    setCursorPhase(cursorSolidPhase);
    // No move_foreground() here: buildEditorScreen creates the block after
    // every band and gutter label, and the reversed glyph after the block, so
    // both are already above the text.
  } else if (cursorBlock) {
    // The caret is not in any visible band; never leave the block parked at
    // a stale position from an earlier, longer document.
    if (cursorBlockShown) {
      lv_obj_add_flag(cursorBlock, LV_OBJ_FLAG_HIDDEN);
      cursorBlockShown = false;
    }
    cursorGlyphHasText = false;
    if (cursorGlyph) lv_obj_add_flag(cursorGlyph, LV_OBJ_FLAG_HIDDEN);
  }

  // Breadcrumb: ~/ + [folder]sub/ + filename. Unsaved names render in italics,
  // reddish pink while dirty and pastel mint once saved. The strings only
  // change when the path or the dirty flag does, so cache them (and the
  // measured width) instead of rebuilding them on every keystroke.
  if (pathLabel && documentLabel) {
    static std::string cachedPath;
    static std::string cachedFile;
    static bool cachedDirty = false;
    static bool cached = false;
    static const lv_obj_t *cachedLabel = nullptr;
    static std::string breadPath;
    static uint16_t breadWidth = 0;
    static int documentX = -1;
    if (!cached || cachedLabel != pathLabel || cachedPath != currentPath ||
        cachedFile != fileName || cachedDirty != dirty) {
      cached = true;
      cachedLabel = pathLabel;
      cachedPath = currentPath;
      cachedFile = fileName;
      cachedDirty = dirty;
      const std::string root = "/WriterInAWell/";
      std::string rel;
      if (!currentPath.empty() && currentPath.rfind(root, 0) == 0) {
        rel = currentPath.substr(root.size());
      } else {
        rel = fileName;
      }
      const size_t slash = rel.rfind('/');
      const std::string dirPart =
          (slash == std::string::npos) ? std::string() : rel.substr(0, slash + 1);
      const std::string filePart =
          (slash == std::string::npos) ? rel : rel.substr(slash + 1);

      breadPath = "~/";
      size_t pos = 0;
      while (pos < dirPart.size()) {
        size_t sep = dirPart.find('/', pos);
        if (sep == std::string::npos) sep = dirPart.size();
        const std::string folder = dirPart.substr(pos, sep - pos);
        if (!folder.empty()) {
          breadPath += ICON_FOLDER;
          breadPath += " ";
          breadPath += folder;
          breadPath += "/";
        }
        pos = sep + 1;
      }
      labelSetTextIfChanged(pathLabel, breadPath.c_str());
      labelSetTextIfChanged(documentLabel, filePart.c_str());

      const bool unsaved = dirty || currentPath.empty();
      lv_obj_set_style_text_font(documentLabel,
          unsaved ? &elms_bold_italic_14 : &elms_bold_14, 0);
      lv_obj_set_style_text_color(documentLabel,
          unsaved ? lv_color_hex(0xF38BA8) : lv_color_hex(0xB5C39D), 0);

      breadWidth = lv_txt_get_width(breadPath.c_str(),
          static_cast<uint32_t>(breadPath.size()), &terminess_14, 0,
          LV_TEXT_FLAG_NONE);
      documentX = -1;  // the name follows the width; re-place it below
    }
    const int docX = 40 + breadWidth + 2;
    if (docX != documentX) {
      lv_obj_set_pos(documentLabel, docX, 7);
      documentX = docX;
    }
  }
}

void buildEditorScreen();
void buildBrowserScreen();
void buildStartScreen();
void buildPairingScreen();
void buildSettingsScreen();
void buildWifiScreen();
void buildMainMenuScreen();
void buildHelpScreen();
void buildHistoryScreen();
void updateStatusBar();
void refreshSettingsSectionSelection();
void rebuildSettingsPopup();
void backClicked(lv_event_t *);
void helpBackClicked(lv_event_t *);
void savePrefs();
std::string untitledFileName();
void showTypingPractice();
void showTypingSettings();
// Lock screen: defined after the settings screens (it returns to Settings on
// cancel/success) but referenced by the input dispatch above it.
void buildLockScreen();
void lockDevice();
void lockHandleText(char c);
void lockHandleSpecial(KeyKind kind);

// Case-insensitive substring match for the browser's typed filter.
bool nameMatchesFilter(const String &name, const std::string &needle) {
  if (needle.empty()) return true;
  if (needle.size() > name.length()) return false;
  for (size_t i = 0; i + needle.size() <= static_cast<size_t>(name.length());
       ++i) {
    size_t j = 0;
    while (j < needle.size() &&
           tolower(static_cast<unsigned char>(name[i + j])) ==
               tolower(static_cast<unsigned char>(needle[j]))) {
      ++j;
    }
    if (j == needle.size()) return true;
  }
  return false;
}

// The visible browser list: the subset of markdownFiles matching the filter,
// or every file while the filter is empty.
void rebuildBrowserView() {
  browserMatches.clear();
  for (size_t i = 0; i < markdownFiles.size(); ++i) {
    if (nameMatchesFilter(markdownFiles[i], browserFilter)) {
      browserMatches.push_back(i);
    }
  }
  if (browserMatches.empty()) browserSelection = 0;
  else if (browserSelection >= browserMatches.size()) {
    browserSelection = browserMatches.size() - 1;
  }
}

// File browser. fromEditor picks the BACK target: the editor (`:e`, Ctrl-O) or
// the start screen (`f`). filterArmed opens straight into filter mode so the
// first keystrokes narrow the list.
void showBrowser(bool fromEditor, bool filterArmed = false) {
  if (dirty) {
    statusMessage = "Save changes first (:w)";
    uiNeedsRender = true;
    return;
  }
  backTarget = fromEditor ? BackTarget::Editor : BackTarget::Start;
  browserFilterMode = filterArmed;
  browserFilter.clear();
  scanMarkdownFiles();
  browserSelection = 0;
  rebuildBrowserView();
  pairingVisible = settingsVisible = menuVisible = startVisible = false;
  editorVisible = false;
  buildBrowserScreen();
}

// The editor's start screen: LazyVim's dashboard shape — command keys on the
// left, recent documents on the right. Only shown while no buffer is open.
void showStartScreen() {
  backTarget = BackTarget::Menu;
  startVisible = true;
  editorVisible = false;
  menuVisible = helpVisible = typingVisible = previewVisible = false;
  settingsVisible = pairingVisible = false;
  buildStartScreen();
}

// "Text Editor" menu entry: resume the open buffer, or fall back to the start
// screen when nothing has been created or opened yet.
void openTextEditor() {
  if (!hasDocument) {
    showStartScreen();
    return;
  }
  editorVisible = true;
  menuVisible = startVisible = helpVisible = typingVisible = previewVisible = false;
  settingsVisible = pairingVisible = false;
  buildEditorScreen();
}

// Full-screen Vim cheat sheet. View-only: unlike the browser it opens even
// while the document is dirty, since it never touches the document.
void showHelp() {
  helpVisible = true;
  editorVisible = false;
  pairingVisible = settingsVisible = menuVisible = startVisible = false;
  buildHelpScreen();
}

void helpBackClicked(lv_event_t *) {
  helpVisible = false;
  editorVisible = true;
  buildEditorScreen();
}

void showSettings(bool fromEditor) {
  backTarget = fromEditor ? BackTarget::Editor : BackTarget::Menu;
  settingsVisible = true;
  pairingVisible = false;
  settingsPopup = settingsReturn = SettingsSection::None;
  settingsSectionSelection = 0;
  settingsPopupSelection = 0;
  menuVisible = startVisible = false;
  editorVisible = false;
  buildSettingsScreen();
}

// Main-menu "Share Over Wi-Fi" entry: a dedicated screen rather than a blind
// toggle, so the name, password, and live address are visible both before and
// after enabling sharing.
void showWifiScreen() {
  backTarget = BackTarget::Menu;
  wifiScreenVisible = true;
  wifiEditingField = WifiField::None;
  settingsVisible = pairingVisible = menuVisible = startVisible = false;
  editorVisible = false;
  helpVisible = typingVisible = typingSettingsVisible = previewVisible = false;
  buildWifiScreen();
}

void showMainMenu() {
  menuVisible = true;
  settingsVisible = pairingVisible = startVisible = false;
  helpVisible = typingVisible = previewVisible = false;
  historyVisible = false;
  editorVisible = false;
  buildMainMenuScreen();
}

void runMenuEntry(size_t entry) {
  menuSelection = std::min(entry, size_t(3));
  switch (entry) {
  case 0: openTextEditor(); break;
  case 1: showTypingPractice(); break;
  case 2: showWifiScreen(); break;
  case 3: showSettings(false); break;
  default: break;
  }
}

void menuEntryClicked(lv_event_t *event) {
  const intptr_t entry = reinterpret_cast<intptr_t>(lv_event_get_user_data(event));
  if (entry >= 0) runMenuEntry(static_cast<size_t>(entry));
}

void newDocument() {
  if (dirty) {
    statusMessage = "Save changes first (:w)";
    uiNeedsRender = true;
    return;
  }
  // A clean outgoing buffer still deserves its caret saved.
  rememberCurrentCursor();
  // Only Markdown gets a heading stub; an empty Python or HTML file should not
  // open with prose in it.
  document = editorFileType == DocLang::Markdown ? "# Untitled\n\n" : "";
  markDocumentChanged();
  fileName = untitledFileName();
  currentPath.clear();
  cursor = 0;
  // Fresh view: both viewport anchors reset with the cursor.
  viewportStart = viewportWrapCol = viewportScrollCol = 0;
  dirty = false;
  hasDocument = true;
  undoBuffer.clear();
  undoAvailable = false;
  redoBuffer.clear();
  redoAvailable = false;
  sessionWordBase = countDocumentWords();
  documentWordCacheValid = false;
  undoGroupActive = false;
  insertMode = visualMode = commandMode = searchMode = false;
  pendingOperator = 0;
  editorVisible = true;
  settingsVisible = pairingVisible = menuVisible = startVisible = false;
  statusMessage = "New document — press i to write";
  uiNeedsRender = true;
  buildEditorScreen();
}

// q / :q in the editor: close the buffer and return to the main menu. A clean
// buffer is discarded, so the Text Editor entry comes back to the start
// screen the way Neovim returns to its dashboard.
void quitEditor() {
  if (dirty) {
    statusMessage = "Unsaved changes — use :w";
    uiNeedsRender = true;
    return;
  }
  rememberCurrentCursor();
  hasDocument = false;
  document.clear();
  markDocumentChanged();
  undoBuffer.clear();
  undoAvailable = false;
  redoBuffer.clear();
  redoAvailable = false;
  undoGroupActive = false;
  fileName = untitledFileName();
  currentPath.clear();
  cursor = 0;
  viewportStart = viewportWrapCol = viewportScrollCol = 0;
  insertMode = visualMode = commandMode = searchMode = false;
  pendingOperator = 0;
  statusMessage = "Ready — press i to write";
  showMainMenu();
}

void fileClicked(lv_event_t *event) {
  const intptr_t index = reinterpret_cast<intptr_t>(lv_event_get_user_data(event));
  if (index < 0 || static_cast<size_t>(index) >= browserMatches.size()) return;
  loadDocument(String("/WriterInAWell/") + browserFileName(index));
  buildEditorScreen();
}

void openRecent(size_t index) {
  if (index >= recentFiles.size()) return;
  const std::string path = recentFiles[index];
  loadDocument(String(path.c_str()));
  if (editorVisible) {
    buildEditorScreen();
    return;
  }
  // The entry vanished from the card (or the card is gone): prune it instead
  // of leaving a row that can never open.
  if (!cardAvailable || !SD.exists(path.c_str())) {
    recentFiles.erase(recentFiles.begin() + index);
    if (index < recentCursors.size()) {
      recentCursors.erase(recentCursors.begin() + index);
    }
    if (recentSelection >= recentFiles.size()) recentSelection = 0;
    saveRecentFiles();
  }
  buildStartScreen();
}

void recentClicked(lv_event_t *event) {
  const intptr_t index = reinterpret_cast<intptr_t>(lv_event_get_user_data(event));
  if (index >= 0) openRecent(static_cast<size_t>(index));
}

void keyboardCandidateClicked(lv_event_t *event) {
  const intptr_t index = reinterpret_cast<intptr_t>(lv_event_get_user_data(event));
  if (index >= 0) keyboardConnect(static_cast<size_t>(index));
}

void rescanKeyboardsClicked(lv_event_t *) {
  keyboardRescan();
  pairSelection = 0;
  buildPairingScreen();
}

void disconnectKeyboardClicked(lv_event_t *) {
  keyboardDisconnect();
}

// Field editing is keyboard-driven (the board has no on-screen keyboard): tap a
// field or press "n"/"p" while sharing is off, type, then Enter/Esc to finish.
void wifiNameFieldClicked(lv_event_t *) {
  if (transferEnabled) return;  // fields lock while the AP is live
  wifiEditingField = WifiField::Name;
  buildWifiScreen();
}

void wifiPasswordFieldClicked(lv_event_t *) {
  if (transferEnabled) return;
  wifiEditingField = WifiField::Password;
  buildWifiScreen();
}

void wifiFieldAppend(char c) {
  if (c < 0x20 || c > 0x7E) return;
  if (wifiEditingField == WifiField::Name) {
    const size_t len = strlen(wifiSsid);
    if (len >= 32) return;
    wifiSsid[len] = c;
    wifiSsid[len + 1] = '\0';
  } else if (wifiEditingField == WifiField::Password) {
    const size_t len = strlen(wifiApPassword);
    if (len >= 63) return;
    wifiApPassword[len] = c;
    wifiApPassword[len + 1] = '\0';
  } else {
    return;
  }
  prefsDirty = true;
}

void wifiFieldBackspace() {
  if (wifiEditingField == WifiField::Name) {
    const size_t len = strlen(wifiSsid);
    if (len == 0) return;
    wifiSsid[len - 1] = '\0';
  } else if (wifiEditingField == WifiField::Password) {
    const size_t len = strlen(wifiApPassword);
    if (len == 0) return;
    wifiApPassword[len - 1] = '\0';
  } else {
    return;
  }
  prefsDirty = true;
}

// Repaint only the active field's value label (+ caret) while typing. A full
// buildWifiScreen() per keystroke was the source of the input lag.
void refreshWifiFieldText() {
  char shown[66];
  if (wifiEditingField == WifiField::Name && wifiNameValueLabel) {
    snprintf(shown, sizeof(shown), "%s_", wifiSsid);
    lv_label_set_text(wifiNameValueLabel, shown);
  } else if (wifiEditingField == WifiField::Password && wifiPasswordValueLabel) {
    snprintf(shown, sizeof(shown), "%s_", wifiApPassword);
    lv_label_set_text(wifiPasswordValueLabel, shown);
  }
}

// Touch toggle: start or stop sharing, then rebuild so the fields lock/unlock
// and the address appears/disappears.
void wifiSwitchChanged(lv_event_t *event) {
  lv_obj_t *sw = lv_event_get_target(event);
  wifiEditingField = WifiField::None;
  // Persist a value typed just before the toggle was flipped.
  saveWifiPrefs();
  if (lv_obj_has_state(sw, LV_STATE_CHECKED)) startTransfer();
  else stopTransfer();
  buildWifiScreen();
}

void newClicked(lv_event_t *) { newDocument(); }
void backClicked(lv_event_t *) {
  savePrefs();
  pairingVisible = settingsVisible = pairingReturnsToSettings = false;
  settingsPopup = settingsReturn = SettingsSection::None;
  historyVisible = false;
  menuVisible = false;
  if (backTarget == BackTarget::Editor) {
    editorVisible = true;
    startVisible = false;
    buildEditorScreen();
  } else if (backTarget == BackTarget::Start) {
    showStartScreen();
  } else {
    showMainMenu();
  }
}
void menuClicked(lv_event_t *) { showMainMenu(); }

void showPairingHelp() {
  // BACK returns to whichever screen opened the picker: Settings keeps its
  // own flag, the editor rides backTarget, and the main menu is the fallback
  // (a stale Start target would otherwise send BACK to the start screen).
  if (settingsVisible) {
    pairingReturnsToSettings = true;
  } else {
    pairingReturnsToSettings = false;
    backTarget = editorVisible ? BackTarget::Editor : BackTarget::Menu;
  }
  settingsVisible = false;
  pairingVisible = true;
  editorVisible = false;
  buildPairingScreen();
}
void pairClicked(lv_event_t *) { showPairingHelp(); }

void savePrefs() {
  if (!prefsDirty) return;
  if (!devicePrefs.begin("wiaw", false)) return;
  devicePrefs.putUChar("bl", brightnessValue);
  devicePrefs.putBool("smooth", editorSmoothText);
  devicePrefs.putBool("hl", editorHighlight);
  devicePrefs.putBool("as", editorAutoSave);
  devicePrefs.putBool("dim", settingsDimIdle);
  devicePrefs.putBool("ln", editorLineNumbers);
  devicePrefs.putBool("ww", editorWordWrap);
  devicePrefs.putBool("wcount", editorWordCount);
  devicePrefs.putBool("rel", editorRelativeNumbers);
  devicePrefs.putBool("zen", editorZenMode);
  devicePrefs.putBool("focus", editorTypewriter);
  devicePrefs.putBool("swrap", editorSearchWrap);
  devicePrefs.putBool("bootedit", bootIntoEditor);
  // "listcont2" deliberately starts a fresh key: the old default was off and
  // would otherwise pin upgraded units to off despite the new default.
  devicePrefs.putBool("listcont2", editorListContinue);
  devicePrefs.putBool("ptypo", editorProseTypo);
  devicePrefs.putBool("snap", snapshotsEnabled);
  devicePrefs.putUChar("vkeep", snapshotRetention);
  devicePrefs.putUChar("soff", screenOffMinutes);
  devicePrefs.putBool("lsleep", lightSleepEnabled);
  // "lockon" is the authority: a stale hash left behind by Remove password
  // must never re-enable the lock by itself.
  devicePrefs.putBool("lockon", lockEnabled);
  if (lockEnabled) {
    devicePrefs.putUInt("locksalt", lockSalt);
    devicePrefs.putUInt("lockhlo", lockHashLo);
    devicePrefs.putUInt("lockhhi", lockHashHi);
  }
  devicePrefs.putUChar("esize", editorFontSizeIndex);
  devicePrefs.putUChar("ftype", static_cast<uint8_t>(editorFileType));
  devicePrefs.putString("ssid", wifiSsid);
  devicePrefs.putString("wpass", wifiApPassword);
  devicePrefs.putUChar("tcomp", typingComplexity);
  devicePrefs.putUChar("tmax", typingMaxLengthIndex);
  devicePrefs.putBool("tsym", typingSymbols);
  devicePrefs.putBool("tnum", typingNumbers);
  devicePrefs.putBool("tcap", typingCapitalization);
  devicePrefs.putBool("tpunc", typingPunctuation);
  devicePrefs.end();
  prefsDirty = false;
}

void loadPrefs() {
  if (!devicePrefs.begin("wiaw", true)) {
    seedDefaultWifiPassword();
    return;
  }
  if (devicePrefs.isKey("bl")) {
    brightnessValue = devicePrefs.getUChar("bl", 170);
  }
  if (devicePrefs.isKey("smooth")) {
    editorSmoothText = devicePrefs.getBool("smooth", false);
  }
  if (devicePrefs.isKey("hl")) {
    editorHighlight = devicePrefs.getBool("hl", true);
  }
  if (devicePrefs.isKey("as")) {
    editorAutoSave = devicePrefs.getBool("as", true);
  }
  if (devicePrefs.isKey("dim")) {
    settingsDimIdle = devicePrefs.getBool("dim", true);
  }
  if (devicePrefs.isKey("ln")) {
    editorLineNumbers = devicePrefs.getBool("ln", false);
  }
  if (devicePrefs.isKey("ww")) {
    editorWordWrap = devicePrefs.getBool("ww", true);
  }
  if (devicePrefs.isKey("wcount")) {
    editorWordCount = devicePrefs.getBool("wcount", false);
  }
  if (devicePrefs.isKey("rel")) {
    editorRelativeNumbers = devicePrefs.getBool("rel", false);
  }
  // Zen is deliberately not restored at boot: it is a transient focus mode,
  // and a saved-on value could otherwise trap a device whose touch input
  // misbehaves. It always starts off; Ctrl-Z/two-finger re-enters it.
  editorZenMode = false;
  if (devicePrefs.isKey("focus")) {
    editorTypewriter = devicePrefs.getBool("focus", false);
  }
  if (devicePrefs.isKey("swrap")) {
    editorSearchWrap = devicePrefs.getBool("swrap", false);
  }
  if (devicePrefs.isKey("bootedit")) {
    bootIntoEditor = devicePrefs.getBool("bootedit", false);
  }
  if (devicePrefs.isKey("listcont2")) {
    editorListContinue = devicePrefs.getBool("listcont2", true);
  }
  if (devicePrefs.isKey("ptypo")) {
    editorProseTypo = devicePrefs.getBool("ptypo", false);
  }
  if (devicePrefs.isKey("snap")) {
    snapshotsEnabled = devicePrefs.getBool("snap", false);
  }
  if (devicePrefs.isKey("vkeep")) {
    const uint8_t stored = devicePrefs.getUChar("vkeep", 3);
    if (stored == 3 || stored == 5 || stored == 10) snapshotRetention = stored;
  }
  if (devicePrefs.isKey("soff")) {
    const uint8_t stored = devicePrefs.getUChar("soff", 0);
    if (stored == 0 || stored == 2 || stored == 5 || stored == 10 ||
        stored == 15) {
      screenOffMinutes = stored;
    }
  }
  if (devicePrefs.isKey("lsleep")) {
    lightSleepEnabled = devicePrefs.getBool("lsleep", false);
  }
  // The lock needs all four keys; a partially written record is treated as
  // "no password" instead of locking the device out with an unmatchable hash.
  if (devicePrefs.isKey("lockon") && devicePrefs.getBool("lockon", false) &&
      devicePrefs.isKey("locksalt") && devicePrefs.isKey("lockhlo") &&
      devicePrefs.isKey("lockhhi")) {
    lockSalt = devicePrefs.getUInt("locksalt", 0);
    lockHashLo = devicePrefs.getUInt("lockhlo", 0);
    lockHashHi = devicePrefs.getUInt("lockhhi", 0);
    lockEnabled = true;
  } else {
    lockEnabled = false;
  }
  if (devicePrefs.isKey("kbdaddr")) {
    const String stored = devicePrefs.getString("kbdaddr", "");
    if (stored.length() < sizeof(bootKeyboardAddress)) {
      snprintf(bootKeyboardAddress, sizeof(bootKeyboardAddress), "%s",
               stored.c_str());
    }
  }
  if (devicePrefs.isKey("esize")) {
    // A stale value from a future build must not index past the font tables.
    const uint8_t stored = devicePrefs.getUChar("esize", EDITOR_FONT_SIZE_DEFAULT);
    if (stored < EDITOR_FONT_SIZE_COUNT)
      editorFontSizeIndex = stored;
  }
  if (devicePrefs.isKey("ftype")) {
    const uint8_t stored = devicePrefs.getUChar("ftype", 0);
    // A stale value from a future build must not index past LANG_ENTRIES.
    if (stored < LANG_COUNT) editorFileType = static_cast<DocLang>(stored);
  }
  if (devicePrefs.isKey("ssid")) {
    const String stored = devicePrefs.getString("ssid", String(wifiSsid));
    if (stored.length() > 0 && stored.length() <= 32) {
      snprintf(wifiSsid, sizeof(wifiSsid), "%s", stored.c_str());
    }
  }
  if (devicePrefs.isKey("wpass")) {
    const String stored = devicePrefs.getString("wpass", String());
    if (stored.length() <= 63) {
      snprintf(wifiApPassword, sizeof(wifiApPassword), "%s", stored.c_str());
    }
  }
  if (devicePrefs.isKey("tcomp")) {
    typingComplexity = devicePrefs.getUChar("tcomp", TYPING_NORMAL);
  }
  if (devicePrefs.isKey("tmax")) {
    typingMaxLengthIndex = devicePrefs.getUChar("tmax", 0);
  }
  if (devicePrefs.isKey("tsym")) {
    typingSymbols = devicePrefs.getBool("tsym", false);
  }
  if (devicePrefs.isKey("tnum")) {
    typingNumbers = devicePrefs.getBool("tnum", false);
  }
  if (devicePrefs.isKey("tcap")) {
    typingCapitalization = devicePrefs.getBool("tcap", false);
  }
  if (devicePrefs.isKey("tpunc")) {
    typingPunctuation = devicePrefs.getBool("tpunc", false);
  }
  for (size_t i = 0; i < TYPING_LENGTH_COUNT; ++i) {
    if (devicePrefs.isKey(typingBestKey(i))) {
      typingBestWpm[i] = devicePrefs.getUInt(typingBestKey(i), 0);
    }
  }
  // Migrate the retired fixed-length records. tb15 was the 15-word Short
  // record; before that tb10 (or the old default tb25) seeded the short end.
  // tb50 seeds Long. New records write tb20/tblong; the old keys stay
  // untouched so a downgrade still finds them.
  if (!devicePrefs.isKey("tb20")) {
    if (devicePrefs.isKey("tb15")) {
      typingBestWpm[0] = devicePrefs.getUInt("tb15", 0);
    } else if (devicePrefs.isKey("tb10")) {
      typingBestWpm[0] = devicePrefs.getUInt("tb10", 0);
    } else if (devicePrefs.isKey("tb25")) {
      typingBestWpm[0] = devicePrefs.getUInt("tb25", 0);
    }
  }
  if (!devicePrefs.isKey("tblong") && devicePrefs.isKey("tb50")) {
    typingBestWpm[1] = devicePrefs.getUInt("tb50", 0);
  }
  devicePrefs.end();
  // First boot (or a cleared value) gets the MAC-derived default passphrase.
  seedDefaultWifiPassword();
  if (brightnessValue < BRIGHTNESS_MIN) brightnessValue = BRIGHTNESS_MIN;
  if (brightnessValue > BRIGHTNESS_MAX) brightnessValue = BRIGHTNESS_MAX;
  if (typingComplexity >= TYPING_COMPLEXITY_COUNT) typingComplexity = TYPING_NORMAL;
  if (typingMaxLengthIndex >= TYPING_MAX_LENGTH_COUNT) typingMaxLengthIndex = 0;
  loadRecentFiles();
}

// Name of a buffer that has never been saved, in the current language.
std::string untitledFileName() {
  return std::string("Untitled") +
         LANG_ENTRIES[static_cast<size_t>(editorFileType)].ext;
}

// Switch the buffer's language. This picks a grammar, not a filename: only a
// buffer still carrying the generated "Untitled" name takes the new extension,
// so opening notes.txt and picking Python never silently renames the file on
// the card.
void applyDocLang(size_t index) {
  if (index >= LANG_COUNT) return;
  const DocLang lang = static_cast<DocLang>(index);
  if (lang == editorFileType) return;
  editorFileType = lang;
  prefsDirty = true;
  if (currentPath.empty() && fileName.rfind("Untitled", 0) == 0) {
    fileName = untitledFileName();
  }
  statusMessage = std::string("File type ") + langLabel(lang);
  uiNeedsRender = true;
  savePrefs();
}

void setBrightnessValue(uint8_t value, bool fromSlider = false) {
  if (value < BRIGHTNESS_MIN) value = BRIGHTNESS_MIN;
  if (value > BRIGHTNESS_MAX) value = BRIGHTNESS_MAX;
  brightnessValue = value;
  wakeFadeActive = false;  // an explicit change owns the backlight now
  display.setBrightness(brightnessValue);
  backlightDimmed = false;
  lastInputMs = millis();
  prefsDirty = true;
  if (brightnessSlider && !fromSlider) {
    lv_slider_set_value(brightnessSlider, brightnessValue, LV_ANIM_OFF);
  }
  if (brightnessStatusLabel) {
    char pct[8];
    snprintf(pct, sizeof(pct), "%u%%",
             unsigned(brightnessValue) * 100 / 255);
    lv_label_set_text(brightnessStatusLabel, pct);
  }
  statusMessage = "Backlight " +
      std::to_string(brightnessValue * 100 / 255) + "%";
}

void cycleBrightness() {
  uint16_t next = brightnessValue + 32;
  if (next > BRIGHTNESS_MAX) next = BRIGHTNESS_MIN;
  setBrightnessValue(static_cast<uint8_t>(next));
}

// Apply one of the four editor text sizes. Recomputing the band geometry
// invalidates the wrap/scroll anchors, so both reset and the next render
// re-clamps them onto the caret (same as a word-wrap toggle). The screen is
// rebuilt so the label count, row pitch and font all switch together.
void setEditorFontSize(int index) {
  if (index < 0) index = 0;
  if (index >= EDITOR_FONT_SIZE_COUNT) index = EDITOR_FONT_SIZE_COUNT - 1;
  if (index == int(editorFontSizeIndex)) return;
  editorFontSizeIndex = static_cast<uint8_t>(index);
  refreshEditorMetrics();
  viewportWrapCol = viewportScrollCol = 0;
  prefsDirty = true;
  savePrefs();
  statusMessage = "Text size " +
      std::to_string(EDITOR_FONT_PIXELS[editorFontSizeIndex]) + " px — " +
      std::to_string(editorVisibleLines) + " lines";
  if (editorVisible) buildEditorScreen();
  else uiNeedsRender = true;
}

void stepEditorFontSize(int delta) {
  const int next = int(editorFontSizeIndex) + delta;
  if (next < 0) {
    statusMessage = "Smallest text size";
    uiNeedsRender = true;
    return;
  }
  if (next >= EDITOR_FONT_SIZE_COUNT) {
    statusMessage = "Largest text size";
    uiNeedsRender = true;
    return;
  }
  setEditorFontSize(next);
}

void editorFontSmallerClicked(lv_event_t *) { stepEditorFontSize(-1); }
void editorFontLargerClicked(lv_event_t *) { stepEditorFontSize(1); }

void brightnessSliderChanged(lv_event_t *event) {
  const int32_t value = lv_slider_get_value(lv_event_get_target(event));
  setBrightnessValue(static_cast<uint8_t>(value), true);
}

void brightnessSliderReleased(lv_event_t *) { savePrefs(); }

// Sync a settings switch widget to its backing bool. lv_obj_add_state and
// lv_obj_clear_state never emit LV_EVENT_VALUE_CHANGED in LVGL 8, so this is
// safe from the key-toggle path without re-entering the tap callbacks.
void syncSwitchState(lv_obj_t *sw, bool on) {
  if (!sw) return;
  if (on) lv_obj_add_state(sw, LV_STATE_CHECKED);
  else lv_obj_clear_state(sw, LV_STATE_CHECKED);
}

// Tap path: LVGL flipped LV_STATE_CHECKED before emitting the event, so
// adopt the switch state instead of toggling a second time. Wakes a dimmed
// backlight like every other tappable control.
void adoptSwitchTap(lv_obj_t *sw, bool &setting) {
  lastInputMs = millis();
  if (backlightDimmed) {
    display.setBrightness(brightnessValue);
    backlightDimmed = false;
  }
  setting = lv_obj_has_state(sw, LV_STATE_CHECKED);
  prefsDirty = true;
}

void toggleTextSmooth() {
  editorSmoothText = !editorSmoothText;
  prefsDirty = true;
  statusMessage = editorSmoothText ? "Text smoothing on"
                                   : "Text smoothing off (crisp)";
  syncSwitchState(smoothSwitch, editorSmoothText);
  savePrefs();
}

void smoothSwitchChanged(lv_event_t *) {
  adoptSwitchTap(smoothSwitch, editorSmoothText);
  statusMessage = editorSmoothText ? "Text smoothing on"
                                   : "Text smoothing off (crisp)";
  savePrefs();
}

void toggleMarkdownHighlight() {
  editorHighlight = !editorHighlight;
  prefsDirty = true;
  statusMessage = editorHighlight ? "Text colors on"
                                 : "Text colors off (plain text)";
  syncSwitchState(markdownSwitch, editorHighlight);
  savePrefs();
}

void markdownSwitchChanged(lv_event_t *) {
  adoptSwitchTap(markdownSwitch, editorHighlight);
  statusMessage = editorHighlight ? "Text colors on"
                                 : "Text colors off (plain text)";
  savePrefs();
}

void toggleAutoSave() {
  editorAutoSave = !editorAutoSave;
  prefsDirty = true;
  statusMessage = editorAutoSave ? "Auto-save on"
                                 : "Auto-save off (:w to save)";
  syncSwitchState(autoSaveSwitch, editorAutoSave);
  savePrefs();
}

void autoSaveSwitchChanged(lv_event_t *) {
  adoptSwitchTap(autoSaveSwitch, editorAutoSave);
  statusMessage = editorAutoSave ? "Auto-save on"
                                 : "Auto-save off (:w to save)";
  savePrefs();
}

void toggleDimIdle() {
  settingsDimIdle = !settingsDimIdle;
  prefsDirty = true;
  statusMessage = settingsDimIdle ? "Idle dimming on"
                                  : "Idle dimming off";
  syncSwitchState(dimSwitch, settingsDimIdle);
  savePrefs();
}

void dimSwitchChanged(lv_event_t *) {
  adoptSwitchTap(dimSwitch, settingsDimIdle);
  statusMessage = settingsDimIdle ? "Idle dimming on"
                                  : "Idle dimming off";
  savePrefs();
}

void toggleLineNumbers() {
  editorLineNumbers = !editorLineNumbers;
  prefsDirty = true;
  statusMessage = editorLineNumbers ? "Line numbers on"
                                    : "Line numbers off";
  syncSwitchState(lineNumberSwitch, editorLineNumbers);
  // The editor rebuilds on return (backClicked -> buildEditorScreen), which
  // creates or removes the gutter; nothing to refresh while Settings shows.
  savePrefs();
}

void lineNumberSwitchChanged(lv_event_t *) {
  adoptSwitchTap(lineNumberSwitch, editorLineNumbers);
  statusMessage = editorLineNumbers ? "Line numbers on"
                                    : "Line numbers off";
  savePrefs();
}

void toggleWordWrap() {
  editorWordWrap = !editorWordWrap;
  // Both viewport anchors go stale when the mode flips; the next render
  // re-clamps them onto the caret.
  viewportWrapCol = 0;
  viewportScrollCol = 0;
  prefsDirty = true;
  statusMessage = editorWordWrap ? "Word wrap on"
                                 : "Word wrap off";
  syncSwitchState(wordWrapSwitch, editorWordWrap);
  savePrefs();
}

void wordWrapSwitchChanged(lv_event_t *) {
  adoptSwitchTap(wordWrapSwitch, editorWordWrap);
  viewportWrapCol = 0;
  viewportScrollCol = 0;
  statusMessage = editorWordWrap ? "Word wrap on"
                                 : "Word wrap off";
  savePrefs();
}

void settingsClicked(lv_event_t *) {
  showSettings(true);
}

void backFromPairing(lv_event_t *) {
  pairingVisible = false;
  if (pairingReturnsToSettings) {
    pairingReturnsToSettings = false;
    settingsVisible = true;
    editorVisible = false;
    buildSettingsScreen();
  } else {
    backClicked(nullptr);
  }
}

lv_obj_t *makeButton(lv_obj_t *parent, const char *label, int x, int width,
                     lv_event_cb_t callback, void *userData = nullptr) {
  lv_obj_t *button = lv_btn_create(parent);
  lv_obj_set_size(button, width, 22);
  // Center the 22 px button in the 30 px header's 28 px content area
  // (30 minus the 1 px borders): (28 - 22) / 2 = 3 px above and below.
  lv_obj_set_pos(button, x, 3);
  stylePanel(button, lv_color_hex(0x1E1E2E), lv_color_hex(0x45475A));
  lv_obj_add_event_cb(button, [](lv_event_t *) {
    lastInputMs = millis();
    if (backlightDimmed) {
      display.setBrightness(brightnessValue);
      backlightDimmed = false;
    }
  }, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb(button, callback, LV_EVENT_CLICKED, userData);
  lv_obj_t *text = lv_label_create(button);
  lv_label_set_text(text, label);
  lv_obj_center(text);
  setUiFont(text);
  lv_obj_set_style_text_color(text, lv_color_hex(0xCDD6F4), 0);
  return button;
}

lv_obj_t *makeToolbarButton(lv_obj_t *parent, const char *label, int x,
                            int width, lv_event_cb_t callback,
                            bool icon = false) {
  lv_obj_t *button = lv_btn_create(parent);
  lv_obj_set_size(button, width, 28);
  // Center the 28 px button in the 36 px editor header's 34 px content area
  // (36 minus the 1 px borders): (34 - 28) / 2 = 3 px above and below. The
  // taller bar is the reason this is not the 22 px that other screens' BACK
  // buttons still use: these five controls sit close together and a BLE
  // keyboard's owner is also tapping at them.
  lv_obj_set_pos(button, x, 3);
  stylePanel(button, lv_color_hex(0x1E1E2E), lv_color_hex(0x45475A));
  lv_obj_add_event_cb(button, [](lv_event_t *) {
    lastInputMs = millis();
    if (backlightDimmed) {
      display.setBrightness(brightnessValue);
      backlightDimmed = false;
    }
  }, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb(button, callback, LV_EVENT_CLICKED, nullptr);
  lv_obj_t *text = lv_label_create(button);
  lv_label_set_text(text, label);
  lv_obj_center(text);
  // Icon buttons keep the Space Mono Nerd Font glyphs; text buttons use the
  // editor's Terminess cut.
  lv_obj_set_style_text_font(text,
      icon ? &space_mono_14_icons : &terminess_14, 0);
  lv_obj_set_style_text_color(text, lv_color_hex(0xCDD6F4), 0);
  return button;
}

// Bluetooth shortcut square: the 30 px toolbar footprint of the editor's
// hamburger and gear, with the 20 px Bluetooth glyph as its only content.
// Tapping it opens the BLE keyboard picker from any header that carries it.
// Header height and button height are parameters because the editor's 36 px
// bar is taller than the 30 px bars this button also appears in.
// Returns the icon label so the 1 Hz refresh can tint it with connection state.
lv_obj_t *makeBluetoothButton(lv_obj_t *parent, int x, int height = 22) {
  lv_obj_t *button = lv_btn_create(parent);
  lv_obj_set_size(button, 30, height);
  // The editor's taller bar grows the button by the same amount (36-28 = 30-22
  // = 8), so both headers keep a 3 px inset above and below.
  lv_obj_set_pos(button, x, 3);
  stylePanel(button, lv_color_hex(0x1E1E2E), lv_color_hex(0x45475A));
  // Wake the backlight before navigating, like every other touch target.
  lv_obj_add_event_cb(button, [](lv_event_t *) {
    lastInputMs = millis();
    if (backlightDimmed) {
      display.setBrightness(brightnessValue);
      backlightDimmed = false;
    }
  }, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb(button, pairClicked, LV_EVENT_CLICKED, nullptr);
  lv_obj_t *icon = lv_label_create(button);
  lv_label_set_text(icon, ICON_BLUETOOTH);
  lv_obj_set_style_text_font(icon, &space_mono_20_icons, 0);
  lv_obj_center(icon);
  return icon;
}

// Menu button: dark panel-style button with a centered label. With key set,
// the label's first matching letter renders underlined — the settings-screen
// convention for shortcut letters ("Pair Keyboard" marks the p key). LVGL 8
// has no inline underline command, so the text splits at the key into up to
// three abutting labels, each centered with its own x offset, the same
// per-glyph-advance trick makeToggleRow uses to hide the seams.
lv_obj_t *makeMenuButton(lv_obj_t *parent, const char *label, int x, int y,
                         int width, int height, lv_event_cb_t callback,
                         void *userData = nullptr, char key = '\0') {
  lv_obj_t *button = lv_btn_create(parent);
  lv_obj_set_size(button, width, height);
  lv_obj_set_pos(button, x, y);
  stylePanel(button, lv_color_hex(0x1E1E2E), lv_color_hex(0x45475A));
  lv_obj_add_event_cb(button, [](lv_event_t *) {
    lastInputMs = millis();
    if (backlightDimmed) {
      display.setBrightness(brightnessValue);
      backlightDimmed = false;
    }
  }, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb(button, callback, LV_EVENT_CLICKED, userData);
  const char *hit = key ? strchr(label, key) : nullptr;
  // lead holds the short prefix before the underlined letter; 32 bytes cover
  // every keyed label (static UI strings) with room to spare, and an
  // oversized prefix just falls through to the plain single label below.
  char lead[32] = "";
  if (hit && hit > label) {
    const size_t leadLen = size_t(hit - label);
    if (leadLen < sizeof(lead)) {
      memcpy(lead, label, leadLen);
      lead[leadLen] = '\0';
    } else {
      hit = nullptr;
    }
  }
  if (!hit) {
    lv_obj_t *text = lv_label_create(button);
    lv_label_set_text(text, label);
    lv_obj_center(text);
    setUiFont(text);
    lv_obj_set_style_text_color(text, lv_color_hex(0xCDD6F4), 0);
    return button;
  }
  const lv_font_t *font = &elms_bold_12;
  char keyText[2] = {key, '\0'};
  const char *tail = hit + 1;
  const lv_coord_t leadW = lead[0]
      ? lv_txt_get_width(lead, strlen(lead), font, 0, LV_TEXT_FLAG_NONE) : 0;
  const lv_coord_t keyW =
      lv_txt_get_width(keyText, 1, font, 0, LV_TEXT_FLAG_NONE);
  const lv_coord_t tailW = tail[0]
      ? lv_txt_get_width(tail, strlen(tail), font, 0, LV_TEXT_FLAG_NONE) : 0;
  // lv_obj_center is LV_ALIGN_CENTER with no offsets; each segment's x offset
  // slides its own center so the whole run stays centered like one label:
  // lead at -(keyW + tailW) / 2, key at (leadW - tailW) / 2, tail at
  // (leadW + keyW) / 2.
  lv_obj_t *keyLabel = lv_label_create(button);
  lv_label_set_text(keyLabel, keyText);
  setUiFont(keyLabel);
  lv_obj_set_style_text_color(keyLabel, lv_color_hex(0xCDD6F4), 0);
  lv_obj_set_style_text_decor(keyLabel, LV_TEXT_DECOR_UNDERLINE, 0);
  lv_obj_align(keyLabel, LV_ALIGN_CENTER, (leadW - tailW) / 2, 0);
  if (lead[0]) {
    lv_obj_t *leadLabel = lv_label_create(button);
    lv_label_set_text(leadLabel, lead);
    setUiFont(leadLabel);
    lv_obj_set_style_text_color(leadLabel, lv_color_hex(0xCDD6F4), 0);
    lv_obj_align(leadLabel, LV_ALIGN_CENTER, -(keyW + tailW) / 2, 0);
  }
  if (tail[0]) {
    lv_obj_t *tailLabel = lv_label_create(button);
    lv_label_set_text(tailLabel, tail);
    setUiFont(tailLabel);
    lv_obj_set_style_text_color(tailLabel, lv_color_hex(0xCDD6F4), 0);
    lv_obj_align(tailLabel, LV_ALIGN_CENTER, (leadW + keyW) / 2, 0);
  }
  return button;
}

// Main-menu grid tile: a large Nerd Font icon over a short label, the 2x2
// replacement for the old stacked full-width menu rows. Icons render from
// the icon-only Space Mono cuts (28 px, or a bigger cut per tile via
// iconFont) so the tiles get crisp large glyphs without shipping another
// full text font. iconY and labelY are the tile-relative y offsets; the
// caller tunes them to the tile height so the icon+label block stays
// centered (see the TYPING PRACTICE tile below). shortcut > 0 also stamps
// that key number into the tile's top-right corner.
lv_obj_t *makeMenuTileButton(lv_obj_t *parent, const char *icon,
                             const char *label, int x, int y, int width,
                             int height, lv_event_cb_t callback,
                             void *userData = nullptr,
                             const lv_font_t *iconFont = nullptr,
                             int iconY = 12, int labelY = 50,
                             int shortcut = 0) {
  lv_obj_t *button = lv_btn_create(parent);
  lv_obj_set_size(button, width, height);
  lv_obj_set_pos(button, x, y);
  stylePanel(button, lv_color_hex(0x1E1E2E), lv_color_hex(0x45475A));
  lv_obj_add_event_cb(button, [](lv_event_t *) {
    lastInputMs = millis();
    if (backlightDimmed) {
      display.setBrightness(brightnessValue);
      backlightDimmed = false;
    }
  }, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb(button, callback, LV_EVENT_CLICKED, userData);
  lv_obj_t *glyph = lv_label_create(button);
  lv_label_set_text(glyph, icon);
  // Icons live in the Space Mono Nerd Font, not Elms Sans.
  lv_obj_set_style_text_font(glyph, iconFont ? iconFont : &space_mono_28, 0);
  // Accent green, the same tone as the screen title and power label.
  lv_obj_set_style_text_color(glyph, lv_color_hex(0x96A57E), 0);
  lv_obj_align(glyph, LV_ALIGN_TOP_MID, 0, iconY);
  lv_obj_t *text = lv_label_create(button);
  lv_label_set_text(text, label);
  lv_obj_set_style_text_font(text, &elms_bold_14, 0);
  lv_obj_set_style_text_color(text, lv_color_hex(0xCDD6F4), 0);
  lv_obj_align(text, LV_ALIGN_TOP_MID, 0, labelY);
  if (shortcut > 0) {
    // The key that jumps straight here. Dim slate keeps it a hint: it must not
    // compete with the mint icon or the tile label. 4 bytes holds "9" + NUL;
    // the menu has four tiles, so a single digit is the whole range.
    char hint[4];
    snprintf(hint, sizeof(hint), "%d", shortcut);
    lv_obj_t *badge = lv_label_create(button);
    lv_label_set_text(badge, hint);
    lv_obj_set_style_text_font(badge, &elms_bold_12, 0);
    lv_obj_set_style_text_color(badge, lv_color_hex(0x7F849C), 0);
    lv_obj_align(badge, LV_ALIGN_TOP_RIGHT, -7, 5);
  }
  return button;
}

// One settings row label: mixed-case name with its shortcut letter
// underlined, drawn as up to three abutting labels (lead / key / tail).
// lv_txt_get_width returns the same per-glyph advances the label renderer
// sums, so the seams are invisible. Shared by toggle and choice rows.
// Same shape as makeShortcutLabel, but hands back the label objects so the
// settings selection can be repainted without rebuilding the row.
void makeTrackedShortcutLabel(lv_obj_t *parent, const char *before, char key,
                              const char *after, int x, int y,
                              lv_color_t color, lv_obj_t *&leadOut,
                              lv_obj_t *&keyOut, lv_obj_t *&tailOut) {
  const lv_font_t *font = &elms_bold_12;
  const lv_coord_t leadW =
      lv_txt_get_width(before, strlen(before), font, 0, LV_TEXT_FLAG_NONE);
  char keyText[2] = {key, '\0'};
  const lv_coord_t keyW =
      lv_txt_get_width(keyText, 1, font, 0, LV_TEXT_FLAG_NONE);
  leadOut = nullptr;
  tailOut = nullptr;
  if (before[0]) {
    lv_obj_t *lead = lv_label_create(parent);
    lv_label_set_text(lead, before);
    lv_obj_set_pos(lead, x, y);
    setUiFont(lead);
    lv_obj_set_style_text_color(lead, color, 0);
    leadOut = lead;
  }
  keyOut = lv_label_create(parent);
  lv_label_set_text(keyOut, keyText);
  lv_obj_set_pos(keyOut, x + leadW, y);
  setUiFont(keyOut);
  lv_obj_set_style_text_color(keyOut, color, 0);
  lv_obj_set_style_text_decor(keyOut, LV_TEXT_DECOR_UNDERLINE, 0);
  if (after[0]) {
    lv_obj_t *tail = lv_label_create(parent);
    lv_label_set_text(tail, after);
    lv_obj_set_pos(tail, x + leadW + keyW, y);
    setUiFont(tail);
    lv_obj_set_style_text_color(tail, color, 0);
    tailOut = tail;
  }
}

void makeShortcutLabel(lv_obj_t *parent, const char *before, char key,
                       const char *after, int x, int y,
                       lv_color_t color = lv_color_hex(0xBAC2DE)) {
  lv_obj_t *lead, *keyLabel, *tail;
  makeTrackedShortcutLabel(parent, before, key, after, x, y, color, lead,
                           keyLabel, tail);
}

// One settings toggle row: mixed-case name on the left with its shortcut
// letter underlined, pill switch right-aligned at x=410. The name renders
// as up to three abutting labels (lead / key / tail); lv_txt_get_width
// returns the same per-glyph advances the label renderer sums, so the
// seams are invisible. Binds the switch callback and returns the switch.
lv_obj_t *makeToggleRow(lv_obj_t *parent, const char *before, char key,
                        const char *after, int y, bool on,
                        lv_event_cb_t callback, int labelX = 264) {
  makeShortcutLabel(parent, before, key, after, labelX, y + 4);
  lv_obj_t *sw = lv_switch_create(parent);
  // Slightly smaller pills (was 52x24) so six rows fit the panel at a
  // 36 px pitch; the right edge stays at 456.
  lv_obj_set_size(sw, 46, 22);
  lv_obj_set_pos(sw, 410, y);
  // Track: same dark panel/border palette as every other control.
  lv_obj_set_style_bg_color(sw, lv_color_hex(0x1E1E2E), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(sw, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_color(sw, lv_color_hex(0x45475A), LV_PART_MAIN);
  lv_obj_set_style_border_width(sw, 1, LV_PART_MAIN);
  lv_obj_set_style_radius(sw, LV_RADIUS_CIRCLE, LV_PART_MAIN);
  // Fill: transparent when off, the slider-accent green when checked.
  lv_obj_set_style_bg_opa(sw, LV_OPA_TRANSP, LV_PART_INDICATOR);
  // Casting one side silences -Wdeprecated-enum-enum-conversion: LVGL 8
  // keeps states and parts in separate enums, but OR-ing them is the
  // documented selector idiom.
  lv_obj_set_style_bg_color(sw, lv_color_hex(0x96A57E),
                            (uint32_t)LV_STATE_CHECKED | LV_PART_INDICATOR);
  lv_obj_set_style_bg_opa(sw, LV_OPA_COVER,
                          (uint32_t)LV_STATE_CHECKED | LV_PART_INDICATOR);
  lv_obj_set_style_radius(sw, LV_RADIUS_CIRCLE, LV_PART_INDICATOR);
  // Knob: light pill. LVGL 8 switch knobs pad outward from the full-height
  // area, so negative padding insets the knob inside the track.
  lv_obj_set_style_bg_color(sw, lv_color_hex(0xCDD6F4), LV_PART_KNOB);
  lv_obj_set_style_bg_opa(sw, LV_OPA_COVER, LV_PART_KNOB);
  lv_obj_set_style_border_color(sw, lv_color_hex(0x313244), LV_PART_KNOB);
  lv_obj_set_style_border_width(sw, 1, LV_PART_KNOB);
  lv_obj_set_style_radius(sw, LV_RADIUS_CIRCLE, LV_PART_KNOB);
  lv_obj_set_style_pad_all(sw, -2, LV_PART_KNOB);
  lv_obj_add_event_cb(sw, callback, LV_EVENT_VALUE_CHANGED, nullptr);
  if (on) lv_obj_add_state(sw, LV_STATE_CHECKED);
  return sw;
}

// Settings row with a clickable value pill instead of a switch, for
// multi-choice options. Tapping the pill cycles the value; the underlined
// shortcut key does the same. Returns the pill's label so the caller can
// refresh the text after a keyboard-driven change.
lv_obj_t *makeChoiceRow(lv_obj_t *parent, const char *before, char key,
                        const char *after, int y, const char *value,
                        lv_event_cb_t callback) {
  makeShortcutLabel(parent, before, key, after, 14, y + 4);
  lv_obj_t *pill = lv_btn_create(parent);
  lv_obj_set_size(pill, 126, 24);
  lv_obj_set_pos(pill, 330, y - 1);
  stylePanel(pill, lv_color_hex(0x1E1E2E), lv_color_hex(0x45475A));
  lv_obj_add_event_cb(pill, [](lv_event_t *) {
    lastInputMs = millis();
    if (backlightDimmed) {
      display.setBrightness(brightnessValue);
      backlightDimmed = false;
    }
  }, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb(pill, callback, LV_EVENT_CLICKED, nullptr);
  lv_obj_t *text = lv_label_create(pill);
  lv_label_set_text(text, value);
  lv_obj_center(text);
  setUiFont(text);
  lv_obj_set_style_text_color(text, lv_color_hex(0xCDD6F4), 0);
  return text;
}

// Boot screen and app root: four tiles, no status line. Saving is the
// editor's job (:w / Ctrl-S), so there is no save entry here.
void buildMainMenuScreen() {
  editorVisible = false;
  menuVisible = true;
  startVisible = settingsVisible = pairingVisible = false;
  helpVisible = typingVisible = typingSettingsVisible = previewVisible = false;
  wifiScreenVisible = false;
  lv_obj_clean(lv_scr_act());
  for (int i = 0; i < 4; ++i) menuTileButtons[i] = nullptr;
  pairingStatusLabel = settingsRadioLabel = brightnessStatusLabel = nullptr;
  menuRadioLabel = nullptr;
  brightnessSlider = smoothSwitch = markdownSwitch = autoSaveSwitch =
      dimSwitch = lineNumberSwitch = wordWrapSwitch = nullptr;
  lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x11111B), 0);

  lv_obj_t *header = lv_obj_create(lv_scr_act());
  lv_obj_set_size(header, SCREEN_W - 8, 30);
  lv_obj_set_pos(header, 4, 4);
  stylePanel(header, lv_color_hex(0x181825), lv_color_hex(0x313244));
  lv_obj_t *heading = lv_label_create(header);
  lv_label_set_text(heading, "WriterInAWell");
  lv_obj_set_pos(heading, 10, 4);
  setTitleFont(heading);
  lv_obj_set_style_text_color(heading, lv_color_hex(0x96A57E), 0);
  // Root screen: no BACK. The Bluetooth square doubles as the shortcut to the
  // BLE keyboard picker; loop() tints its icon with the connection state at
  // 1 Hz.
  const String keyboard = keyboardStatus();
  menuRadioLabel = makeBluetoothButton(header, 438);
  lv_obj_set_style_text_color(menuRadioLabel,
      keyboard.startsWith("Connected:") ? lv_color_hex(0x96A57E)
                                        : lv_color_hex(0x89B4FA), 0);

  lv_obj_t *panel = lv_obj_create(lv_scr_act());
  lv_obj_set_size(panel, SCREEN_W - 8, 240);
  lv_obj_set_pos(panel, 4, 39);
  stylePanel(panel, lv_color_hex(0x11111B), lv_color_hex(0x313244));
  // 2x2 grid of icon tiles: 222 px cells with a 4 px row gap and 12 px outer
  // margins inside the 472x240 px panel, so each tile is
  // (240 - 2*12 - 4) / 2 = 106 px tall and the grid fills the full height
  // the old status line used to reserve. Index order matches the 1-4 keys.
  // Icons sit at y=20 (y=11 for the 40 px keyboard cut) and labels at y=62,
  // which centers the block: both cuts' ink lands on the same 32..49 px band
  // (the 40 px keyboard glyph is only 0.43 em tall, vs 0.61 em at 28 px, so
  // its box rides 9 px higher to match).
  // The tile now opens the Wi-Fi screen (where the password and address live);
  // it still flags a live AP at a glance.
  const char *wifiLabel = transferEnabled ? "WI-FI ON" : "SHARE OVER WI-FI";
  struct { const char *icon; const char *label;
           const lv_font_t *iconFont; int iconY; } tiles[4] = {
      {ICON_MENU, "TEXT EDITOR", &space_mono_28, 20},
      {ICON_KEYBOARD, "TYPING PRACTICE", &space_mono_40, 11},
      {ICON_WIFI, wifiLabel, &space_mono_28, 20},
      {ICON_SETTINGS, "SETTINGS", &space_mono_28, 20},
  };
  for (intptr_t i = 0; i < 4; ++i) {
    lv_obj_t *button = makeMenuTileButton(panel, tiles[i].icon, tiles[i].label,
                                          12 + (i % 2) * 226,
                                          12 + (i / 2) * 110,
                                          222, 106, menuEntryClicked,
                                          reinterpret_cast<void *>(i),
                                          tiles[i].iconFont, tiles[i].iconY,
                                          62, static_cast<int>(i) + 1);
    menuTileButtons[i] = button;
    lv_obj_set_style_bg_color(button,
        i == static_cast<intptr_t>(menuSelection)
            ? lv_color_hex(0x313244) : lv_color_hex(0x1E1E2E), 0);
  }

  lv_obj_t *footRow = makeHintRow();
  addHintPair(footRow, "1-4", "jump");
  addHintPair(footRow, "↑↓←→", "select");
  addHintPair(footRow, "enter", "open");
}

// Retint only the tiles whose selection state changed. Used by the arrow keys
// and h/j/k/l so moving the highlight no longer tears the screen down.
void refreshMenuSelection() {
  for (int i = 0; i < 4; ++i) {
    if (!menuTileButtons[i]) continue;
    lv_obj_set_style_bg_color(menuTileButtons[i],
        i == static_cast<int>(menuSelection) ? lv_color_hex(0x313244)
                                             : lv_color_hex(0x1E1E2E), 0);
  }
}

// --- Typing practice screen --------------------------------------------------
// Layout mirrors the other app screens: a 30 px header with SETTINGS/RETRY/
// BACK, a segmented stats bar, a wrapped recolor paragraph, the run info in
// the strip above the shared keycap hint row. The paragraph shows the word
// stream with the active word picked out in color bands, monkeytype-style.

// xorshift32 seeded once from the hardware RNG; word order only needs to
// not repeat between runs, not crypto strength. The | 1 keeps it odd.
uint32_t typingRand() {
  static uint32_t seed = 0;
  if (!seed) seed = esp_random() | 1;
  seed ^= seed << 13;
  seed ^= seed >> 17;
  seed ^= seed << 5;
  return seed;
}

bool typingWordCorrect(size_t index) {
  const char *word = typingWords[index];
  const size_t length = strlen(word);
  if (typingTypedLen[index] != length) return false;
  for (size_t i = 0; i < length; ++i) {
    if (typingTyped[index][i] != word[i]) return false;
  }
  return true;
}

// Recolor span "#RRGGBB text#": balanced by construction. Generated words
// may contain digits, punctuation, and the symbol set, but never '#', so no
// literal '#' ever needs escaping.
void typingSpan(size_t &pos, const char *hex, const char *text,
                size_t length) {
  if (pos + 9 + length + 1 > sizeof(typingText)) return;
  typingText[pos++] = '#';
  memcpy(typingText + pos, hex, 6);
  pos += 6;
  typingText[pos++] = ' ';
  memcpy(typingText + pos, text, length);
  pos += length;
  typingText[pos++] = '#';
  typingText[pos] = '\0';
}

void typingPlain(size_t &pos, char c) {
  if (pos + 2 > sizeof(typingText)) return;
  typingText[pos++] = c;
  typingText[pos] = '\0';
}

void updateTypingWords() {
  if (!typingWordsLabel) return;
  size_t pos = 0;
  typingText[0] = '\0';
  for (size_t i = 0; i < typingWordCount; ++i) {
    if (i) typingPlain(pos, ' ');
    const char *word = typingWords[i];
    const size_t length = strlen(word);
    if (i < typingCurrentWord) {
      // Finished words: mint when clean, pink when anything went wrong.
      typingSpan(pos, typingWordWrong[i] ? "F38BA8" : "96A57E",
                word, length);
    } else if (i > typingCurrentWord) {
      typingSpan(pos, "5F707A", word, length);
    } else {
      // Active word in bands: the correctly typed prefix (bright), the red
      // mismatch run, the untyped tail (light slate), then any letters
      // typed past the end appended in red, exactly like monkeytype.
      const uint8_t typedLen = typingTypedLen[i];
      const char *typed = typingTyped[i];
      size_t correct = 0;
      while (correct < typedLen && correct < length &&
             typed[correct] == word[correct]) {
        ++correct;
      }
      if (correct) typingSpan(pos, "E8EEF2", word, correct);
      const size_t mismatchEnd = std::min<size_t>(typedLen, length);
      if (mismatchEnd > correct) {
        typingSpan(pos, "F38BA8", word + correct, mismatchEnd - correct);
      }
      if (length > mismatchEnd) {
        typingSpan(pos, "A9BAC2", word + mismatchEnd, length - mismatchEnd);
      }
      if (typedLen > length) {
        typingSpan(pos, "F38BA8", typed + length, typedLen - length);
      }
    }
  }
  lv_label_set_text(typingWordsLabel, typingText);
}

// Repaint one cell value only when it actually changed: the live tick runs
// four times a second, and lv_label_set_text reallocates its buffer even for
// identical text, which would churn the 48 KiB LVGL heap.
void typingSetStatText(lv_obj_t *label, const char *text) {
  if (!label) return;
  const char *current = lv_label_get_text(label);
  if (current && strcmp(current, text) == 0) return;
  lv_label_set_text(label, text);
}

void typingShowSummaryBar(bool shown) {
  if (typingSummaryShown == shown) return;
  typingSummaryShown = shown;
  setWidgetHidden(typingLiveBar, shown);
  setWidgetHidden(typingSummaryBar, !shown);
}

// Numbers only: the per-cell captions never change, so a tick formats a
// handful of small stack buffers and leaves untouched cells alone.
void updateTypingStats(bool final) {
  if (!typingLiveBar || !typingSummaryBar) return;
  const uint32_t elapsed = typingStartMs ? millis() - typingStartMs : 0;
  const uint32_t seconds = elapsed / 1000;
  const uint32_t wpm = elapsed >= 250
      ? (typingCorrectKeys * 12000UL) / elapsed : 0;
  const uint32_t raw = elapsed >= 250
      ? (typingKeystrokes * 12000UL) / elapsed : 0;
  const uint32_t acc = typingKeystrokes
      ? (typingCorrectKeys * 100) / typingKeystrokes : 100;
  const uint16_t best = typingBestWpm[typingLengthIndex];
  char buf[16];

  snprintf(buf, sizeof(buf), "%lu", static_cast<unsigned long>(wpm));
  typingSetStatText(typingLiveValue[TYPING_LIVE_WPM], buf);
  typingSetStatText(typingSummaryValue[TYPING_SUM_WPM], buf);
  snprintf(buf, sizeof(buf), "%lu%%", static_cast<unsigned long>(acc));
  typingSetStatText(typingLiveValue[TYPING_LIVE_ACC], buf);
  typingSetStatText(typingSummaryValue[TYPING_SUM_ACC], buf);
  snprintf(buf, sizeof(buf), "%lu:%02lu",
           static_cast<unsigned long>(seconds / 60),
           static_cast<unsigned long>(seconds % 60));
  typingSetStatText(typingLiveValue[TYPING_LIVE_TIME], buf);
  typingSetStatText(typingSummaryValue[TYPING_SUM_TIME], buf);
  if (best) {
    snprintf(buf, sizeof(buf), "%u", static_cast<unsigned>(best));
  } else {
    snprintf(buf, sizeof(buf), "--");
  }
  typingSetStatText(typingLiveValue[TYPING_LIVE_PB], buf);
  typingSetStatText(typingSummaryValue[TYPING_SUM_BEST], buf);

  if (final) {
    snprintf(buf, sizeof(buf), "%lu", static_cast<unsigned long>(raw));
    typingSetStatText(typingSummaryValue[TYPING_SUM_RAW], buf);
    snprintf(buf, sizeof(buf), "%lu/%lu",
             static_cast<unsigned long>(typingCorrectWords),
             static_cast<unsigned long>(typingWordCount));
    typingSetStatText(typingSummaryValue[TYPING_SUM_WORDS], buf);
  }
  typingShowSummaryBar(final);
}


size_t typingMaxWordLength() {
  return TYPING_MAX_LENGTHS[typingMaxLengthIndex];
}

bool typingBaseAllowed(const char *word) {
  const size_t length = strlen(word);
  if (typingComplexity == TYPING_EASY && length > 4) return false;
  const size_t cap = typingMaxWordLength();
  return cap == 0 || length <= cap;
}

// Uniform draw from the allowed subset: one count pass, then one index pass.
// A few hundred strlen calls at run start, never per keystroke.
const char *typingDrawBase(const char *const *pool, size_t poolSize) {
  size_t candidates = 0;
  for (size_t i = 0; i < poolSize; ++i) {
    if (typingBaseAllowed(pool[i])) ++candidates;
  }
  if (!candidates) return pool[typingRand() % poolSize];
  size_t pick = typingRand() % candidates;
  for (size_t i = 0; i < poolSize; ++i) {
    if (!typingBaseAllowed(pool[i])) continue;
    if (pick-- == 0) return pool[i];
  }
  return pool[0];
}

// Uniform draw from a curated list, filtered by complexity and max length.
// Returns nullptr when every entry is filtered out so the caller can fall
// back to a plain bank word.
const char *typingPickCurated(const char *const *list, size_t size) {
  size_t candidates = 0;
  for (size_t i = 0; i < size; ++i) {
    if (typingBaseAllowed(list[i])) ++candidates;
  }
  if (!candidates) return nullptr;
  size_t pick = typingRand() % candidates;
  for (size_t i = 0; i < size; ++i) {
    if (!typingBaseAllowed(list[i])) continue;
    if (pick-- == 0) return list[i];
  }
  return nullptr;
}

bool typingPoolHasWord(const char *const *pool, size_t size) {
  for (size_t i = 0; i < size; ++i) {
    if (typingBaseAllowed(pool[i])) return true;
  }
  return false;
}

// Hard keeps a mix: its bank holds only 7-12 letter words, and a run made
// entirely of those packs half as many words into the screen. Three draws in
// five come from the long bank, the rest from the common one.
const char *typingDrawWord() {
  if (typingComplexity == TYPING_HARD && typingRand() % 5 < 3 &&
      typingPoolHasWord(TYPING_HARD_WORDS, TYPING_HARD_POOL)) {
    return typingDrawBase(TYPING_HARD_WORDS, TYPING_HARD_POOL);
  }
  return typingDrawBase(TYPING_WORDS, TYPING_WORD_POOL);
}

// Build one display/target token into its fixed slot. Decorations are chosen
// only where English would put them: a contraction's apostrophe and a
// compound's hyphen come with the curated word, and a bracket/asterisk pair
// wraps a whole word. Sentence punctuation is attached last, then the
// capitalization pass (so a wrapped "(the" becomes "(The").
void typingBuildWord(size_t index) {
  char *out = typingWords[index];
  out[0] = '\0';

  // Clauses run 3..7 words. Only a clause's opening word may start a
  // sentence, and only a clause closed by . ? ! hands that on: a semicolon
  // keeps the same sentence running in lower case.
  if (typingClauseLeft == 0) {
    typingClauseLeft = static_cast<uint8_t>(3 + typingRand() % 5);
    typingClauseFinal = !typingPunctuation || typingRand() % 4 != 0;
  }
  const bool opensSentence = typingSentenceStart;
  typingSentenceStart = false;
  const bool closesClause = typingClauseLeft <= 1;
  if (closesClause) typingSentenceStart = typingClauseFinal;
  if (typingClauseLeft) --typingClauseLeft;

  size_t length = 0;
  bool plainWord = false;
  bool commaWord = false;
  if (typingNumbers && !opensSentence && typingRand() % 16 == 0) {
    // A number is its own token, never glued to letters: 1..999 keeps it on
    // the digit row and free of leading zeros.
    const unsigned long value = 1 + typingRand() % 999;
    snprintf(out, TYPING_WORD_STORE, "%lu", value);
    length = strlen(out);
    typingLastBase = nullptr;
  } else {
    // One decoration at most: a hyphenated compound is drawn whole; a wrap
    // is applied to an otherwise plain word below. A comma word is drawn only
    // where its comma reads naturally, i.e. not at a sentence start and not
    // at the clause end (that slot takes . ? ! or ;).
    const char *base = nullptr;
    if (typingPunctuation && !opensSentence && !closesClause &&
        typingRand() % 12 == 0) {
      base = typingPickCurated(TYPING_COMMA_WORDS, TYPING_COMMA_POOL);
      commaWord = base != nullptr;
    }
    if (!base && typingSymbols && typingRand() % 14 == 0) {
      base = typingPickCurated(TYPING_COMPOUNDS, TYPING_COMPOUND_POOL);
    }
    if (!base && typingCapitalization && typingRand() % 18 == 0) {
      base = typingPickCurated(TYPING_ACRONYMS, TYPING_ACRONYM_POOL);
    }
    if (!base && typingPunctuation && typingRand() % 8 == 0) {
      base = typingPickCurated(TYPING_CONTRACTIONS, TYPING_CONTRACTION_POOL);
    }
    if (base) {
      typingLastBase = base;
    } else {
      // Reject the previous word: with pools this size four draws are ample,
      // and the clause plan above already ran, so a retry cannot shift it.
      for (int tries = 0; tries < 4; ++tries) {
        base = typingDrawWord();
        if (base != typingLastBase) break;
      }
      typingLastBase = base;
      plainWord = true;
    }
    length = strlen(base);
    if (length >= TYPING_WORD_STORE) length = TYPING_WORD_STORE - 1;
    memcpy(out, base, length);
    out[length] = '\0';
    // A mid-sentence aside or emphasis: the only decoration that reads as
    // punctuation on an arbitrary word. Never at a sentence start.
    if (typingSymbols && plainWord && !opensSentence &&
        length + 2 < TYPING_WORD_STORE && typingRand() % 12 == 0) {
      static constexpr char WRAPS[][2] = {{'(', ')'}, {'[', ']'}, {'*', '*'}};
      const char *wrap = WRAPS[typingRand() % 3];
      memmove(out + 1, out, length);
      out[0] = wrap[0];
      out[length + 1] = wrap[1];
      length += 2;
      out[length] = '\0';
    }
  }

  if (typingCapitalization && opensSentence) {
    for (size_t i = 0; i < length; ++i) {
      if (out[i] >= 'a' && out[i] <= 'z') {
        out[i] = static_cast<char>(out[i] - 'a' + 'A');
        break;
      }
    }
  }

  if (typingPunctuation && length + 1 < TYPING_WORD_STORE) {
    char mark = 0;
    if (closesClause) {
      static constexpr char ENDS[] = ".?!";
      mark = typingClauseFinal ? ENDS[typingRand() % 3] : ';';
    } else if (commaWord) {
      mark = ',';
    }
    if (mark) {
      out[length++] = mark;
      out[length] = '\0';
    }
  }
}

// A run can stop mid-clause (the 20-word cut, or the last band). Close the
// paragraph with a period so the fragment reads as prose; a trailing
// semicolon becomes that period instead of gaining a second mark. Returns the
// appended mark, or 0 when nothing grew (the Long caller re-measures).
char typingCloseFinalWord(size_t index) {
  if (!typingPunctuation) return 0;
  char *word = typingWords[index];
  const size_t length = strlen(word);
  if (!length || length + 1 >= TYPING_WORD_STORE) return 0;
  const char last = word[length - 1];
  if (last == ';') {
    word[length - 1] = '.';
    return 0;
  }
  if (last == '.' || last == '?' || last == '!') return 0;
  word[length] = '.';
  word[length + 1] = '\0';
  return '.';
}

void typingPickWords() {
  // Generate the paragraph in one pass: the sentence planner must run exactly
  // once per token, so the fit pass below only measures what already exists.
  typingClauseLeft = 0;
  typingClauseFinal = true;
  typingSentenceStart = true;
  typingLastBase = nullptr;
  for (size_t i = 0; i < TYPING_MAX_WORDS; ++i) {
    typingBuildWord(i);
    typingTypedLen[i] = 0;
    typingWordWrong[i] = false;
  }

  const size_t wanted = TYPING_LENGTHS[typingLengthIndex];
  const bool fill = wanted == 0;  // Long: as many words as the area holds
  size_t count = wanted ? wanted : TYPING_MAX_WORDS;
  if (count > TYPING_MAX_WORDS) count = TYPING_MAX_WORDS;

  // Measure at slightly less than the label's content width: a conservative
  // wrap count can only leave a word out, never push one past the last band.
  const lv_coord_t labelW = typingWordsLabel
      ? lv_obj_get_content_width(typingWordsLabel) : TYPING_WORD_AREA_W;
  const lv_coord_t width =
      labelW > 20 ? labelW - 4 : TYPING_WORD_AREA_W - 4;

  size_t measureLen = 0;
  typingMeasureText[0] = '\0';
  if (fill) {
    count = 0;
    for (size_t i = 0; i < TYPING_MAX_WORDS; ++i) {
      const size_t length = strlen(typingWords[i]);
      const size_t spacer = count ? 1 : 0;
      if (measureLen + spacer + length + 1 >= sizeof(typingMeasureText)) break;
      if (spacer) typingMeasureText[measureLen++] = ' ';
      memcpy(typingMeasureText + measureLen, typingWords[i], length);
      measureLen += length;
      typingMeasureText[measureLen] = '\0';
      lv_point_t size;
      lv_txt_get_size(&size, typingMeasureText, &terminess_20, 0, 0, width,
                      LV_TEXT_FLAG_NONE);
      if (size.y > TYPING_WORD_AREA_H && count > 0) {
        measureLen -= length + spacer;
        typingMeasureText[measureLen] = '\0';
        break;
      }
      ++count;
    }
  }
  // The first word always fits one band; keep the invariant even if the
  // measure buffer ever runs out of room early.
  if (count == 0) count = 1;

  // Close the paragraph where the run stops. For Long the extra glyph can
  // wrap the last word onto a tenth band, so re-measure and undo it then.
  const char mark = typingCloseFinalWord(count - 1);
  if (mark && fill) {
    char *last = typingWords[count - 1];
    const size_t lastLen = strlen(last);
    bool appended = measureLen + 1 < sizeof(typingMeasureText);
    if (appended) {
      typingMeasureText[measureLen++] = mark;
      typingMeasureText[measureLen] = '\0';
      lv_point_t size;
      lv_txt_get_size(&size, typingMeasureText, &terminess_20, 0, 0, width,
                      LV_TEXT_FLAG_NONE);
      if (size.y > TYPING_WORD_AREA_H) {
        appended = false;
      }
    }
    if (!appended) {
      last[lastLen - 1] = '\0';
      typingMeasureText[--measureLen] = '\0';
    }
  }
  typingWordCount = count;
  typingCurrentWord = 0;
}

void startTypingRun() {
  typingPickWords();
  // The clock arms on the first counted keystroke, not on screen entry, so
  // reading the words before starting costs nothing.
  typingStartMs = 0;
  typingLastTickMs = 0;
  typingKeystrokes = typingCorrectKeys = 0;
  typingCorrectWords = 0;
  typingRunActive = true;
  // typingPickWords ran above, so the Long run's count is already known.
  snprintf(typingStatusText, sizeof(typingStatusText),
           "%s run, %lu words — type the bright word",
           TYPING_LENGTH_NAMES[typingLengthIndex],
           static_cast<unsigned long>(typingWordCount));
  if (typingStatusLabel) {
    lv_label_set_text(typingStatusLabel, typingStatusText);
  }
  updateTypingWords();
  updateTypingStats(false);
}

void finishTypingRun() {
  typingRunActive = false;
  const uint32_t elapsed = typingStartMs ? millis() - typingStartMs : 0;
  const uint32_t wpm = elapsed >= 250
      ? (typingCorrectKeys * 12000UL) / elapsed : 0;
  if (wpm > typingBestWpm[typingLengthIndex]) {
    typingBestWpm[typingLengthIndex] = static_cast<uint16_t>(wpm);
    // Written immediately — one small NVS write per new record, never from
    // a keystroke path — so a best set just before a power cut survives.
    if (devicePrefs.begin("wiaw", false)) {
      devicePrefs.putUInt(typingBestKey(typingLengthIndex), wpm);
      devicePrefs.end();
    }
    snprintf(typingStatusText, sizeof(typingStatusText),
             "New personal best — %lu wpm", static_cast<unsigned long>(wpm));
  } else {
    snprintf(typingStatusText, sizeof(typingStatusText),
             "Done — tab for another run");
  }
  if (typingStatusLabel) {
    lv_label_set_text(typingStatusLabel, typingStatusText);
  }
  updateTypingStats(true);
}

void typingHandleText(char c) {
  const char lastLengthKey = static_cast<char>('1' + TYPING_LENGTH_COUNT - 1);
  if (c >= '1' && c <= lastLengthKey) {
    // Length keys double as restarts, but a number token can start with the
    // same digit: when it is the next expected character, type it instead of
    // starting a new run.
    bool expected = false;
    if (typingRunActive) {
      const char *word = typingWords[typingCurrentWord];
      const uint8_t typedLen = typingTypedLen[typingCurrentWord];
      expected = typedLen < strlen(word) && word[typedLen] == c;
    }
    if (!expected) {
      const size_t index = static_cast<size_t>(c - '1');
      if (index < TYPING_LENGTH_COUNT) {
        typingLengthIndex = index;
        startTypingRun();
      }
      return;
    }
  }
  if (!typingRunActive) return;
  if (c == ' ') {
    // Leading spaces are skipped uncounted: an accidental double-space must
    // not arm the clock or poison accuracy.
    if (!typingTypedLen[typingCurrentWord]) return;
    if (!typingStartMs) typingStartMs = millis();
    const bool clean = typingWordCorrect(typingCurrentWord);
    ++typingKeystrokes;
    if (clean) {
      // The space itself is a correct character, monkeytype-style.
      ++typingCorrectKeys;
      ++typingCorrectWords;
    }
    typingWordWrong[typingCurrentWord] = !clean;
    if (typingCurrentWord + 1 >= typingWordCount) {
      finishTypingRun();
      return;
    }
    ++typingCurrentWord;
    return;
  }
  const char *word = typingWords[typingCurrentWord];
  const size_t length = strlen(word);
  const uint8_t typedLen = typingTypedLen[typingCurrentWord];
  // Past word length plus the extras cap, input is dropped: the red extras
  // run is bounded the same way monkeytype bounds it.
  if (typedLen >= TYPING_TYPED_CAP ||
      typedLen >= length + TYPING_EXTRA_CHARS) {
    return;
  }
  if (!typingStartMs) typingStartMs = millis();
  typingTyped[typingCurrentWord][typedLen] = c;
  typingTypedLen[typingCurrentWord] = typedLen + 1;
  ++typingKeystrokes;
  const bool hit = typedLen < length && c == word[typedLen];
  if (hit) ++typingCorrectKeys;
  // Auto-finish: the run ends on the last letter of the last word; no
  // trailing space is required.
  if (hit && typingCurrentWord + 1 == typingWordCount &&
      typingWordCorrect(typingCurrentWord)) {
    ++typingCorrectWords;
    finishTypingRun();
  }
}

void typingBackspace() {
  // Erases within the current word only; words already advanced past keep
  // their verdict. While the summary shows there is nothing left to fix.
  if (!typingRunActive || !typingTypedLen[typingCurrentWord]) return;
  --typingTypedLen[typingCurrentWord];
}

void typingRestartClicked(lv_event_t *) { startTypingRun(); }

void typingBackClicked(lv_event_t *) {
  typingVisible = false;
  showMainMenu();
}

void typingSettingsClicked(lv_event_t *) { showTypingSettings(); }

// --- Typing stats bar --------------------------------------------------------
// A segmented control: tinted cells separated by the bar background showing
// through 1 px gaps, each cell a dim caption plus a bright bold value. The
// old single recolor label used "#RRGGBB|#" separators, but LVGL only accepts
// a color command when a literal space follows the six hex digits, so the
// parser swallowed the separators and printed the hex code as text.

// One segment inside a row. Returns the value label so live repaints only
// touch numbers. The height is explicit (not LV_PCT) because the row's height
// itself comes from flex_grow, and a percentage inside a computed row is the
// kind of layout dependency that silently collapses a bar.
lv_obj_t *makeTypingStatCell(lv_obj_t *row, const char *caption, int height,
                             uint32_t bgHex, uint32_t valueHex) {
  lv_obj_t *cell = lv_obj_create(row);
  lv_obj_set_size(cell, 0, height);  // flex_grow supplies the width
  lv_obj_set_flex_grow(cell, 1);
  lv_obj_set_style_bg_color(cell, lv_color_hex(bgHex), 0);
  lv_obj_set_style_bg_opa(cell, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(cell, 0, 0);
  lv_obj_set_style_radius(cell, 2, 0);
  lv_obj_set_style_pad_left(cell, 6, 0);
  lv_obj_set_style_pad_right(cell, 6, 0);
  lv_obj_set_style_pad_top(cell, 0, 0);
  lv_obj_set_style_pad_bottom(cell, 0, 0);
  lv_obj_clear_flag(cell, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_layout(cell, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(cell, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(cell, LV_FLEX_ALIGN_SPACE_BETWEEN,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  lv_obj_t *label = lv_label_create(cell);
  lv_label_set_text(label, caption);
  lv_obj_set_style_text_font(label, &elms_bold_12, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(TYPING_STAT_CAPTION), 0);

  lv_obj_t *value = lv_label_create(cell);
  lv_label_set_text(value, "--");
  lv_obj_set_style_text_font(value, &elms_bold_14, 0);
  lv_obj_set_style_text_color(value, lv_color_hex(valueHex), 0);
  return value;
}

// A flex row of cells at a 1 px pitch: the bar background is the divider.
lv_obj_t *makeTypingStatRow(lv_obj_t *bar, int height) {
  lv_obj_t *row = lv_obj_create(bar);
  lv_obj_set_size(row, LV_PCT(100), height);
  lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(row, 0, 0);
  lv_obj_set_style_pad_all(row, 0, 0);
  lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_layout(row, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(row, 1, 0);
  return row;
}

// The bar itself: divider-colored background, 1 px inset, so the gaps between
// cells and the outer hairline share one color. `rows` picks a single live
// row or the two-row end-of-run summary; both start at the panel's top edge
// and end above TYPING_WORDS_Y.
lv_obj_t *makeTypingStatBar(lv_obj_t *panel, int y, int height, bool rows) {
  lv_obj_t *bar = lv_obj_create(panel);
  lv_obj_set_pos(bar, 6, y);
  lv_obj_set_size(bar, TYPING_STAT_BAR_W, height);
  lv_obj_set_style_bg_color(bar, lv_color_hex(0x313244), 0);
  lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(bar, 0, 0);
  lv_obj_set_style_radius(bar, 3, 0);
  lv_obj_set_style_pad_all(bar, 1, 0);
  lv_obj_set_style_pad_column(bar, 1, 0);
  lv_obj_set_style_pad_row(bar, 1, 0);
  lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_layout(bar, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(bar, rows ? LV_FLEX_FLOW_COLUMN : LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  return bar;
}

void buildTypingPracticeScreen() {
  editorVisible = false;
  typingVisible = true;
  typingSettingsVisible = false;
  settingsVisible = pairingVisible = menuVisible = startVisible = false;
  helpVisible = previewVisible = false;
  wifiScreenVisible = false;
  lv_obj_clean(lv_scr_act());
  pairingStatusLabel = settingsRadioLabel = brightnessStatusLabel = nullptr;
  brightnessSlider = smoothSwitch = markdownSwitch = autoSaveSwitch =
      dimSwitch = lineNumberSwitch = wordWrapSwitch = nullptr;
  typingComplexityValue = typingMaxLengthValue = nullptr;
  typingSymbolsSwitch = typingNumbersSwitch = typingCapitalSwitch =
      typingPunctuationSwitch = nullptr;
  typingWordsLabel = typingStatusLabel = nullptr;
  typingLiveBar = typingSummaryBar = nullptr;
  for (size_t i = 0; i < TYPING_LIVE_COUNT; ++i) typingLiveValue[i] = nullptr;
  for (size_t i = 0; i < TYPING_SUM_COUNT; ++i) typingSummaryValue[i] = nullptr;
  lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x11111B), 0);

  lv_obj_t *header = lv_obj_create(lv_scr_act());
  lv_obj_set_size(header, SCREEN_W - 8, 30);
  lv_obj_set_pos(header, 4, 4);
  stylePanel(header, lv_color_hex(0x181825), lv_color_hex(0x313244));
  lv_obj_t *heading = lv_label_create(header);
  lv_label_set_text(heading, "TYPING PRACTICE");
  lv_obj_set_pos(heading, 10, 4);
  setTitleFont(heading);
  lv_obj_set_style_text_color(heading, lv_color_hex(0x96A57E), 0);
  // Practice rules live one touch target away: the same SETTINGS button the
  // main menu uses, plus the ctrl-o command chip in the hint row.
  makeButton(header, "SETTINGS", 246, 92, typingSettingsClicked);
  // RETRY = separate icon and text labels. The Nerd Font glyph only exists
  // in the Space Mono icon-only 14 px cut (13 px line box), while the text
  // stays Elms 12 like every other button label, so
  // the pair no longer reads taller than BACK. Each label centers on its
  // own line box and the two split horizontally around the button center.
  lv_obj_t *retry = makeButton(header, "RETRY", 347, 68,
                               typingRestartClicked);
  lv_obj_t *retryText = lv_obj_get_child(retry, 0);
  lv_obj_t *retryIcon = lv_label_create(retry);
  lv_label_set_text(retryIcon, ICON_RETRY);
  lv_obj_set_style_text_font(retryIcon, &space_mono_14_icons, 0);
  lv_obj_set_style_text_color(retryIcon, lv_color_hex(0xCDD6F4), 0);
  const lv_coord_t retryIconW =
      lv_font_get_glyph_width(&space_mono_14_icons, 0xF01E, 0);
  const lv_coord_t retryTextW =
      lv_txt_get_width("RETRY", 5, &elms_bold_12, 0, LV_TEXT_FLAG_NONE);
  lv_obj_align(retryIcon, LV_ALIGN_CENTER, -((retryTextW + 3) / 2), 0);
  lv_obj_align(retryText, LV_ALIGN_CENTER, (retryIconW + 3) / 2, 0);
  // Right edges line up with the editor header's gear button (437 + 30),
  // the app's rightmost control — the menu screens' BACK at 412 leaves a
  // dead strip here that read as the pair sitting too far left.
  makeButton(header, "BACK", 423, 44, typingBackClicked);

  lv_obj_t *panel = lv_obj_create(lv_scr_act());
  lv_obj_set_size(panel, SCREEN_W - 8, 240);
  lv_obj_set_pos(panel, 4, 39);
  stylePanel(panel, lv_color_hex(0x11111B), lv_color_hex(0x313244));

  // Stats bars. Both are built once and swapped with the HIDDEN flag, so the
  // word stream below never moves between the live run and the summary.
  typingLiveBar = makeTypingStatBar(panel, 4, TYPING_STAT_LIVE_H, false);
  typingLiveValue[TYPING_LIVE_WPM] = makeTypingStatCell(typingLiveBar, "WPM",
      TYPING_STAT_LIVE_CELL_H, TYPING_STAT_BG_BLUE, TYPING_STAT_BLUE);
  typingLiveValue[TYPING_LIVE_ACC] = makeTypingStatCell(typingLiveBar, "ACC",
      TYPING_STAT_LIVE_CELL_H, TYPING_STAT_BG_GREEN, TYPING_STAT_GREEN);
  typingLiveValue[TYPING_LIVE_TIME] = makeTypingStatCell(typingLiveBar, "TIME",
      TYPING_STAT_LIVE_CELL_H, TYPING_STAT_BG_SLATE, TYPING_STAT_SLATE);
  typingLiveValue[TYPING_LIVE_PB] = makeTypingStatCell(typingLiveBar, "PB",
      TYPING_STAT_LIVE_CELL_H, TYPING_STAT_BG_AMBER, TYPING_STAT_AMBER);

  typingSummaryBar = makeTypingStatBar(panel, 2, TYPING_STAT_SUMMARY_H, true);
  lv_obj_t *summaryTop =
      makeTypingStatRow(typingSummaryBar, TYPING_STAT_SUMMARY_CELL_H);
  typingSummaryValue[TYPING_SUM_WPM] = makeTypingStatCell(summaryTop, "WPM",
      TYPING_STAT_SUMMARY_CELL_H, TYPING_STAT_BG_BLUE, TYPING_STAT_BLUE);
  typingSummaryValue[TYPING_SUM_RAW] = makeTypingStatCell(summaryTop, "RAW",
      TYPING_STAT_SUMMARY_CELL_H, TYPING_STAT_BG_SLATE, TYPING_STAT_CAPTION);
  typingSummaryValue[TYPING_SUM_ACC] = makeTypingStatCell(summaryTop, "ACC",
      TYPING_STAT_SUMMARY_CELL_H, TYPING_STAT_BG_GREEN, TYPING_STAT_GREEN);
  lv_obj_t *summaryBottom =
      makeTypingStatRow(typingSummaryBar, TYPING_STAT_SUMMARY_CELL_H);
  typingSummaryValue[TYPING_SUM_TIME] = makeTypingStatCell(summaryBottom,
      "TIME", TYPING_STAT_SUMMARY_CELL_H, TYPING_STAT_BG_SLATE,
      TYPING_STAT_SLATE);
  typingSummaryValue[TYPING_SUM_WORDS] = makeTypingStatCell(summaryBottom,
      "WORDS", TYPING_STAT_SUMMARY_CELL_H, TYPING_STAT_BG_SLATE,
      TYPING_STAT_SLATE);
  typingSummaryValue[TYPING_SUM_BEST] = makeTypingStatCell(summaryBottom,
      "BEST", TYPING_STAT_SUMMARY_CELL_H, TYPING_STAT_BG_AMBER,
      TYPING_STAT_AMBER);
  typingSummaryShown = false;
  setWidgetHidden(typingSummaryBar, true);

  // Word stream uses the 20 px Terminess cut (native 20 px strike, line height
  // 20). The 1-bpp crisp variants only exist for the editor ladder, so practice
  // text is always smooth. It starts under the bars and runs to the panel's
  // bottom edge: nine word bands, which the Long fit check measures against.
  typingWordsLabel = lv_label_create(panel);
  lv_obj_set_pos(typingWordsLabel, 10, TYPING_WORDS_Y);
  lv_obj_set_width(typingWordsLabel, TYPING_WORD_AREA_W);
  lv_label_set_recolor(typingWordsLabel, true);
  lv_obj_set_style_text_font(typingWordsLabel, &terminess_20, 0);
  // Base color covers the separating spaces; spans override per band.
  lv_obj_set_style_text_color(typingWordsLabel, lv_color_hex(0x7F849C), 0);

  // Run info sits in the strip between the panel and the hint row, the same
  // place the editor keeps its status line.
  typingStatusLabel = lv_label_create(lv_scr_act());
  lv_obj_set_pos(typingStatusLabel, 8, 282);
  lv_obj_set_width(typingStatusLabel, SCREEN_W - 16);
  lv_label_set_long_mode(typingStatusLabel, LV_LABEL_LONG_DOT);
  setUiFont(typingStatusLabel);
  lv_obj_set_style_text_color(typingStatusLabel, lv_color_hex(0xA6ADC8), 0);
  lv_label_set_text(typingStatusLabel, typingStatusText);

  lv_obj_t *footRow = makeHintRow();
  addHintPair(footRow, "1", "short");
  addHintPair(footRow, "2", "long");
  addHintPair(footRow, "bksp", "fix");
  addHintPair(footRow, "esc", "back");
  // Right-aligned twin of the hint row, the editor menuRow trick: settings
  // and restart as blue command chips so they read as actions rather than
  // another mode key.
  lv_obj_t *retryRow = makeHintRow();
  lv_obj_set_flex_align(retryRow, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  makeHintKeycapMenu(retryRow, "ctrl-o");
  makeHintWord(retryRow, "settings");
  makeHintKeycapMenu(retryRow, "tab");
  makeHintWord(retryRow, "retry");
}

void showTypingPractice() {
  // Like the help sheet: reachable with unsaved changes and never touches
  // the document. BACK always returns to the main menu.
  buildTypingPracticeScreen();
  startTypingRun();
}

// --- Typing practice settings ------------------------------------------------
// The rule set lives one screen down from the sprint, in the same panel/row
// language as Settings: two choice rows and four toggles at a 36 px pitch.

void cycleTypingComplexity() {
  typingComplexity = static_cast<uint8_t>(
      (typingComplexity + 1) % TYPING_COMPLEXITY_COUNT);
  prefsDirty = true;
  if (typingComplexityValue) {
    lv_label_set_text(typingComplexityValue,
                      TYPING_COMPLEXITY_NAMES[typingComplexity]);
  }
  statusMessage = std::string("Complexity: ") +
                  TYPING_COMPLEXITY_NAMES[typingComplexity];
  savePrefs();
}
void complexityClicked(lv_event_t *) { cycleTypingComplexity(); }

void cycleTypingMaxLength() {
  typingMaxLengthIndex =
      (typingMaxLengthIndex + 1) % TYPING_MAX_LENGTH_COUNT;
  prefsDirty = true;
  if (typingMaxLengthValue) {
    lv_label_set_text(typingMaxLengthValue,
                      TYPING_MAX_LENGTH_NAMES[typingMaxLengthIndex]);
  }
  statusMessage = std::string("Max length: ") +
                  TYPING_MAX_LENGTH_NAMES[typingMaxLengthIndex];
  savePrefs();
}
void maxLengthClicked(lv_event_t *) { cycleTypingMaxLength(); }

void toggleTypingSymbols() {
  typingSymbols = !typingSymbols;
  prefsDirty = true;
  statusMessage = typingSymbols ? "Typing symbols on" : "Typing symbols off";
  syncSwitchState(typingSymbolsSwitch, typingSymbols);
  savePrefs();
}
void typingSymbolsSwitchChanged(lv_event_t *) {
  adoptSwitchTap(typingSymbolsSwitch, typingSymbols);
  statusMessage = typingSymbols ? "Typing symbols on" : "Typing symbols off";
  savePrefs();
}

void toggleTypingNumbers() {
  typingNumbers = !typingNumbers;
  prefsDirty = true;
  statusMessage = typingNumbers ? "Typing numbers on" : "Typing numbers off";
  syncSwitchState(typingNumbersSwitch, typingNumbers);
  savePrefs();
}
void typingNumbersSwitchChanged(lv_event_t *) {
  adoptSwitchTap(typingNumbersSwitch, typingNumbers);
  statusMessage = typingNumbers ? "Typing numbers on" : "Typing numbers off";
  savePrefs();
}

void toggleTypingCapitalization() {
  typingCapitalization = !typingCapitalization;
  prefsDirty = true;
  statusMessage = typingCapitalization ? "Capitalization on"
                                       : "Capitalization off";
  syncSwitchState(typingCapitalSwitch, typingCapitalization);
  savePrefs();
}
void typingCapitalSwitchChanged(lv_event_t *) {
  adoptSwitchTap(typingCapitalSwitch, typingCapitalization);
  statusMessage = typingCapitalization ? "Capitalization on"
                                       : "Capitalization off";
  savePrefs();
}

void toggleTypingPunctuation() {
  typingPunctuation = !typingPunctuation;
  prefsDirty = true;
  statusMessage = typingPunctuation ? "Punctuation on"
                                    : "Punctuation off";
  syncSwitchState(typingPunctuationSwitch, typingPunctuation);
  savePrefs();
}
void typingPunctuationSwitchChanged(lv_event_t *) {
  adoptSwitchTap(typingPunctuationSwitch, typingPunctuation);
  statusMessage = typingPunctuation ? "Punctuation on"
                                    : "Punctuation off";
  savePrefs();
}

void typingSettingsBackClicked(lv_event_t *) {
  savePrefs();
  typingSettingsVisible = false;
  // The new rules apply immediately: rebuild and start a fresh sprint.
  showTypingPractice();
}

void buildTypingSettingsScreen() {
  editorVisible = false;
  typingVisible = false;
  typingSettingsVisible = true;
  settingsVisible = pairingVisible = menuVisible = startVisible = false;
  helpVisible = previewVisible = false;
  wifiScreenVisible = false;
  lv_obj_clean(lv_scr_act());
  pairingStatusLabel = settingsRadioLabel = brightnessStatusLabel = nullptr;
  brightnessSlider = smoothSwitch = markdownSwitch = autoSaveSwitch =
      dimSwitch = lineNumberSwitch = wordWrapSwitch = nullptr;
  typingWordsLabel = typingStatusLabel = nullptr;
  typingComplexityValue = typingMaxLengthValue = nullptr;
  typingSymbolsSwitch = typingNumbersSwitch = typingCapitalSwitch =
      typingPunctuationSwitch = nullptr;
  typingLiveBar = typingSummaryBar = nullptr;
  for (size_t i = 0; i < TYPING_LIVE_COUNT; ++i) typingLiveValue[i] = nullptr;
  for (size_t i = 0; i < TYPING_SUM_COUNT; ++i) typingSummaryValue[i] = nullptr;
  lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x11111B), 0);

  lv_obj_t *header = lv_obj_create(lv_scr_act());
  lv_obj_set_size(header, SCREEN_W - 8, 30);
  lv_obj_set_pos(header, 4, 4);
  stylePanel(header, lv_color_hex(0x181825), lv_color_hex(0x313244));
  lv_obj_t *heading = lv_label_create(header);
  lv_label_set_text(heading, "TYPING SETTINGS");
  lv_obj_set_pos(heading, 10, 4);
  setTitleFont(heading);
  lv_obj_set_style_text_color(heading, lv_color_hex(0x96A57E), 0);
  makeButton(header, "BACK", 412, 54, typingSettingsBackClicked);

  lv_obj_t *panel = lv_obj_create(lv_scr_act());
  lv_obj_set_size(panel, SCREEN_W - 8, 240);
  lv_obj_set_pos(panel, 4, 39);
  stylePanel(panel, lv_color_hex(0x11111B), lv_color_hex(0x313244));

  // Two choice rows then four toggles, full-width above the note. Tapping a
  // pill cycles it; the underlined letter does the same from the keyboard.
  typingComplexityValue = makeChoiceRow(panel, "", 'C', "omplexity", 10,
      TYPING_COMPLEXITY_NAMES[typingComplexity], complexityClicked);
  typingMaxLengthValue = makeChoiceRow(panel, "", 'M', "ax Length", 46,
      TYPING_MAX_LENGTH_NAMES[typingMaxLengthIndex], maxLengthClicked);
  typingSymbolsSwitch = makeToggleRow(panel, "", 'S', "ymbols", 82,
      typingSymbols, typingSymbolsSwitchChanged, 14);
  typingNumbersSwitch = makeToggleRow(panel, "", 'N', "umbers", 118,
      typingNumbers, typingNumbersSwitchChanged, 14);
  typingCapitalSwitch = makeToggleRow(panel, "C", 'a', "pitalization", 154,
      typingCapitalization, typingCapitalSwitchChanged, 14);
  typingPunctuationSwitch = makeToggleRow(panel, "", 'P', "unctuation", 190,
      typingPunctuation, typingPunctuationSwitchChanged, 14);

  lv_obj_t *note = lv_label_create(panel);
  lv_label_set_text(note, "Changes start a fresh run.");
  lv_obj_set_pos(note, 14, 220);
  setUiFont(note);
  lv_obj_set_style_text_color(note, lv_color_hex(0x7F849C), 0);

  lv_obj_t *footRow = makeHintRow();
  addHintPair(footRow, "c/m", "change");
  makeHintKeycap(footRow, "s");
  makeHintKeycap(footRow, "n");
  makeHintKeycap(footRow, "a");
  makeHintKeycap(footRow, "p");
  addHintPair(footRow, "esc", "back");
}

void showTypingSettings() {
  typingSettingsVisible = true;
  typingVisible = false;
  buildTypingSettingsScreen();
}

#ifdef WIAW_BATTERY_SENSE
// --- Optional battery gauge; hardware recipe in docs/USAGE.md "Power" ------
// EXT_IO1 (GPIO10, ADC1_CH9) on the expansion connector is one of only two
// broken-out ADC1 pins; the other ADC pins there (GPIO12-14) are ADC2, which
// cannot be read while Wi-Fi runs, and the transfer AP uses Wi-Fi.
// Divider: battery+ -> 100k -> pin, pin -> 100k -> GND, 100 nF pin -> GND
// (the cap gives the ADC sampler a low-impedance source through the ~50k
// divider impedance).
constexpr int BATTERY_SENSE_PIN = 10;
// Equal-value divider, the pin sees battery/2; num/den stay integer so a
// different resistor pair is a two-line change.
constexpr int BATTERY_DIVIDER_NUM = 2;
constexpr int BATTERY_DIVIDER_DEN = 1;
// One-time trim: multimeter reading minus reported millivolts. The eFuse
// calibration behind analogReadMilliVolts is usually within ~30 mV.
constexpr int BATTERY_CALIBRATION_MV = 0;
// Averaged because single shots hop around at divider impedances.
constexpr int BATTERY_SAMPLES = 16;
// The status bar repaints ~1 Hz but LiPo voltage moves on the scale of
// minutes; throttle the ADC to this period.
constexpr uint32_t BATTERY_READ_INTERVAL_MS = 10000;
// Piecewise-linear LiPo rest curve. Coarse on purpose: the cell's discharge
// is flat through the middle and sags under load, so this is an indicator,
// not a fuel gauge. The last point is the usable floor, not zero volts.
constexpr int BATTERY_CURVE[][2] = {
    {4200, 100}, {4090, 90}, {3990, 80}, {3890, 70}, {3790, 60},
    {3700, 50},  {3610, 40},  {3520, 30},  {3440, 15}, {3300, 0},
};
constexpr size_t BATTERY_CURVE_POINTS =
    sizeof(BATTERY_CURVE) / sizeof(BATTERY_CURVE[0]);
uint32_t lastBatteryReadMs = 0;
int batteryMillivolts = 0;
int batteryPercent = 0;

int readBatteryMillivolts() {
  uint32_t total = 0;
  for (int i = 0; i < BATTERY_SAMPLES; ++i) {
    total += analogReadMilliVolts(BATTERY_SENSE_PIN);
  }
  return int(total / BATTERY_SAMPLES) * BATTERY_DIVIDER_NUM /
         BATTERY_DIVIDER_DEN + BATTERY_CALIBRATION_MV;
}

int batteryPercentFor(int mv) {
  if (mv >= BATTERY_CURVE[0][0]) return BATTERY_CURVE[0][1];
  for (size_t i = 1; i < BATTERY_CURVE_POINTS; ++i) {
    if (mv >= BATTERY_CURVE[i][0]) {
      const int vHigh = BATTERY_CURVE[i - 1][0];
      const int pHigh = BATTERY_CURVE[i - 1][1];
      const int vLow = BATTERY_CURVE[i][0];
      const int pLow = BATTERY_CURVE[i][1];
      return pLow + (mv - vLow) * (pHigh - pLow) / (vHigh - vLow);
    }
  }
  return 0;
}

// Called from updateStatusBar() (~1 Hz); self-throttled to
// BATTERY_READ_INTERVAL_MS so the steady-state cost is one short ADC burst
// every 10 s, with no allocation.
void refreshBattery() {
  const uint32_t now = millis();
  if (now - lastBatteryReadMs < BATTERY_READ_INTERVAL_MS) return;
  lastBatteryReadMs = now;
  batteryMillivolts = readBatteryMillivolts();
  batteryPercent = batteryPercentFor(batteryMillivolts);
}
#endif  // WIAW_BATTERY_SENSE

// Top-right header indicator: measured charge when WIAW_BATTERY_SENSE is
// built in, the static external-power label otherwise.
void setPowerLabelText() {
#ifdef WIAW_BATTERY_SENSE
  // Percent is 0-100, so "100%" plus NUL cannot overflow 8 bytes.
  char text[8];
  snprintf(text, sizeof(text), "%d%%", batteryPercent);
  labelSetTextIfChanged(powerLabel, text);
#else
  labelSetTextIfChanged(powerLabel, "5V");
#endif
}

// Start-screen command rows. Touch users get the same four actions as the
// n/f/r/q keys; the row's user data is the StartCommand index.
enum StartCommand : intptr_t { START_NEW, START_FIND, START_RECENT, START_QUIT };

void startCommandClicked(lv_event_t *event) {
  const intptr_t command =
      reinterpret_cast<intptr_t>(lv_event_get_user_data(event));
  switch (command) {
  case START_NEW: newDocument(); break;
  // `f` arms the browser's filter so typing starts narrowing immediately.
  case START_FIND: showBrowser(false, true); break;
  case START_RECENT:
    if (!recentFiles.empty()) {
      recentSelection = 0;
      buildStartScreen();
    }
    break;
  case START_QUIT: showMainMenu(); break;
  default: break;
  }
}

// The editor's start screen: LazyVim's dashboard shape — command keys on the
// left, recent documents on the right. Reached only from the main menu while
// no buffer is open; Esc / BACK / q return to the menu.
void buildStartScreen() {
  editorVisible = false;
  startVisible = true;
  menuVisible = settingsVisible = pairingVisible = false;
  helpVisible = typingVisible = typingSettingsVisible = previewVisible = false;
  wifiScreenVisible = false;
  lv_obj_clean(lv_scr_act());
  pairingStatusLabel = settingsRadioLabel = brightnessStatusLabel = nullptr;
  menuRadioLabel = nullptr;
  brightnessSlider = smoothSwitch = markdownSwitch = autoSaveSwitch =
      dimSwitch = lineNumberSwitch = wordWrapSwitch = nullptr;
  lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x11111B), 0);

  lv_obj_t *header = lv_obj_create(lv_scr_act());
  lv_obj_set_size(header, SCREEN_W - 8, 30);
  lv_obj_set_pos(header, 4, 4);
  stylePanel(header, lv_color_hex(0x181825), lv_color_hex(0x313244));
  lv_obj_t *heading = lv_label_create(header);
  lv_label_set_text(heading, "TEXT EDITOR");
  lv_obj_set_pos(heading, 10, 4);
  setTitleFont(heading);
  lv_obj_set_style_text_color(heading, lv_color_hex(0x96A57E), 0);
  makeButton(header, "BACK", 412, 54, backClicked);  // backTarget is Menu

  // Left column: n / f / r / q, one button row each.
  lv_obj_t *commands = lv_obj_create(lv_scr_act());
  lv_obj_set_size(commands, 222, 240);
  lv_obj_set_pos(commands, 4, 39);
  stylePanel(commands, lv_color_hex(0x11111B), lv_color_hex(0x313244));
  lv_obj_t *cmdTitle = lv_label_create(commands);
  lv_label_set_text(cmdTitle, "COMMANDS");
  lv_obj_set_pos(cmdTitle, 12, 8);
  setUiFont(cmdTitle);
  lv_obj_set_style_text_color(cmdTitle, lv_color_hex(0x7F849C), 0);
  static const struct { const char *key; const char *label; } rows[4] = {
      {"n", "New file"}, {"f", "Find a file"},
      {"r", "Recent files"}, {"q", "Quit to menu"},
  };
  for (intptr_t i = 0; i < 4; ++i) {
    lv_obj_t *button = lv_btn_create(commands);
    lv_obj_set_size(button, 194, 30);
    lv_obj_set_pos(button, 12, 30 + i * 38);
    stylePanel(button, lv_color_hex(0x1E1E2E), lv_color_hex(0x45475A));
    lv_obj_add_event_cb(button, [](lv_event_t *) {
      lastInputMs = millis();
      if (backlightDimmed) {
        display.setBrightness(brightnessValue);
        backlightDimmed = false;
      }
    }, LV_EVENT_CLICKED, nullptr);
    lv_obj_add_event_cb(button, startCommandClicked, LV_EVENT_CLICKED,
                        reinterpret_cast<void *>(i));
    lv_obj_t *cap = makeHintKeycap(button, rows[i].key);
    lv_obj_set_pos(cap, 10, 7);
    lv_obj_t *text = lv_label_create(button);
    lv_label_set_text(text, rows[i].label);
    lv_obj_set_pos(text, 40, 7);
    setUiFont(text);
    lv_obj_set_style_text_color(text, lv_color_hex(0xCDD6F4), 0);
  }

  // Right column: the persisted recent documents, newest first.
  lv_obj_t *recent = lv_obj_create(lv_scr_act());
  lv_obj_set_size(recent, 246, 240);
  lv_obj_set_pos(recent, 230, 39);
  stylePanel(recent, lv_color_hex(0x11111B), lv_color_hex(0x313244));
  lv_obj_t *recTitle = lv_label_create(recent);
  lv_label_set_text(recTitle, "RECENT");
  lv_obj_set_pos(recTitle, 12, 8);
  setUiFont(recTitle);
  lv_obj_set_style_text_color(recTitle, lv_color_hex(0x7F849C), 0);
  if (recentFiles.empty()) {
    lv_obj_t *none = lv_label_create(recent);
    lv_label_set_text(none, "No recent files");
    lv_obj_set_pos(none, 12, 38);
    setUiFont(none);
    lv_obj_set_style_text_color(none, lv_color_hex(0x7F849C), 0);
  } else {
    for (size_t i = 0; i < recentFiles.size(); ++i) {
      const size_t slash = recentFiles[i].rfind('/');
      const std::string name = slash == std::string::npos
          ? recentFiles[i] : recentFiles[i].substr(slash + 1);
      lv_obj_t *row = lv_btn_create(recent);
      lv_obj_set_size(row, 222, 30);
      lv_obj_set_pos(row, 8, 30 + static_cast<int>(i) * 34);
      stylePanel(row, i == recentSelection ? lv_color_hex(0x313244)
                                           : lv_color_hex(0x11111B),
                 lv_color_hex(0x313244));
      lv_obj_add_event_cb(row, recentClicked, LV_EVENT_CLICKED,
                          reinterpret_cast<void *>(static_cast<intptr_t>(i)));
      lv_obj_t *label = lv_label_create(row);
      lv_obj_set_width(label, 204);
      lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
      lv_label_set_text(label, name.c_str());
      lv_obj_set_pos(label, 7, 7);
      setUiFont(label);
      lv_obj_set_style_text_color(label,
          i == recentSelection ? lv_color_hex(0xB5C39D)
                               : lv_color_hex(0xBAC2DE), 0);
    }
  }

  lv_obj_t *footRow = makeHintRow();
  addHintPair(footRow, "n", "new");
  addHintPair(footRow, "f", "find");
  addHintPair(footRow, "r", "recent");
  addHintPair(footRow, "q", "menu");
  addHintPair(footRow, "enter", "open");
}

// Selection nudge shared by j/k and the arrow keys on the start screen.
void moveRecentSelection(int step) {
  if (recentFiles.empty()) return;
  const size_t count = recentFiles.size();
  recentSelection = (recentSelection + count + static_cast<size_t>(step)) % count;
  buildStartScreen();
}

void buildEditorScreen() {
  editorVisible = true;
  pairingVisible = settingsVisible = menuVisible = startVisible =
      pairingReturnsToSettings = false;
  helpVisible = typingVisible = typingSettingsVisible = previewVisible = false;
  wifiScreenVisible = false;
  historyVisible = false;
  refreshEditorMetrics();
  lv_obj_clean(lv_scr_act());
  // lv_obj_clean deleted the previous screen's labels; drop every stale
  // pointer so a shorter band table can never read one.
  for (int i = 0; i < MAX_VISIBLE_LINES; ++i) {
    lineLabels[i] = nullptr;
    gutterLabels[i] = nullptr;
  }
  cursorBlock = nullptr;
  cursorGlyph = nullptr;
  resetEditorPaint();
  pairingStatusLabel = nullptr;
  settingsRadioLabel = nullptr;
  brightnessStatusLabel = nullptr;
  zenHintLabel = nullptr;
  brightnessSlider = smoothSwitch = markdownSwitch = autoSaveSwitch =
      dimSwitch = lineNumberSwitch = wordWrapSwitch = nullptr;
  lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x11111B), 0);

  lv_obj_t *header = lv_obj_create(lv_scr_act());
  // 36 px tall (was 30) so the five editor controls are 28 px tall and easier
  // to tap. The canvas below starts at 41 (editorCanvasY) with its height
  // reduced by 2, so the page bottom and the 221 px text area are unchanged.
  // Other screens keep their 30 px bar.
  lv_obj_set_size(header, SCREEN_W - 8, 36);
  lv_obj_set_pos(header, 4, 4);
  stylePanel(header, lv_color_hex(0x181825), lv_color_hex(0x313244));
  makeToolbarButton(header, ICON_MENU, 3, 30, menuClicked, true);

  pathLabel = lv_label_create(header);
  lv_label_set_text(pathLabel, "~/");
  // 14 px line box centered in the 34 px content area: (34 - 14) / 2 = 10.
  lv_obj_set_pos(pathLabel, 40, 10);
  lv_obj_set_style_text_font(pathLabel, &terminess_14, 0);
  lv_obj_set_style_text_color(pathLabel, lv_color_hex(0x89B4FA), 0);

  documentLabel = lv_label_create(header);
  lv_obj_set_width(documentLabel, 200);
  lv_label_set_long_mode(documentLabel, LV_LABEL_LONG_DOT);
  lv_obj_set_pos(documentLabel, 70, 10);
  lv_obj_set_style_text_font(documentLabel, &elms_bold_14, 0);
  lv_obj_set_style_text_color(documentLabel, lv_color_hex(0xBAC2DE), 0);
  // Text-size pair in the gap between the filename and the power indicator:
  // 30 px buttons at 286 and 320, both clear of the filename's 200 px box
  // (70..270) and the power readout at 374. Clamped, not cyclic.
  makeToolbarButton(header, "A-", 286, 30, editorFontSmallerClicked);
  makeToolbarButton(header, "A+", 320, 30, editorFontLargerClicked);
  // Same 30 px footprint as the menu button, mirrored into the right corner:
  // 472 (header) - 4 (outer gap) - 1 (border) - 30 (width) = 437.
  makeToolbarButton(header, ICON_SETTINGS, 437, 30, settingsClicked, true);
  // Power indicator, then the Bluetooth shortcut square: the pair sits to the
  // left of the gear in that order. The label is right-aligned inside a 31 px
  // box ending at 405, so "5V" hugs the button (1 px gap) and the widest
  // reading, "100%" at 28.75 px, still ends flush at 405. radioLabel is the
  // icon label inside the button, so updateStatusBar() keeps tinting it by
  // BLE state.
  powerLabel = lv_label_create(header);
  setPowerLabelText();
  lv_obj_set_width(powerLabel, 31);
  lv_label_set_long_mode(powerLabel, LV_LABEL_LONG_CLIP);
  lv_obj_set_style_text_align(powerLabel, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_pos(powerLabel, 374, 10);
  setUiFont(powerLabel);
  lv_obj_set_style_text_color(powerLabel, lv_color_hex(0x96A57E), 0);
  // 28 px tall to match the editor's other header buttons.
  radioLabel = makeBluetoothButton(header, 406, 28);

  lv_obj_t *canvas = lv_obj_create(lv_scr_act());
  // 4 px screen margins, matching the header when it is present; Zen grows
  // the page to the full screen.
  lv_obj_set_size(canvas, SCREEN_W - 8, editorCanvasH());
  lv_obj_set_pos(canvas, 4, editorCanvasY());
  stylePanel(canvas, lv_color_hex(0x11111B), lv_color_hex(0x313244));
  // Line-number rail shifts text right when the gutter toggle is on; label
  // width shrinks by the same 34 px so LV_LABEL_LONG_CLIP stays at the edge.
  const int textX = editorLineNumbers ? 41 : 7;
  const int textW = editorLineNumbers ? SCREEN_W - 28 - 34 : SCREEN_W - 28;
  // Terminess's line box equals the glyph cell, so its ink already fills the
  // row; unlike the old Space Mono cuts it needs no lift.
  const int lineInkLift = 0;
  for (int i = 0; i < editorVisibleLines; ++i) {
    lineLabels[i] = lv_label_create(canvas);
    lv_obj_set_size(lineLabels[i], textW, editorLineHeight);
    lv_obj_set_pos(lineLabels[i], textX,
                   editorTextTop() + i * editorLineHeight);
    lv_label_set_recolor(lineLabels[i], true);
    lv_label_set_long_mode(lineLabels[i], LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_color(lineLabels[i], lv_color_hex(0xBAC2DE), 0);
    lv_obj_set_style_text_font(lineLabels[i], editorFont(), 0);
    lv_obj_set_style_pad_left(lineLabels[i], 4, 0);
    // Terminess's line box equals its glyph cell, so the pad stays 0: any
    // lift would clip the top row of the bitmap.
    lv_obj_set_style_pad_top(lineLabels[i], lineInkLift, 0);
    lv_obj_set_style_pad_bottom(lineLabels[i], 0, 0);
    lv_obj_set_style_radius(lineLabels[i], 2, 0);
    // No left border on the cursor row: a border would shift that row's
    // content 1 px right and desync it from the cursor block. Set once here;
    // the render loop no longer restamps it per band.
    lv_obj_set_style_border_width(lineLabels[i], 0, 0);
  }

  // Line-number gutter labels, right-aligned into a 34 px rail. elms_bold_12
  // keeps a 4-digit number inside the rail; the render loop fills the text.
  // The UI font's line box is 14 px, so rows shorter than that keep a 14 px
  // label (transparent overflow) and rows taller center it in the pitch.
  const int gutterH = std::max(editorLineHeight, 14);
  const int gutterPad = std::max(0, (editorLineHeight - 13) / 2);
  for (int i = 0; i < editorVisibleLines; ++i) {
    if (editorLineNumbers) {
      gutterLabels[i] = lv_label_create(canvas);
      lv_obj_set_size(gutterLabels[i], 34, gutterH);
      lv_obj_set_pos(gutterLabels[i], 2,
                     editorTextTop() + i * editorLineHeight);
      lv_label_set_long_mode(gutterLabels[i], LV_LABEL_LONG_CLIP);
      setUiFont(gutterLabels[i]);
      lv_obj_set_style_text_align(gutterLabels[i], LV_TEXT_ALIGN_RIGHT, 0);
      lv_obj_set_style_text_color(gutterLabels[i], lv_color_hex(0x7F849C), 0);
      lv_obj_set_style_pad_top(gutterLabels[i], gutterPad, 0);
    } else {
      gutterLabels[i] = nullptr;
    }
  }

  // Live cursor block: a filled rectangle that blinks at the caret. Its
  // width is re-measured every refresh to exactly one character cell.
  cursorBlock = lv_obj_create(canvas);
  lv_obj_remove_style_all(cursorBlock);
  lv_obj_set_size(cursorBlock, editorGlyphWidth, editorLineHeight - 2);
  lv_obj_set_style_bg_color(cursorBlock, lv_color_hex(CURSOR_BLUE), 0);
  lv_obj_set_style_bg_opa(cursorBlock, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(cursorBlock, 0, 0);
  lv_obj_set_style_border_width(cursorBlock, 0, 0);
  // The outline phase reuses this object: stamp its border once and let the
  // blink toggle only bg_opa and the border width (two props, no restyle).
  lv_obj_set_style_border_color(cursorBlock, lv_color_hex(CURSOR_BLUE), 0);
  lv_obj_set_style_border_opa(cursorBlock, LV_OPA_COVER, 0);
  lv_obj_set_style_border_side(cursorBlock, LV_BORDER_SIDE_FULL, 0);
  lv_obj_clear_flag(cursorBlock, LV_OBJ_FLAG_SCROLLABLE);

  // The character reversed out of the solid phase: one glyph in the band's
  // own background colour, drawn above the block. A sibling (not a child) so
  // the block's cell height cannot clip the glyph's ascenders. Its geometry
  // is set by the render pass; created hidden so a rebuild never flashes it at
  // a stale position.
  cursorGlyph = lv_label_create(canvas);
  lv_label_set_long_mode(cursorGlyph, LV_LABEL_LONG_CLIP);
  lv_label_set_recolor(cursorGlyph, false);
  lv_obj_set_size(cursorGlyph, editorGlyphWidth + 4, editorLineHeight);
  lv_obj_set_style_text_font(cursorGlyph, editorFont(), 0);
  lv_obj_set_style_text_color(cursorGlyph, lv_color_hex(0x181825), 0);
  // No left pad: x is already the caret's text origin, which the block shares.
  // The same ink lift as the band labels puts the reversed glyph exactly on
  // the one it covers.
  lv_obj_set_style_pad_left(cursorGlyph, 0, 0);
  lv_obj_set_style_pad_top(cursorGlyph, lineInkLift, 0);
  lv_obj_set_style_pad_bottom(cursorGlyph, 0, 0);
  lv_obj_add_flag(cursorGlyph, LV_OBJ_FLAG_HIDDEN);

  lv_obj_t *statusPanel = lv_obj_create(lv_scr_act());
  // 4 px screen margins, matching the header and text area.
  lv_obj_set_size(statusPanel, SCREEN_W - 8, 25);
  lv_obj_set_pos(statusPanel, 4, 274);
  stylePanel(statusPanel, lv_color_hex(0x181825), lv_color_hex(0x313244));
  modeLabel = lv_label_create(statusPanel);
  // Vertically center the pill inside the 25 px status panel: the pill is
  // line_height(14) + 1 + 1 padding = 16 px tall, and the panel's content
  // area is 23 px (25 minus the 1 px borders), so y = (23 - 16) / 2 = 3.
  lv_obj_set_pos(modeLabel, 4, 3);
  lv_obj_set_style_text_font(modeLabel, &terminess_14, 0);
  lv_obj_set_style_text_color(modeLabel, lv_color_hex(0x11111B), 0);
  lv_obj_set_style_bg_color(modeLabel, lv_color_hex(0x96A57E), 0);
  lv_obj_set_style_bg_opa(modeLabel, LV_OPA_COVER, 0);
  lv_obj_set_style_pad_left(modeLabel, 6, 0);
  lv_obj_set_style_pad_right(modeLabel, 6, 0);
  lv_obj_set_style_pad_top(modeLabel, 1, 0);
  lv_obj_set_style_pad_bottom(modeLabel, 1, 0);
  lv_obj_set_style_radius(modeLabel, 3, 0);
  statusLabel = lv_label_create(statusPanel);
  lv_obj_set_width(statusLabel, 228);
  // One line only. The panel's content area is 23 px tall, so a wrapped
  // second line would be clipped by the panel border; LONG_SCROLL_CIRCULAR
  // keeps the label's self height at a single line and marquees any overflow
  // through the 228 px window instead. 17 px height leaves 3 px of descender
  // room under the 14 px line and still ends inside the panel (5 + 17 = 22).
  lv_obj_set_height(statusLabel, 17);
  lv_label_set_long_mode(statusLabel, LV_LABEL_LONG_SCROLL_CIRCULAR);
  // Default marquee speed is dpi / 3 (~43 px/s); 60 px/s reads comfortably
  // for a 40-character status line without becoming a blur.
  lv_obj_set_style_anim_speed(statusLabel, 60, 0);
  lv_obj_set_pos(statusLabel, 104, 5);
  setUiFont(statusLabel);
  lv_obj_set_style_text_color(statusLabel, lv_color_hex(0xA6ADC8), 0);
  footerLabel = lv_label_create(statusPanel);
  // Right-justified: the line/column (and touch) coordinates hug the
  // panel's right edge, mirroring the top bar's settings button.
  lv_obj_set_width(footerLabel, 126);
  lv_obj_set_style_text_align(footerLabel, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_pos(footerLabel, 340, 5);
  setUiFont(footerLabel);
  lv_obj_set_style_text_color(footerLabel, lv_color_hex(0x89B4FA), 0);

  // Bottom hint line: keycap chips for the mode-switch keys plus muted
  // captions, refreshed per mode in updateStatusBar(). While a ":" or "/"
  // command is typed the chips hide and hintLabel echoes the command.
  // The row is centered in the strip below the status panel (panel spans
  // 274..299) so the chips clear both the panel border and the screen
  // edge: chip tops at 301, bottoms at 317.
  hintLabel = lv_label_create(lv_scr_act());
  lv_obj_set_pos(hintLabel, 8, 302);
  lv_obj_set_width(hintLabel, SCREEN_W - 16);
  lv_label_set_long_mode(hintLabel, LV_LABEL_LONG_DOT);
  setUiFont(hintLabel);
  lv_obj_set_style_text_color(hintLabel, lv_color_hex(0x7F849C), 0);
  // Invisible flex row for the chips: left-aligned, vertically centered.
  // Styled transparent rather than lv_obj_remove_style_all(), which trips
  // a deprecation warning inside this LVGL version's own header.
  hintRow = lv_obj_create(lv_scr_act());
  lv_obj_set_style_bg_opa(hintRow, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(hintRow, 0, 0);
  lv_obj_set_style_pad_all(hintRow, 0, 0);
  lv_obj_set_pos(hintRow, 8, 299);
  lv_obj_set_size(hintRow, SCREEN_W - 16, 21);
  lv_obj_clear_flag(hintRow, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_layout(hintRow, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(hintRow, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(hintRow, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(hintRow, 4, 0);
  hintKeyI = makeHintKeycap(hintRow, "i");
  hintWordInput = makeHintWord(hintRow, "input");
  hintKeyV = makeHintKeycap(hintRow, "v");
  hintWordVisual = makeHintWord(hintRow, "visual");
  hintKeyColon = makeHintKeycap(hintRow, ":");
  hintWordCommands = makeHintWord(hintRow, "commands");
  hintKeyQuestion = makeHintKeycap(hintRow, "?");
  hintWordHelp = makeHintWord(hintRow, "help");
  // Invisible trailing padding after each caption keeps the key groups
  // apart (4 px flex gap + 4 px pad), mirroring the old "i write   v
  // visual" rhythm across all four pairs.
  lv_obj_set_style_pad_right(hintWordInput, 4, 0);
  lv_obj_set_style_pad_right(hintWordVisual, 4, 0);
  lv_obj_set_style_pad_right(hintWordCommands, 4, 0);
  hintKeyEsc = makeHintKeycap(hintRow, "esc");
  hintWordNormal = makeHintWord(hintRow, "normal");

  // Right-aligned twin of the hint row for the menu shortcuts: same
  // transparent geometry, children hugging the right edge. The two groups
  // coexist only in Normal (236 px of chips at left, ~112 px at right),
  // which the glyph advances of elms_bold_12 were summed to verify fits
  // the 464 px row without colliding.
  lv_obj_t *menuRow = makeHintRow();
  lv_obj_set_flex_align(menuRow, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  hintKeyMenu = makeHintKeycapMenu(menuRow, ":menu");
  hintKeySettings = makeHintKeycapMenu(menuRow, ":settings");

  if (editorZenMode) {
    // The chrome is still built (the pointers above expect it), then hidden;
    // the canvas already covers the full screen underneath.
    lv_obj_add_flag(header, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(statusPanel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(hintLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(hintRow, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(menuRow, LV_OBJ_FLAG_HIDDEN);
    // Bottom helper: visible for 10 s, re-shown by any single tap, and also a
    // tap target that exits even if the panel reports only one point.
    zenHintLabel = lv_btn_create(lv_scr_act());
    lv_obj_set_size(zenHintLabel, 224, 20);
    lv_obj_align(zenHintLabel, LV_ALIGN_BOTTOM_MID, 0, -2);
    stylePanel(zenHintLabel, lv_color_hex(0x181825), lv_color_hex(0x45475A));
    lv_obj_add_event_cb(zenHintLabel, zenHintClicked, LV_EVENT_CLICKED, nullptr);
    lv_obj_t *hintText = lv_label_create(zenHintLabel);
    lv_label_set_text(hintText, "esc or two-finger tap to exit");
    lv_obj_center(hintText);
    setUiFont(hintText);
    lv_obj_set_style_text_color(hintText, lv_color_hex(0x96A57E), 0);
    zenHintHideMs = millis() + 10000;
  }

  updateEditorLines();
  updateStatusBar();
  uiNeedsRender = false;
}

// Help-screen entry: a null key marks a section heading row. Descriptions
// follow the classic cheat sheet at vim.rtorr.com, shortened where a 182 px
// column cannot hold the full sentence. Only keys this editor actually
// implements are listed.
struct HelpEntry {
  const char *key;
  const char *text;
};

const HelpEntry HELP_COLUMN_A[] = {
    {nullptr, "MOVEMENT"},
    // One chip for the four motions: the sheet has no room for a row each.
    {"hjkl", "move cursor"},
    {"w", "jump to the start of a word"},
    {"b", "jump back to word start"},
    {"0", "jump to the start of the line"},
    {"$", "jump to end of line"},
    {"G", "end of document (:42 jumps)"},
    {nullptr, "INSERT"},
    {"i", "insert before the cursor"},
    {"a", "insert after the cursor"},
    {"A", "insert at end of line"},
    {"o", "open a new line below"},
    {"O", "open a new line above"},
    {"Esc", "exit mode / leave zen"},
    {nullptr, "COMMANDS"},
    {":font", "10, 12, 14 or 16 px"},
    {"+-", "text size up / down"},
};

const HelpEntry HELP_COLUMN_B[] = {
    {nullptr, "EDIT"},
    {"x", "delete (cut) character"},
    {"dd", "delete (cut) a line"},
    {"dw", "delete (cut) to next word"},
    {"d$", "delete (cut) to end of line"},
    {"yy", "yank (copy) a line"},
    {"y$", "yank (copy) to end of the line"},
    {"cc", "change (replace) entire line"},
    {"cw", "change to end of word"},
    {"p", "put (paste) after the cursor"},
    {"u", "undo (Ctrl-R redo)"},
    {nullptr, "VISUAL"},
    {"v", "start visual mode, mark text"},
    {"y", "yank (copy) marked text"},
    {"d/x", "delete (cut) marked text"},
    {nullptr, "SEARCH"},
    {"/", "search; n/N repeat"},
};

// One help column: heading rows in green, otherwise a keycap chip plus a
// DOT-truncated description that can never bleed into the other column.
// The description's height must be exactly one line: LVGL only elides to dots
// when the text overflows a bounded label box. With LV_SIZE_CONTENT the label
// grows to a second line instead and prints over the row below it.
void renderHelpColumn(const HelpEntry *entries, size_t count, int chipX,
                      int descX, int descW) {
  int y = 44;
  for (size_t i = 0; i < count; ++i, y += 16) {
    const HelpEntry &entry = entries[i];
    if (!entry.key) {
      lv_obj_t *section = lv_label_create(lv_scr_act());
      lv_label_set_text(section, entry.text);
      lv_obj_set_pos(section, chipX, y);
      setUiFont(section);
      lv_obj_set_style_text_color(section, lv_color_hex(0x96A57E), 0);
      continue;
    }
    lv_obj_t *chip = makeHintKeycap(lv_scr_act(), entry.key);
    lv_obj_set_pos(chip, chipX, y);
    lv_obj_t *desc = lv_label_create(lv_scr_act());
    lv_label_set_text(desc, entry.text);
    lv_obj_set_pos(desc, descX, y + 1);
    lv_obj_set_size(desc, descW, lv_font_get_line_height(&elms_bold_12));
    lv_label_set_long_mode(desc, LV_LABEL_LONG_DOT);
    setUiFont(desc);
    lv_obj_set_style_text_color(desc, lv_color_hex(0xA6ADC8), 0);
  }
}

void buildHelpScreen() {
  editorVisible = false;
  helpVisible = true;
  pairingVisible = settingsVisible = menuVisible = startVisible = false;
  typingVisible = typingSettingsVisible = previewVisible = false;
  wifiScreenVisible = false;
  lv_obj_clean(lv_scr_act());
  pairingStatusLabel = nullptr;
  settingsRadioLabel = nullptr;
  brightnessStatusLabel = nullptr;
  brightnessSlider = smoothSwitch = markdownSwitch = autoSaveSwitch =
      dimSwitch = lineNumberSwitch = wordWrapSwitch = nullptr;
  lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x11111B), 0);
  lv_obj_t *header = lv_obj_create(lv_scr_act());
  lv_obj_set_size(header, SCREEN_W - 8, 30);
  lv_obj_set_pos(header, 4, 4);
  stylePanel(header, lv_color_hex(0x181825), lv_color_hex(0x313244));
  lv_obj_t *heading = lv_label_create(header);
  lv_label_set_text(heading, "VIM COMMANDS — TEXT EDITING");
  lv_obj_set_pos(heading, 10, 4);
  setTitleFont(heading);
  lv_obj_set_style_text_color(heading, lv_color_hex(0x96A57E), 0);
  makeButton(header, "BACK", 408, 58, helpBackClicked);
  // 17 rows per column at a 16 px pitch: content spans y 44..316, 8 px
  // clear of the screen bottom. Column A's descriptions start 4 px further
  // right than before so the 5-char ":font" chip keeps a gap; each column's
  // text ends at x 234/470, well clear of the neighbouring chips.
  renderHelpColumn(HELP_COLUMN_A, sizeof(HELP_COLUMN_A) / sizeof(HelpEntry),
                   10, 52, 182);
  renderHelpColumn(HELP_COLUMN_B, sizeof(HELP_COLUMN_B) / sizeof(HelpEntry),
                   246, 284, 186);
  uiNeedsRender = false;
}

void buildBrowserScreen() {
  editorVisible = false;
  startVisible = false;
  pairingVisible = settingsVisible = menuVisible = false;
  helpVisible = typingVisible = typingSettingsVisible = previewVisible = false;
  wifiScreenVisible = false;
  lv_obj_clean(lv_scr_act());
  for (int i = 0; i < 10; ++i) browserRowButtons[i] = nullptr;
  browserRowFirst = SIZE_MAX;
  pairingStatusLabel = settingsRadioLabel = brightnessStatusLabel = nullptr;
  menuRadioLabel = nullptr;
  brightnessSlider = smoothSwitch = markdownSwitch = autoSaveSwitch =
      dimSwitch = lineNumberSwitch = wordWrapSwitch = nullptr;
  lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x11111B), 0);

  lv_obj_t *header = lv_obj_create(lv_scr_act());
  lv_obj_set_size(header, SCREEN_W - 8, 30);
  lv_obj_set_pos(header, 4, 4);
  stylePanel(header, lv_color_hex(0x181825), lv_color_hex(0x313244));
  lv_obj_t *heading = lv_label_create(header);
  // While filtering, the typed query replaces the path: it is the more useful
  // thing to see, and the header stays inside the BACK button.
  std::string headingText = "DOCUMENTS  /WriterInAWell";
  if (browserFilterMode) {
    headingText = browserFilter.empty()
        ? std::string("DOCUMENTS  (type to filter)")
        : "DOCUMENTS  \"" + browserFilter + "\"";
  }
  lv_label_set_text(heading, headingText.c_str());
  lv_obj_set_width(heading, 330);
  lv_label_set_long_mode(heading, LV_LABEL_LONG_DOT);
  lv_obj_set_pos(heading, 10, 4);
  setTitleFont(heading);
  lv_obj_set_style_text_color(heading, lv_color_hex(0x96A57E), 0);
  makeButton(header, "BACK", 347, 58, backClicked);
  makeButton(header, "NEW", 412, 54, newClicked);

  browserPanel = lv_obj_create(lv_scr_act());
  lv_obj_set_size(browserPanel, SCREEN_W - 8, 240);
  lv_obj_set_pos(browserPanel, 4, 39);
  stylePanel(browserPanel, lv_color_hex(0x11111B), lv_color_hex(0x313244));
  if (!cardAvailable) {
    lv_obj_t *message = lv_label_create(browserPanel);
    lv_label_set_text(message, "No microSD card detected.\nInsert a card to browse and save documents.");
    lv_obj_set_pos(message, 16, 22);
    setUiFont(message);
    lv_obj_set_style_text_color(message, lv_color_hex(0xBAC2DE), 0);
  } else if (browserMatches.empty()) {
    lv_obj_t *message = lv_label_create(browserPanel);
    lv_label_set_text(message, browserFilter.empty()
        ? "No documents yet. Press NEW to start writing."
        : "No files match the filter.");
    lv_obj_set_pos(message, 16, 22);
    setUiFont(message);
    lv_obj_set_style_text_color(message, lv_color_hex(0xBAC2DE), 0);
  } else {
    const size_t first = browserSelection >= 10 ? browserSelection - 9 : 0;
    const size_t last = std::min(browserMatches.size(), first + 10);
    browserRowFirst = first;
    for (size_t i = first; i < last; ++i) {
      lv_obj_t *button = lv_btn_create(browserPanel);
      browserRowButtons[i - first] = button;
      lv_obj_set_size(button, 442, 20);
      lv_obj_set_pos(button, 8, 7 + static_cast<int>(i - first) * 21);
      stylePanel(button, i == browserSelection ? lv_color_hex(0x313244)
                                               : lv_color_hex(0x11111B),
                 lv_color_hex(0x313244));
      lv_obj_add_event_cb(button, fileClicked, LV_EVENT_CLICKED,
                          reinterpret_cast<void *>(static_cast<intptr_t>(i)));
      lv_obj_t *name = lv_label_create(button);
      lv_label_set_text(name, browserFileName(i).c_str());
      lv_obj_set_pos(name, 7, 2);
      setUiFont(name);
      lv_obj_set_style_text_color(name, lv_color_hex(0xBAC2DE), 0);
    }
  }
  lv_obj_t *footRow = makeHintRow();
  if (browserFilterMode) {
    addHintPair(footRow, "type", "filter");
    addHintPair(footRow, "enter", "open");
    addHintPair(footRow, "esc", "cancel");
  } else {
    addHintPair(footRow, "↑/↓", "select");
    addHintPair(footRow, "enter", "open");
    addHintPair(footRow, "f", "filter");
    addHintPair(footRow, "n", "new");
    addHintPair(footRow, "esc", "back");
  }
  uiNeedsRender = false;
}

// Highlight-only refresh for the browser list. Returns false when the visible
// window moved or the rows are not the ones built (filter changed, screen
// rebuilt), so the caller can fall back to a full build. A plain j/k inside
// the window then costs ten small background writes instead of a
// destroy-and-recreate of every row.
bool refreshBrowserSelection() {
  const size_t first = browserSelection >= 10 ? browserSelection - 9 : 0;
  const size_t count = std::min<size_t>(browserMatches.size() - first, 10);
  if (first != browserRowFirst) return false;
  for (size_t i = 0; i < count; ++i) {
    if (!browserRowButtons[i]) return false;
    lv_obj_set_style_bg_color(browserRowButtons[i],
        first + i == browserSelection ? lv_color_hex(0x313244)
                                      : lv_color_hex(0x11111B), 0);
  }
  return true;
}

// --- Settings: keyboard status and file type ---------------------------------

// Value pill for a control in the settings screen's left column. Returns the
// inner label so a keyboard-driven change can refresh it in place.
lv_obj_t *makeValuePill(lv_obj_t *parent, int x, int y, int width,
                        const char *value, lv_event_cb_t callback) {
  lv_obj_t *pill = lv_btn_create(parent);
  lv_obj_set_size(pill, width, 26);
  lv_obj_set_pos(pill, x, y);
  stylePanel(pill, lv_color_hex(0x1E1E2E), lv_color_hex(0x45475A));
  lv_obj_add_event_cb(pill, [](lv_event_t *) {
    lastInputMs = millis();
    if (backlightDimmed) {
      display.setBrightness(brightnessValue);
      backlightDimmed = false;
    }
  }, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb(pill, callback, LV_EVENT_CLICKED, nullptr);
  lv_obj_t *text = lv_label_create(pill);
  lv_label_set_text(text, value);
  lv_obj_center(text);
  setUiFont(text);
  lv_obj_set_style_text_color(text, lv_color_hex(0xCDD6F4), 0);
  return text;
}

// Two-word BLE state for the Settings line. The full keyboardStatus() text is
// a sentence, and it reflowed the row on every scan tick.
const char *keyboardShortStatus() {
  if (keyboardIsConnected()) return "paired";
  const char *status = keyboardStatus();
  if (status && (strstr(status, "search") || strstr(status, "found"))) {
    return "searching";
  }
  return "not paired";
}

// --- Settings sections and popup panels -------------------------------------
// The old screen stacked six toggles into a fixed right column. That fits no
// more controls once writing, files and power settings arrive, so Settings is
// now a short section list plus popup panels — the same pattern as the File
// Type picker. Each popup keeps the underlined-shortcut-letter convention.

enum class SettingsRowKind : uint8_t { Toggle, Action, Value };

struct SettingsRow {
  const char *before;
  char key;
  const char *after;
  SettingsRowKind kind;
};

constexpr size_t SETTINGS_SECTION_COUNT = 7;

struct SettingsSectionDef {
  const char *before;
  char key;
  const char *after;
};

// Section labels split around the underlined shortcut letter ("D" + "isplay").
const SettingsSectionDef SETTINGS_SECTIONS[SETTINGS_SECTION_COUNT] = {
    {"", 'D', "isplay"},
    {"", 'E', "ditor"},
    {"", 'W', "riting"},
    {"Files & ", 'D', "ata"},
    {"", 'P', "ower"},
    {"", 'K', "eyboard"},
    {"", 'S', "ecurity"},
};

const SettingsRow SETTINGS_EDITOR_ROWS[] = {
    // Value rows are index-checked against this table by number, so the text
    // size is entry 0 and every toggle below is one index higher.
    {"Text s", 'i', "ze", SettingsRowKind::Value},
    {"", 'T', "ext Smoothing", SettingsRowKind::Toggle},
    {"Text ", 'C', "olors", SettingsRowKind::Toggle},
    {"", 'W', "ord Wrap", SettingsRowKind::Toggle},
    {"Line ", 'N', "umbers", SettingsRowKind::Toggle},
    {"", 'R', "elative numbers", SettingsRowKind::Toggle},
    {"", 'F', "ocus line", SettingsRowKind::Toggle},
    {"", 'Z', "en mode", SettingsRowKind::Toggle},
    {"", 'S', "earch wrap", SettingsRowKind::Toggle},
};
const SettingsRow SETTINGS_WRITING_ROWS[] = {
    {"", 'L', "ist continuation", SettingsRowKind::Toggle},
    {"Prose ", 'P', "unctuation", SettingsRowKind::Toggle},
};
const SettingsRow SETTINGS_FILES_ROWS[] = {
    {"", 'A', "uto-Save", SettingsRowKind::Toggle},
    {"File ", 'T', "ype", SettingsRowKind::Action},
    {"", 'W', "ord count", SettingsRowKind::Toggle},
    {"", 'S', "napshots", SettingsRowKind::Toggle},
    {"", 'K', "eep versions", SettingsRowKind::Value},
};
const SettingsRow SETTINGS_POWER_ROWS[] = {
    {"", 'D', "im When Idle", SettingsRowKind::Toggle},
    {"", 'B', "oot in editor", SettingsRowKind::Toggle},
    {"", 'O', "ff after", SettingsRowKind::Value},
    {"", 'L', "ight sleep", SettingsRowKind::Toggle},
};
const SettingsRow SETTINGS_KEYBOARD_ROWS[] = {
    {"", 'P', "air Keyboard", SettingsRowKind::Action},
    {"", 'D', "isconnect", SettingsRowKind::Action},
};
// Every row opens the lock screen: Set asks for the current password first
// when one exists, Remove always verifies, and Lock now goes straight there.
const SettingsRow SETTINGS_SECURITY_ROWS[] = {
    {"", 'S', "et / change password", SettingsRowKind::Action},
    {"", 'R', "emove password", SettingsRowKind::Action},
    {"", 'L', "ock now", SettingsRowKind::Action},
};

// Popup geometry: 380x262 at (50, 28). It clears the 21 px hint row at y=299
// and fits ten 21 px language rows under the title.
constexpr int SETTINGS_POPUP_X = 50;
constexpr int SETTINGS_POPUP_Y = 28;
constexpr int SETTINGS_POPUP_W = 380;
constexpr int SETTINGS_POPUP_H = 262;

SettingsSection settingsSectionAt(size_t index) {
  switch (index) {
  case 0: return SettingsSection::Display;
  case 1: return SettingsSection::Editor;
  case 2: return SettingsSection::Writing;
  case 3: return SettingsSection::Files;
  case 4: return SettingsSection::Power;
  case 5: return SettingsSection::Keyboard;
  case 6: return SettingsSection::Security;
  default: return SettingsSection::None;
  }
}

const char *settingsSectionTitle(SettingsSection section) {
  switch (section) {
  case SettingsSection::Display: return "DISPLAY";
  case SettingsSection::Editor: return "EDITOR";
  case SettingsSection::Writing: return "WRITING";
  case SettingsSection::Files: return "FILES & DATA";
  case SettingsSection::Power: return "POWER";
  case SettingsSection::Keyboard: return "KEYBOARD";
  case SettingsSection::Security: return "SECURITY";
  case SettingsSection::FileType: return "FILE TYPE";
  default: return "";
  }
}

const SettingsRow *settingsRowsFor(SettingsSection section, size_t &count) {
  switch (section) {
  case SettingsSection::Editor:
    count = sizeof(SETTINGS_EDITOR_ROWS) / sizeof(SETTINGS_EDITOR_ROWS[0]);
    return SETTINGS_EDITOR_ROWS;
  case SettingsSection::Writing:
    count = sizeof(SETTINGS_WRITING_ROWS) / sizeof(SETTINGS_WRITING_ROWS[0]);
    return SETTINGS_WRITING_ROWS;
  case SettingsSection::Files:
    count = sizeof(SETTINGS_FILES_ROWS) / sizeof(SETTINGS_FILES_ROWS[0]);
    return SETTINGS_FILES_ROWS;
  case SettingsSection::Power:
    count = sizeof(SETTINGS_POWER_ROWS) / sizeof(SETTINGS_POWER_ROWS[0]);
    return SETTINGS_POWER_ROWS;
  case SettingsSection::Keyboard:
    // Disconnect only exists while a keyboard is attached, so the row count
    // follows the live connection state.
    count = keyboardIsConnected() ? 2 : 1;
    return SETTINGS_KEYBOARD_ROWS;
  case SettingsSection::Security:
    count = sizeof(SETTINGS_SECURITY_ROWS) / sizeof(SETTINGS_SECURITY_ROWS[0]);
    return SETTINGS_SECURITY_ROWS;
  default:
    count = 0;
    return nullptr;
  }
}

bool settingsToggleValue(SettingsSection section, size_t index) {
  switch (section) {
  case SettingsSection::Editor:
    switch (index) {
    // Index 0 is the Text size value row; the toggles start at 1.
    case 1: return editorSmoothText;
    case 2: return editorHighlight;
    case 3: return editorWordWrap;
    case 4: return editorLineNumbers;
    case 5: return editorRelativeNumbers;
    case 6: return editorTypewriter;
    case 7: return editorZenMode;
    case 8: return editorSearchWrap;
    default: break;
    }
    break;
  case SettingsSection::Writing:
    if (index == 0) return editorListContinue;
    if (index == 1) return editorProseTypo;
    break;
  case SettingsSection::Files:
    if (index == 0) return editorAutoSave;
    if (index == 2) return editorWordCount;
    if (index == 3) return snapshotsEnabled;
    break;
  case SettingsSection::Power:
    if (index == 0) return settingsDimIdle;
    if (index == 1) return bootIntoEditor;
    if (index == 3) return lightSleepEnabled;
    break;
  default:
    break;
  }
  return false;
}

const char *settingsToggleMessage(SettingsSection section, size_t index,
                                  bool on) {
  if (section == SettingsSection::Editor) {
    switch (index) {
    case 1: return on ? "Text smoothing on" : "Text smoothing off (crisp)";
    case 2: return on ? "Text colors on" : "Text colors off (plain text)";
    case 3: return on ? "Word wrap on" : "Word wrap off";
    case 4: return on ? "Line numbers on" : "Line numbers off";
    case 5: return on ? "Relative numbers on" : "Relative numbers off";
    case 6: return on ? "Focus line on" : "Focus line off";
    case 7: return on ? "Zen mode on" : "Zen mode off";
    case 8: return on ? "Search wrap on" : "Search wrap off";
    default: break;
    }
  } else if (section == SettingsSection::Writing) {
    if (index == 0) {
      return on ? "List continuation on" : "List continuation off";
    }
    if (index == 1) {
      return on ? "Prose punctuation on" : "Prose punctuation off";
    }
  } else if (section == SettingsSection::Files) {
    if (index == 0) return on ? "Auto-save on" : "Auto-save off (:w to save)";
    if (index == 2) return on ? "Word count on" : "Word count off";
    if (index == 3) return on ? "Snapshots on" : "Snapshots off";
  } else if (section == SettingsSection::Power) {
    if (index == 0) return on ? "Idle dimming on" : "Idle dimming off";
    if (index == 1) return on ? "Boot into editor on" : "Boot into editor off";
    if (index == 3) {
      return on ? "Light sleep on (keyboard may reconnect)"
                : "Light sleep off";
    }
  }
  return "Setting updated";
}

void settingsApplyToggle(SettingsSection section, size_t index, bool on) {
  if (section == SettingsSection::Editor) {
    switch (index) {
    case 0: return;  // Text size is a Value row, handled by settingsCycleValue.
    case 1: editorSmoothText = on; break;
    case 2: editorHighlight = on; break;
    case 3:
      editorWordWrap = on;
      // Both viewport anchors go stale when the mode flips; the next render
      // re-clamps them onto the caret.
      viewportWrapCol = 0;
      viewportScrollCol = 0;
      break;
    case 4: editorLineNumbers = on; break;
    case 5: editorRelativeNumbers = on; break;
    case 6: editorTypewriter = on; break;
    case 7: editorZenMode = on; break;
    case 8: editorSearchWrap = on; break;
    default: return;
    }
  } else if (section == SettingsSection::Writing) {
    if (index == 0) editorListContinue = on;
    else if (index == 1) editorProseTypo = on;
    else return;
  } else if (section == SettingsSection::Files) {
    if (index == 0) editorAutoSave = on;
    else if (index == 2) {
      editorWordCount = on;
      documentWordCacheValid = false;
    } else if (index == 3) {
      snapshotsEnabled = on;
    } else {
      return;
    }
  } else if (section == SettingsSection::Power) {
    if (index == 0) settingsDimIdle = on;
    else if (index == 1) bootIntoEditor = on;
    else if (index == 3) lightSleepEnabled = on;
    else return;
  } else {
    return;
  }
  prefsDirty = true;
  statusMessage = settingsToggleMessage(section, index, on);
  savePrefs();
}

const char *settingsActionValue(SettingsSection section, size_t index) {
  if (section == SettingsSection::Files && index == 1) {
    return langLabel(editorFileType);
  }
  if (section == SettingsSection::Keyboard) {
    if (index == 0) return keyboardShortStatus();
    if (index == 1) return "connected";
  }
  return "";
}

// Value rows cycle on tap/Enter/←→. Only one is on screen at a time, so a
// single static buffer is safe on this single-threaded UI.
const char *settingsValueLabel(SettingsSection section, size_t index) {
  static char buf[10];
  if (section == SettingsSection::Editor && index == 0) {
    // The one value row here: the current editor text size, in pixels.
    snprintf(buf, sizeof(buf), "%u px",
             unsigned(EDITOR_FONT_PIXELS[editorFontSizeIndex]));
    return buf;
  }
  if (section == SettingsSection::Files && index == 4) {
    snprintf(buf, sizeof(buf), "%u", unsigned(snapshotRetention));
    return buf;
  }
  if (section == SettingsSection::Power && index == 2) {
    if (screenOffMinutes == 0) return "off";
    snprintf(buf, sizeof(buf), "%u min", unsigned(screenOffMinutes));
    return buf;
  }
  return "";
}

void settingsCycleValue(SettingsSection section, size_t index) {
  if (section == SettingsSection::Editor && index == 0) {
    // Tap/Enter/the 'i' shortcut step to the next text size and wrap, the same
    // way Keep versions and Off after cycle. setEditorFontSize() saves the
    // pref; the rebuild below refreshes this row and the EDITOR summary.
    const int next = int(editorFontSizeIndex) + 1 >= EDITOR_FONT_SIZE_COUNT
                         ? 0
                         : int(editorFontSizeIndex) + 1;
    setEditorFontSize(next);
    buildSettingsScreen();
    return;
  }
  if (section == SettingsSection::Files && index == 4) {
    snapshotRetention = snapshotRetention == 3 ? 5
                        : snapshotRetention == 5 ? 10
                                                 : 3;
    statusMessage = std::string("Keep ") + std::to_string(snapshotRetention) +
                    " versions";
  } else if (section == SettingsSection::Power && index == 2) {
    screenOffMinutes = screenOffMinutes == 0 ? 2
                       : screenOffMinutes == 2 ? 5
                       : screenOffMinutes == 5 ? 10
                       : screenOffMinutes == 10 ? 15
                                                : 0;
    statusMessage = screenOffMinutes == 0
                        ? "Screen-off timeout off"
                        : "Screen off after " +
                              std::to_string(screenOffMinutes) + " min";
  } else {
    return;
  }
  prefsDirty = true;
  savePrefs();
  // Refresh the value text (and any sibling row state) immediately.
  buildSettingsScreen();
}

void openSettingsPopup(SettingsSection section) {
  // A popup opened from another popup remembers its parent so Esc walks back
  // one level (File Type -> Files & Data -> section list).
  settingsReturn = settingsPopup == SettingsSection::None
                       ? SettingsSection::None
                       : settingsPopup;
  settingsPopup = section;
  settingsPopupSelection = 0;
  if (section == SettingsSection::FileType) {
    settingsPopupSelection = static_cast<uint8_t>(editorFileType);
  }
  buildSettingsScreen();
}

void closeSettingsPopup() {
  if (settingsReturn != SettingsSection::None) {
    settingsPopup = settingsReturn;
    settingsReturn = SettingsSection::None;
    settingsPopupSelection = 0;
  } else {
    settingsPopup = SettingsSection::None;
  }
  buildSettingsScreen();
}

void closeSettingsPopupClicked(lv_event_t *) { closeSettingsPopup(); }

// Header BACK: close the open popup first, then leave Settings. Touch and
// keyboard then agree — Esc/CLOSE pops one level and BACK eventually exits.
void settingsBackClicked(lv_event_t *) {
  if (settingsPopup != SettingsSection::None) closeSettingsPopup();
  else backClicked(nullptr);
}

void settingsSectionClicked(lv_event_t *event) {
  // Stray taps beside a popup must not switch sections under it.
  if (settingsPopup != SettingsSection::None) return;
  const intptr_t index =
      reinterpret_cast<intptr_t>(lv_event_get_user_data(event));
  if (index < 0 || static_cast<size_t>(index) >= SETTINGS_SECTION_COUNT) return;
  // Move the highlight to the tapped row so closing the popup returns to the
  // section the user actually opened.
  settingsSectionSelection = static_cast<uint8_t>(index);
  openSettingsPopup(settingsSectionAt(static_cast<size_t>(index)));
}

void settingsRunAction(SettingsSection section, size_t index) {
  if (section == SettingsSection::Files && index == 1) {
    openSettingsPopup(SettingsSection::FileType);
  } else if (section == SettingsSection::Keyboard) {
    if (index == 0) {
      showPairingHelp();
    } else if (index == 1) {
      keyboardDisconnect();
      buildSettingsScreen();
    }
  } else if (section == SettingsSection::Security) {
    if (index == 2) {
      // Lock now leaves Settings for good: unlocking returns to the menu.
      if (!lockEnabled) {
        statusMessage = "Set a password first";
        buildSettingsScreen();
      } else {
        lockReturnToSettings = false;
        lockReturnTarget = LockReturn::Menu;
        lockDevice();
      }
      return;
    }
    if (index == 1 && !lockEnabled) {
      statusMessage = "No password set";
      buildSettingsScreen();
      return;
    }
    // Set/change and Remove both start on the lock screen's keyboard. Reset
    // the transient keypad state so a previous SYM/SHIFT choice never leaks
    // into a password being typed.
    lockReturnToSettings = true;
    lockVerifyForRemove = index == 1;
    lockEntry[0] = '\0';
    lockPending[0] = '\0';
    lockShift = lockSymbols = false;
    lockMode = lockEnabled ? LockMode::VerifyOld : LockMode::EnterNew;
    buildLockScreen();
  }
}

void settingsDecodeRow(void *data, SettingsSection &section, size_t &index) {
  const intptr_t code = reinterpret_cast<intptr_t>(data);
  section = static_cast<SettingsSection>((code >> 8) & 0xFF);
  index = static_cast<size_t>(code & 0xFF);
}

void settingsRowTapped(lv_event_t *event) {
  SettingsSection section;
  size_t index;
  settingsDecodeRow(lv_event_get_user_data(event), section, index);
  if (section != settingsPopup) return;
  size_t count = 0;
  const SettingsRow *rows = settingsRowsFor(section, count);
  if (!rows || index >= count) return;
  if (rows[index].kind == SettingsRowKind::Toggle) {
    const bool on = !settingsToggleValue(section, index);
    settingsApplyToggle(section, index, on);
    if (index < settingsPopupSwitchCount && settingsPopupSwitches[index]) {
      syncSwitchState(settingsPopupSwitches[index], on);
    }
  } else if (rows[index].kind == SettingsRowKind::Value) {
    settingsCycleValue(section, index);
  } else {
    settingsRunAction(section, index);
  }
}

void fileTypeRowClicked(lv_event_t *event) {
  const intptr_t index =
      reinterpret_cast<intptr_t>(lv_event_get_user_data(event));
  if (index < 0 || static_cast<size_t>(index) >= LANG_COUNT) return;
  applyDocLang(static_cast<size_t>(index));
  closeSettingsPopup();
}

size_t settingsPopupRowCount() {
  if (settingsPopup == SettingsSection::FileType) return LANG_COUNT;
  // The Display popup's single row is the brightness slider.
  if (settingsPopup == SettingsSection::Display) return 1;
  size_t count = 0;
  settingsRowsFor(settingsPopup, count);
  return count;
}

void settingsPopupActivateRow(size_t index) {
  if (settingsPopup == SettingsSection::FileType) {
    if (index < LANG_COUNT) {
      applyDocLang(index);
      closeSettingsPopup();
    }
    return;
  }
  if (settingsPopup == SettingsSection::Display) return;  // slider only
  size_t count = 0;
  const SettingsRow *rows = settingsRowsFor(settingsPopup, count);
  if (!rows || index >= count) return;
  if (rows[index].kind == SettingsRowKind::Toggle) {
    const bool on = !settingsToggleValue(settingsPopup, index);
    settingsApplyToggle(settingsPopup, index, on);
    if (index < settingsPopupSwitchCount && settingsPopupSwitches[index]) {
      syncSwitchState(settingsPopupSwitches[index], on);
    }
  } else if (rows[index].kind == SettingsRowKind::Value) {
    settingsCycleValue(settingsPopup, index);
  } else {
    settingsRunAction(settingsPopup, index);
  }
}

void settingsPopupAdjustRow(int delta) {
  if (settingsPopup == SettingsSection::Display) {
    int32_t next = int32_t(brightnessValue) + delta * 16;
    if (next < BRIGHTNESS_MIN) next = BRIGHTNESS_MIN;
    if (next > BRIGHTNESS_MAX) next = BRIGHTNESS_MAX;
    setBrightnessValue(static_cast<uint8_t>(next));
    return;
  }
  // ←/→ cycles a Value row too, matching the tap/Enter behavior. The text
  // size is the exception: it steps in the pressed direction (and clamps at
  // the ends like the header's A−/A+ pair) instead of always cycling up.
  if (settingsPopup == SettingsSection::Editor && settingsPopupSelection == 0) {
    stepEditorFontSize(delta);
    buildSettingsScreen();
    return;
  }
  size_t count = 0;
  const SettingsRow *rows = settingsRowsFor(settingsPopup, count);
  if (rows && settingsPopupSelection < count &&
      rows[settingsPopupSelection].kind == SettingsRowKind::Value) {
    settingsCycleValue(settingsPopup, settingsPopupSelection);
  }
}

bool settingsPopupLetter(char c) {
  if (settingsPopup == SettingsSection::FileType ||
      settingsPopup == SettingsSection::Display) {
    return false;
  }
  size_t count = 0;
  const SettingsRow *rows = settingsRowsFor(settingsPopup, count);
  if (!rows) return false;
  const char upper =
      (c >= 'a' && c <= 'z') ? static_cast<char>(c - 'a' + 'A') : c;
  for (size_t i = 0; i < count; ++i) {
    if (rows[i].key == upper) {
      settingsPopupSelection = static_cast<uint8_t>(i);
      settingsPopupActivateRow(i);
      return true;
    }
  }
  return false;
}

void settingsMovePopupSelection(int step) {
  const size_t count = settingsPopupRowCount();
  if (!count) return;
  int next = static_cast<int>(settingsPopupSelection) + step;
  if (next < 0) next += static_cast<int>(count);
  if (next >= static_cast<int>(count)) next -= static_cast<int>(count);
  settingsPopupSelection = static_cast<uint8_t>(next);
  // Only the popup's own highlight changed; keep the section list as it is.
  rebuildSettingsPopup();
}

void settingsMoveSection(int step) {
  int next = static_cast<int>(settingsSectionSelection) + step;
  if (next < 0) next += static_cast<int>(SETTINGS_SECTION_COUNT);
  if (next >= static_cast<int>(SETTINGS_SECTION_COUNT)) {
    next -= static_cast<int>(SETTINGS_SECTION_COUNT);
  }
  settingsSectionSelection = static_cast<uint8_t>(next);
  // Only the highlight moved: retint the rows instead of rebuilding the screen
  // (header, six rows with their labels, hint row) on every arrow key.
  refreshSettingsSectionSelection();
}

void settingsSectionSummary(SettingsSection section, char *out, size_t len) {
  switch (section) {
  case SettingsSection::Display:
    snprintf(out, len, "%u%%", unsigned(brightnessValue) * 100 / 255);
    break;
  case SettingsSection::Editor:
    snprintf(out, len, "%u px",
             unsigned(EDITOR_FONT_PIXELS[editorFontSizeIndex]));
    break;
  case SettingsSection::Writing:
    snprintf(out, len, "%s",
             editorListContinue ? "list on" : "list off");
    break;
  case SettingsSection::Files:
    snprintf(out, len, "%s", langLabel(editorFileType));
    break;
  case SettingsSection::Power:
    snprintf(out, len, "%s", settingsDimIdle ? "dim on" : "dim off");
    break;
  case SettingsSection::Keyboard:
    snprintf(out, len, "%s", keyboardShortStatus());
    break;
  case SettingsSection::Security:
    snprintf(out, len, "%s", lockEnabled ? "on" : "off");
    break;
  default:
    out[0] = '\0';
    break;
  }
}

// A row in the top-level section list: label on the left, live summary on the
// right, and the selection drawn with the same green border as the picker.
// Parts of each row that the highlight touches. settingsMoveSection() retints
// these instead of rebuilding the screen for one selection step.
struct SettingsSectionPaint {
  lv_obj_t *row;
  lv_obj_t *lead;
  lv_obj_t *key;
  lv_obj_t *tail;
  lv_obj_t *value;  // right-aligned live summary
};
SettingsSectionPaint settingsSectionPaint[SETTINGS_SECTION_COUNT] = {};

void refreshSettingsSectionSelection() {
  for (size_t i = 0; i < SETTINGS_SECTION_COUNT; ++i) {
    SettingsSectionPaint &paint = settingsSectionPaint[i];
    if (!paint.row) continue;
    const bool selected = i == settingsSectionSelection;
    lv_obj_set_style_bg_color(paint.row,
        lv_color_hex(selected ? 0x313244 : 0x1E1E2E), 0);
    lv_obj_set_style_border_color(paint.row,
        lv_color_hex(selected ? 0x96A57E : 0x45475A), 0);
    const lv_color_t ink = lv_color_hex(selected ? 0xB5C39D : 0xCDD6F4);
    if (paint.lead) lv_obj_set_style_text_color(paint.lead, ink, 0);
    if (paint.key) lv_obj_set_style_text_color(paint.key, ink, 0);
    if (paint.tail) lv_obj_set_style_text_color(paint.tail, ink, 0);
  }
}

lv_obj_t *makeSettingsSectionRow(lv_obj_t *parent, size_t index) {
  const SettingsSection section = settingsSectionAt(index);
  const bool selected = index == settingsSectionSelection;
  lv_obj_t *row = lv_btn_create(parent);
  // Seven rows at a 31 px pitch inside the 240 px panel: 12..226, 14 px clear
  // at the bottom. The Security section added the seventh row, so both the
  // pitch and the height shrink by 5 px to keep the panel scroll-free.
  lv_obj_set_size(row, 444, 28);
  lv_obj_set_pos(row, 14, 12 + static_cast<int>(index) * 31);
  stylePanel(row, lv_color_hex(selected ? 0x313244 : 0x1E1E2E),
             lv_color_hex(selected ? 0x96A57E : 0x45475A));
  lv_obj_add_event_cb(row, [](lv_event_t *) {
    lastInputMs = millis();
    if (backlightDimmed) {
      display.setBrightness(brightnessValue);
      backlightDimmed = false;
    }
  }, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb(row, settingsSectionClicked, LV_EVENT_CLICKED,
                      reinterpret_cast<void *>(static_cast<intptr_t>(index)));

  const SettingsSectionDef &def = SETTINGS_SECTIONS[index];
  SettingsSectionPaint &paint = settingsSectionPaint[index];
  paint.row = row;
  makeTrackedShortcutLabel(row, def.before, def.key, def.after, 8, 7,
                           lv_color_hex(selected ? 0xB5C39D : 0xCDD6F4),
                           paint.lead, paint.key, paint.tail);

  char summary[24];
  settingsSectionSummary(section, summary, sizeof(summary));
  lv_obj_t *value = lv_label_create(row);
  lv_label_set_text(value, summary);
  // Right-aligned against the row's 8 px inner margin.
  lv_obj_set_width(value, 150);
  lv_obj_set_pos(value, 444 - 8 - 150, 7);
  lv_obj_set_style_text_align(value, LV_TEXT_ALIGN_RIGHT, 0);
  setUiFont(value);
  lv_obj_set_style_text_color(value, lv_color_hex(0x7F849C), 0);
  paint.value = value;
  return row;
}

// One popup row container. The switch or value sits inside it; tapping the
// row itself drives the action, so the switch can stay non-clickable and
// keyboard and touch share one code path.
lv_obj_t *makeSettingsPopupRow(lv_obj_t *panel, int y, bool selected,
                               int height) {
  lv_obj_t *row = lv_btn_create(panel);
  lv_obj_set_size(row, SETTINGS_POPUP_W - 16, height);
  lv_obj_set_pos(row, 8, y);
  stylePanel(row, lv_color_hex(selected ? 0x313244 : 0x1E1E2E),
             lv_color_hex(selected ? 0x96A57E : 0x45475A));
  lv_obj_add_event_cb(row, [](lv_event_t *) {
    lastInputMs = millis();
    if (backlightDimmed) {
      display.setBrightness(brightnessValue);
      backlightDimmed = false;
    }
  }, LV_EVENT_CLICKED, nullptr);
  return row;
}

// Switch for a popup row, styled exactly like makeToggleRow's. It is not
// clickable: the containing row handles the tap, so the switch never flips
// itself behind the setting it represents.
lv_obj_t *makeSettingsPopupSwitch(lv_obj_t *row, bool on, int y) {
  lv_obj_t *sw = lv_switch_create(row);
  // 40x18 leaves 2 px of the row's content area visible above and below even
  // in the dense 24 px Editor rows, so the row border is never covered.
  lv_obj_set_size(sw, 40, 18);
  lv_obj_set_pos(sw, SETTINGS_POPUP_W - 16 - 6 - 40, y);
  lv_obj_set_style_bg_color(sw, lv_color_hex(0x1E1E2E), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(sw, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_color(sw, lv_color_hex(0x45475A), LV_PART_MAIN);
  lv_obj_set_style_border_width(sw, 1, LV_PART_MAIN);
  lv_obj_set_style_radius(sw, LV_RADIUS_CIRCLE, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(sw, LV_OPA_TRANSP, LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(sw, lv_color_hex(0x96A57E),
                            (uint32_t)LV_STATE_CHECKED | LV_PART_INDICATOR);
  lv_obj_set_style_bg_opa(sw, LV_OPA_COVER,
                          (uint32_t)LV_STATE_CHECKED | LV_PART_INDICATOR);
  lv_obj_set_style_radius(sw, LV_RADIUS_CIRCLE, LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(sw, lv_color_hex(0xCDD6F4), LV_PART_KNOB);
  lv_obj_set_style_bg_opa(sw, LV_OPA_COVER, LV_PART_KNOB);
  lv_obj_set_style_border_color(sw, lv_color_hex(0x313244), LV_PART_KNOB);
  lv_obj_set_style_border_width(sw, 1, LV_PART_KNOB);
  lv_obj_set_style_radius(sw, LV_RADIUS_CIRCLE, LV_PART_KNOB);
  lv_obj_set_style_pad_all(sw, -2, LV_PART_KNOB);
  // The containing row handles the tap; the switch is display-only.
  // Transitions are off globally (LV_THEME_DEFAULT_TRANSITION_TIME 0), so
  // rebuilding the popup on each arrow key cannot replay a green fade.
  lv_obj_clear_flag(sw, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_shadow_width(sw, 0, LV_PART_MAIN);
  if (on) lv_obj_add_state(sw, LV_STATE_CHECKED);
  return sw;
}

// Display popup body: the same slider geometry and palette as the old flat
// screen, narrowed to the popup panel. '1'..'4' still jump to the presets.
void buildDisplayPopupBody(lv_obj_t *panel) {
  lv_obj_t *label = lv_label_create(panel);
  lv_label_set_text(label, "BACKLIGHT");
  lv_obj_set_pos(label, 12, 38);
  setUiFont(label);
  lv_obj_set_style_text_color(label, lv_color_hex(0x89B4FA), 0);

  brightnessSlider = lv_slider_create(panel);
  // 260 wide so the knob's max-value overhang stays clear of the readout.
  lv_obj_set_size(brightnessSlider, 260, 16);
  lv_obj_set_pos(brightnessSlider, 14, 58);
  lv_slider_set_mode(brightnessSlider, LV_SLIDER_MODE_NORMAL);
  lv_slider_set_range(brightnessSlider, BRIGHTNESS_MIN, BRIGHTNESS_MAX);
  lv_slider_set_value(brightnessSlider, brightnessValue, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(brightnessSlider, lv_color_hex(0x1E1E2E),
                            LV_PART_MAIN);
  lv_obj_set_style_border_color(brightnessSlider, lv_color_hex(0x45475A),
                                LV_PART_MAIN);
  lv_obj_set_style_border_width(brightnessSlider, 1, LV_PART_MAIN);
  lv_obj_set_style_radius(brightnessSlider, 3, LV_PART_MAIN);
  lv_obj_set_style_bg_color(brightnessSlider, lv_color_hex(0x96A57E),
                            LV_PART_INDICATOR);
  lv_obj_set_style_radius(brightnessSlider, 3, LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(brightnessSlider, lv_color_hex(0xCDD6F4),
                            LV_PART_KNOB);
  lv_obj_set_style_border_color(brightnessSlider, lv_color_hex(0x313244),
                                LV_PART_KNOB);
  lv_obj_set_style_border_width(brightnessSlider, 1, LV_PART_KNOB);
  lv_obj_set_style_pad_all(brightnessSlider, 3, LV_PART_KNOB);
  lv_obj_add_event_cb(brightnessSlider, brightnessSliderChanged,
                      LV_EVENT_VALUE_CHANGED, nullptr);
  lv_obj_add_event_cb(brightnessSlider, brightnessSliderReleased,
                      LV_EVENT_RELEASED, nullptr);

  brightnessStatusLabel = lv_label_create(panel);
  char pct[8];
  snprintf(pct, sizeof(pct), "%u%%", unsigned(brightnessValue) * 100 / 255);
  lv_label_set_text(brightnessStatusLabel, pct);
  lv_obj_set_pos(brightnessStatusLabel, 286, 61);
  setUiFont(brightnessStatusLabel);
  lv_obj_set_style_text_color(brightnessStatusLabel, lv_color_hex(0xCDD6F4), 0);

  lv_obj_t *hint = lv_label_create(panel);
  lv_label_set_text(hint, "1-4 presets    ←/→ fine");
  lv_obj_set_pos(hint, 12, 92);
  setUiFont(hint);
  lv_obj_set_style_text_color(hint, lv_color_hex(0x7F849C), 0);
}

// One popup panel over the section list. File Type keeps its language rows;
// the others are drawn from their SettingsRow table so adding a toggle is a
// one-line change.
void buildSettingsPopup() {
  const SettingsSection section = settingsPopup;
  if (section == SettingsSection::None) return;

  // Size the panel to its content: a one-row Power popup should not look like
  // a mostly empty sheet. File Type keeps the full height for ten rows.
  int panelH = SETTINGS_POPUP_H;
  if (section == SettingsSection::FileType) {
    panelH = SETTINGS_POPUP_H;
  } else if (section == SettingsSection::Display) {
    panelH = 150;
  } else {
    size_t rowCount = 0;
    settingsRowsFor(section, rowCount);
    // Eight Editor rows at the normal 32 px pitch would overflow the screen,
    // so dense popups switch to a 26 px pitch and 24 px rows. The Editor now
    // has nine rows (text size plus eight toggles), which needs the tighter
    // 24 px pitch to stay inside SETTINGS_POPUP_H.
    const int pitch = rowCount > 8 ? 24 : rowCount > 6 ? 26 : 32;
    panelH = 44 + static_cast<int>(rowCount) * pitch + 10;
    if (section == SettingsSection::Keyboard) panelH += 22;
    if (panelH < 110) panelH = 110;
    if (panelH > SETTINGS_POPUP_H) panelH = SETTINGS_POPUP_H;
  }

  lv_obj_t *panel = lv_obj_create(lv_scr_act());
  settingsPopupPanel = panel;
  lv_obj_set_size(panel, SETTINGS_POPUP_W, panelH);
  lv_obj_set_pos(panel, SETTINGS_POPUP_X, SETTINGS_POPUP_Y);
  stylePanel(panel, lv_color_hex(0x181825), lv_color_hex(0x45475A));

  lv_obj_t *title = lv_label_create(panel);
  lv_label_set_text(title, settingsSectionTitle(section));
  lv_obj_set_pos(title, 10, 5);
  setTitleFont(title);
  lv_obj_set_style_text_color(title, lv_color_hex(0x96A57E), 0);
  makeButton(panel, "CLOSE", SETTINGS_POPUP_W - 68, 60,
             closeSettingsPopupClicked);

  if (section == SettingsSection::FileType) {
    for (size_t i = 0; i < LANG_COUNT; ++i) {
      lv_obj_t *row = lv_btn_create(panel);
      lv_obj_set_size(row, SETTINGS_POPUP_W - 16, 21);
      lv_obj_set_pos(row, 8, 32 + static_cast<int>(i) * 22);
      const bool selected = i == settingsPopupSelection;
      stylePanel(row, lv_color_hex(selected ? 0x313244 : 0x1E1E2E),
                 lv_color_hex(selected ? 0x96A57E : 0x45475A));
      lv_obj_add_event_cb(row, [](lv_event_t *) {
        lastInputMs = millis();
        if (backlightDimmed) {
          display.setBrightness(brightnessValue);
          backlightDimmed = false;
        }
      }, LV_EVENT_CLICKED, nullptr);
      lv_obj_add_event_cb(row, fileTypeRowClicked, LV_EVENT_CLICKED,
                          reinterpret_cast<void *>(static_cast<intptr_t>(i)));

      lv_obj_t *name = lv_label_create(row);
      lv_label_set_text(name, LANG_ENTRIES[i].label);
      lv_obj_set_pos(name, 9, 3);
      setUiFont(name);
      lv_obj_set_style_text_color(name,
          lv_color_hex(selected ? 0xB5C39D : 0xCDD6F4), 0);

      lv_obj_t *hint = lv_label_create(row);
      lv_label_set_text(hint, LANG_ENTRIES[i].hint);
      lv_obj_set_width(hint, 150);
      lv_obj_set_pos(hint, SETTINGS_POPUP_W - 16 - 9 - 150, 3);
      lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_RIGHT, 0);
      setUiFont(hint);
      lv_obj_set_style_text_color(hint, lv_color_hex(0x7F849C), 0);
    }
    return;
  }

  if (section == SettingsSection::Display) {
    buildDisplayPopupBody(panel);
    return;
  }

  size_t count = 0;
  const SettingsRow *rows = settingsRowsFor(section, count);
  if (!rows) return;
  if (count && settingsPopupSelection >= count) {
    settingsPopupSelection = static_cast<uint8_t>(count - 1);
  }
  settingsPopupSwitchCount = 0;
  for (size_t i = 0; i < SETTINGS_POPUP_SWITCH_MAX; ++i) {
    settingsPopupSwitches[i] = nullptr;
  }

  int rowTop = 36;
  if (section == SettingsSection::Keyboard) {
    // Keep the live pairing status above the two action rows; loop() repaints
    // this label at 1 Hz like the old flat screen did.
    settingsRadioLabel = lv_label_create(panel);
    lv_label_set_text(settingsRadioLabel, keyboardShortStatus());
    lv_obj_set_pos(settingsRadioLabel, 12, 34);
    setUiFont(settingsRadioLabel);
    lv_obj_set_style_text_color(settingsRadioLabel,
        keyboardIsConnected() ? lv_color_hex(0x96A57E) : lv_color_hex(0xBAC2DE),
        0);
    rowTop = 58;
  }

  // Nine rows need 24 px rows / pitch: rowTop 36 + 8*24 lands the last row at
  // 228, and 22 px of row keeps it (and its value pill) inside the 262 px
  // panel. Anything fewer keeps the roomier 26/24 or 32/28 pairs.
  const int pitch = count > 8 ? 24 : count > 6 ? 26 : 32;
  const int rowH = count > 8 ? 22 : count > 6 ? 24 : 28;
  const int labelY = rowH <= 24 ? 3 : 5;
  // Center the 18 px switch in the row's content box (row height minus its
  // two 1 px borders): 2 px top margin in the dense rows, 4 px otherwise.
  const int switchY = rowH <= 24 ? 2 : 4;
  for (size_t i = 0; i < count; ++i) {
    const bool selected = i == settingsPopupSelection;
    lv_obj_t *row = makeSettingsPopupRow(
        panel, rowTop + static_cast<int>(i) * pitch, selected, rowH);
    makeShortcutLabel(row, rows[i].before, rows[i].key, rows[i].after, 6,
                      labelY,
                      lv_color_hex(selected ? 0xB5C39D : 0xBAC2DE));

    const intptr_t code = (static_cast<intptr_t>(section) << 8) |
                          static_cast<intptr_t>(i);
    if (rows[i].kind == SettingsRowKind::Toggle) {
      lv_obj_t *sw = makeSettingsPopupSwitch(
          row, settingsToggleValue(section, i), switchY);
      if (i < SETTINGS_POPUP_SWITCH_MAX) settingsPopupSwitches[i] = sw;
      settingsPopupSwitchCount = i + 1;
      lv_obj_add_event_cb(row, settingsRowTapped, LV_EVENT_CLICKED,
                          reinterpret_cast<void *>(code));
    } else {
      lv_obj_t *value = lv_label_create(row);
      lv_label_set_text(value, rows[i].kind == SettingsRowKind::Value
                                   ? settingsValueLabel(section, i)
                                   : settingsActionValue(section, i));
      // Right-aligned against the row's 6 px inner margin.
      lv_obj_set_width(value, 150);
      lv_obj_set_pos(value, (SETTINGS_POPUP_W - 16) - 6 - 150, labelY);
      lv_obj_set_style_text_align(value, LV_TEXT_ALIGN_RIGHT, 0);
      setUiFont(value);
      lv_obj_set_style_text_color(value, lv_color_hex(0x7F849C), 0);
      lv_obj_add_event_cb(row, settingsRowTapped, LV_EVENT_CLICKED,
                          reinterpret_cast<void *>(code));
    }
  }
}

// Rebuild only the popup layer. A row move inside a popup used to tear down the
// whole settings screen underneath it (header, six section rows, hint row).
// The panel is the screen's last child, so deleting it reveals the untouched
// list and only the popup area needs repainting.
void rebuildSettingsPopup() {
  if (settingsPopupPanel) {
    lv_obj_del(settingsPopupPanel);
    settingsPopupPanel = nullptr;
  }
  // These point inside the panel that just went away.
  brightnessSlider = nullptr;
  brightnessStatusLabel = nullptr;
  settingsRadioLabel = nullptr;
  settingsPopupSwitchCount = 0;
  for (size_t i = 0; i < SETTINGS_POPUP_SWITCH_MAX; ++i) {
    settingsPopupSwitches[i] = nullptr;
  }
  buildSettingsPopup();
}

void buildSettingsScreen() {
  editorVisible = false;
  settingsVisible = true;
  pairingVisible = menuVisible = startVisible = false;
  helpVisible = typingVisible = typingSettingsVisible = previewVisible = false;
  wifiScreenVisible = false;
  lv_obj_clean(lv_scr_act());
  pairingStatusLabel = nullptr;
  settingsRadioLabel = nullptr;
  brightnessStatusLabel = nullptr;
  brightnessSlider = smoothSwitch = markdownSwitch = autoSaveSwitch =
      dimSwitch = lineNumberSwitch = wordWrapSwitch = nullptr;
  settingsPopupSwitchCount = 0;
  settingsPopupPanel = nullptr;
  for (size_t i = 0; i < SETTINGS_POPUP_SWITCH_MAX; ++i) {
    settingsPopupSwitches[i] = nullptr;
  }
  lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x11111B), 0);

  lv_obj_t *header = lv_obj_create(lv_scr_act());
  lv_obj_set_size(header, SCREEN_W - 8, 30);
  lv_obj_set_pos(header, 4, 4);
  stylePanel(header, lv_color_hex(0x181825), lv_color_hex(0x313244));
  lv_obj_t *heading = lv_label_create(header);
  lv_label_set_text(heading, "SETTINGS");
  lv_obj_set_pos(heading, 10, 4);
  setTitleFont(heading);
  lv_obj_set_style_text_color(heading, lv_color_hex(0x96A57E), 0);
  makeButton(header, "BACK", 412, 54, settingsBackClicked);

  lv_obj_t *panel = lv_obj_create(lv_scr_act());
  lv_obj_set_size(panel, SCREEN_W - 8, 240);
  lv_obj_set_pos(panel, 4, 39);
  stylePanel(panel, lv_color_hex(0x11111B), lv_color_hex(0x313244));

  // Section list: seven 28 px rows at a 31 px pitch. Each row's summary (the
  // live brightness, text size, language, pairing state or lock state) keeps
  // the common settings readable without opening a panel.
  for (size_t i = 0; i < SETTINGS_SECTION_COUNT; ++i) {
    makeSettingsSectionRow(panel, i);
  }

  lv_obj_t *footRow = makeHintRow();
  if (settingsPopup == SettingsSection::None) {
    addHintPair(footRow, "↑↓", "section");
    addHintPair(footRow, "enter", "open");
    addHintPair(footRow, "1-7", "jump");
    addHintPair(footRow, "←/→", "brightness");
    addHintPair(footRow, "esc", "back");
  } else if (settingsPopup == SettingsSection::Display) {
    addHintPair(footRow, "←/→", "brightness");
    makeHintKeycap(footRow, "1-4");
    addHintPair(footRow, "esc", "close");
  } else if (settingsPopup == SettingsSection::FileType) {
    addHintPair(footRow, "↑↓", "choose");
    makeHintKeycap(footRow, "1-9");
    addHintPair(footRow, "enter", "apply");
    addHintPair(footRow, "esc", "close");
  } else {
    addHintPair(footRow, "↑↓", "row");
    // Value rows (text size, keep versions, off after) also take ←/→, so say
    // so instead of leaving "enter change" to imply the only way in.
    size_t rowCount = 0;
    const SettingsRow *rows = settingsRowsFor(settingsPopup, rowCount);
    if (rows && settingsPopupSelection < rowCount &&
        rows[settingsPopupSelection].kind == SettingsRowKind::Value) {
      addHintPair(footRow, "←/→", "adjust");
    }
    addHintPair(footRow, "enter", "change");
    addHintPair(footRow, "esc", "close");
  }

  // Drawn last so it sits above the section list and the hint row.
  if (settingsPopup != SettingsSection::None) buildSettingsPopup();
}

// Wi-Fi sharing screen: a live toggle, editable hotspot name and password, and
// — once the AP is up — the address to open in a browser in bold soft green.
// The fields lock while sharing so the SSID/passphrase cannot change under a
// live AP; the switch stays live to stop it. Typing updates only the active
// field's label (refreshWifiFieldText), not this whole screen.
void buildWifiScreen() {
  editorVisible = false;
  wifiScreenVisible = true;
  settingsVisible = pairingVisible = menuVisible = startVisible = false;
  helpVisible = typingVisible = typingSettingsVisible = previewVisible = false;
  lv_obj_clean(lv_scr_act());
  pairingStatusLabel = settingsRadioLabel = brightnessStatusLabel = nullptr;
  menuRadioLabel = nullptr;
  brightnessSlider = smoothSwitch = markdownSwitch = autoSaveSwitch =
      dimSwitch = lineNumberSwitch = wordWrapSwitch = nullptr;
  wifiNameValueLabel = wifiPasswordValueLabel = nullptr;
  lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x11111B), 0);

  const bool editingName = wifiEditingField == WifiField::Name;
  const bool editingPassword = wifiEditingField == WifiField::Password;

  lv_obj_t *header = lv_obj_create(lv_scr_act());
  lv_obj_set_size(header, SCREEN_W - 8, 30);
  lv_obj_set_pos(header, 4, 4);
  stylePanel(header, lv_color_hex(0x181825), lv_color_hex(0x313244));
  lv_obj_t *heading = lv_label_create(header);
  lv_label_set_text(heading, "WI-FI SHARE");
  lv_obj_set_pos(heading, 10, 4);
  setTitleFont(heading);
  lv_obj_set_style_text_color(heading, lv_color_hex(0x96A57E), 0);
  makeButton(header, "BACK", 412, 54, backClicked);

  lv_obj_t *panel = lv_obj_create(lv_scr_act());
  lv_obj_set_size(panel, SCREEN_W - 8, 240);
  lv_obj_set_pos(panel, 4, 39);
  stylePanel(panel, lv_color_hex(0x11111B), lv_color_hex(0x313244));

  // Toggle row. labelX=14 puts "Share over Wi-Fi" at the panel's left edge.
  makeToggleRow(panel, "", 'S', "hare over Wi-Fi", 10, transferEnabled,
                wifiSwitchChanged, 14);

  // Hotspot name: editable while sharing is off, dimmed and locked while on.
  lv_obj_t *nameTitle = lv_label_create(panel);
  lv_label_set_text(nameTitle, "HOTSPOT NAME");
  lv_obj_set_pos(nameTitle, 14, 52);
  setUiFont(nameTitle);
  lv_obj_set_style_text_color(nameTitle, lv_color_hex(0x89B4FA), 0);

  lv_obj_t *nameField = lv_obj_create(panel);
  lv_obj_set_size(nameField, 444, 32);
  lv_obj_set_pos(nameField, 14, 70);
  stylePanel(nameField, lv_color_hex(0x1E1E2E),
             lv_color_hex(editingName ? 0x96A57E : 0x45475A));
  lv_obj_add_event_cb(nameField, wifiNameFieldClicked, LV_EVENT_CLICKED,
                      nullptr);
  lv_obj_t *nameValue = lv_label_create(nameField);
  {
    // 32-char SSID + caret + NUL fits in 36 bytes.
    char shown[36];
    snprintf(shown, sizeof(shown), "%s%s", wifiSsid, editingName ? "_" : "");
    lv_label_set_text(nameValue, shown);
  }
  lv_obj_set_pos(nameValue, 10, 8);
  lv_obj_set_style_text_font(nameValue, &elms_bold_16, 0);
  lv_obj_set_style_text_color(nameValue,
      lv_color_hex(transferEnabled ? 0x7F849C
                   : (editingName ? 0x96A57E : 0xCDD6F4)), 0);
  wifiNameValueLabel = nameValue;

  // Password: editable while off (WPA2 needs 8..63 chars), locked while on.
  lv_obj_t *passTitle = lv_label_create(panel);
  lv_label_set_text(passTitle, "PASSWORD");
  lv_obj_set_pos(passTitle, 14, 112);
  setUiFont(passTitle);
  lv_obj_set_style_text_color(passTitle, lv_color_hex(0x89B4FA), 0);

  lv_obj_t *passField = lv_obj_create(panel);
  lv_obj_set_size(passField, 444, 32);
  lv_obj_set_pos(passField, 14, 130);
  stylePanel(passField, lv_color_hex(0x1E1E2E),
             lv_color_hex(editingPassword ? 0x96A57E : 0x45475A));
  lv_obj_add_event_cb(passField, wifiPasswordFieldClicked, LV_EVENT_CLICKED,
                      nullptr);
  lv_obj_t *passValue = lv_label_create(passField);
  {
    // 63-char passphrase + caret + NUL fits in 66 bytes.
    char shown[66];
    snprintf(shown, sizeof(shown), "%s%s", wifiApPassword,
             editingPassword ? "_" : "");
    lv_label_set_text(passValue, shown);
  }
  lv_obj_set_pos(passValue, 10, 8);
  lv_obj_set_style_text_font(passValue, &elms_bold_16, 0);
  lv_obj_set_style_text_color(passValue,
      lv_color_hex(transferEnabled ? 0x7F849C
                   : (editingPassword ? 0x96A57E : 0xCDD6F4)), 0);
  wifiPasswordValueLabel = passValue;

  // Address block, or a hint why sharing cannot start.
  if (transferEnabled) {
    char url[24];
    formatWifiUrl(url, sizeof(url));
    lv_obj_t *connectTitle = lv_label_create(panel);
    lv_label_set_text(connectTitle, "CONNECT TO");
    lv_obj_set_pos(connectTitle, 14, 174);
    setUiFont(connectTitle);
    lv_obj_set_style_text_color(connectTitle, lv_color_hex(0x89B4FA), 0);
    lv_obj_t *urlLabel = lv_label_create(panel);
    lv_label_set_text(urlLabel, url);
    lv_obj_set_pos(urlLabel, 14, 192);
    setTitleFont(urlLabel);
    lv_obj_set_style_text_color(urlLabel, lv_color_hex(0x96A57E), 0);
    lv_obj_t *note = lv_label_create(panel);
    lv_label_set_text(note, "Open this address in a browser");
    lv_obj_set_pos(note, 14, 214);
    setUiFont(note);
    lv_obj_set_style_text_color(note, lv_color_hex(0x7F849C), 0);
  } else if (!cardAvailable) {
    lv_obj_t *warning = lv_label_create(panel);
    lv_label_set_text(warning, "Insert an SD card to share files");
    lv_obj_set_pos(warning, 14, 184);
    setUiFont(warning);
    lv_obj_set_style_text_color(warning, lv_color_hex(0xF38BA8), 0);
  } else {
    lv_obj_t *idle = lv_label_create(panel);
    lv_label_set_text(idle,
        editingName ? "Type a name, then press Enter"
        : editingPassword ? "Type a password (8-63), then Enter"
        : "Turn on sharing to get the address");
    lv_obj_set_pos(idle, 14, 184);
    setUiFont(idle);
    lv_obj_set_style_text_color(idle, lv_color_hex(0x7F849C), 0);
  }

  // Footer hints follow the mode.
  lv_obj_t *footRow = makeHintRow();
  if (editingName) {
    addHintPair(footRow, "type", "name");
    addHintPair(footRow, "enter", "done");
  } else if (editingPassword) {
    addHintPair(footRow, "type", "password");
    addHintPair(footRow, "enter", "done");
  } else if (transferEnabled) {
    addHintPair(footRow, "enter", "stop");
    addHintPair(footRow, "esc", "back");
  } else {
    addHintPair(footRow, "enter", "start");
    addHintPair(footRow, "n", "name");
    addHintPair(footRow, "p", "password");
    addHintPair(footRow, "esc", "back");
  }
}

void buildPairingScreen() {
  editorVisible = false;
  pairingVisible = true;
  settingsVisible = menuVisible = startVisible = false;
  helpVisible = typingVisible = typingSettingsVisible = previewVisible = false;
  wifiScreenVisible = false;
  lv_obj_clean(lv_scr_act());
  pairingStatusLabel = nullptr;
  settingsRadioLabel = nullptr;
  brightnessStatusLabel = nullptr;
  brightnessSlider = smoothSwitch = markdownSwitch = autoSaveSwitch =
      dimSwitch = lineNumberSwitch = wordWrapSwitch = nullptr;
  lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x11111B), 0);

  lv_obj_t *header = lv_obj_create(lv_scr_act());
  lv_obj_set_size(header, SCREEN_W - 8, 30);
  lv_obj_set_pos(header, 4, 4);
  stylePanel(header, lv_color_hex(0x181825), lv_color_hex(0x313244));
  lv_obj_t *heading = lv_label_create(header);
  lv_label_set_text(heading, "BLE KEYBOARDS");
  lv_obj_set_pos(heading, 10, 4);
  setTitleFont(heading);
  lv_obj_set_style_text_color(heading, lv_color_hex(0x96A57E), 0);
  makeButton(header, "RESCAN", 327, 76, rescanKeyboardsClicked);
  makeButton(header, "BACK", 412, 54, backFromPairing);

  lv_obj_t *panel = lv_obj_create(lv_scr_act());
  lv_obj_set_size(panel, SCREEN_W - 8, 240);
  lv_obj_set_pos(panel, 4, 39);
  stylePanel(panel, lv_color_hex(0x11111B), lv_color_hex(0x313244));
  pairingStatusLabel = lv_label_create(panel);
  lv_obj_set_width(pairingStatusLabel, 442);
  lv_label_set_long_mode(pairingStatusLabel, LV_LABEL_LONG_DOT);
  lv_label_set_text(pairingStatusLabel, keyboardStatus());
  lv_obj_set_pos(pairingStatusLabel, 14, 8);
  setUiFont(pairingStatusLabel);
  lv_obj_set_style_text_color(pairingStatusLabel, lv_color_hex(0xBAC2DE), 0);

  const size_t count = keyboardCandidateCount();
  pairListRenderedCount = count;
  if (pairSelection >= count && count) pairSelection = count - 1;
  if (count == 0) {
    lv_obj_t *empty = lv_label_create(panel);
    lv_label_set_text(empty, "Turn on a BLE keyboard in pairing mode.\n"
                             "Scanning is automatic; tap RESCAN to search again.");
    lv_obj_set_pos(empty, 14, 45);
    setUiFont(empty);
    lv_obj_set_style_text_color(empty, lv_color_hex(0xA6ADC8), 0);
  } else {
    const size_t first = pairSelection >= 6 ? pairSelection - 5 : 0;
    const size_t last = std::min(count, first + 6);
    for (size_t i = first; i < last; ++i) {
      const char *name = keyboardCandidateName(i);
      lv_obj_t *button = makeMenuButton(panel, name && name[0] ? name : "BLE keyboard",
          10, 34 + static_cast<int>(i - first) * 28, 450, 25,
          keyboardCandidateClicked,
          reinterpret_cast<void *>(static_cast<intptr_t>(i)));
      lv_obj_set_style_bg_color(button,
          i == pairSelection ? lv_color_hex(0x313244) : lv_color_hex(0x1E1E2E), 0);
      lv_obj_set_style_border_color(button,
          i == pairSelection ? lv_color_hex(0x96A57E) : lv_color_hex(0x45475A), 0);
    }
  }

  lv_obj_t *footRow = makeHintRow();
  lv_obj_t *lead = makeHintWord(footRow, "tap a keyboard or");
  lv_obj_set_style_pad_right(lead, 4, 0);
  addHintPair(footRow, "↑/↓", "select");
  addHintPair(footRow, "Enter", "connect");
  addHintPair(footRow, "r", "rescan");
  addHintPair(footRow, "esc", "back");
  uiNeedsRender = false;
}

void moveVertical(int direction) {
  const size_t col = columnFor(cursor);
  const size_t line = lineNumber(cursor);
  if (direction < 0) {
    if (line == 0) return;
    cursor = lineOffsetForColumn(line - 1, col);
    return;
  }
  // A document ending in '\n' has a real, empty last line at offset size:
  // that is a valid caret position and the only way to reach (and Backspace)
  // the trailing empty line. Only refuse to move past the actual last line.
  const size_t totalLines = lineNumber(document.size());
  if (line >= totalLines) return;
  cursor = lineOffsetForColumn(line + 1, col);
}

// Word boundaries are ASCII alphanumerics/underscore; any non-ASCII codepoint
// is treated as a word character so w/b never land inside a multi-byte glyph.
bool editorWordAt(size_t off) {
  if (off >= document.size()) return false;
  const unsigned char c = static_cast<unsigned char>(document[off]);
  if (c < 0x80) return std::isalnum(c) || c == '_';
  return true;
}

void moveWordForward() {
  while (cursor < document.size() && editorWordAt(cursor)) {
    cursor = nextCursor(cursor);
  }
  while (cursor < document.size() && !editorWordAt(cursor)) {
    cursor = nextCursor(cursor);
  }
}

void moveWordBackward() {
  if (cursor) cursor = prevCursor(cursor);
  while (cursor && !editorWordAt(cursor)) cursor = prevCursor(cursor);
  while (cursor && editorWordAt(prevCursor(cursor))) {
    cursor = prevCursor(cursor);
  }
}

void deleteCurrentLine(bool copyOnly, bool changeLine) {
  const size_t start = lineStart(cursor);
  const size_t end = lineEnd(cursor);
  const size_t after = end < document.size() ? end + 1 : end;
  if (changeLine && !undoGroupActive) rememberUndo();
  yankBuffer.assign(document.data() + start, after - start);
  if (!copyOnly) eraseRange(start, after);
  if (changeLine) {
    undoGroupActive = true;
    insertMode = true;
    // Typing resumes where the erased line began. eraseRange ran before
    // insert mode was set, so its Normal clamp parks the caret one left
    // of that gap when the change consumed the buffer tail.
    cursor = std::min(start, document.size());
  }
}

void executeOperator(char op, char motion) {
  if (op == 'c' && !undoGroupActive) rememberUndo();
  if (motion == op) {
    deleteCurrentLine(op == 'y', op == 'c');
    return;
  }
  size_t from = cursor, to = cursor;
  switch (motion) {
  case 'w': to = cursor; moveWordForward(); to = cursor; break;
  case '$': to = lineEnd(cursor); break;
  case '0': to = lineStart(cursor); break;
  case 'h': to = prevCursor(to); break;
  case 'l': to = nextCursor(to); break;
  default: pendingOperator = 0; return;
  }
  size_t first = std::min(from, to);
  size_t last = std::max(from, to);
  if (last < document.size() && motion != '$' && motion != '0') {
    last = nextCursor(last);
  }
  if (op == 'y') yankBuffer.assign(document.data() + first, last - first);
  else {
    eraseRange(first, last);
    if (op == 'c') {
      undoGroupActive = true;
      insertMode = true;
      // Same gap rule as cc: typing resumes where the erased range began.
      cursor = std::min(first, document.size());
    }
  }
}

// ---------------------------------------------------------------------------
// Rendered Markdown preview (:p / :preview). The editor stays a raw
// 64-column Vim grid; this screen renders the same document for reading.
// One block per source line, materialized into a pooled set of labels:
// headings use the sans title font with their '#' dropped, quotes get a
// purple bar, fenced code a continuous tint, and emphasis markers are
// replaced by colored spans. Inline spans still ride LVGL recolor, so every
// literal '#' is escaped the same way lexMarkdown() does — an unescaped
// '#' is eaten by the command parser and never renders.
// ---------------------------------------------------------------------------

std::string previewMarkup(const std::string &raw) {
  // lexMarkdown()'s span grammar, minus the emphasis markers: the view
  // should read "bold", not "**bold**". Spans must stay balanced and literal
  // '#' escaped (## outside a span, close+##+reopen inside one).
  std::string out;
  bool inSpan = false;
  const char *spanColor = C_TEXT;

  auto openSpan = [&](const char *color) {
    if (inSpan) out += '#';
    out += '#';
    out += color;
    out += ' ';
    spanColor = color;
    inSpan = true;
  };
  auto closeSpan = [&]() {
    if (inSpan) {
      out += '#';
      inSpan = false;
      spanColor = C_TEXT;
    }
  };
  auto emit = [&](char c) {
    if (c == '#') {
      if (inSpan) {
        out += '#';                  // close span
        out += "##";                 // literal '#'
        out += '#';
        out += spanColor;
        out += ' ';                  // reopen span
      } else {
        out += "##";
      }
    } else {
      out += c;
    }
  };

  const size_t n = raw.size();
  size_t i = 0;
  while (i < n) {
    const char c = raw[i];

    if (c == '\\' && i + 1 < n) {
      emit(raw[i + 1]);
      i += 2;
      continue;
    }

    // Leading list marker ("- ", "* ", "+ ", "N. ", "N) ") stays visible,
    // colored like the editor's bullet spans. Checked before the italic
    // rule so "* item" is a bullet, not an unclosed emphasis.
    if (i == 0) {
      size_t marker = 0;
      if ((c == '-' || c == '+' || c == '*') && n > 1 && raw[1] == ' ') {
        marker = 2;
      } else {
        size_t j = 0;
        while (j < n && isdigit(static_cast<unsigned char>(raw[j]))) ++j;
        if (j > 0 && j + 1 < n && (raw[j] == '.' || raw[j] == ')') &&
            raw[j + 1] == ' ') {
          marker = j + 2;
        }
      }
      if (marker) {
        openSpan(C_BULLET);
        for (size_t k = 0; k < marker; ++k) emit(raw[k]);
        closeSpan();
        i = marker;
        continue;
      }
    }

    // Bold **...** / __...__ — markers dropped, text kept.
    if ((c == '*' || c == '_') && i + 1 < n && raw[i + 1] == c) {
      const size_t close = raw.find(std::string(2, c), i + 2);
      if (close != std::string::npos && close > i + 2) {
        openSpan(C_BOLD);
        for (size_t k = i + 2; k < close; ++k) emit(raw[k]);
        closeSpan();
        i = close + 2;
        continue;
      }
    }

    // Italic *...* / _..._ (single marker, not a double).
    if (c == '*' || c == '_') {
      const size_t close = raw.find(c, i + 1);
      if (close != std::string::npos && close > i + 1 &&
          !(close + 1 < n && raw[close + 1] == c)) {
        openSpan(C_ITALIC);
        for (size_t k = i + 1; k < close; ++k) emit(raw[k]);
        closeSpan();
        i = close + 1;
        continue;
      }
    }

    // Inline code `...` — backticks dropped.
    if (c == '`') {
      const size_t close = raw.find('`', i + 1);
      if (close != std::string::npos && close > i + 1) {
        openSpan(C_CODE);
        for (size_t k = i + 1; k < close; ++k) emit(raw[k]);
        closeSpan();
        i = close + 1;
        continue;
      }
    }

    emit(c);
    ++i;
  }
  return out;
}

uint8_t previewTypeFor(const std::string &raw) {
  if (raw.empty() || raw.find_first_not_of(' ') == std::string::npos) {
    return PB_BLANK;
  }
  if (raw[0] == '#') return PB_HEAD;
  if (raw[0] == '>') return PB_QUOTE;
  size_t j = 0;
  while (j < raw.size() && isdigit(static_cast<unsigned char>(raw[j]))) ++j;
  if ((raw[0] == '-' || raw[0] == '+' || raw[0] == '*') && raw.size() > 1 &&
      raw[1] == ' ') {
    return PB_LIST;
  }
  if (j > 0 && j + 1 < raw.size() && (raw[j] == '.' || raw[j] == ')') &&
      raw[j + 1] == ' ') {
    return PB_LIST;
  }
  return PB_PARA;
}

// Preview lines clip at the cap, but never through a UTF-8 sequence.
size_t previewLineLength(size_t off, size_t cap) {
  const size_t end = lineEnd(off);
  size_t n = std::min(end - off, cap);
  while (n > 0 && off + n < end &&
         utf8Continuation(static_cast<unsigned char>(document[off + n]))) {
    --n;
  }
  return n;
}

void buildPreviewBlocks() {
  previewBlockCount = 0;
  previewTotalHeight = 0;
  previewTruncated = false;
  bool inCode = false;
  size_t off = 0;
  while (off < document.size() && previewBlockCount < PREVIEW_MAX_BLOCKS) {
    const size_t end = lineEnd(off);
    std::string raw(document.data() + off,
                    previewLineLength(off, PREVIEW_LINE_CAP));
    if (raw.rfind("```", 0) == 0) {
      // Fences bound the tinted block; the marker line itself renders as
      // nothing, so an unclosed fence simply runs to the end.
      inCode = !inCode;
      off = end + 1;
      continue;
    }
    const uint8_t type = inCode ? uint8_t(PB_CODE) : previewTypeFor(raw);
    uint16_t height = 8;  // blank source line = paragraph spacing
    if (type != PB_BLANK) {
      std::string text = raw;
      if (type == PB_HEAD) {
        size_t s = 0;
        while (s < text.size() && text[s] == '#') ++s;
        if (s < text.size() && text[s] == ' ') ++s;
        text = text.substr(s);
      } else if (type == PB_QUOTE) {
        if (text.size() > 1 && text[1] == ' ') text = text.substr(2);
        else text = text.substr(1);
      }
      const lv_font_t *font = type == PB_HEAD ? &elms_bold_16 : editorFont();
      const std::string markup =
          type == PB_CODE ? text : previewMarkup(text);
      // Wrap width must equal the label's content width (label width 448;
      // quotes lose 3 px to the bar border and 6 px to its padding).
      const int wrapW = type == PB_QUOTE ? 439 : 448;
      lv_point_t size;
      lv_txt_get_size(&size, markup.c_str(), font, 0, 0, wrapW,
                      type == PB_CODE ? LV_TEXT_FLAG_NONE
                                      : LV_TEXT_FLAG_RECOLOR);
      if (size.y < font->line_height) size.y = font->line_height;
      const int gap = type == PB_HEAD ? 6 : type == PB_CODE ? 4 : 2;
      height = uint16_t(size.y + gap);
    }
    previewBlocks[previewBlockCount++] = {uint32_t(off), height, type};
    previewTotalHeight += height;
    off = end + 1;
  }
  if (off < document.size()) previewTruncated = true;
}

void previewMaterialize() {
  int y = -previewScrollY;
  int slot = 0;
  for (size_t b = 0; b < previewBlockCount && slot < PREVIEW_POOL_LABELS; ++b) {
    const PreviewBlock &blk = previewBlocks[b];
    if (y + blk.height <= 0) {  // fully above the viewport
      y += blk.height;
      continue;
    }
    if (y >= PREVIEW_VIEW_H) break;  // nothing below can fit either
    if (blk.type != PB_BLANK) {
      lv_obj_t *label = previewPool[slot++];
      std::string text(document.data() + blk.offset,
                       previewLineLength(blk.offset, PREVIEW_LINE_CAP));
      if (blk.type == PB_HEAD) {
        size_t s = 0;
        while (s < text.size() && text[s] == '#') ++s;
        if (s < text.size() && text[s] == ' ') ++s;
        text = text.substr(s);
      } else if (blk.type == PB_QUOTE) {
        if (text.size() > 1 && text[1] == ' ') text = text.substr(2);
        else text = text.substr(1);
      }
      // Plain code must never parse '#' as a command; every other type
      // carries recolor spans and escaped hashes.
      const bool code = blk.type == PB_CODE;
      lv_label_set_recolor(label, !code);
      lv_label_set_text(label,
          code ? text.c_str() : previewMarkup(text).c_str());
      lv_obj_set_style_text_font(label,
          blk.type == PB_HEAD ? &elms_bold_16 : editorFont(), 0);
      const char *base = blk.type == PB_HEAD ? C_HEADING
                        : blk.type == PB_QUOTE ? C_QUOTE
                        : code ? C_CODE : C_TEXT;
      lv_obj_set_style_text_color(label,
          lv_color_hex(strtoul(base, nullptr, 16)), 0);
      // Quote bar via a left border, which also indents the text — wanted.
      lv_obj_set_style_border_width(label, blk.type == PB_QUOTE ? 3 : 0, 0);
      lv_obj_set_style_border_side(label, LV_BORDER_SIDE_LEFT, 0);
      lv_obj_set_style_border_color(label,
          lv_color_hex(strtoul(C_QUOTE, nullptr, 16)), 0);
      lv_obj_set_style_pad_left(label, blk.type == PB_QUOTE ? 6 : 0, 0);
      // Code rows keep the full slot height with an opaque background so
      // consecutive lines join with no seam.
      if (code) {
        lv_obj_set_height(label, blk.height);
        lv_obj_set_style_bg_color(label, lv_color_hex(0x181825), 0);
        lv_obj_set_style_bg_opa(label, LV_OPA_COVER, 0);
        lv_obj_set_style_pad_top(label, 2, 0);
      } else {
        lv_obj_set_height(label, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(label, LV_OPA_TRANSP, 0);
        lv_obj_set_style_pad_top(label, 0, 0);
      }
      lv_obj_set_pos(label, blk.type == PB_QUOTE ? 8 : 10, y);
      lv_obj_clear_flag(label, LV_OBJ_FLAG_HIDDEN);
    }
    y += blk.height;
  }
  for (; slot < PREVIEW_POOL_LABELS; ++slot) {
    setWidgetHidden(previewPool[slot], true);
  }
}

void previewScrollBy(int dy) {
  const int maxScroll = previewTotalHeight > PREVIEW_VIEW_H
                            ? previewTotalHeight - PREVIEW_VIEW_H : 0;
  int next = previewScrollY + dy;
  if (next < 0) next = 0;
  if (next > maxScroll) next = maxScroll;
  if (next == previewScrollY) return;
  previewScrollY = next;
  previewMaterialize();
}

void previewBackClicked(lv_event_t *) {
  previewVisible = false;
  editorVisible = true;
  buildEditorScreen();
}

void buildPreviewScreen() {
  editorVisible = false;
  previewVisible = true;
  pairingVisible = settingsVisible = menuVisible = helpVisible = startVisible =
      false;
  typingVisible = typingSettingsVisible = false;
  wifiScreenVisible = false;
  lv_obj_clean(lv_scr_act());
  pairingStatusLabel = nullptr;
  settingsRadioLabel = nullptr;
  brightnessStatusLabel = nullptr;
  brightnessSlider = smoothSwitch = markdownSwitch = autoSaveSwitch =
      dimSwitch = lineNumberSwitch = wordWrapSwitch = nullptr;
  lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x11111B), 0);

  lv_obj_t *header = lv_obj_create(lv_scr_act());
  lv_obj_set_size(header, SCREEN_W - 8, 30);
  lv_obj_set_pos(header, 4, 4);
  stylePanel(header, lv_color_hex(0x181825), lv_color_hex(0x313244));
  lv_obj_t *title = lv_label_create(header);
  std::string heading = previewTruncated
      ? "PREVIEW — " + fileName + " (TRUNCATED)"
      : "PREVIEW — " + fileName;
  lv_label_set_text(title, heading.c_str());
  lv_obj_set_pos(title, 10, 4);
  setTitleFont(title);
  lv_obj_set_style_text_color(title, lv_color_hex(0x96A57E), 0);
  makeButton(header, "BACK", 408, 58, previewBackClicked);

  lv_obj_t *panel = lv_obj_create(lv_scr_act());
  lv_obj_set_size(panel, SCREEN_W - 8, PREVIEW_VIEW_H);
  lv_obj_set_pos(panel, 4, 39);
  stylePanel(panel, lv_color_hex(0x11111B), lv_color_hex(0x313244));
  lv_obj_set_style_pad_all(panel, 0, 0);
  lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
  for (size_t i = 0; i < PREVIEW_POOL_LABELS; ++i) {
    previewPool[i] = lv_label_create(panel);
    // 448 px inner width leaves 24 px for margins + future scrollbar art.
    lv_obj_set_size(previewPool[i], 448, LV_SIZE_CONTENT);
    lv_obj_add_flag(previewPool[i], LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_pad_all(previewPool[i], 0, 0);
  }

  lv_obj_t *foot = lv_label_create(lv_scr_act());
  lv_label_set_text(foot, "j/k or arrows scroll · pgup/pgdn page · q back");
  lv_obj_set_pos(foot, 10, 284);
  setUiFont(foot);
  lv_obj_set_style_text_color(foot, lv_color_hex(0x7F849C), 0);
  previewMaterialize();
}

void showPreview() {
  if (!previewBlocks) {  // PSRAM table failed to allocate at boot
    statusMessage = "Preview unavailable without PSRAM";
    uiNeedsRender = true;
    return;
  }
  buildPreviewBlocks();
  previewScrollY = 0;
  buildPreviewScreen();
}

// --- Version history (:history) ---------------------------------------------
void scanHistoryFiles() {
  historyFiles.clear();
  if (currentPath.empty() || !cardAvailable) return;
  File dir = SD.open(SNAPSHOT_DIR);
  if (!dir || !dir.isDirectory()) {
    if (dir) dir.close();
    return;
  }
  const String base = fileName.c_str();
  for (File entry = dir.openNextFile(); entry; entry = dir.openNextFile()) {
    String name = entry.name();
    const int slash = name.lastIndexOf('/');
    if (slash >= 0) name = name.substring(slash + 1);
    if (snapshotNumber(name, base) >= 0) historyFiles.push_back(name);
    entry.close();
  }
  dir.close();
  // Zero-padded version names sort correctly as plain strings; newest first.
  std::sort(historyFiles.begin(), historyFiles.end(),
            [](const String &a, const String &b) { return a > b; });
  // Retention tops out at 10, which is exactly what the panel can show.
  if (historyFiles.size() > 10) historyFiles.resize(10);
}

void historyBackClicked(lv_event_t *) {
  historyVisible = false;
  editorVisible = true;
  buildEditorScreen();
}

void restoreHistory(size_t index) {
  if (index >= historyFiles.size()) return;
  if (dirty) {
    statusMessage = "Save changes first (:w)";
    uiNeedsRender = true;
    return;
  }
  const String path = String(SNAPSHOT_DIR) + "/" + historyFiles[index];
  File file = SD.open(path.c_str(), FILE_READ);
  if (!file || file.isDirectory()) {
    statusMessage = "Could not open version";
    if (file) file.close();
    return;
  }
  if (file.size() > documentLimit) {
    file.close();
    statusMessage = "Version exceeds editing memory";
    uiNeedsRender = true;
    return;
  }
  // Undo returns to the buffer that was on screen before the restore.
  rememberUndo();
  document.clear();
  while (file.available() && document.size() < documentLimit) {
    char ch = static_cast<char>(file.read());
    if (ch == '\n' && !document.empty() && document.back() == '\r') {
      document.back() = '\n';
      continue;
    }
    document.push_back(ch);
  }
  file.close();
  markDocumentChanged();
  cursor = 0;
  viewportStart = viewportWrapCol = viewportScrollCol = 0;
  dirty = true;  // the restored text is not saved yet
  undoGroupActive = false;
  insertMode = visualMode = commandMode = searchMode = false;
  pendingOperator = 0;
  sessionWordBase = countDocumentWords();
  documentWordCacheValid = false;
  historyVisible = false;
  editorVisible = true;
  statusMessage =
      "Restored " + std::string(historyFiles[index].c_str()) + " (unsaved)";
  buildEditorScreen();
}

void historyRowClicked(lv_event_t *event) {
  const intptr_t index =
      reinterpret_cast<intptr_t>(lv_event_get_user_data(event));
  if (index >= 0) restoreHistory(static_cast<size_t>(index));
}

void showHistory() {
  if (currentPath.empty()) {
    statusMessage = "Save the file first (:w)";
    uiNeedsRender = true;
    return;
  }
  scanHistoryFiles();
  if (historyFiles.empty()) {
    statusMessage = snapshotsEnabled ? "No versions yet" : "Snapshots are off";
    uiNeedsRender = true;
    return;
  }
  backTarget = BackTarget::Editor;
  historyVisible = true;
  historySelection = 0;
  editorVisible = false;
  pairingVisible = settingsVisible = menuVisible = startVisible = false;
  helpVisible = typingVisible = typingSettingsVisible = previewVisible = false;
  wifiScreenVisible = false;
  buildHistoryScreen();
}

// One row per snapshot, newest first. Ten rows fit the panel at a 22 px pitch.
void buildHistoryScreen() {
  historyVisible = true;
  editorVisible = false;
  pairingVisible = settingsVisible = menuVisible = startVisible = false;
  helpVisible = typingVisible = typingSettingsVisible = previewVisible = false;
  wifiScreenVisible = false;
  lv_obj_clean(lv_scr_act());
  pairingStatusLabel = settingsRadioLabel = brightnessStatusLabel = nullptr;
  menuRadioLabel = nullptr;
  brightnessSlider = smoothSwitch = markdownSwitch = autoSaveSwitch =
      dimSwitch = lineNumberSwitch = wordWrapSwitch = nullptr;
  lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x11111B), 0);

  lv_obj_t *header = lv_obj_create(lv_scr_act());
  lv_obj_set_size(header, SCREEN_W - 8, 30);
  lv_obj_set_pos(header, 4, 4);
  stylePanel(header, lv_color_hex(0x181825), lv_color_hex(0x313244));
  lv_obj_t *heading = lv_label_create(header);
  lv_label_set_text(heading, "VERSION HISTORY");
  lv_obj_set_pos(heading, 10, 4);
  setTitleFont(heading);
  lv_obj_set_style_text_color(heading, lv_color_hex(0x96A57E), 0);
  makeButton(header, "BACK", 412, 54, historyBackClicked);

  lv_obj_t *panel = lv_obj_create(lv_scr_act());
  lv_obj_set_size(panel, SCREEN_W - 8, 240);
  lv_obj_set_pos(panel, 4, 39);
  stylePanel(panel, lv_color_hex(0x11111B), lv_color_hex(0x313244));

  if (historySelection >= historyFiles.size()) historySelection = 0;
  for (size_t i = 0; i < historyFiles.size() && i < 10; ++i) {
    const bool selected = i == historySelection;
    lv_obj_t *row = lv_btn_create(panel);
    lv_obj_set_size(row, 444, 20);
    lv_obj_set_pos(row, 14, 14 + static_cast<int>(i) * 22);
    stylePanel(row, lv_color_hex(selected ? 0x313244 : 0x1E1E2E),
               lv_color_hex(selected ? 0x96A57E : 0x45475A));
    lv_obj_add_event_cb(row, [](lv_event_t *) {
      lastInputMs = millis();
      if (screenOff) {
        display.setBrightness(brightnessValue);
        screenOff = false;
      }
      if (backlightDimmed) {
        display.setBrightness(brightnessValue);
        backlightDimmed = false;
      }
    }, LV_EVENT_CLICKED, nullptr);
    lv_obj_add_event_cb(row, historyRowClicked, LV_EVENT_CLICKED,
                        reinterpret_cast<void *>(static_cast<intptr_t>(i)));

    const String &name = historyFiles[i];
    const int marker = name.lastIndexOf(".v");
    String label = marker >= 0 ? name.substring(marker + 1, name.length() - 4)
                               : name;
    if (!label.startsWith("v")) label = "v" + label;

    lv_obj_t *nameLabel = lv_label_create(row);
    lv_label_set_text(nameLabel, label.c_str());
    lv_obj_set_pos(nameLabel, 9, 3);
    setUiFont(nameLabel);
    lv_obj_set_style_text_color(nameLabel,
        lv_color_hex(selected ? 0xB5C39D : 0xCDD6F4), 0);

    lv_obj_t *hint = lv_label_create(row);
    lv_label_set_text(hint, "restore");
    lv_obj_set_width(hint, 90);
    lv_obj_set_pos(hint, 444 - 9 - 90, 3);
    lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_RIGHT, 0);
    setUiFont(hint);
    lv_obj_set_style_text_color(hint, lv_color_hex(0x7F849C), 0);
  }

  lv_obj_t *footRow = makeHintRow();
  if (dirty) {
    // Restoring would replace unsaved text; say so where it is visible.
    addHintPair(footRow, "esc", "back");
    makeHintWord(footRow, "save first (:w) to restore");
  } else {
    addHintPair(footRow, "↑↓", "select");
    addHintPair(footRow, "enter", "restore");
    addHintPair(footRow, "esc", "back");
  }
  uiNeedsRender = false;
}

// Built-in Markdown skeletons for :tpl. They live in flash, use placeholders
// instead of a timestamp (the board has no RTC), and stay smaller than the
// document limit by a wide margin.
struct TemplateDef {
  const char *name;
  const char *body;
};

const TemplateDef TEMPLATES[] = {
    {"journal",
     "# Journal - YYYY-MM-DD\n\n## Morning\n\n\n## Evening\n\n"},
    {"meeting",
     "# Meeting - YYYY-MM-DD\n\n**Attendees:**\n\n## Agenda\n\n- \n\n"
     "## Notes\n\n\n## Actions\n\n- [ ] \n"},
    {"blog",
     "# Title\n\n_YYYY-MM-DD_\n\n\n\n## Section\n\n"},
    {"bug",
     "# Bug: short title\n\n**Steps**\n\n1. \n2. \n\n**Expected**\n\n\n"
     "**Actual**\n\n\n"},
    {"daily",
     "# Daily log - YYYY-MM-DD\n\n## Done\n\n- \n\n## Next\n\n- [ ] \n"},
};
constexpr size_t TEMPLATE_COUNT = sizeof(TEMPLATES) / sizeof(TEMPLATES[0]);

const TemplateDef *findTemplate(const std::string &name) {
  for (size_t i = 0; i < TEMPLATE_COUNT; ++i) {
    if (name == TEMPLATES[i].name) return &TEMPLATES[i];
  }
  return nullptr;
}

void insertTemplate(const char *body) {
  if (!body) return;
  // One undo snapshot for the skeleton, then a fresh insert group so typing
  // after it can be undone separately from the template.
  rememberUndo();
  undoGroupActive = true;
  for (const char *p = body; *p; ++p) {
    if (document.size() >= documentLimit) break;
    insertAtCursor(*p);
  }
  // Keep the group open: one undo returns to the text before the template,
  // taking any typing done straight after it along too.
  undoGroupActive = true;
  insertMode = true;
  statusMessage = "Template inserted";
  uiNeedsRender = true;
}

void runCommand() {
  // A bare number is a jump target: ":42" goes to line 42, the way Vim's
  // ex command does without a ":" prefix of its own.
  bool gotoCommand = !commandLine.empty();
  for (char ch : commandLine) {
    if (ch < '0' || ch > '9') {
      gotoCommand = false;
      break;
    }
  }
  if (gotoCommand) {
    const size_t target =
        static_cast<size_t>(strtoul(commandLine.c_str(), nullptr, 10));
    const size_t total = lineNumber(document.size()) + 1;
    if (target == 0 || target > total) {
      statusMessage = "Line out of range";
    } else {
      cursor = offsetForLine(target - 1);
      statusMessage = "Line " + std::to_string(target) + " of " +
                      std::to_string(total);
    }
  } else if (commandLine == "w" || commandLine == "write") {
    saveDocument();
  } else if (commandLine.rfind("w ", 0) == 0 ||
             commandLine.rfind("write ", 0) == 0) {
    const size_t firstSpace = commandLine.find(' ');
    const String name = safeDocumentName(
        String(commandLine.substr(firstSpace + 1).c_str()));
    if (name.isEmpty()) {
      statusMessage = "Use :w name.ext";
    } else {
      fileName = name.c_str();
      currentPath.clear();
      // A named save can change the grammar (":w notes.py" from a Markdown
      // buffer), so follow the extension.
      editorFileType = docLangOrPlain(fileName);
      saveDocument();
    }
  } else if (commandLine == "wq") {
    if (saveDocument()) quitEditor();
  } else if (commandLine == "q" || commandLine == "quit") {
    quitEditor();
  } else if (commandLine == "e" || commandLine == "edit") {
    showBrowser(true);
  } else if (commandLine == "new") {
    newDocument();
  } else if (commandLine == "wifi") {
    startTransfer();
  } else if (commandLine == "wifi-off") {
    stopTransfer();
  } else if (commandLine == "bright") {
    cycleBrightness();
  } else if (commandLine == "settings") {
    showSettings(true);
  } else if (commandLine == "menu") {
    showMainMenu();
  } else if (commandLine == "pair") {
    // :pair comes from the editor, so BACK returns there (not to Settings).
    backTarget = BackTarget::Editor;
    showPairingHelp();
  } else if (commandLine == "zen") {
    editorZenMode = !editorZenMode;
    prefsDirty = true;
    savePrefs();
    buildEditorScreen();
  } else if (commandLine == "lock") {
    lockReturnTarget = LockReturn::Editor;
    lockDevice();
  } else if (commandLine == "history") {
    showHistory();
  } else if (commandLine == "tpl") {
    statusMessage = "Templates: journal meeting blog bug daily";
  } else if (commandLine.rfind("tpl ", 0) == 0) {
    const std::string name = commandLine.substr(4);
    const TemplateDef *tpl = findTemplate(name);
    if (tpl) insertTemplate(tpl->body);
    else statusMessage = "No template: " + name;
  } else if (commandLine == "p" || commandLine == "preview") {
    showPreview();
  } else if (commandLine == "font") {
    stepEditorFontSize(1);
  } else if (commandLine.rfind("font ", 0) == 0) {
    const int px = atoi(commandLine.c_str() + 5);
    int index = -1;
    for (int i = 0; i < EDITOR_FONT_SIZE_COUNT; ++i) {
      if (EDITOR_FONT_PIXELS[i] == px) index = i;
    }
    if (index < 0) {
      statusMessage = "Text size must be 10, 12, 14 or 16";
    } else {
      setEditorFontSize(index);
    }
  } else {
    statusMessage =
        "Commands: :w :e :new :p :history :tpl :font :zen :lock :menu :settings";
  }
  commandLine.clear();
  commandMode = searchMode = false;
  uiNeedsRender = true;
}

// Lowercase an ASCII letter and leave every other byte alone, so multi-byte
// UTF-8 in the document is never corrupted by case folding.
inline char lowerAscii(char c) {
  return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c;
}

// Smart case: an all-lowercase pattern matches any case, while any capital
// makes it exact. This is what makes "/the" find "The" without surprising
// anyone who deliberately types "/London".
bool patternHasUpper(const char *needle) {
  for (; *needle; ++needle) {
    if (*needle >= 'A' && *needle <= 'Z') return true;
  }
  return false;
}

// Literal search over the document, scanning in place so even a 512 KiB draft
// costs no allocation on the search path. `from` is clamped the same way as
// std::string::find/rfind, and a backward search returns the last match whose
// start is at or before `from`.
size_t findLiteral(const char *needle, size_t needleLen, size_t from,
                   bool foldCase, bool backward) {
  const size_t size = document.size();
  if (needleLen == 0 || needleLen > size) return DocumentString::npos;
  auto matchesAt = [&](size_t at) {
    for (size_t j = 0; j < needleLen; ++j) {
      const char hay = document[at + j];
      const char pat = needle[j];
      if (foldCase ? lowerAscii(hay) != lowerAscii(pat) : hay != pat) {
        return false;
      }
    }
    return true;
  };
  if (!backward) {
    if (from > size - needleLen) return DocumentString::npos;
    for (size_t at = from; at + needleLen <= size; ++at) {
      if (matchesAt(at)) return at;
    }
    return DocumentString::npos;
  }
  size_t at = std::min(from, size - needleLen);
  for (;; --at) {
    if (matchesAt(at)) return at;
    if (at == 0) break;
  }
  return DocumentString::npos;
}

// Search the document for `needle`, starting just after (or before) the caret.
// The explicit "/" search always wraps: a word typed just above the caret is
// the common case, and reporting "Pattern not found" for text that is clearly
// on the page feels broken. n/N repeat only when Search wrap is enabled, so
// repeated jumps stay predictable.
bool applySearch(const std::string &needle, bool forward, bool wrap) {
  if (needle.empty()) return false;
  const bool foldCase = !patternHasUpper(needle.c_str());
  const char *pat = needle.c_str();
  const size_t len = needle.size();
  size_t found = DocumentString::npos;
  if (forward) {
    const size_t start = cursor < document.size() ? cursor + 1 : cursor;
    found = findLiteral(pat, len, start, foldCase, false);
    // Wrap to the top even when the only match starts at the caret itself, so
    // "/word" at the end of a freshly typed word still lands on the word.
    if (found == DocumentString::npos && wrap) {
      found = findLiteral(pat, len, 0, foldCase, false);
    }
  } else {
    if (cursor > 0) {
      found = findLiteral(pat, len, cursor - 1, foldCase, true);
    }
    if (found == DocumentString::npos && wrap) {
      found = findLiteral(pat, len, document.size(), foldCase, true);
    }
  }
  if (found == DocumentString::npos) {
    statusMessage = "Pattern not found";
    return false;
  }
  cursor = found;
  statusMessage = "Search: line " + std::to_string(lineNumber(found) + 1);
  return true;
}

void repeatSearch(bool forward) {
  if (lastSearch.empty()) {
    statusMessage = "No previous search (use /)";
    uiNeedsRender = true;
    return;
  }
  applySearch(lastSearch, forward, editorSearchWrap);
  uiNeedsRender = true;
}

void handleText(char c) {
  // The login screen owns every keystroke while it is up, including printable
  // characters that would otherwise drive the menu shortcuts.
  if (lockActive) {
    lockHandleText(c);
    return;
  }
  if (!editorVisible) {
    if (historyVisible) {
      if (c == 'j' || c == 'k') {
        if (!historyFiles.empty()) {
          int next = static_cast<int>(historySelection) +
                     (c == 'j' ? 1 : -1);
          if (next < 0) next = static_cast<int>(historyFiles.size()) - 1;
          if (next >= static_cast<int>(historyFiles.size())) next = 0;
          historySelection = static_cast<size_t>(next);
          buildHistoryScreen();
        }
      } else if (c == 'q' || c == 27) {
        historyBackClicked(nullptr);
      } else if (c == '\n' || c == '\r') {
        restoreHistory(historySelection);
      }
      return;
    }
    if (wifiScreenVisible) {
      if (wifiEditingField != WifiField::None && c >= 0x20 && c < 0x7F) {
        wifiFieldAppend(c);
        refreshWifiFieldText();
      } else if (wifiEditingField == WifiField::None && !transferEnabled &&
                 (c == 'n' || c == 'N' || c == 'p' || c == 'P')) {
        wifiEditingField = (c == 'n' || c == 'N') ? WifiField::Name
                                                  : WifiField::Password;
        buildWifiScreen();
      }
      return;
    }
    if (typingVisible) {
      typingHandleText(c);
      return;
    }
    if (typingSettingsVisible) {
      if (c == 'c' || c == 'C') cycleTypingComplexity();
      else if (c == 'm' || c == 'M') cycleTypingMaxLength();
      else if (c == 's' || c == 'S') toggleTypingSymbols();
      else if (c == 'n' || c == 'N') toggleTypingNumbers();
      else if (c == 'a' || c == 'A') toggleTypingCapitalization();
      else if (c == 'p' || c == 'P') toggleTypingPunctuation();
      else if (c == 'q') typingSettingsBackClicked(nullptr);
      return;
    }
    if (pairingVisible) {
      if (c == 'q') backFromPairing(nullptr);
      else if (c == 'r' || c == 'R') rescanKeyboardsClicked(nullptr);
      else if ((c == 'j' || c == 'k') && keyboardCandidateCount()) {
        const size_t count = keyboardCandidateCount();
        if (c == 'j') pairSelection = (pairSelection + 1) % count;
        else pairSelection = (pairSelection + count - 1) % count;
        buildPairingScreen();
      }
      return;
    }
    if (menuVisible) {
      if (c >= '1' && c <= '4') runMenuEntry(c - '1');
      else if (c == 'j' || c == 'k') {
        // Grid rows: up and down both step one row, and +2 / -2 is the
        // same move in a 2x2 grid (mod 4).
        menuSelection = (menuSelection + 2) % 4;
        refreshMenuSelection();
      } else if (c == 'h' || c == 'l') {
        menuSelection = (menuSelection + (c == 'l' ? 1 : 3)) % 4;
        refreshMenuSelection();
      }
      // Root screen: q and Esc have nowhere to go, so they do nothing.
      return;
    }
    if (startVisible) {
      if (c == 'n') newDocument();
      else if (c == 'f' || c == '/') showBrowser(false, true);
      else if (c == 'r') {
        if (!recentFiles.empty()) {
          recentSelection = 0;
          buildStartScreen();
        }
      } else if (c == 'q' || c == 27) showMainMenu();
      else if (c == 'j' || c == 'k') moveRecentSelection(c == 'j' ? 1 : -1);
      else if (c >= '1' && c <= '5') {
        const size_t index = static_cast<size_t>(c - '1');
        if (index < recentFiles.size()) openRecent(index);
      }
      return;
    }
    if (settingsVisible) {
      if (settingsPopup != SettingsSection::None) {
        // The popup owns the keys while it is open.
        if (c == 'q' || c == 27) {
          closeSettingsPopup();
        } else if (settingsPopup == SettingsSection::FileType) {
          if (c == 'j' || c == 'k') {
            const int step = c == 'j' ? 1 : -1;
            int next = static_cast<int>(settingsPopupSelection) + step;
            if (next < 0) next += static_cast<int>(LANG_COUNT);
            if (next >= static_cast<int>(LANG_COUNT)) {
              next -= static_cast<int>(LANG_COUNT);
            }
            settingsPopupSelection = static_cast<uint8_t>(next);
            // Only the picker's highlight moved; rebuild just the popup.
            rebuildSettingsPopup();
          } else if (c >= '1' && c <= '9') {
            const size_t index = static_cast<size_t>(c - '1');
            if (index < LANG_COUNT) {
              applyDocLang(index);
              closeSettingsPopup();
            }
          } else if (c == '0' && LANG_COUNT >= 10) {
            applyDocLang(9);
            closeSettingsPopup();
          } else if (c == '\n' || c == '\r') {
            settingsPopupActivateRow(settingsPopupSelection);
          }
        } else if (settingsPopup == SettingsSection::Display &&
                   c >= '1' && c <= '4') {
          setBrightnessValue(BRIGHTNESS_PRESETS[c - '1']);
        } else if (c == 'j' || c == 'k') {
          settingsMovePopupSelection(c == 'j' ? 1 : -1);
        } else if (c == 'h' || c == 'l') {
          settingsPopupAdjustRow(c == 'l' ? 1 : -1);
        } else if (c == '\n' || c == '\r') {
          settingsPopupActivateRow(settingsPopupSelection);
        } else {
          settingsPopupLetter(c);
        }
        return;
      }
      // Section list.
      if (c >= '1' &&
          c <= static_cast<char>('0' + static_cast<int>(SETTINGS_SECTION_COUNT))) {
        settingsSectionSelection = static_cast<uint8_t>(c - '1');
        openSettingsPopup(settingsSectionAt(settingsSectionSelection));
      } else if (c == 'j' || c == 'k') {
        settingsMoveSection(c == 'j' ? 1 : -1);
      } else if (c == '\n' || c == '\r') {
        openSettingsPopup(settingsSectionAt(settingsSectionSelection));
      } else if (c == 'p' || c == 'P') {
        // Preserve the old single-key route to pairing: Keyboard is one jump.
        settingsSectionSelection = 4;
        openSettingsPopup(SettingsSection::Keyboard);
      } else if (c == 'q' || c == 27) {
        backClicked(nullptr);
      }
      return;
    }
    if (helpVisible) {
      // q mirrors the browser's "back" key; ? toggles, like Esc.
      if (c == 'q' || c == '?') helpBackClicked(nullptr);
      return;
    }
    if (previewVisible) {
      // Reading view: j/k scroll, any of q/p/? returns to editing.
      if (c == 'q' || c == 'p' || c == '?') previewBackClicked(nullptr);
      else if (c == 'j') previewScrollBy(34);
      else if (c == 'k') previewScrollBy(-34);
      return;
    }
    // File browser (fallthrough: it has no visibility flag of its own).
    if (browserFilterMode) {
      if (c == 27) {
        browserFilterMode = false;
        browserFilter.clear();
        browserSelection = 0;
        rebuildBrowserView();
        buildBrowserScreen();
      } else if (c == 8 || c == 127) {
        if (!browserFilter.empty()) {
          browserFilter.pop_back();
          browserSelection = 0;
          rebuildBrowserView();
          buildBrowserScreen();
        }
      } else if (c >= 32 && c < 127) {
        // Typed characters narrow the list; the header echoes the query.
        browserFilter.push_back(c);
        browserSelection = 0;
        rebuildBrowserView();
        buildBrowserScreen();
      }
      return;
    }
    if ((c == 'j' || c == 'k') && !browserMatches.empty()) {
      const int step = c == 'k' ? -1 : 1;
      browserSelection = (browserSelection + browserMatches.size() + step) %
                         browserMatches.size();
      if (!refreshBrowserSelection()) buildBrowserScreen();
    } else if (c == 'f' || c == '/') {
      browserFilterMode = true;
      browserFilter.clear();
      browserSelection = 0;
      buildBrowserScreen();
    } else if (c == 'n') {
      newDocument();
    } else if (c == 'q') {
      backClicked(nullptr);
    }
    return;
  }

  if (commandMode || searchMode) {
    if (c == '\n' || c == '\r') {
      if (searchMode) {
        // The first search wraps so the only occurrence, often the word just
        // typed above the caret, is still found. An empty search just closes
        // the prompt instead of printing the command list.
        if (!commandLine.empty()) {
          lastSearch = commandLine;
          applySearch(lastSearch, true, true);
        }
        commandLine.clear();
        searchMode = false;
      } else {
        runCommand();
      }
    } else if (c == 8 || c == 127) {
      if (!commandLine.empty()) commandLine.pop_back();
    } else if (commandLine.size() < 64 && c >= 32) commandLine += c;
    return;
  }

  if (insertMode) {
    if (c == 27) {
      leaveInsertMode();
      cursor = prevCursor(cursor);
    } else if (c == 8 || c == 127) {
      backspaceInEditor();
    } else if (c == '\n' || c == '\r') {
      insertNewlineInEditor();
    } else if (c == '\t') {
      insertTabInList();
    } else if (c >= 32) {
      insertTypedChar(c);
    }
    return;
  }

  if (pendingOperator) {
    const char op = pendingOperator;
    pendingOperator = 0;
    executeOperator(op, c);
    return;
  }
  switch (c) {
  case ':': commandMode = true; commandLine.clear(); break;
  case '/': searchMode = true; commandLine.clear(); break;
  case '?': showHelp(); break;
  case 'i': enterInsertMode(); break;
  case 'a':
    if (cursor < document.size()) cursor = nextCursor(cursor);
    enterInsertMode();
    break;
  case 'A': cursor = lineEnd(cursor); enterInsertMode(); break;
  case 'I': cursor = lineStart(cursor); enterInsertMode(); break;
  case 'o': {
    cursor = lineEnd(cursor);
    enterInsertMode();
    insertNewlineInEditor();
    break;
  }
  case 'O': {
    const size_t start = lineStart(cursor);
    const size_t end = lineEnd(start);
    size_t indent = 0;
    while (start + indent < end && document[start + indent] == ' ') ++indent;
    cursor = start;
    enterInsertMode();
    insertAtCursor('\n');
    if (cursor) --cursor;
    for (size_t i = 0; i < indent; ++i) insertAtCursor(' ');
    break;
  }
  case 'h': cursor = prevCursor(cursor); break;
  case 'l':
    if (cursor < document.size() && document[cursor] != '\n') {
      cursor = nextCursor(cursor);
    }
    break;
  case 'j': moveVertical(1); break;
  case 'k': moveVertical(-1); break;
  case 'w': moveWordForward(); break;
  case 'b': moveWordBackward(); break;
  case 'n': repeatSearch(true); break;
  case 'N': repeatSearch(false); break;
  case '0': cursor = lineStart(cursor); break;
  case '^': {
    cursor = lineStart(cursor);
    while (cursor < lineEnd(cursor) && document[cursor] == ' ') ++cursor;
    break;
  }
  case '$': {
    const size_t end = lineEnd(cursor);
    cursor = end > lineStart(cursor) ? prevCursor(end) : end;
    break;
  }
  case 'G': cursor = document.empty() ? 0 : prevCursor(document.size()); break;
  case 'x':
    if (cursor < document.size()) eraseRange(cursor, nextCursor(cursor));
    break;
  case 'd': case 'c': case 'y': pendingOperator = c; break;
  case 'p':
    if (!yankBuffer.empty()) {
      rememberUndo();
      undoGroupActive = true;
      if (cursor < document.size()) cursor = nextCursor(cursor);
      for (char ch : yankBuffer) insertAtCursor(ch);
      undoGroupActive = false;
    }
    break;
  case 'u': undoEdit(); break;
  // Text size from Normal mode: the keyboard twin of the header's A−/A+ pair.
  // '=' is accepted as well because the BLE report parser does not special-case
  // Shift, so a layout may deliver the unshifted key.
  case '+': case '=': stepEditorFontSize(1); break;
  case '-': stepEditorFontSize(-1); break;
  case 'v': visualMode = !visualMode; visualAnchor = cursor; break;
  // q quits the buffer back to the main menu, the way LazyVim's q closes the
  // last window and drops to the dashboard. quitEditor() keeps the dirty guard.
  case 'q': quitEditor(); break;
  default: break;
  }
  if (visualMode && (c == 'd' || c == 'x')) {
    const size_t first = std::min(cursor, visualAnchor);
    const size_t last = nextCursor(std::max(cursor, visualAnchor));
    eraseRange(first, last);
    visualMode = false;
  } else if (visualMode && c == 'y') {
    const size_t first = std::min(cursor, visualAnchor);
    const size_t last = nextCursor(std::max(cursor, visualAnchor));
    yankBuffer.assign(document.data() + first, last - first);
    visualMode = false;
  }
}

void handleSpecial(KeyKind kind) {
  if (lockActive) {
    lockHandleSpecial(kind);
    return;
  }
  if (kind == KEY_CTRL_L) {
    // Manual lock from anywhere; a no-op until a password has been set.
    // Unlocking returns to the editor when that is where the user was.
    lockReturnTarget = editorVisible ? LockReturn::Editor : LockReturn::Menu;
    lockDevice();
    return;
  }
  if (kind == KEY_CTRL_O && editorVisible) {
    showBrowser(true);
    return;
  }
  if (kind == KEY_CTRL_S) {
    // Only meaningful in the editor; on the menu/start screen it would save an
    // empty untitled buffer.
    if (editorVisible) saveDocument();
    return;
  }
  if (kind == KEY_CTRL_R) {
    if (editorVisible) redoEdit();
    return;
  }
  if (kind == KEY_CTRL_Z) {
    if (editorVisible && !commandMode && !searchMode) {
      editorZenMode = !editorZenMode;
      prefsDirty = true;
      savePrefs();
      buildEditorScreen();
    }
    return;
  }
  if (kind == KEY_CTRL_ENTER) {
    if (editorVisible) toggleTaskAtCursor();
    return;
  }
  if (!editorVisible) {
    if (historyVisible) {
      if (kind == KEY_ESCAPE) {
        historyBackClicked(nullptr);
      } else if (kind == KEY_UP || kind == KEY_DOWN) {
        if (!historyFiles.empty()) {
          int next = static_cast<int>(historySelection) +
                     (kind == KEY_DOWN ? 1 : -1);
          if (next < 0) next = static_cast<int>(historyFiles.size()) - 1;
          if (next >= static_cast<int>(historyFiles.size())) next = 0;
          historySelection = static_cast<size_t>(next);
          buildHistoryScreen();
        }
      } else if (kind == KEY_ENTER) {
        restoreHistory(historySelection);
      }
      return;
    }
    if (wifiScreenVisible) {
      if (wifiEditingField != WifiField::None) {
        if (kind == KEY_BACKSPACE || kind == KEY_DELETE) {
          wifiFieldBackspace();
          refreshWifiFieldText();
        } else if (kind == KEY_ENTER || kind == KEY_ESCAPE) {
          wifiEditingField = WifiField::None;
          saveWifiPrefs();
          buildWifiScreen();
        }
      } else if (kind == KEY_ESCAPE) {
        backClicked(nullptr);
      } else if (kind == KEY_ENTER || kind == KEY_TAB) {
        if (transferEnabled) stopTransfer();
        else startTransfer();
        buildWifiScreen();
      }
      return;
    }
    if (typingVisible) {
      if (kind == KEY_ESCAPE) typingBackClicked(nullptr);
      else if (kind == KEY_TAB) startTypingRun();
      else if (kind == KEY_BACKSPACE) typingBackspace();
      else if (kind == KEY_ENTER && !typingRunActive) startTypingRun();
      else if (kind == KEY_CTRL_O) showTypingSettings();
      return;
    }
    if (typingSettingsVisible) {
      if (kind == KEY_ESCAPE) typingSettingsBackClicked(nullptr);
      return;
    }
    if (pairingVisible) {
      if (kind == KEY_ESCAPE) {
        backFromPairing(nullptr);
      } else if (kind == KEY_UP || kind == KEY_DOWN) {
        const size_t count = keyboardCandidateCount();
        if (count) {
          if (kind == KEY_DOWN) pairSelection = (pairSelection + 1) % count;
          else pairSelection = (pairSelection + count - 1) % count;
          buildPairingScreen();
        }
      } else if (kind == KEY_ENTER && keyboardCandidateCount()) {
        keyboardConnect(pairSelection);
      }
      return;
    }
    if (menuVisible) {
      // Root screen: Esc has nowhere to go.
      if (kind == KEY_UP || kind == KEY_DOWN) {
        // Up/down step one grid row; +2 and -2 coincide in a 2x2 grid.
        menuSelection = (menuSelection + 2) % 4;
        refreshMenuSelection();
      } else if (kind == KEY_LEFT || kind == KEY_RIGHT) {
        menuSelection =
            (menuSelection + (kind == KEY_RIGHT ? 1 : 3)) % 4;
        refreshMenuSelection();
      } else if (kind == KEY_ENTER) runMenuEntry(menuSelection);
      return;
    }
    if (startVisible) {
      if (kind == KEY_ESCAPE) showMainMenu();
      else if (kind == KEY_UP || kind == KEY_DOWN)
        moveRecentSelection(kind == KEY_DOWN ? 1 : -1);
      else if (kind == KEY_ENTER) openRecent(recentSelection);
      return;
    }
    if (settingsVisible) {
      if (settingsPopup != SettingsSection::None) {
        if (kind == KEY_ESCAPE) {
          closeSettingsPopup();
        } else if (kind == KEY_UP || kind == KEY_DOWN) {
          settingsMovePopupSelection(kind == KEY_DOWN ? 1 : -1);
        } else if (kind == KEY_ENTER) {
          settingsPopupActivateRow(settingsPopupSelection);
        } else if (kind == KEY_LEFT || kind == KEY_RIGHT) {
          settingsPopupAdjustRow(kind == KEY_RIGHT ? 1 : -1);
        }
        return;
      }
      if (kind == KEY_ESCAPE) {
        backClicked(nullptr);
      } else if (kind == KEY_UP || kind == KEY_DOWN) {
        settingsMoveSection(kind == KEY_DOWN ? 1 : -1);
      } else if (kind == KEY_ENTER) {
        openSettingsPopup(settingsSectionAt(settingsSectionSelection));
      } else if (kind == KEY_LEFT || kind == KEY_RIGHT) {
        // Keep the old quick brightness adjustment from the section list.
        int32_t next = brightnessValue + (kind == KEY_RIGHT ? 16 : -16);
        if (next < BRIGHTNESS_MIN) next = BRIGHTNESS_MIN;
        if (next > BRIGHTNESS_MAX) next = BRIGHTNESS_MAX;
        setBrightnessValue(static_cast<uint8_t>(next));
      }
      return;
    }
    if (helpVisible) {
      if (kind == KEY_ESCAPE) helpBackClicked(nullptr);
      return;
    }
    if (previewVisible) {
      if (kind == KEY_ESCAPE) previewBackClicked(nullptr);
      else if (kind == KEY_UP) previewScrollBy(-34);
      else if (kind == KEY_DOWN) previewScrollBy(34);
      else if (kind == KEY_PAGE_UP) previewScrollBy(-PREVIEW_VIEW_H);
      else if (kind == KEY_PAGE_DOWN) previewScrollBy(PREVIEW_VIEW_H);
      else if (kind == KEY_HOME) previewScrollBy(-(1 << 30));
      else if (kind == KEY_END) previewScrollBy(1 << 30);
      return;
    }
    if (browserFilterMode) {
      if (kind == KEY_ESCAPE) {
        browserFilterMode = false;
        browserFilter.clear();
        browserSelection = 0;
        rebuildBrowserView();
        buildBrowserScreen();
      } else if (kind == KEY_BACKSPACE || kind == KEY_DELETE) {
        if (!browserFilter.empty()) {
          browserFilter.pop_back();
          browserSelection = 0;
          rebuildBrowserView();
          buildBrowserScreen();
        }
      } else if (kind == KEY_ENTER && browserSelection < browserMatches.size()) {
        loadDocument(String("/WriterInAWell/") + browserFileName(browserSelection));
        buildEditorScreen();
      } else if (kind == KEY_UP || kind == KEY_DOWN) {
        const int step = kind == KEY_UP ? -1 : 1;
        if (!browserMatches.empty()) {
          browserSelection = (browserSelection + browserMatches.size() + step) %
                             browserMatches.size();
          if (!refreshBrowserSelection()) buildBrowserScreen();
        }
      }
      return;
    }
    if (kind == KEY_ESCAPE) {
      backClicked(nullptr);
    } else if (kind == KEY_UP || kind == KEY_DOWN) {
      const int step = kind == KEY_UP ? -1 : 1;
      if (!browserMatches.empty()) {
        browserSelection = (browserSelection + browserMatches.size() + step) %
                           browserMatches.size();
        if (!refreshBrowserSelection()) buildBrowserScreen();
      }
    } else if (kind == KEY_ENTER && browserSelection < browserMatches.size()) {
      loadDocument(String("/WriterInAWell/") + browserFileName(browserSelection));
      buildEditorScreen();
    }
    return;
  }
  if (kind == KEY_ESCAPE) {
    if (editorZenMode && !insertMode && !commandMode && !searchMode &&
        !visualMode && !pendingOperator) {
      editorZenMode = false;
      prefsDirty = true;
      savePrefs();
      buildEditorScreen();
      return;
    }
    if (insertMode) {
      leaveInsertMode();
      cursor = prevCursor(cursor);
    } else if (commandMode || searchMode) {
      commandMode = searchMode = false;
      commandLine.clear();
    } else if (visualMode) visualMode = false;
    else if (pendingOperator) pendingOperator = 0;
  } else if (commandMode || searchMode) {
    if (kind == KEY_ENTER) handleText('\n');
    else if (kind == KEY_BACKSPACE) handleText(8);
  } else if (insertMode) {
    switch (kind) {
    case KEY_BACKSPACE: backspaceInEditor(); break;
    case KEY_DELETE:
      if (cursor < document.size()) eraseRange(cursor, nextCursor(cursor));
      break;
    case KEY_ENTER: insertNewlineInEditor(); break;
    case KEY_TAB: insertTabInList(); break;
    case KEY_SHIFT_TAB: outdentListLine(); break;
    case KEY_LEFT: cursor = prevCursor(cursor); break;
    case KEY_RIGHT:
      if (cursor < document.size()) cursor = nextCursor(cursor);
      break;
    case KEY_UP: moveVertical(-1); break;
    case KEY_DOWN: moveVertical(1); break;
    default: break;
    }
  } else {
    switch (kind) {
    case KEY_LEFT: cursor = prevCursor(cursor); break;
    case KEY_RIGHT:
      if (cursor < document.size()) cursor = nextCursor(cursor);
      break;
    case KEY_UP: moveVertical(-1); break;
    case KEY_DOWN: moveVertical(1); break;
    case KEY_HOME: cursor = lineStart(cursor); break;
    case KEY_END: {
      const size_t end = lineEnd(cursor);
      cursor = end > lineStart(cursor) ? prevCursor(end) : end;
      break;
    }
    case KEY_PAGE_UP: for (int i = 0; i < editorVisibleLines; ++i) moveVertical(-1); break;
    case KEY_PAGE_DOWN: for (int i = 0; i < editorVisibleLines; ++i) moveVertical(1); break;
    default: break;
    }
  }
}

void processKeyboard() {
  // Remember the peer the first time a keyboard connects, so the next boot
  // can auto-connect without making the user pair again.
  const bool nowConnected = keyboardIsConnected();
  if (nowConnected && !keyboardWasConnected) {
    const char *addr = keyboardConnectedAddress();
    if (addr && addr[0] && strcmp(addr, bootKeyboardAddress) != 0) {
      snprintf(bootKeyboardAddress, sizeof(bootKeyboardAddress), "%s", addr);
      if (devicePrefs.begin("wiaw", false)) {
        devicePrefs.putString("kbdaddr", bootKeyboardAddress);
        devicePrefs.end();
      }
    }
  }
  keyboardWasConnected = nowConnected;

  KeyEvent event;
  while (keyboardNext(event)) {
    lastInputMs = millis();
    if (screenOff) {
      display.setBrightness(brightnessValue);
      screenOff = false;
    }
    if (backlightDimmed) {
      display.setBrightness(brightnessValue);
      backlightDimmed = false;
    }
    if (event.kind == KEY_TEXT) handleText(event.text);
    else handleSpecial(event.kind);
    uiNeedsRender = true;
  }
}

void updateStatusBar() {
  // Zen hides the chrome; the widgets may also be stale after a rebuild.
  if (!editorVisible || editorZenMode) return;
  // keyboardStatus() returns a static string; no Arduino String copies here —
  // this runs on every keystroke.
  const bool connected = strncmp(keyboardStatus(), "Connected:", 10) == 0;
  // The Bluetooth shortcut's glyph never changes; only its tint tracks the
  // keyboard state (set text every second would just churn the label heap).
  lv_obj_set_style_text_color(radioLabel,
      connected ? lv_color_hex(0x96A57E) : lv_color_hex(0x89B4FA), 0);
  const char *modeText;
  uint32_t modeBg;
  if (insertMode) { modeText = "INSERT"; modeBg = 0x89B4FA; }
  else if (visualMode) { modeText = "VISUAL"; modeBg = 0xCBA6F7; }
  else if (commandMode || searchMode) {
    modeText = commandMode ? "COMMAND" : "SEARCH";
    modeBg = 0xF9E2AF;
  }
  else { modeText = "NORMAL"; modeBg = 0x96A57E; }
  labelSetTextIfChanged(modeLabel, modeText);
  lv_obj_set_style_bg_color(modeLabel, lv_color_hex(modeBg), 0);
  // Bottom hint line: keycap chips "i input  v visual  : commands  ? help"
  // in Normal — with the highlighted ":menu" and ":settings" chips
  // right-aligned in the same strip — the "esc normal" escape hint in
  // Insert/Visual, or the live ":" command line / "find: " search while one
  // is being typed (the chips hide for it).
  const bool commandHint = commandMode || searchMode;
  const bool exitHint = !commandHint && (insertMode || visualMode);
  const bool switchHint = !commandHint && !exitHint;
  if (commandHint) {
    // commandLine is capped at 64 chars by the input handler; "find: " adds 6,
    // so 80 bytes stays safe.
    char hint[80];
    if (commandMode) {
      snprintf(hint, sizeof(hint), ":%s", commandLine.c_str());
    } else {
      snprintf(hint, sizeof(hint), "find: %s", commandLine.c_str());
    }
    labelSetTextIfChanged(hintLabel, hint);
    lv_obj_set_style_text_color(hintLabel, lv_color_hex(0xF9E2AF), 0);
  }
  setWidgetHidden(hintLabel, !commandHint);
  setWidgetHidden(hintKeyI, !switchHint);
  setWidgetHidden(hintWordInput, !switchHint);
  setWidgetHidden(hintKeyV, !switchHint);
  setWidgetHidden(hintWordVisual, !switchHint);
  setWidgetHidden(hintKeyColon, !switchHint);
  setWidgetHidden(hintWordCommands, !switchHint);
  setWidgetHidden(hintKeyQuestion, !switchHint);
  setWidgetHidden(hintWordHelp, !switchHint);
  setWidgetHidden(hintKeyMenu, !switchHint);
  setWidgetHidden(hintKeySettings, !switchHint);
  setWidgetHidden(hintKeyEsc, !exitHint);
  setWidgetHidden(hintWordNormal, !exitHint);
  const char *status = transferEnabled ? "WiFi share  192.168.4.1"
                                       : statusMessage.c_str();
  labelSetTextIfChanged(statusLabel, status);
  if (transferEnabled) {
    labelSetTextIfChanged(footerLabel, wifiApPassword);
  } else if (millis() - lastTouchMs < 1800) {
    char touchText[20];
    snprintf(touchText, sizeof(touchText), "T%03u,%03u",
             unsigned(lastTouchX), unsigned(lastTouchY));
    labelSetTextIfChanged(footerLabel, touchText);
    showingTouchCoords = true;
  } else if (editorWordCount) {
    // Word count is opt-in. Recompute at most twice a second so typing in a
    // 512 KiB draft never pays a full scan per keystroke.
    const uint32_t now = millis();
    if (!documentWordCacheValid || now - documentWordCacheMs > 500) {
      documentWordCache = countDocumentWords();
      documentWordCacheMs = now;
      documentWordCacheValid = true;
    }
    const size_t gained = documentWordCache > sessionWordBase
                              ? documentWordCache - sessionWordBase
                              : 0;
    char stat[32];
    if (gained) {
      snprintf(stat, sizeof(stat), "%u w +%u",
               unsigned(documentWordCache), unsigned(gained));
    } else {
      snprintf(stat, sizeof(stat), "%u w", unsigned(documentWordCache));
    }
    labelSetTextIfChanged(footerLabel, stat);
    showingTouchCoords = false;
  } else {
    const size_t line = lineNumber(cursor) + 1;
    const size_t col = columnFor(cursor) + 1;
    char position[20];
    snprintf(position, sizeof(position), "%03u:%02u", unsigned(line), unsigned(col));
    labelSetTextIfChanged(footerLabel, position);
    showingTouchCoords = false;
  }
#ifdef WIAW_BATTERY_SENSE
  // updateStatusBar() repaints ~1 Hz; refreshBattery() throttles the ADC
  // burst, so most passes only restamp the cached percent.
  refreshBattery();
#endif
  setPowerLabelText();
  if (settingsRadioLabel) {
    labelSetTextIfChanged(settingsRadioLabel, keyboardShortStatus());
    lv_obj_set_style_text_color(settingsRadioLabel,
        connected ? lv_color_hex(0x96A57E) : lv_color_hex(0xBAC2DE), 0);
  }
}

void recoverSaveBackups() {
  File dir = SD.open("/WriterInAWell");
  if (!dir || !dir.isDirectory()) {
    if (dir) dir.close();
    return;
  }
  for (File entry = dir.openNextFile(); entry; entry = dir.openNextFile()) {
    if (!entry.isDirectory()) {
      String path = entry.name();
      const int slash = path.lastIndexOf('/');
      if (slash >= 0) path = path.substring(slash + 1);
      if (path.endsWith(".bak")) {
        const String originalName = path.substring(0, path.length() - 4);
        if (isEditableFileName(originalName)) {
          const String backupPath = String("/WriterInAWell/") + path;
          const String originalPath = String("/WriterInAWell/") + originalName;
          if (!SD.exists(originalPath.c_str())) {
            SD.rename(backupPath.c_str(), originalPath.c_str());
          } else {
            SD.remove(backupPath.c_str());
          }
        }
      }
    }
    entry.close();
  }
  dir.close();
}

// --- Display lock -----------------------------------------------------------
// A minimalist login screen that owns every screen flag while up. It is built
// from labels and plain buttons (no textarea, no on-screen LVGL keyboard): the
// password is entered either on the paired BLE keyboard or on the 4x10 touch
// keypad below, and both paths call the same append/backspace/submit helpers.
//
// The hash is FNV-1a 64-bit over the password with a per-password salt. It is
// deliberately not advertised as encryption: the device has no secure element
// and the SD card stays readable, so this keeps a casual user out of the app.
void lockCompute(const char *password, uint32_t salt, uint32_t &lo,
                 uint32_t &hi) {
  uint64_t hash =
      1469598103934665603ULL ^ (static_cast<uint64_t>(salt) << 32);
  for (const unsigned char *p =
           reinterpret_cast<const unsigned char *>(password);
       *p; ++p) {
    hash ^= *p;
    hash *= 1099511628211ULL;
  }
  hash ^= 0x9E3779B97F4A7C15ULL;
  lo = static_cast<uint32_t>(hash);
  hi = static_cast<uint32_t>(hash >> 32);
}

bool lockMatches(const char *password) {
  if (!lockEnabled) return false;
  uint32_t lo = 0;
  uint32_t hi = 0;
  lockCompute(password, lockSalt, lo, hi);
  return lo == lockHashLo && hi == lockHashHi;
}

enum class LockKeyAction : uint8_t { Char, Shift, Symbols, Backspace, Enter };

struct LockKeyDef {
  LockKeyAction action;
  char lower;
  char upper;
  char symbol;
};

// 4 rows x 10 keys. Row 1 is always digits; rows 2-4 are QWERTY in letter mode
// and punctuation in SYM mode. SHIFT is one-shot for letters.
constexpr LockKeyDef LOCK_KEY_LAYOUT[LOCK_KEY_COUNT] = {
    {LockKeyAction::Char, '1', '1', '!'},
    {LockKeyAction::Char, '2', '2', '@'},
    {LockKeyAction::Char, '3', '3', '#'},
    {LockKeyAction::Char, '4', '4', '$'},
    {LockKeyAction::Char, '5', '5', '%'},
    {LockKeyAction::Char, '6', '6', '^'},
    {LockKeyAction::Char, '7', '7', '&'},
    {LockKeyAction::Char, '8', '8', '*'},
    {LockKeyAction::Char, '9', '9', '('},
    {LockKeyAction::Char, '0', '0', ')'},
    {LockKeyAction::Char, 'q', 'Q', '['},
    {LockKeyAction::Char, 'w', 'W', ']'},
    {LockKeyAction::Char, 'e', 'E', '{'},
    {LockKeyAction::Char, 'r', 'R', '}'},
    {LockKeyAction::Char, 't', 'T', '+'},
    {LockKeyAction::Char, 'y', 'Y', '-'},
    {LockKeyAction::Char, 'u', 'U', '_'},
    {LockKeyAction::Char, 'i', 'I', '='},
    {LockKeyAction::Char, 'o', 'O', '/'},
    {LockKeyAction::Char, 'p', 'P', '?'},
    {LockKeyAction::Char, 'a', 'A', ';'},
    {LockKeyAction::Char, 's', 'S', ':'},
    {LockKeyAction::Char, 'd', 'D', '\''},
    {LockKeyAction::Char, 'f', 'F', '"'},
    {LockKeyAction::Char, 'g', 'G', ','},
    {LockKeyAction::Char, 'h', 'H', '.'},
    {LockKeyAction::Char, 'j', 'J', '<'},
    {LockKeyAction::Char, 'k', 'K', '>'},
    {LockKeyAction::Char, 'l', 'L', '\\'},
    {LockKeyAction::Backspace, 0, 0, 0},
    {LockKeyAction::Shift, 0, 0, 0},
    {LockKeyAction::Char, 'z', 'Z', '|'},
    {LockKeyAction::Char, 'x', 'X', '`'},
    {LockKeyAction::Char, 'c', 'C', '~'},
    {LockKeyAction::Char, 'v', 'V', '%'},
    {LockKeyAction::Char, 'b', 'B', '^'},
    {LockKeyAction::Char, 'n', 'N', '&'},
    {LockKeyAction::Char, 'm', 'M', '*'},
    {LockKeyAction::Symbols, 0, 0, 0},
    {LockKeyAction::Enter, 0, 0, 0},
};

char lockKeyChar(size_t index) {
  const LockKeyDef &def = LOCK_KEY_LAYOUT[index];
  if (def.action != LockKeyAction::Char) return 0;
  if (lockSymbols) return def.symbol;
  if (def.lower >= 'a' && def.lower <= 'z') {
    return lockShift ? def.upper : def.lower;
  }
  return def.lower;
}

void lockSetMessage(const char *text, bool error) {
  if (!lockMessageLabel) return;
  lv_label_set_text(lockMessageLabel, text);
  lv_obj_set_style_text_color(lockMessageLabel,
      lv_color_hex(error ? 0xF38BA8 : 0x7F849C), 0);
}

void lockRefreshField() {
  if (!lockFieldLabel) return;
  // The mask is rebuilt in a fixed buffer: no String and no heap on a key.
  char masked[LOCK_MAX_PASSWORD + 1];
  const size_t len = strlen(lockEntry);
  for (size_t i = 0; i < len; ++i) masked[i] = '*';
  masked[len] = '\0';
  lv_label_set_text(lockFieldLabel, masked);
}

void lockRefreshKeypad() {
  for (size_t i = 0; i < LOCK_KEY_COUNT; ++i) {
    lv_obj_t *label = lockKeyLabels[i];
    if (!label) continue;
    const LockKeyDef &def = LOCK_KEY_LAYOUT[i];
    char text[8] = {};
    bool special = false;
    switch (def.action) {
    case LockKeyAction::Char:
      text[0] = lockKeyChar(i);
      text[1] = '\0';
      break;
    case LockKeyAction::Shift: {
      snprintf(text, sizeof(text), "SHIFT");
      special = true;
      break;
    }
    case LockKeyAction::Symbols: {
      snprintf(text, sizeof(text), "SYM");
      special = true;
      break;
    }
    case LockKeyAction::Backspace: {
      snprintf(text, sizeof(text), "DEL");
      special = true;
      break;
    }
    case LockKeyAction::Enter: {
      snprintf(text, sizeof(text), "ENTER");
      special = true;
      break;
    }
    }
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label,
        special ? &elms_bold_12 : &terminess_16, 0);
    lv_obj_set_style_text_color(label,
        lv_color_hex(special ? 0x89B4FA : 0xCDD6F4), 0);
    if (lockKeys[i]) {
      const bool active =
          (def.action == LockKeyAction::Shift && lockShift) ||
          (def.action == LockKeyAction::Symbols && lockSymbols);
      lv_obj_set_style_bg_color(lockKeys[i],
          lv_color_hex(active ? 0x313244 : 0x1E1E2E), 0);
      lv_obj_set_style_border_color(lockKeys[i],
          lv_color_hex(active ? 0x96A57E : 0x45475A), 0);
    }
  }
}

void lockAppendChar(char c) {
  if (c < 0x20 || c >= 0x7F) return;
  const size_t len = strlen(lockEntry);
  if (len >= LOCK_MAX_PASSWORD) {
    lockSetMessage("Password is too long", true);
    return;
  }
  lockEntry[len] = c;
  lockEntry[len + 1] = '\0';
  lockRefreshField();
  lockSetMessage("", false);
}

void lockBackspace() {
  const size_t len = strlen(lockEntry);
  if (len) lockEntry[len - 1] = '\0';
  lockRefreshField();
  lockSetMessage("", false);
}

void lockCancel() {
  lockEntry[0] = '\0';
  lockPending[0] = '\0';
  lockVerifyForRemove = false;
  if (lockReturnToSettings) {
    lockActive = false;
    lockReturnToSettings = false;
    statusMessage = "Password unchanged";
    buildSettingsScreen();
    return;
  }
  lockRefreshField();
  lockSetMessage("", false);
}

void lockSubmit() {
  if (lockMode == LockMode::Unlock) {
    if (lockMatches(lockEntry)) {
      lockActive = false;
      lockEntry[0] = '\0';
      lockPending[0] = '\0';
      statusMessage.clear();
      if (lockReturnTarget == LockReturn::Editor) buildEditorScreen();
      else buildMainMenuScreen();
      lockReturnTarget = LockReturn::Menu;
    } else {
      lockEntry[0] = '\0';
      lockRefreshField();
      lockSetMessage("Incorrect password", true);
    }
    return;
  }
  if (lockMode == LockMode::VerifyOld) {
    if (!lockMatches(lockEntry)) {
      lockEntry[0] = '\0';
      lockRefreshField();
      lockSetMessage("Incorrect password", true);
      return;
    }
    lockEntry[0] = '\0';
    if (lockVerifyForRemove) {
      // "lockon" false is the authority; the stale hash is harmless and is
      // overwritten by the next Set password.
      lockEnabled = false;
      lockSalt = lockHashLo = lockHashHi = 0;
      prefsDirty = true;
      savePrefs();
      lockActive = false;
      lockReturnToSettings = false;
      lockVerifyForRemove = false;
      statusMessage = "Password removed";
      buildSettingsScreen();
    } else {
      lockMode = LockMode::EnterNew;
      buildLockScreen();
    }
    return;
  }
  if (lockMode == LockMode::EnterNew) {
    if (strlen(lockEntry) < LOCK_MIN_PASSWORD) {
      lockSetMessage("Use at least 4 characters", true);
      return;
    }
    memcpy(lockPending, lockEntry, strlen(lockEntry) + 1);
    memset(lockEntry, 0, sizeof(lockEntry));
    lockMode = LockMode::ConfirmNew;
    buildLockScreen();
    return;
  }
  // ConfirmNew
  if (strcmp(lockEntry, lockPending) != 0) {
    memset(lockEntry, 0, sizeof(lockEntry));
    memset(lockPending, 0, sizeof(lockPending));
    lockMode = LockMode::EnterNew;
    buildLockScreen();
    lockSetMessage("Passwords differ", true);
    return;
  }
  lockSalt = esp_random();
  lockCompute(lockEntry, lockSalt, lockHashLo, lockHashHi);
  lockEnabled = true;
  prefsDirty = true;
  savePrefs();
  memset(lockEntry, 0, sizeof(lockEntry));
  memset(lockPending, 0, sizeof(lockPending));
  lockActive = false;
  lockReturnToSettings = false;
  statusMessage = "Password set";
  buildSettingsScreen();
}

void lockKeyPressed(size_t index) {
  if (index >= LOCK_KEY_COUNT) return;
  const LockKeyDef &def = LOCK_KEY_LAYOUT[index];
  switch (def.action) {
  case LockKeyAction::Char: {
    const char c = lockKeyChar(index);
    lockAppendChar(c);
    // SHIFT behaves like a phone keyboard: one capital, then back to lower.
    if (lockShift && !lockSymbols) {
      lockShift = false;
      lockRefreshKeypad();
    }
    break;
  }
  case LockKeyAction::Shift:
    lockShift = !lockShift;
    lockRefreshKeypad();
    break;
  case LockKeyAction::Symbols:
    lockSymbols = !lockSymbols;
    lockRefreshKeypad();
    break;
  case LockKeyAction::Backspace:
    lockBackspace();
    break;
  case LockKeyAction::Enter:
    lockSubmit();
    break;
  }
}

void lockKeyClicked(lv_event_t *event) {
  const intptr_t index =
      reinterpret_cast<intptr_t>(lv_event_get_user_data(event));
  if (index < 0) return;
  lockKeyPressed(static_cast<size_t>(index));
}

void lockHandleText(char c) {
  if (c >= 0x20 && c < 0x7F) lockAppendChar(c);
}

void lockHandleSpecial(KeyKind kind) {
  switch (kind) {
  case KEY_BACKSPACE:
  case KEY_DELETE:
    lockBackspace();
    break;
  case KEY_ENTER:
    lockSubmit();
    break;
  case KEY_ESCAPE:
    if (lockMode == LockMode::Unlock) {
      lockEntry[0] = '\0';
      lockRefreshField();
      lockSetMessage("", false);
    } else {
      lockCancel();
    }
    break;
  case KEY_TAB:
    lockShift = !lockShift;
    lockRefreshKeypad();
    break;
  default:
    break;
  }
}

void buildLockScreen() {
  editorVisible = false;
  menuVisible = false;
  startVisible = settingsVisible = pairingVisible = false;
  helpVisible = typingVisible = typingSettingsVisible = previewVisible = false;
  wifiScreenVisible = historyVisible = false;
  lockActive = true;
  lv_obj_clean(lv_scr_act());
  lockFieldLabel = lockHintLabel = lockMessageLabel = nullptr;
  for (size_t i = 0; i < LOCK_KEY_COUNT; ++i) {
    lockKeys[i] = lockKeyLabels[i] = nullptr;
  }
  lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x11111B), 0);

  const char *title = "";
  const char *hint = "";
  switch (lockMode) {
  case LockMode::Unlock:
    title = "LOCKED";
    hint = "Type on the keyboard or tap the keys";
    break;
  case LockMode::VerifyOld:
    title = "CURRENT PASSWORD";
    hint = "Enter the current password";
    break;
  case LockMode::EnterNew:
    title = "NEW PASSWORD";
    hint = "At least 4 characters";
    break;
  case LockMode::ConfirmNew:
    title = "CONFIRM PASSWORD";
    hint = "Type the new password again";
    break;
  }

  // Padlock drawn from two LVGL objects: a rounded shackle with a transparent
  // fill, then the body on top. No Nerd Font glyph and no font regeneration.
  lv_obj_t *shackle = lv_obj_create(lv_scr_act());
  lv_obj_set_size(shackle, 30, 30);
  lv_obj_set_pos(shackle, (SCREEN_W - 30) / 2, 12);
  lv_obj_set_style_bg_opa(shackle, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_color(shackle, lv_color_hex(0x96A57E), 0);
  lv_obj_set_style_border_width(shackle, 2, 0);
  lv_obj_set_style_radius(shackle, 15, 0);
  lv_obj_clear_flag(shackle, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t *body = lv_obj_create(lv_scr_act());
  lv_obj_set_size(body, 44, 28);
  lv_obj_set_pos(body, (SCREEN_W - 44) / 2, 30);
  stylePanel(body, lv_color_hex(0x1E1E2E), lv_color_hex(0x96A57E));
  lv_obj_clear_flag(body, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *heading = lv_label_create(lv_scr_act());
  lv_label_set_text(heading, title);
  setTitleFont(heading);
  lv_obj_set_width(heading, SCREEN_W - 20);
  lv_obj_set_pos(heading, 10, 68);
  lv_obj_set_style_text_align(heading, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(heading, lv_color_hex(0x96A57E), 0);

  lockFieldLabel = lv_label_create(lv_scr_act());
  lv_label_set_text(lockFieldLabel, "");
  lv_obj_set_width(lockFieldLabel, 300);
  lv_obj_set_pos(lockFieldLabel, (SCREEN_W - 300) / 2, 96);
  lv_obj_set_style_text_align(lockFieldLabel, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(lockFieldLabel, &terminess_16, 0);
  lv_obj_set_style_text_color(lockFieldLabel, lv_color_hex(0xCDD6F4), 0);

  lv_obj_t *underline = lv_obj_create(lv_scr_act());
  lv_obj_set_size(underline, 200, 1);
  lv_obj_set_pos(underline, (SCREEN_W - 200) / 2, 120);
  lv_obj_set_style_bg_color(underline, lv_color_hex(0x45475A), 0);
  lv_obj_set_style_bg_opa(underline, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(underline, 0, 0);
  lv_obj_clear_flag(underline, LV_OBJ_FLAG_SCROLLABLE);

  lockHintLabel = lv_label_create(lv_scr_act());
  lv_label_set_text(lockHintLabel, hint);
  setUiFont(lockHintLabel);
  lv_obj_set_width(lockHintLabel, SCREEN_W - 20);
  lv_obj_set_pos(lockHintLabel, 10, 126);
  lv_obj_set_style_text_align(lockHintLabel, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(lockHintLabel, lv_color_hex(0x7F849C), 0);

  lockMessageLabel = lv_label_create(lv_scr_act());
  lv_label_set_text(lockMessageLabel, "");
  setUiFont(lockMessageLabel);
  lv_obj_set_width(lockMessageLabel, SCREEN_W - 20);
  lv_obj_set_pos(lockMessageLabel, 10, 140);
  lv_obj_set_style_text_align(lockMessageLabel, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(lockMessageLabel, lv_color_hex(0x7F849C), 0);

  // Keypad: 10 columns x 4 rows, 44x28 px keys. 458 px total, centered.
  constexpr int KEY_W = 44;
  constexpr int KEY_H = 28;
  constexpr int KEY_GAP_X = 2;
  constexpr int KEY_GAP_Y = 2;
  constexpr int KEY_X = (SCREEN_W - (10 * KEY_W + 9 * KEY_GAP_X)) / 2;
  constexpr int KEY_Y = 160;
  for (size_t i = 0; i < LOCK_KEY_COUNT; ++i) {
    const int col = static_cast<int>(i % 10);
    const int row = static_cast<int>(i / 10);
    lv_obj_t *button = lv_btn_create(lv_scr_act());
    lv_obj_set_size(button, KEY_W, KEY_H);
    lv_obj_set_pos(button, KEY_X + col * (KEY_W + KEY_GAP_X),
                   KEY_Y + row * (KEY_H + KEY_GAP_Y));
    stylePanel(button, lv_color_hex(0x1E1E2E), lv_color_hex(0x45475A));
    lv_obj_add_event_cb(button, lockKeyClicked, LV_EVENT_CLICKED,
                        reinterpret_cast<void *>(static_cast<intptr_t>(i)));
    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, "");
    lv_obj_center(label);
    lockKeys[i] = button;
    lockKeyLabels[i] = label;
  }

  lv_obj_t *footRow = makeHintRow();
  if (lockMode == LockMode::Unlock) {
    addHintPair(footRow, "type", "or tap keys");
    addHintPair(footRow, "enter", "unlock");
  } else {
    addHintPair(footRow, "esc", "cancel");
    addHintPair(footRow, "enter", "continue");
  }

  lockRefreshField();
  lockRefreshKeypad();
}

void lockDevice() {
  if (!lockEnabled) return;
  lockEntry[0] = '\0';
  lockPending[0] = '\0';
  lockMode = LockMode::Unlock;
  lockReturnToSettings = false;
  lockVerifyForRemove = false;
  lockShift = lockSymbols = false;
  buildLockScreen();
}

void setupSdCard() {
  SPI.begin(39, 38, 40, SD_CS);
  cardAvailable = SD.begin(SD_CS, SPI, 20000000);
  if (cardAvailable) {
    if (!SD.exists("/WriterInAWell")) SD.mkdir("/WriterInAWell");
    directoryReady = SD.exists("/WriterInAWell");
    if (directoryReady) recoverSaveBackups();
  }
}
} // namespace

void setup() {
  Serial.begin(115200);
  if (psramFound()) {
    psramAvailable = true;
    documentLimit = MAX_DOCUMENT_PSRAM;
    Serial.println("PSRAM available: document limit 512 KiB");
  } else {
    psramAvailable = false;
    documentLimit = MAX_DOCUMENT_FALLBACK;
    Serial.println("PSRAM unavailable: document limit 32 KiB");
  }
  // Rendered-preview block table: fixed boot-time PSRAM allocation so :p
  // never allocates from a key handler. 16384 blocks x 8 B = 128 KiB.
  if (psramAvailable) {
    previewBlocks = static_cast<PreviewBlock *>(heap_caps_malloc(
        PREVIEW_MAX_BLOCKS * sizeof(PreviewBlock),
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  }
  document.reserve(documentLimit);
  undoBuffer.reserve(documentLimit);
  redoBuffer.reserve(documentLimit);
  display.init();
  display.setRotation(1);
  loadPrefs();
  display.setBrightness(brightnessValue);

  lv_init();
  // The saved editor size decides the line box, band count and glyph
  // advance; the first screen build must not fall back to 14 px defaults.
  refreshEditorMetrics();
  bool bufferInPsram = true;
  lvBuffer = static_cast<lv_color_t *>(
      heap_caps_malloc(SCREEN_W * 32 * sizeof(lv_color_t),
                       MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (!lvBuffer) {
    bufferInPsram = false;
    lvBuffer = static_cast<lv_color_t *>(
        heap_caps_malloc(SCREEN_W * 16 * sizeof(lv_color_t), MALLOC_CAP_8BIT));
  }
  if (!lvBuffer) {
    Serial.println("Not enough memory for the display buffer");
    while (true) delay(1000);
  }
  static lv_disp_drv_t displayDriver;
  lv_disp_draw_buf_init(&drawBuffer, lvBuffer, nullptr,
                        SCREEN_W * (bufferInPsram ? 32 : 16));
  lv_disp_drv_init(&displayDriver);
  displayDriver.hor_res = SCREEN_W;
  displayDriver.ver_res = SCREEN_H;
  displayDriver.flush_cb = displayFlush;
  displayDriver.draw_buf = &drawBuffer;
  lv_disp_drv_register(&displayDriver);
  static lv_indev_drv_t inputDriver;
  lv_indev_drv_init(&inputDriver);
  inputDriver.type = LV_INDEV_TYPE_POINTER;
  inputDriver.read_cb = touchRead;
  lv_indev_drv_register(&inputDriver);

  setupSdCard();
  keyboardBegin(bootKeyboardAddress);
#ifdef WIAW_BATTERY_SENSE
  // Prime the cache so the header shows a real value on the first frame.
  // Relies on the core's default ADC attenuation (widest range, ~3.1 V);
  // the divider never puts more than 2.1 V on the pin.
  batteryMillivolts = readBatteryMillivolts();
  batteryPercent = batteryPercentFor(batteryMillivolts);
#endif
  lastInputMs = lastEditMs = millis();
  // A stored password makes the login screen the first thing on screen. With
  // no password the old resume/menu behaviour is untouched, so upgrading a
  // device never locks its owner out.
  if (lockEnabled) {
    lockReturnTarget = LockReturn::Menu;
    lockDevice();
  } else {
    // Boot into the main menu by default. With "Start in editor" enabled,
    // resume the newest recent file straight into the page instead.
    bool resumed = false;
    if (bootIntoEditor && !recentFiles.empty()) {
      loadDocument(String(recentFiles.front().c_str()));
      if (editorVisible) {
        buildEditorScreen();
        resumed = true;
      }
    }
    if (!resumed) {
      buildMainMenuScreen();
      if (cardAvailable) statusMessage = "SD ready";
      else statusMessage = "No SD card — draft is temporary";
    }
  }
  uiNeedsRender = true;
}

void loop() {
  // Two-finger taps set these from touchRead(); rebuilding here keeps
  // lv_obj_clean out of the input callback.
  if (zenEntryRequested) {
    zenEntryRequested = false;
    if (editorVisible && !editorZenMode) {
      editorZenMode = true;
      prefsDirty = true;
      savePrefs();
      buildEditorScreen();
    }
  }
  if (zenExitRequested) {
    zenExitRequested = false;
    if (editorZenMode) {
      editorZenMode = false;
      prefsDirty = true;
      savePrefs();
      buildEditorScreen();
    }
  }
  processKeyboard();
  if (transferEnabled) transferServer.handleClient();
  if (pairingVisible && keyboardCandidateCount() != pairListRenderedCount) {
    buildPairingScreen();
  }
  if (editorVisible && showingTouchCoords &&
      millis() - lastTouchMs >= 1800) {
    uiNeedsRender = true;
  }
  if (zenHintLabel && zenHintHideMs && millis() >= zenHintHideMs) {
    lv_obj_add_flag(zenHintLabel, LV_OBJ_FLAG_HIDDEN);
    zenHintHideMs = 0;
  }
  if ((editorVisible || menuVisible || settingsVisible || pairingVisible) &&
      millis() - lastRadioUpdateMs >= 1000) {
    lastRadioUpdateMs = millis();
    if (editorVisible && !editorZenMode) {
      // Only the Bluetooth tint is time-based. Forcing a full band render
      // here used to re-lex and re-set every visible line once a second while
      // the document sat idle; the text cannot change without a keypress.
      if (radioLabel) {
        lv_obj_set_style_text_color(radioLabel,
            strncmp(keyboardStatus(), "Connected:", 10) == 0
                ? lv_color_hex(0x96A57E)
                : lv_color_hex(0x89B4FA), 0);
      }
#ifdef WIAW_BATTERY_SENSE
      setPowerLabelText();
#endif
    } else if (menuVisible && menuRadioLabel) {
      const String keyboard = keyboardStatus();
      lv_obj_set_style_text_color(menuRadioLabel,
          keyboard.startsWith("Connected:") ? lv_color_hex(0x96A57E)
                                            : lv_color_hex(0x89B4FA), 0);
    } else if (settingsRadioLabel) {
      labelSetTextIfChanged(settingsRadioLabel, keyboardShortStatus());
      lv_obj_set_style_text_color(settingsRadioLabel,
          keyboardIsConnected() ? lv_color_hex(0x96A57E)
                                : lv_color_hex(0xBAC2DE), 0);
    } else if (settingsVisible) {
      // The Keyboard section's summary is the only live value on the list.
      // Selection moves no longer rebuild the screen, so refresh it here.
      for (size_t i = 0; i < SETTINGS_SECTION_COUNT; ++i) {
        if (settingsSectionAt(i) == SettingsSection::Keyboard) {
          if (settingsSectionPaint[i].value) {
            labelSetTextIfChanged(settingsSectionPaint[i].value,
                                  keyboardShortStatus());
          }
          break;
        }
      }
    } else if (pairingStatusLabel) {
      const char *keyboard = keyboardStatus();
      labelSetTextIfChanged(pairingStatusLabel, keyboard);
      lv_obj_set_style_text_color(pairingStatusLabel,
          strncmp(keyboard, "Connected:", 10) == 0 ? lv_color_hex(0x96A57E)
                                                  : lv_color_hex(0xBAC2DE), 0);
    }
  }
  if (editorVisible && uiNeedsRender) {
    updateEditorLines();
    updateStatusBar();
    uiNeedsRender = false;
  }
  if (typingVisible) {
    // Keystrokes rebuild the paragraph; the timer only repaints the stats
    // line so wpm/elapsed move while typing pauses.
    if (uiNeedsRender) {
      updateTypingWords();
      updateTypingStats(false);
      uiNeedsRender = false;
    } else if (typingRunActive && typingStartMs &&
               millis() - typingLastTickMs >= TYPING_LIVE_TICK_MS) {
      typingLastTickMs = millis();
      updateTypingStats(false);
    }
  }
  // Blink the caret cursor independently of full-screen redraws: the cell
  // alternates between a solid block with the glyph reversed out and a 1 px
  // outline that lets the glyph show through.
  if (editorVisible && cursorBlock && millis() - lastCursorBlinkMs >= 480) {
    lastCursorBlinkMs = millis();
    setCursorPhase(!cursorSolidPhase);
  }
  if (editorAutoSave && dirty && cardAvailable && millis() - lastEditMs > 5000) {
    saveDocument();
  }
  updateWakeFade(millis());
  if (settingsDimIdle && !backlightDimmed && millis() - lastInputMs > 60000) {
    display.setBrightness(48);
    backlightDimmed = true;
  }
  // Screen-off timeout: backlight to zero after the chosen idle period. Any
  // key or tap restores it in processKeyboard()/touchRead().
  if (screenOffMinutes > 0 && !screenOff &&
      millis() - lastInputMs >
          static_cast<uint32_t>(screenOffMinutes) * 60000UL) {
    display.setBrightness(0);
    screenOff = true;
  }
  // Opt-in light sleep, only once the screen is off and idle is long. A 1 s
  // timer wake lets the loop poll BLE and touch; if a key or tap arrived it
  // has already reset lastInputMs and restored the backlight.
  const bool sleepIdle = lightSleepEnabled && screenOff &&
      millis() - lastInputMs > 15UL * 60UL * 1000UL;
  // The long idle is also the lock point: build the login screen once, then
  // keep sleeping (the backlight is already off, so nothing is visible until
  // the user wakes the device). Building it before the sleep call means the
  // wake tap lands on the login screen, not the page that was open.
  if (sleepIdle && lockEnabled && !lockActive) {
    lockReturnTarget = LockReturn::Menu;
    lockDevice();
  }
  if (sleepIdle) {
    esp_sleep_enable_timer_wakeup(1000000ULL);
    esp_light_sleep_start();
  }
  lv_timer_handler();
  delay(8);
}
