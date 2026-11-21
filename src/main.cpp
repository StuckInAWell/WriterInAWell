#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <lvgl.h>
#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <WiFi.h>
#include <WebServer.h>
#include <esp_heap_caps.h>
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
constexpr int VISIBLE_LINES = 13;
constexpr int LINE_HEIGHT = 17;

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
lv_obj_t *lineLabels[VISIBLE_LINES] = {};
lv_obj_t *modeLabel = nullptr;
lv_obj_t *statusLabel = nullptr;
lv_obj_t *footerLabel = nullptr;
lv_obj_t *hintLabel = nullptr;
lv_obj_t *documentLabel = nullptr;
lv_obj_t *pathLabel = nullptr;
lv_obj_t *radioLabel = nullptr;
lv_obj_t *powerLabel = nullptr;
lv_obj_t *settingsRadioLabel = nullptr;
lv_obj_t *brightnessStatusLabel = nullptr;
lv_obj_t *brightnessSlider = nullptr;
lv_obj_t *smoothButton = nullptr;
lv_obj_t *pairingStatusLabel = nullptr;
lv_obj_t *actionStatusLabel = nullptr;
lv_obj_t *browserPanel = nullptr;
lv_obj_t *cursorBlock = nullptr;
WebServer transferServer(80);
File uploadFile;

DocumentString document = "# Untitled\n\n";
std::string fileName = "Untitled.md";
std::string currentPath;
std::string commandLine;
DocumentString yankBuffer;
DocumentString undoBuffer;
std::string statusMessage = "Ready — press i to write";
size_t documentLimit = MAX_DOCUMENT_FALLBACK;
String wifiPassword;
String uploadDestination;
size_t cursor = 0;
size_t visualAnchor = 0;
size_t viewportStart = 0;
size_t browserSelection = 0;
size_t actionSelection = 0;
size_t pairSelection = 0;
size_t pairListRenderedCount = SIZE_MAX;
uint32_t lastEditMs = 0;
uint32_t lastInputMs = 0;
uint32_t lastRadioUpdateMs = 0;
uint32_t lastTouchMs = 0;
uint32_t lastCursorBlinkMs = 0;
uint16_t lastTouchX = 0;
uint16_t lastTouchY = 0;
bool cursorBlinkOn = true;
bool cardAvailable = false;
bool dirty = false;
bool insertMode = false;
bool visualMode = false;
bool commandMode = false;
bool searchMode = false;
bool backlightDimmed = false;
bool editorVisible = true;
bool pairingVisible = false;
bool settingsVisible = false;
bool actionsVisible = false;
bool pairingReturnsToSettings = false;
bool directoryReady = false;
bool transferEnabled = false;
bool uploadFailed = false;
bool uiNeedsRender = true;
bool undoGroupActive = false;
bool showingTouchCoords = false;
uint8_t brightnessValue = 170;
constexpr uint8_t BRIGHTNESS_MIN = 24;
constexpr uint8_t BRIGHTNESS_MAX = 255;
constexpr uint8_t BRIGHTNESS_PRESETS[] = {64, 112, 170, 230};
bool editorSmoothText = false;
Preferences devicePrefs;
bool prefsDirty = false;
char pendingOperator = 0;
std::vector<String> markdownFiles;

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

void touchRead(lv_indev_drv_t *, lv_indev_data_t *data) {
  uint16_t x = 0, y = 0;
  if (display.getTouch(&x, &y)) {
    int32_t cx = static_cast<int32_t>(x) - TOUCH_OFFSET_X;
    int32_t cy = static_cast<int32_t>(y) - TOUCH_OFFSET_Y;
    if (cx < 0) cx = 0;
    if (cy < 0) cy = 0;
    lastTouchX = static_cast<uint16_t>(cx);
    lastTouchY = static_cast<uint16_t>(cy);
    lastTouchMs = millis();
    // A tap anywhere on the screen counts as input: wake the dimmed
    // backlight and reset the idle timer, exactly like a keypress does.
    lastInputMs = millis();
    if (backlightDimmed) {
      display.setBrightness(brightnessValue);
      backlightDimmed = false;
    }
    uiNeedsRender = true;
    data->state = LV_INDEV_STATE_PR;
    data->point.x = static_cast<lv_coord_t>(cx);
    data->point.y = static_cast<lv_coord_t>(cy);
  } else {
    data->state = LV_INDEV_STATE_REL;
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

void setUiFont(lv_obj_t *obj) {
  lv_obj_set_style_text_font(obj, &elms_bold_12, 0);
}

void setTitleFont(lv_obj_t *obj) {
  lv_obj_set_style_text_font(obj, &elms_bold_16, 0);
}

// The editor text font: anti-aliased Space Mono, or the crisp 1-bpp variant
// when smoothing is off (thin stems stay fully black).
const lv_font_t *editorFont() {
  return editorSmoothText ? &space_mono_14 : &space_mono_14_cr;
}

// Monospace glyph advance width, measured once at startup for cursor math.
uint16_t editorGlyphWidth = 8;

size_t lineStart(size_t offset) {
  size_t start = document.rfind('\n', offset == 0 ? 0 : offset - 1);
  return start == std::string::npos ? 0 : start + 1;
}

size_t lineEnd(size_t offset) {
  const size_t end = document.find('\n', offset);
  return end == std::string::npos ? document.size() : end;
}

size_t lineNumber(size_t offset) {
  return static_cast<size_t>(std::count(document.begin(),
                                        document.begin() + std::min(offset, document.size()),
                                        '\n'));
}

size_t offsetForLine(size_t targetLine) {
  size_t offset = 0;
  while (targetLine-- && offset < document.size()) {
    const size_t end = document.find('\n', offset);
    if (end == std::string::npos) return document.size();
    offset = end + 1;
  }
  return offset;
}

size_t columnFor(size_t offset) {
  return std::min(offset, document.size()) - lineStart(offset);
}

size_t lineOffsetForColumn(size_t line, size_t column) {
  const size_t start = offsetForLine(line);
  const size_t end = lineEnd(start);
  return std::min(start + column, end);
}

void rememberUndo() {
  undoBuffer = document;
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
  uiNeedsRender = true;
}

void insertAtCursor(char c) {
  if (document.size() >= documentLimit) {
    statusMessage = "Document limit reached";
    uiNeedsRender = true;
    return;
  }
  if (!undoGroupActive) rememberUndo();
  document.insert(document.begin() + cursor, c);
  ++cursor;
  markEdited();
}

void eraseRange(size_t first, size_t last) {
  if (first >= last || first >= document.size()) return;
  if (!undoGroupActive) rememberUndo();
  last = std::min(last, document.size());
  document.erase(first, last - first);
  cursor = std::min(first, document.empty() ? size_t(0) : document.size() - 1);
  markEdited();
}

void loadDocument(const String &path) {
  if (!cardAvailable) {
    statusMessage = "Insert a microSD card to open files";
    return;
  }
  File file = SD.open(path, FILE_READ);
  if (!file || file.isDirectory()) {
    statusMessage = "Could not open file";
    if (file) file.close();
    return;
  }
  if (file.size() > documentLimit) {
    file.close();
    statusMessage = "File exceeds the available editing memory";
    uiNeedsRender = true;
    return;
  }
  document.clear();
  while (file.available() && document.size() < documentLimit) {
    document.push_back(static_cast<char>(file.read()));
  }
  file.close();
  fileName = String(path.substring(path.lastIndexOf('/') + 1)).c_str();
  currentPath = path.c_str();
  cursor = 0;
  dirty = false;
  undoBuffer.clear();
  visualMode = insertMode = commandMode = searchMode = false;
  pendingOperator = 0;
  undoGroupActive = false;
  editorVisible = true;
  statusMessage = "Opened " + fileName;
  lastInputMs = millis();
  uiNeedsRender = true;
}

bool saveDocument() {
  if (!cardAvailable || !directoryReady) {
    statusMessage = "No SD card — draft remains in memory";
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
    statusMessage = "Save failed — could not stage original";
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
  if (hadOriginal) SD.remove(backupPath.c_str());
  dirty = false;
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
      if (name.endsWith(".md") || name.endsWith(".markdown")) {
        markdownFiles.push_back(name);
      }
    }
    entry.close();
  }
  dir.close();
  std::sort(markdownFiles.begin(), markdownFiles.end(),
            [](const String &a, const String &b) { return a < b; });
}

String safeMarkdownName(String name) {
  if (name.length() == 0 || name.length() > 64 ||
      name.indexOf('/') >= 0 || name.indexOf('\\') >= 0 ||
      name == "." || name == "..") return "";
  for (size_t i = 0; i < name.length(); ++i) {
    const char c = name[i];
    if (!(isalnum(static_cast<unsigned char>(c)) || c == ' ' || c == '-' ||
          c == '_' || c == '.')) return "";
  }
  if (!name.endsWith(".md") && !name.endsWith(".markdown")) return "";
  return name;
}

void handleTransferHome() {
  static const char page[] =
      "<!doctype html><meta name='viewport' content='width=device-width'>"
      "<title>WriterInAWell</title><style>"
      "body{background:#10171c;color:#d4dfe5;font:16px sans-serif;"
      "max-width:680px;margin:2rem auto;padding:0 1rem}"
      "a,button{color:#8de6b3}li{margin:.8rem 0}"
      "input,button{background:#18272d;border:1px solid #315360;"
      "border-radius:3px;padding:.6rem;color:#d4dfe5}"
      "</style><h1>WriterInAWell</h1><p>Markdown files on the SD card</p>"
      "<ul id='files'></ul><form action='/upload' method='post' "
      "enctype='multipart/form-data'><input type='file' name='file' "
      "accept='.md,.markdown,text/markdown' required><button>Upload .md</button>"
      "</form><script>fetch('/api/files').then(r=>r.json()).then(a=>{"
      "document.getElementById('files').innerHTML=a.map(n=>'<li><a href=\"/download?name='+"
      "encodeURIComponent(n)+'\">'+n+'</a></li>').join('')})</script>";
  transferServer.send(200, "text/html; charset=utf-8", page);
}

void handleTransferFileList() {
  scanMarkdownFiles();
  String json = "[";
  for (size_t i = 0; i < markdownFiles.size(); ++i) {
    if (i) json += ',';
    json += '"';
    json += markdownFiles[i];
    json += '"';
  }
  json += ']';
  transferServer.send(200, "application/json", json);
}

void handleTransferDownload() {
  const String name = safeMarkdownName(transferServer.arg("name"));
  if (!cardAvailable || name.isEmpty()) {
    transferServer.send(400, "text/plain", "Invalid Markdown filename");
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
  transferServer.streamFile(file, "text/markdown; charset=utf-8");
  file.close();
}

void handleTransferUploadData() {
  HTTPUpload &upload = transferServer.upload();
  const String tempPath = "/WriterInAWell/.incoming.tmp";
  switch (upload.status) {
  case UPLOAD_FILE_START:
    uploadFailed = !cardAvailable || !directoryReady;
    uploadDestination = safeMarkdownName(upload.filename);
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
  else transferServer.send(201, "text/plain", "Markdown file saved to SD card");
  statusMessage = uploadFailed ? "WiFi upload failed" : "WiFi upload saved to SD";
  uiNeedsRender = true;
}

void startTransfer() {
  if (!cardAvailable || !directoryReady) {
    statusMessage = "WiFi sharing needs an SD card";
    uiNeedsRender = true;
    return;
  }
  if (transferEnabled) return;
  if (dirty && !saveDocument()) return;
  char password[16];
  snprintf(password, sizeof(password), "well-%06llX",
           static_cast<unsigned long long>(ESP.getEfuseMac() & 0xFFFFFF));
  wifiPassword = password;
  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP("WriterInAWell", wifiPassword.c_str(), 1, false, 1)) {
    WiFi.mode(WIFI_OFF);
    statusMessage = "Could not start WiFi share";
    uiNeedsRender = true;
    return;
  }
  transferServer.on("/", HTTP_GET, handleTransferHome);
  transferServer.on("/api/files", HTTP_GET, handleTransferFileList);
  transferServer.on("/download", HTTP_GET, handleTransferDownload);
  transferServer.on("/upload", HTTP_POST, handleTransferUploadComplete,
                    handleTransferUploadData);
  transferServer.onNotFound([]() {
    transferServer.send(404, "text/plain", "Not found");
  });
  transferServer.begin();
  transferEnabled = true;
  statusMessage = "Share at 192.168.4.1 — password shown below";
  uiNeedsRender = true;
  Serial.printf("WriterInAWell WiFi: SSID WriterInAWell, password %s, "
                "http://192.168.4.1/\n", wifiPassword.c_str());
}

void stopTransfer() {
  if (!transferEnabled) return;
  transferServer.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  transferEnabled = false;
  wifiPassword.clear();
  statusMessage = "WiFi sharing off";
  uiNeedsRender = true;
}

void toggleTransfer() {
  if (transferEnabled) stopTransfer();
  else startTransfer();
}

// Warm Markdown palette: oranges, purples, yellows, peach.
constexpr const char *C_HEADING = "FFA94D";  // orange
constexpr const char *C_BOLD    = "C792EA";  // purple
constexpr const char *C_ITALIC  = "FFD8A8";  // peach
constexpr const char *C_BULLET  = "FFD866";  // yellow
constexpr const char *C_CODE    = "F6C177";  // soft orange-peach (inline code)
constexpr const char *C_QUOTE   = "C792EA";  // purple (blockquote)
constexpr const char *C_TEXT    = "D4DFE5";  // default prose

std::string markdownMarkup(const std::string &raw) {
  // Build an LVGL recolor string from a Markdown line. Spans are only opened
  // when a matching close marker exists later in the same line, so the result
  // is always balanced and cannot vanish. Literal '#' characters are escaped
  // with the LVGL recolor rules (## outside a span, #+##+#RRGGBB inside one).
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
        out += '#';                   // close span
        out += "##";                  // literal '#'
        out += '#';
        out += spanColor;
        out += ' ';                   // reopen span
      } else {
        out += "##";                  // literal '#'
      }
    } else {
      out += c;
    }
  };

  const size_t n = raw.size();
  size_t i = 0;

  // Leading list bullet: "- ", "* ", "+ ", or "N. " / "N) ".
  size_t lead = 0;
  while (lead < n && raw[lead] == ' ') ++lead;
  auto bulletLength = [&]() -> size_t {
    if (i != lead || i >= n) return 0;
    const char m = raw[i];
    if ((m == '-' || m == '+' ) && i + 1 < n && raw[i + 1] == ' ') return 1;
    if (m == '*' && i + 1 < n && raw[i + 1] == ' ') return 1;
    size_t j = i;
    while (j < n && isdigit(static_cast<unsigned char>(raw[j]))) ++j;
    if (j > i && j < n && (raw[j] == '.' || raw[j] == ')') &&
        j + 1 < n && raw[j + 1] == ' ') {
      return j - i + 1;
    }
    return 0;
  };

  while (i < n) {
    const char c = raw[i];

    if (c == '\\' && i + 1 < n) {
      emit(raw[i + 1]);
      i += 2;
      continue;
    }

    if (size_t bl = bulletLength()) {
      openSpan(C_BULLET);
      for (size_t k = 0; k < bl; ++k) emit(raw[i + k]);
      closeSpan();
      i += bl;
      continue;
    }

    // Bold **...** or __...__
    if ((c == '*' || c == '_') && i + 1 < n && raw[i + 1] == c) {
      const size_t close = raw.find(std::string(2, c), i + 2);
      if (close != std::string::npos && close > i + 2) {
        openSpan(C_BOLD);
        for (size_t k = i; k < close + 2; ++k) emit(raw[k]);
        closeSpan();
        i = close + 2;
        continue;
      }
    }

    // Italic *...* or _..._ (single marker, not a double, not a bullet)
    if (c == '*' || c == '_') {
      const size_t close = raw.find(c, i + 1);
      if (close != std::string::npos && close > i + 1 &&
          !(close + 1 < n && raw[close + 1] == c)) {
        openSpan(C_ITALIC);
        for (size_t k = i; k <= close; ++k) emit(raw[k]);
        closeSpan();
        i = close + 1;
        continue;
      }
    }

    // Inline code `...`
    if (c == '`') {
      const size_t close = raw.find('`', i + 1);
      if (close != std::string::npos && close > i + 1) {
        openSpan(C_CODE);
        for (size_t k = i; k <= close; ++k) emit(raw[k]);
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

void updateEditorLines() {
  const size_t cursorLine = lineNumber(cursor);
  const size_t totalLines = lineNumber(document.size());
  if (cursorLine < viewportStart) viewportStart = cursorLine;
  if (cursorLine >= viewportStart + VISIBLE_LINES) {
    viewportStart = cursorLine - VISIBLE_LINES + 1;
  }

  bool code = false;
  for (size_t i = 0; i < viewportStart && i < document.size(); ++i) {
    if (document[i] == '\n' && i + 1 < document.size() && document[i + 1] == '`') {
      code = !code;
    }
  }
  for (int row = 0; row < VISIBLE_LINES; ++row) {
    const size_t ln = viewportStart + row;
    const size_t start = offsetForLine(ln);
    if (ln > totalLines || start > document.size()) {
      lv_label_set_text(lineLabels[row], "");
      continue;
    }
    const size_t end = lineEnd(start);
    std::string raw(document.data() + start,
                    std::min(end - start, size_t(64)));
    const bool fence = raw.rfind("```", 0) == 0;
    const bool heading = raw.rfind("#", 0) == 0;
    const bool quote = raw.rfind(">", 0) == 0;
    const bool rowCode = code || fence;
    const size_t selectionStart = std::min(cursor, visualAnchor);
    const size_t selectionEnd = std::max(cursor, visualAnchor);
    const bool selected = visualMode && start <= selectionEnd &&
                          end >= selectionStart;
    lv_label_set_text(lineLabels[row], markdownMarkup(raw).c_str());
    lv_obj_set_style_text_font(lineLabels[row], editorFont(), 0);
    // Base color for the whole line (inline spans override via recolor).
    const char *base = rowCode ? C_CODE : heading ? C_HEADING
                         : quote ? C_QUOTE : C_TEXT;
    const uint32_t baseRGB = strtoul(base, nullptr, 16);
    lv_obj_set_style_text_color(lineLabels[row], lv_color_hex(baseRGB), 0);
    lv_obj_set_style_bg_color(lineLabels[row],
        selected ? lv_color_hex(0x17343A) :
        ln == cursorLine ? lv_color_hex(0x17242A) : lv_color_hex(0x10171C), 0);
    lv_obj_set_style_bg_opa(lineLabels[row], LV_OPA_COVER, 0);
    // No left border on the cursor row: a border would shift that row's
    // content 1 px right and desync it from the cursor block.
    lv_obj_set_style_border_width(lineLabels[row], 0, 0);
    if (fence) code = !code;
    else if (raw.find("```") != std::string::npos) code = !code;
  }

  // Position the live cursor block. Measure the rendered prefix with
  // lv_txt_get_width — the same per-glyph advance the label renderer sums —
  // and size the block to the exact cell width of the character under the
  // caret, so it always covers the glyph precisely.
  if (cursorBlock) {
    const size_t row = cursorLine - viewportStart;
    if (row < VISIBLE_LINES) {
      const lv_font_t *font = editorFont();
      const size_t lineBegin = offsetForLine(cursorLine);
      const size_t lineFinish = lineEnd(lineBegin);
      std::string lineText(document.data() + lineBegin,
                           std::min(lineFinish - lineBegin, size_t(64)));
      const size_t col = columnFor(cursor);
      const uint32_t prefixLen =
          static_cast<uint32_t>(std::min(col, lineText.size()));
      const lv_coord_t prefixW = lv_txt_get_width(lineText.c_str(),
          prefixLen, font, 0, LV_TEXT_FLAG_NONE);
      lv_coord_t cellW = editorGlyphWidth;
      if (col < lineText.size()) {
        const lv_coord_t inclNext = lv_txt_get_width(lineText.c_str(),
            static_cast<uint32_t>(col + 1), font, 0, LV_TEXT_FLAG_NONE);
        cellW = inclNext - prefixW;
      }
      lv_obj_set_size(cursorBlock, cellW, 15);
      lv_obj_set_pos(cursorBlock, 7 + 4 + prefixW,
          6 + static_cast<int>(row) * LINE_HEIGHT);
      lv_obj_move_foreground(cursorBlock);
    }
  }

  // Breadcrumb: ~/ + [folder]sub/ + filename. Unsaved names render in italics.
  if (pathLabel && documentLabel) {
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

    std::string breadPath = "~/";
    if (!dirPart.empty()) {
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
    }
    lv_label_set_text(pathLabel, breadPath.c_str());

    const bool unsaved = dirty || currentPath.empty();
    lv_label_set_text(documentLabel, filePart.c_str());
    lv_obj_set_style_text_font(documentLabel,
        unsaved ? &elms_bold_italic_14 : &elms_bold_14, 0);
    lv_obj_set_style_text_color(documentLabel,
        unsaved ? lv_color_hex(0xFFD8A8) : lv_color_hex(0xD4DFE5), 0);

    const uint16_t pathW = lv_txt_get_width(breadPath.c_str(),
        static_cast<uint32_t>(breadPath.size()), &space_mono_14, 0,
        LV_TEXT_FLAG_NONE);
    lv_obj_set_pos(documentLabel, 40 + pathW + 2, 7);
  }
}

void buildEditorScreen();
void buildBrowserScreen();
void buildPairingScreen();
void buildSettingsScreen();
void buildActionMenuScreen();
void updateStatusBar();
void backClicked(lv_event_t *);
void savePrefs();

void showBrowser() {
  if (dirty) {
    statusMessage = "Save changes before opening another document";
    uiNeedsRender = true;
    return;
  }
  scanMarkdownFiles();
  pairingVisible = false;
  settingsVisible = false;
  actionsVisible = false;
  if (markdownFiles.empty()) browserSelection = 0;
  else if (browserSelection >= markdownFiles.size()) {
    browserSelection = markdownFiles.size() - 1;
  }
  editorVisible = false;
  buildBrowserScreen();
}

void showSettings() {
  settingsVisible = true;
  pairingVisible = false;
  actionsVisible = false;
  editorVisible = false;
  buildSettingsScreen();
}

void showActionMenu() {
  actionsVisible = true;
  settingsVisible = pairingVisible = false;
  editorVisible = false;
  buildActionMenuScreen();
}

void runAction(size_t action) {
  actionSelection = std::min(action, size_t(2));
  if (action == 0) {
    if (dirty && !saveDocument()) {
      if (actionStatusLabel) lv_label_set_text(actionStatusLabel, statusMessage.c_str());
      return;
    }
    showBrowser();
  } else if (action == 1) {
    if (saveDocument()) backClicked(nullptr);
    else if (actionStatusLabel) lv_label_set_text(actionStatusLabel, statusMessage.c_str());
  } else if (action == 2) {
    const bool wasEnabled = transferEnabled;
    toggleTransfer();
    if (transferEnabled || wasEnabled) {
      backClicked(nullptr);
    } else if (actionStatusLabel) {
      lv_label_set_text(actionStatusLabel, statusMessage.c_str());
    }
  }
}

void actionClicked(lv_event_t *event) {
  const intptr_t action = reinterpret_cast<intptr_t>(lv_event_get_user_data(event));
  if (action >= 0) runAction(static_cast<size_t>(action));
}

void newDocument() {
  if (dirty) {
    statusMessage = "Save changes before starting another document";
    uiNeedsRender = true;
    return;
  }
  document = "# Untitled\n\n";
  fileName = "Untitled.md";
  currentPath.clear();
  cursor = 0;
  dirty = false;
  undoBuffer.clear();
  undoGroupActive = false;
  insertMode = visualMode = commandMode = searchMode = false;
  pendingOperator = 0;
  editorVisible = true;
  settingsVisible = pairingVisible = actionsVisible = false;
  statusMessage = "New Markdown document — press i to write";
  uiNeedsRender = true;
  buildEditorScreen();
}

void fileClicked(lv_event_t *event) {
  const intptr_t index = reinterpret_cast<intptr_t>(lv_event_get_user_data(event));
  if (index < 0 || static_cast<size_t>(index) >= markdownFiles.size()) return;
  loadDocument(String("/WriterInAWell/") + markdownFiles[index]);
  buildEditorScreen();
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

void newClicked(lv_event_t *) { newDocument(); }
void backClicked(lv_event_t *) {
  savePrefs();
  pairingVisible = settingsVisible = pairingReturnsToSettings = false;
  actionsVisible = false;
  editorVisible = true;
  buildEditorScreen();
}
void menuClicked(lv_event_t *) { showActionMenu(); }

void actionBackClicked(lv_event_t *) { backClicked(nullptr); }

void showPairingHelp() {
  pairingReturnsToSettings = settingsVisible;
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
  devicePrefs.end();
  prefsDirty = false;
}

void loadPrefs() {
  if (!devicePrefs.begin("wiaw", true)) return;
  if (devicePrefs.isKey("bl")) {
    brightnessValue = devicePrefs.getUChar("bl", 170);
  }
  if (devicePrefs.isKey("smooth")) {
    editorSmoothText = devicePrefs.getBool("smooth", false);
  }
  devicePrefs.end();
  if (brightnessValue < BRIGHTNESS_MIN) brightnessValue = BRIGHTNESS_MIN;
  if (brightnessValue > BRIGHTNESS_MAX) brightnessValue = BRIGHTNESS_MAX;
}

void setBrightnessValue(uint8_t value, bool fromSlider = false) {
  if (value < BRIGHTNESS_MIN) value = BRIGHTNESS_MIN;
  if (value > BRIGHTNESS_MAX) value = BRIGHTNESS_MAX;
  brightnessValue = value;
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

void brightnessSliderChanged(lv_event_t *event) {
  const int32_t value = lv_slider_get_value(lv_event_get_target(event));
  setBrightnessValue(static_cast<uint8_t>(value), true);
}

void brightnessSliderReleased(lv_event_t *) { savePrefs(); }

void toggleTextSmooth() {
  editorSmoothText = !editorSmoothText;
  prefsDirty = true;
  statusMessage = editorSmoothText ? "Text smoothing on"
                                   : "Text smoothing off (crisp)";
  if (smoothButton) {
    lv_obj_t *label = lv_obj_get_child(smoothButton, 0);
    if (label) {
      lv_label_set_text(label, editorSmoothText ? "TEXT SMOOTH: ON"
                                                : "TEXT SMOOTH: OFF");
    }
  }
  savePrefs();
}

void textSmoothClicked(lv_event_t *) { toggleTextSmooth(); }

void settingsClicked(lv_event_t *) {
  showSettings();
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
  // Center the 22 px button in the 30 px header's 28 px content area,
  // like makeToolbarButton does on the editor screen.
  lv_obj_set_pos(button, x, 3);
  stylePanel(button, lv_color_hex(0x18272D), lv_color_hex(0x315360));
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
  lv_obj_set_style_text_color(text, lv_color_hex(0xE8EEF2), 0);
  return button;
}

lv_obj_t *makeToolbarButton(lv_obj_t *parent, const char *label, int x,
                            int width, lv_event_cb_t callback) {
  lv_obj_t *button = lv_btn_create(parent);
  lv_obj_set_size(button, width, 22);
  // Center the 22 px button in the 30 px header's 28 px content area
  // (30 minus the 1 px borders): (28 - 22) / 2 = 3 px above and below.
  lv_obj_set_pos(button, x, 3);
  stylePanel(button, lv_color_hex(0x18272D), lv_color_hex(0x315360));
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
  // Icons live in the Space Mono Nerd Font, not Elms Sans.
  lv_obj_set_style_text_font(text, &space_mono_14, 0);
  lv_obj_set_style_text_color(text, lv_color_hex(0xE8EEF2), 0);
  return button;
}

lv_obj_t *makeMenuButton(lv_obj_t *parent, const char *label, int x, int y,
                         int width, int height, lv_event_cb_t callback,
                         void *userData = nullptr) {
  lv_obj_t *button = lv_btn_create(parent);
  lv_obj_set_size(button, width, height);
  lv_obj_set_pos(button, x, y);
  stylePanel(button, lv_color_hex(0x18272D), lv_color_hex(0x315360));
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
  lv_obj_set_style_text_color(text, lv_color_hex(0xE8EEF2), 0);
  return button;
}

void buildActionMenuScreen() {
  editorVisible = false;
  actionsVisible = true;
  settingsVisible = pairingVisible = false;
  lv_obj_clean(lv_scr_act());
  pairingStatusLabel = settingsRadioLabel = brightnessStatusLabel = nullptr;
  actionStatusLabel = nullptr;
  brightnessSlider = smoothButton = nullptr;
  lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x10171C), 0);

  lv_obj_t *header = lv_obj_create(lv_scr_act());
  lv_obj_set_size(header, SCREEN_W - 8, 30);
  lv_obj_set_pos(header, 4, 4);
  stylePanel(header, lv_color_hex(0x141E24), lv_color_hex(0x2C424B));
  lv_obj_t *heading = lv_label_create(header);
  lv_label_set_text(heading, "FILE ACTIONS");
  lv_obj_set_pos(heading, 10, 4);
  setTitleFont(heading);
  lv_obj_set_style_text_color(heading, lv_color_hex(0x8DE6B3), 0);
  makeButton(header, "BACK", 412, 54, actionBackClicked);

  lv_obj_t *panel = lv_obj_create(lv_scr_act());
  lv_obj_set_size(panel, SCREEN_W - 8, 240);
  lv_obj_set_pos(panel, 4, 39);
  stylePanel(panel, lv_color_hex(0x10171C), lv_color_hex(0x2C424B));
  const char *actions[] = {"FILE BROWSER", "SAVE DOCUMENT",
                           transferEnabled ? "STOP WI-FI SHARE" : "SHARE OVER WI-FI"};
  for (intptr_t i = 0; i < 3; ++i) {
    lv_obj_t *button = makeMenuButton(panel, actions[i], 12, 12 + i * 46,
                                      442, 36, actionClicked,
                                      reinterpret_cast<void *>(i));
    lv_obj_set_style_bg_color(button,
        i == static_cast<intptr_t>(actionSelection)
            ? lv_color_hex(0x1A2D32) : lv_color_hex(0x18272D), 0);
  }
  actionStatusLabel = lv_label_create(panel);
  lv_obj_set_width(actionStatusLabel, 442);
  lv_label_set_long_mode(actionStatusLabel, LV_LABEL_LONG_DOT);
  lv_label_set_text(actionStatusLabel, statusMessage.c_str());
  lv_obj_set_pos(actionStatusLabel, 14, 160);
  setUiFont(actionStatusLabel);
  lv_obj_set_style_text_color(actionStatusLabel, lv_color_hex(0xA9BAC2), 0);

  lv_obj_t *foot = lv_label_create(lv_scr_act());
  lv_label_set_text(foot, "1 files   2 save   3 Wi-Fi   ↑/↓ select   Esc back");
  lv_obj_set_pos(foot, 10, 291);
  setUiFont(foot);
  lv_obj_set_style_text_color(foot, lv_color_hex(0x71858F), 0);
}

void buildEditorScreen() {
  editorVisible = true;
  pairingVisible = settingsVisible = actionsVisible = pairingReturnsToSettings = false;
  lv_obj_clean(lv_scr_act());
  pairingStatusLabel = nullptr;
  settingsRadioLabel = nullptr;
  brightnessStatusLabel = nullptr;
  brightnessSlider = smoothButton = nullptr;
  lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x10171C), 0);

  lv_obj_t *header = lv_obj_create(lv_scr_act());
  lv_obj_set_size(header, SCREEN_W - 8, 30);
  lv_obj_set_pos(header, 4, 4);
  stylePanel(header, lv_color_hex(0x141E24), lv_color_hex(0x2C424B));
  makeToolbarButton(header, ICON_MENU, 3, 30, menuClicked);
  radioLabel = lv_label_create(header);
  lv_label_set_text(radioLabel, ICON_BLUETOOTH);
  // Larger bluetooth logo (20 px cut of the same Nerd Font), pushed right
  // beside the power indicator and the settings button.
  lv_obj_set_pos(radioLabel, 394, 3);
  lv_obj_set_style_text_font(radioLabel, &space_mono_20, 0);

  pathLabel = lv_label_create(header);
  lv_label_set_text(pathLabel, "~/");
  lv_obj_set_pos(pathLabel, 40, 7);
  lv_obj_set_style_text_font(pathLabel, &space_mono_14, 0);
  lv_obj_set_style_text_color(pathLabel, lv_color_hex(0x78C8F4), 0);

  documentLabel = lv_label_create(header);
  lv_obj_set_width(documentLabel, 200);
  lv_label_set_long_mode(documentLabel, LV_LABEL_LONG_DOT);
  lv_obj_set_pos(documentLabel, 70, 7);
  lv_obj_set_style_text_font(documentLabel, &elms_bold_14, 0);
  lv_obj_set_style_text_color(documentLabel, lv_color_hex(0xD4DFE5), 0);
  makeToolbarButton(header, ICON_SETTINGS, 430, 36, settingsClicked);
  // Power indicator lives in the top bar, next to the bluetooth icon.
  powerLabel = lv_label_create(header);
  lv_label_set_text(powerLabel, "5V");
  lv_obj_set_pos(powerLabel, 411, 7);
  setUiFont(powerLabel);
  lv_obj_set_style_text_color(powerLabel, lv_color_hex(0x8DE6B3), 0);

  lv_obj_t *canvas = lv_obj_create(lv_scr_act());
  // 4 px screen margins, matching the header above.
  lv_obj_set_size(canvas, SCREEN_W - 8, 231);
  lv_obj_set_pos(canvas, 4, 39);
  stylePanel(canvas, lv_color_hex(0x10171C), lv_color_hex(0x2C424B));
  for (int i = 0; i < VISIBLE_LINES; ++i) {
    lineLabels[i] = lv_label_create(canvas);
    lv_obj_set_size(lineLabels[i], SCREEN_W - 28, LINE_HEIGHT);
    lv_obj_set_pos(lineLabels[i], 7, 6 + i * LINE_HEIGHT);
    lv_label_set_recolor(lineLabels[i], true);
    lv_label_set_long_mode(lineLabels[i], LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_color(lineLabels[i], lv_color_hex(0xD4DFE5), 0);
    lv_obj_set_style_text_font(lineLabels[i], editorFont(), 0);
    lv_obj_set_style_pad_left(lineLabels[i], 4, 0);
    // Lift the glyph ink ~2 px inside each line (the font's ascent leaves
    // 2 px of empty space above the ink and 3 px below).
    lv_obj_set_style_pad_top(lineLabels[i], -2, 0);
    lv_obj_set_style_pad_bottom(lineLabels[i], 0, 0);
    lv_obj_set_style_radius(lineLabels[i], 2, 0);
  }

  // Live cursor block: a filled rectangle that blinks at the caret. Its
  // width is re-measured every refresh to exactly one character cell.
  cursorBlock = lv_obj_create(canvas);
  lv_obj_remove_style_all(cursorBlock);
  lv_obj_set_size(cursorBlock, editorGlyphWidth, 15);
  lv_obj_set_style_bg_color(cursorBlock, lv_color_hex(0xFFD866), 0);
  lv_obj_set_style_bg_opa(cursorBlock, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(cursorBlock, 0, 0);
  lv_obj_set_style_border_width(cursorBlock, 0, 0);
  lv_obj_clear_flag(cursorBlock, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *statusPanel = lv_obj_create(lv_scr_act());
  // 4 px screen margins, matching the header and text area.
  lv_obj_set_size(statusPanel, SCREEN_W - 8, 25);
  lv_obj_set_pos(statusPanel, 4, 274);
  stylePanel(statusPanel, lv_color_hex(0x141E24), lv_color_hex(0x2C424B));
  modeLabel = lv_label_create(statusPanel);
  // Vertically center the pill inside the 25 px status panel: the pill is
  // line_height(17) + 1 + 1 padding = 19 px tall, and the panel's content
  // area is 23 px (25 minus the 1 px borders), so y = (23 - 19) / 2 = 2.
  lv_obj_set_pos(modeLabel, 4, 2);
  lv_obj_set_style_text_font(modeLabel, &space_mono_14, 0);
  lv_obj_set_style_text_color(modeLabel, lv_color_hex(0x10171C), 0);
  lv_obj_set_style_bg_color(modeLabel, lv_color_hex(0x4CCB8B), 0);
  lv_obj_set_style_bg_opa(modeLabel, LV_OPA_COVER, 0);
  lv_obj_set_style_pad_left(modeLabel, 6, 0);
  lv_obj_set_style_pad_right(modeLabel, 6, 0);
  lv_obj_set_style_pad_top(modeLabel, 1, 0);
  lv_obj_set_style_pad_bottom(modeLabel, 1, 0);
  lv_obj_set_style_radius(modeLabel, 3, 0);
  statusLabel = lv_label_create(statusPanel);
  lv_obj_set_width(statusLabel, 228);
  lv_label_set_long_mode(statusLabel, LV_LABEL_LONG_DOT);
  lv_obj_set_pos(statusLabel, 104, 5);
  setUiFont(statusLabel);
  lv_obj_set_style_text_color(statusLabel, lv_color_hex(0xA9BAC2), 0);
  footerLabel = lv_label_create(statusPanel);
  // Right-justified: the line/column (and touch) coordinates hug the
  // panel's right edge, mirroring the top bar's settings button.
  lv_obj_set_width(footerLabel, 126);
  lv_obj_set_style_text_align(footerLabel, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_pos(footerLabel, 340, 5);
  setUiFont(footerLabel);
  lv_obj_set_style_text_color(footerLabel, lv_color_hex(0x78C8F4), 0);

  // Bottom hint line: shows the mode-switch keys and doubles as the live
  // ":" / "/" command line while a command is being typed. Its text is
  // refreshed in updateStatusBar().
  hintLabel = lv_label_create(lv_scr_act());
  lv_obj_set_pos(hintLabel, 10, 303);
  lv_obj_set_width(hintLabel, SCREEN_W - 20);
  lv_label_set_long_mode(hintLabel, LV_LABEL_LONG_DOT);
  setUiFont(hintLabel);
  lv_obj_set_style_text_color(hintLabel, lv_color_hex(0x71858F), 0);
  updateEditorLines();
  updateStatusBar();
  uiNeedsRender = false;
}

void buildBrowserScreen() {
  editorVisible = false;
  pairingVisible = settingsVisible = actionsVisible = false;
  lv_obj_clean(lv_scr_act());
  pairingStatusLabel = nullptr;
  settingsRadioLabel = nullptr;
  brightnessStatusLabel = nullptr;
  brightnessSlider = smoothButton = nullptr;
  lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x10171C), 0);

  lv_obj_t *header = lv_obj_create(lv_scr_act());
  lv_obj_set_size(header, SCREEN_W - 8, 30);
  lv_obj_set_pos(header, 4, 4);
  stylePanel(header, lv_color_hex(0x141E24), lv_color_hex(0x2C424B));
  lv_obj_t *heading = lv_label_create(header);
  lv_label_set_text(heading, "MARKDOWN FILES  /WriterInAWell");
  lv_obj_set_pos(heading, 10, 4);
  setTitleFont(heading);
  lv_obj_set_style_text_color(heading, lv_color_hex(0x8DE6B3), 0);
  makeButton(header, "BACK", 347, 58, backClicked);
  makeButton(header, "NEW", 412, 54, newClicked);

  browserPanel = lv_obj_create(lv_scr_act());
  lv_obj_set_size(browserPanel, SCREEN_W - 8, 240);
  lv_obj_set_pos(browserPanel, 4, 39);
  stylePanel(browserPanel, lv_color_hex(0x10171C), lv_color_hex(0x2C424B));
  if (!cardAvailable) {
    lv_obj_t *message = lv_label_create(browserPanel);
    lv_label_set_text(message, "No microSD card detected.\nInsert a card to browse and save Markdown files.");
    lv_obj_set_pos(message, 16, 22);
    setUiFont(message);
    lv_obj_set_style_text_color(message, lv_color_hex(0xD4DFE5), 0);
  } else if (markdownFiles.empty()) {
    lv_obj_t *message = lv_label_create(browserPanel);
    lv_label_set_text(message, "No .md files yet. Press NEW to start writing.");
    lv_obj_set_pos(message, 16, 22);
    setUiFont(message);
    lv_obj_set_style_text_color(message, lv_color_hex(0xD4DFE5), 0);
  } else {
    const size_t first = browserSelection >= 10 ? browserSelection - 9 : 0;
    const size_t last = std::min(markdownFiles.size(), first + 10);
    for (size_t i = first; i < last; ++i) {
      lv_obj_t *button = lv_btn_create(browserPanel);
      lv_obj_set_size(button, 442, 20);
      lv_obj_set_pos(button, 8, 7 + static_cast<int>(i - first) * 21);
      stylePanel(button, i == browserSelection ? lv_color_hex(0x1A2D32)
                                               : lv_color_hex(0x10171C),
                 lv_color_hex(0x2C424B));
      lv_obj_add_event_cb(button, fileClicked, LV_EVENT_CLICKED,
                          reinterpret_cast<void *>(static_cast<intptr_t>(i)));
      lv_obj_t *name = lv_label_create(button);
      lv_label_set_text(name, markdownFiles[i].c_str());
      lv_obj_set_pos(name, 7, 2);
      setUiFont(name);
      lv_obj_set_style_text_color(name, lv_color_hex(0xD4DFE5), 0);
    }
  }
  lv_obj_t *foot = lv_label_create(lv_scr_act());
  lv_label_set_text(foot, "↑/↓ select   Enter open   n new   Esc back");
  lv_obj_set_pos(foot, 10, 291);
  setUiFont(foot);
  lv_obj_set_style_text_color(foot, lv_color_hex(0x71858F), 0);
  uiNeedsRender = false;
}

void buildSettingsScreen() {
  editorVisible = false;
  settingsVisible = true;
  pairingVisible = actionsVisible = false;
  lv_obj_clean(lv_scr_act());
  pairingStatusLabel = nullptr;
  settingsRadioLabel = nullptr;
  brightnessStatusLabel = nullptr;
  brightnessSlider = smoothButton = nullptr;
  lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x10171C), 0);

  lv_obj_t *header = lv_obj_create(lv_scr_act());
  lv_obj_set_size(header, SCREEN_W - 8, 30);
  lv_obj_set_pos(header, 4, 4);
  stylePanel(header, lv_color_hex(0x141E24), lv_color_hex(0x2C424B));
  lv_obj_t *heading = lv_label_create(header);
  lv_label_set_text(heading, "SETTINGS");
  lv_obj_set_pos(heading, 10, 4);
  setTitleFont(heading);
  lv_obj_set_style_text_color(heading, lv_color_hex(0x8DE6B3), 0);
  makeButton(header, "BACK", 412, 54, backClicked);

  lv_obj_t *panel = lv_obj_create(lv_scr_act());
  lv_obj_set_size(panel, SCREEN_W - 8, 229);
  lv_obj_set_pos(panel, 4, 39);
  stylePanel(panel, lv_color_hex(0x10171C), lv_color_hex(0x2C424B));

  lv_obj_t *brightnessTitle = lv_label_create(panel);
  lv_label_set_text(brightnessTitle, "BACKLIGHT");
  lv_obj_set_pos(brightnessTitle, 14, 10);
  setUiFont(brightnessTitle);
  lv_obj_set_style_text_color(brightnessTitle, lv_color_hex(0x78C8F4), 0);

  brightnessSlider = lv_slider_create(panel);
  lv_obj_set_size(brightnessSlider, 300, 16);
  lv_obj_set_pos(brightnessSlider, 14, 30);
  lv_slider_set_mode(brightnessSlider, LV_SLIDER_MODE_NORMAL);
  lv_slider_set_range(brightnessSlider, BRIGHTNESS_MIN, BRIGHTNESS_MAX);
  lv_slider_set_value(brightnessSlider, brightnessValue, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(brightnessSlider, lv_color_hex(0x18272D), LV_PART_MAIN);
  lv_obj_set_style_border_color(brightnessSlider, lv_color_hex(0x315360), LV_PART_MAIN);
  lv_obj_set_style_border_width(brightnessSlider, 1, LV_PART_MAIN);
  lv_obj_set_style_radius(brightnessSlider, 3, LV_PART_MAIN);
  lv_obj_set_style_bg_color(brightnessSlider, lv_color_hex(0x4CCB8B), LV_PART_INDICATOR);
  lv_obj_set_style_radius(brightnessSlider, 3, LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(brightnessSlider, lv_color_hex(0xE8EEF2), LV_PART_KNOB);
  lv_obj_set_style_border_color(brightnessSlider, lv_color_hex(0x2C424B), LV_PART_KNOB);
  lv_obj_set_style_border_width(brightnessSlider, 1, LV_PART_KNOB);
  lv_obj_set_style_pad_all(brightnessSlider, 3, LV_PART_KNOB);
  lv_obj_add_event_cb(brightnessSlider, brightnessSliderChanged,
                      LV_EVENT_VALUE_CHANGED, nullptr);
  lv_obj_add_event_cb(brightnessSlider, brightnessSliderReleased,
                      LV_EVENT_RELEASED, nullptr);

  brightnessStatusLabel = lv_label_create(panel);
  {
    char pct[8];
    snprintf(pct, sizeof(pct), "%u%%", unsigned(brightnessValue) * 100 / 255);
    lv_label_set_text(brightnessStatusLabel, pct);
  }
  lv_obj_set_pos(brightnessStatusLabel, 330, 34);
  setUiFont(brightnessStatusLabel);
  lv_obj_set_style_text_color(brightnessStatusLabel, lv_color_hex(0xE8EEF2), 0);

  smoothButton = makeMenuButton(panel,
      editorSmoothText ? "TEXT SMOOTH: ON" : "TEXT SMOOTH: OFF",
      14, 54, 200, 30, textSmoothClicked);

  lv_obj_t *bleTitle = lv_label_create(panel);
  lv_label_set_text(bleTitle, "BLUETOOTH KEYBOARD");
  lv_obj_set_pos(bleTitle, 14, 101);
  setUiFont(bleTitle);
  lv_obj_set_style_text_color(bleTitle, lv_color_hex(0x78C8F4), 0);
  settingsRadioLabel = lv_label_create(panel);
  lv_label_set_text(settingsRadioLabel, keyboardStatus());
  lv_obj_set_width(settingsRadioLabel, 300);
  lv_label_set_long_mode(settingsRadioLabel, LV_LABEL_LONG_DOT);
  lv_obj_set_pos(settingsRadioLabel, 14, 123);
  setUiFont(settingsRadioLabel);
  lv_obj_set_style_text_color(settingsRadioLabel, lv_color_hex(0xD4DFE5), 0);
  makeMenuButton(panel, "KEYBOARDS", 346, 115, 110, 32, pairClicked);
  if (keyboardIsConnected()) {
    makeMenuButton(panel, "DISCONNECT", 346, 151, 110, 28,
                   disconnectKeyboardClicked);
  }

  lv_obj_t *wifiNote = lv_label_create(panel);
  lv_label_set_text(wifiNote, "Wi-Fi sharing is off by default; use SHARE in the editor.");
  lv_obj_set_pos(wifiNote, 14, 187);
  setUiFont(wifiNote);
  lv_obj_set_style_text_color(wifiNote, lv_color_hex(0x71858F), 0);

  lv_obj_t *foot = lv_label_create(lv_scr_act());
  lv_label_set_text(foot, "drag/←/→ brightness   1-4 presets   a smooth   p keyboards   Esc back");
  lv_obj_set_pos(foot, 10, 291);
  setUiFont(foot);
  lv_obj_set_style_text_color(foot, lv_color_hex(0x71858F), 0);
}

void buildPairingScreen() {
  editorVisible = false;
  pairingVisible = true;
  settingsVisible = actionsVisible = false;
  lv_obj_clean(lv_scr_act());
  pairingStatusLabel = nullptr;
  settingsRadioLabel = nullptr;
  brightnessStatusLabel = nullptr;
  brightnessSlider = smoothButton = nullptr;
  lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x10171C), 0);

  lv_obj_t *header = lv_obj_create(lv_scr_act());
  lv_obj_set_size(header, SCREEN_W - 8, 30);
  lv_obj_set_pos(header, 4, 4);
  stylePanel(header, lv_color_hex(0x141E24), lv_color_hex(0x2C424B));
  lv_obj_t *heading = lv_label_create(header);
  lv_label_set_text(heading, "BLE KEYBOARDS");
  lv_obj_set_pos(heading, 10, 4);
  setTitleFont(heading);
  lv_obj_set_style_text_color(heading, lv_color_hex(0x8DE6B3), 0);
  makeButton(header, "RESCAN", 327, 76, rescanKeyboardsClicked);
  makeButton(header, "BACK", 412, 54, backFromPairing);

  lv_obj_t *panel = lv_obj_create(lv_scr_act());
  lv_obj_set_size(panel, SCREEN_W - 8, 229);
  lv_obj_set_pos(panel, 4, 39);
  stylePanel(panel, lv_color_hex(0x10171C), lv_color_hex(0x2C424B));
  pairingStatusLabel = lv_label_create(panel);
  lv_obj_set_width(pairingStatusLabel, 442);
  lv_label_set_long_mode(pairingStatusLabel, LV_LABEL_LONG_DOT);
  lv_label_set_text(pairingStatusLabel, keyboardStatus());
  lv_obj_set_pos(pairingStatusLabel, 14, 8);
  setUiFont(pairingStatusLabel);
  lv_obj_set_style_text_color(pairingStatusLabel, lv_color_hex(0xD4DFE5), 0);

  const size_t count = keyboardCandidateCount();
  pairListRenderedCount = count;
  if (pairSelection >= count && count) pairSelection = count - 1;
  if (count == 0) {
    lv_obj_t *empty = lv_label_create(panel);
    lv_label_set_text(empty, "Turn on a BLE keyboard in pairing mode.\n"
                             "Scanning is automatic; tap RESCAN to search again.");
    lv_obj_set_pos(empty, 14, 45);
    setUiFont(empty);
    lv_obj_set_style_text_color(empty, lv_color_hex(0xA9BAC2), 0);
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
          i == pairSelection ? lv_color_hex(0x1A2D32) : lv_color_hex(0x18272D), 0);
      lv_obj_set_style_border_color(button,
          i == pairSelection ? lv_color_hex(0x4CCB8B) : lv_color_hex(0x315360), 0);
    }
  }

  lv_obj_t *foot = lv_label_create(lv_scr_act());
  lv_label_set_text(foot, "Tap a keyboard or ↑/↓ + Enter to connect   R rescan   Esc back");
  lv_obj_set_pos(foot, 10, 280);
  setUiFont(foot);
  lv_obj_set_style_text_color(foot, lv_color_hex(0x71858F), 0);
  uiNeedsRender = false;
}

void moveVertical(int direction) {
  const size_t col = columnFor(cursor);
  const size_t line = lineNumber(cursor);
  const size_t target = direction < 0 ? (line ? line - 1 : 0) : line + 1;
  cursor = lineOffsetForColumn(target, col);
  if (cursor == document.size() && cursor && document.back() == '\n') --cursor;
}

void moveWordForward() {
  while (cursor < document.size() &&
         std::isalnum(static_cast<unsigned char>(document[cursor]))) ++cursor;
  while (cursor < document.size() &&
         !std::isalnum(static_cast<unsigned char>(document[cursor]))) ++cursor;
}

void moveWordBackward() {
  if (cursor) --cursor;
  while (cursor && !std::isalnum(static_cast<unsigned char>(document[cursor]))) --cursor;
  while (cursor && std::isalnum(static_cast<unsigned char>(document[cursor - 1]))) --cursor;
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
  case 'h': if (to) --to; break;
  case 'l': if (to < document.size()) ++to; break;
  default: pendingOperator = 0; return;
  }
  size_t first = std::min(from, to);
  size_t last = std::max(from, to);
  if (last < document.size() && motion != '$' && motion != '0') ++last;
  if (op == 'y') yankBuffer.assign(document.data() + first, last - first);
  else {
    eraseRange(first, last);
    if (op == 'c') {
      undoGroupActive = true;
      insertMode = true;
    }
  }
}

void runCommand() {
  if (commandLine == "w" || commandLine == "write") {
    saveDocument();
  } else if (commandLine.rfind("w ", 0) == 0 ||
             commandLine.rfind("write ", 0) == 0) {
    const size_t firstSpace = commandLine.find(' ');
    const String name = safeMarkdownName(
        String(commandLine.substr(firstSpace + 1).c_str()));
    if (name.isEmpty()) {
      statusMessage = "Use :w filename.md";
    } else {
      fileName = name.c_str();
      currentPath.clear();
      saveDocument();
    }
  } else if (commandLine == "wq") {
    if (saveDocument()) showBrowser();
  } else if (commandLine == "q" || commandLine == "quit") {
    if (dirty) statusMessage = "Unsaved changes — use :w before leaving";
    else showBrowser();
  } else if (commandLine == "e" || commandLine == "edit") {
    showBrowser();
  } else if (commandLine == "new") {
    newDocument();
  } else if (commandLine == "wifi") {
    startTransfer();
  } else if (commandLine == "wifi-off") {
    stopTransfer();
  } else if (commandLine == "bright") {
    cycleBrightness();
  } else if (commandLine == "settings") {
    showSettings();
  } else if (commandLine == "menu") {
    showActionMenu();
  } else if (commandLine == "pair") {
    showPairingHelp();
  } else {
    statusMessage = "Commands: :w [name.md]  :e  :new  :menu  :settings";
  }
  commandLine.clear();
  commandMode = searchMode = false;
  uiNeedsRender = true;
}

void handleText(char c) {
  if (!editorVisible) {
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
    if (actionsVisible) {
      if (c >= '1' && c <= '3') runAction(c - '1');
      else if ((c == 'j' || c == 'k')) {
        if (c == 'j') actionSelection = (actionSelection + 1) % 3;
        else actionSelection = (actionSelection + 2) % 3;
        buildActionMenuScreen();
      } else if (c == 'q') actionBackClicked(nullptr);
      return;
    }
    if (settingsVisible) {
      if (c >= '1' && c <= '4')
        setBrightnessValue(BRIGHTNESS_PRESETS[c - '1']);
      else if (c == 'a' || c == 'A') toggleTextSmooth();
      else if (c == 'p') showPairingHelp();
      else if (c == 'q') backClicked(nullptr);
      return;
    }
    if ((c == 'j' || c == 'k') && !markdownFiles.empty()) {
      const int step = c == 'k' ? -1 : 1;
      browserSelection = (browserSelection + markdownFiles.size() + step) %
                         markdownFiles.size();
      buildBrowserScreen();
    } else if (c == 'n') {
      newDocument();
    } else if (c == 'q') {
      editorVisible = true;
      buildEditorScreen();
    }
    return;
  }
  if (commandMode || searchMode) {
    if (c == '\n' || c == '\r') {
      if (searchMode && !commandLine.empty()) {
        const size_t found = document.find(commandLine.c_str(), cursor + (cursor < document.size()));
        if (found != DocumentString::npos) {
          cursor = found;
          statusMessage = "Search match";
        } else statusMessage = "Pattern not found";
        commandLine.clear();
        searchMode = false;
      } else runCommand();
    } else if (c == 8 || c == 127) {
      if (!commandLine.empty()) commandLine.pop_back();
    } else if (commandLine.size() < 64 && c >= 32) commandLine += c;
    return;
  }

  if (insertMode) {
    if (c == 27) {
      leaveInsertMode();
      if (cursor) --cursor;
    } else if (c == 8 || c == 127) {
      if (cursor) eraseRange(cursor - 1, cursor);
    } else if (c == '\n' || c == '\r') {
      insertAtCursor('\n');
    } else if (c >= 32) {
      insertAtCursor(c);
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
  case 'i': enterInsertMode(); break;
  case 'a': if (cursor < document.size()) ++cursor; enterInsertMode(); break;
  case 'A': cursor = lineEnd(cursor); enterInsertMode(); break;
  case 'I': cursor = lineStart(cursor); enterInsertMode(); break;
  case 'o': {
    const size_t at = lineEnd(cursor);
    cursor = at;
    enterInsertMode();
    insertAtCursor('\n');
    break;
  }
  case 'O': {
    cursor = lineStart(cursor);
    enterInsertMode();
    insertAtCursor('\n');
    --cursor;
    break;
  }
  case 'h': if (cursor) --cursor; break;
  case 'l': if (cursor < document.size() && document[cursor] != '\n') ++cursor; break;
  case 'j': moveVertical(1); break;
  case 'k': moveVertical(-1); break;
  case 'w': moveWordForward(); break;
  case 'b': moveWordBackward(); break;
  case '0': cursor = lineStart(cursor); break;
  case '^': {
    cursor = lineStart(cursor);
    while (cursor < lineEnd(cursor) && document[cursor] == ' ') ++cursor;
    break;
  }
  case '$': cursor = lineEnd(cursor); if (cursor) --cursor; break;
  case 'G': cursor = document.empty() ? 0 : document.size() - 1; break;
  case 'x': if (cursor < document.size()) eraseRange(cursor, cursor + 1); break;
  case 'd': case 'c': case 'y': pendingOperator = c; break;
  case 'p':
    if (!yankBuffer.empty()) {
      rememberUndo();
      undoGroupActive = true;
      ++cursor;
      for (char ch : yankBuffer) insertAtCursor(ch);
      undoGroupActive = false;
    }
    break;
  case 'u':
    if (!undoBuffer.empty()) {
      std::swap(document, undoBuffer);
      cursor = std::min(cursor, document.empty() ? size_t(0) : document.size() - 1);
      dirty = true;
      undoGroupActive = false;
    }
    break;
  case 'v': visualMode = !visualMode; visualAnchor = cursor; break;
  default: break;
  }
  if (visualMode && (c == 'd' || c == 'x')) {
    const size_t first = std::min(cursor, visualAnchor);
    const size_t last = std::max(cursor, visualAnchor) + 1;
    eraseRange(first, last);
    visualMode = false;
  } else if (visualMode && c == 'y') {
    const size_t first = std::min(cursor, visualAnchor);
    const size_t last = std::max(cursor, visualAnchor) + 1;
    yankBuffer.assign(document.data() + first, last - first);
    visualMode = false;
  }
}

void handleSpecial(KeyKind kind) {
  if (kind == KEY_CTRL_O && editorVisible) {
    showBrowser();
    return;
  }
  if (kind == KEY_CTRL_S) {
    saveDocument();
    return;
  }
  if (!editorVisible) {
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
    if (actionsVisible) {
      if (kind == KEY_ESCAPE) actionBackClicked(nullptr);
      else if (kind == KEY_UP || kind == KEY_DOWN) {
        if (kind == KEY_DOWN) actionSelection = (actionSelection + 1) % 3;
        else actionSelection = (actionSelection + 2) % 3;
        buildActionMenuScreen();
      } else if (kind == KEY_ENTER) runAction(actionSelection);
      return;
    }
    if (settingsVisible) {
      if (kind == KEY_ESCAPE) backClicked(nullptr);
      else if (kind == KEY_LEFT || kind == KEY_RIGHT) {
        int32_t next = brightnessValue + (kind == KEY_RIGHT ? 16 : -16);
        if (next < BRIGHTNESS_MIN) next = BRIGHTNESS_MIN;
        if (next > BRIGHTNESS_MAX) next = BRIGHTNESS_MAX;
        setBrightnessValue(static_cast<uint8_t>(next));
      }
      return;
    }
    if (kind == KEY_ESCAPE) {
      editorVisible = true;
      buildEditorScreen();
    } else if (kind == KEY_UP || kind == KEY_DOWN) {
      const int step = kind == KEY_UP ? -1 : 1;
      if (!markdownFiles.empty()) {
        browserSelection = (browserSelection + markdownFiles.size() + step) %
                           markdownFiles.size();
        buildBrowserScreen();
      }
    } else if (kind == KEY_ENTER && browserSelection < markdownFiles.size()) {
      loadDocument(String("/WriterInAWell/") + markdownFiles[browserSelection]);
      buildEditorScreen();
    }
    return;
  }
  if (kind == KEY_ESCAPE) {
    if (insertMode) {
      leaveInsertMode();
      if (cursor) --cursor;
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
    case KEY_BACKSPACE: if (cursor) eraseRange(cursor - 1, cursor); break;
    case KEY_DELETE: if (cursor < document.size()) eraseRange(cursor, cursor + 1); break;
    case KEY_ENTER: insertAtCursor('\n'); break;
    case KEY_LEFT: if (cursor) --cursor; break;
    case KEY_RIGHT: if (cursor < document.size()) ++cursor; break;
    case KEY_UP: moveVertical(-1); break;
    case KEY_DOWN: moveVertical(1); break;
    default: break;
    }
  } else {
    switch (kind) {
    case KEY_LEFT: if (cursor) --cursor; break;
    case KEY_RIGHT: if (cursor < document.size()) ++cursor; break;
    case KEY_UP: moveVertical(-1); break;
    case KEY_DOWN: moveVertical(1); break;
    case KEY_HOME: cursor = lineStart(cursor); break;
    case KEY_END: cursor = lineEnd(cursor); if (cursor) --cursor; break;
    case KEY_PAGE_UP: for (int i = 0; i < VISIBLE_LINES; ++i) moveVertical(-1); break;
    case KEY_PAGE_DOWN: for (int i = 0; i < VISIBLE_LINES; ++i) moveVertical(1); break;
    default: break;
    }
  }
}

void processKeyboard() {
  KeyEvent event;
  while (keyboardNext(event)) {
    lastInputMs = millis();
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
  if (!editorVisible) return;
  const String keyboard = keyboardStatus();
  const bool connected = keyboard.startsWith("Connected:");
  lv_label_set_text(radioLabel, ICON_BLUETOOTH);
  lv_obj_set_style_text_color(radioLabel,
      connected ? lv_color_hex(0x8DE6B3) : lv_color_hex(0x78C8F4), 0);
  const char *modeText;
  uint32_t modeBg;
  if (insertMode) { modeText = "INSERT"; modeBg = 0x78C8F4; }
  else if (visualMode) { modeText = "VISUAL"; modeBg = 0xC792EA; }
  else if (commandMode || searchMode) {
    modeText = commandMode ? "COMMAND" : "SEARCH";
    modeBg = 0xFFD866;
  }
  else { modeText = "NORMAL"; modeBg = 0x4CCB8B; }
  lv_label_set_text(modeLabel, modeText);
  lv_obj_set_style_bg_color(modeLabel, lv_color_hex(modeBg), 0);
  // Bottom hint line: the mode-switch keys in Normal, the way back to Normal
  // in Insert/Visual, or the live ":" / "/" command line while one is typed.
  if (commandMode || searchMode) {
    const String prompt = commandMode ? ":" : "/";
    lv_label_set_text(hintLabel, (prompt + String(commandLine.c_str())).c_str());
    lv_obj_set_style_text_color(hintLabel, lv_color_hex(0xFFD866), 0);
  } else {
    lv_label_set_text(hintLabel,
        (insertMode || visualMode) ? "esc normal" : "i write  v visual");
    lv_obj_set_style_text_color(hintLabel, lv_color_hex(0x71858F), 0);
  }
  String status = statusMessage.c_str();
  if (transferEnabled) status = "WiFi share  192.168.4.1";
  lv_label_set_text(statusLabel, status.c_str());
  if (transferEnabled) {
    lv_label_set_text(footerLabel, wifiPassword.c_str());
  } else if (millis() - lastTouchMs < 1800) {
    char touchText[20];
    snprintf(touchText, sizeof(touchText), "T%03u,%03u",
             unsigned(lastTouchX), unsigned(lastTouchY));
    lv_label_set_text(footerLabel, touchText);
    showingTouchCoords = true;
  } else {
    const size_t line = lineNumber(cursor) + 1;
    const size_t col = columnFor(cursor) + 1;
    char position[20];
    snprintf(position, sizeof(position), "%03u:%02u", unsigned(line), unsigned(col));
    lv_label_set_text(footerLabel, position);
    showingTouchCoords = false;
  }
  lv_label_set_text(powerLabel, "5V");
  if (settingsRadioLabel) {
    lv_label_set_text(settingsRadioLabel, keyboardStatus());
    lv_obj_set_style_text_color(settingsRadioLabel,
        connected ? lv_color_hex(0x8DE6B3) : lv_color_hex(0xD4DFE5), 0);
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
        if (originalName.endsWith(".md") || originalName.endsWith(".markdown")) {
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
  document.reserve(documentLimit);
  undoBuffer.reserve(documentLimit);
  display.init();
  display.setRotation(1);
  loadPrefs();
  display.setBrightness(brightnessValue);

  lv_init();
  editorGlyphWidth = lv_font_get_glyph_width(editorFont(), '0', 0);
  if (editorGlyphWidth == 0) editorGlyphWidth = 9;
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
  keyboardBegin();
  lastInputMs = lastEditMs = millis();
  buildEditorScreen();
  if (cardAvailable) statusMessage = "SD ready — press i to write";
  else statusMessage = "No SD card — draft is temporary";
  uiNeedsRender = true;
}

void loop() {
  processKeyboard();
  if (transferEnabled) transferServer.handleClient();
  if (pairingVisible && keyboardCandidateCount() != pairListRenderedCount) {
    buildPairingScreen();
  }
  if (editorVisible && showingTouchCoords &&
      millis() - lastTouchMs >= 1800) {
    uiNeedsRender = true;
  }
  if ((editorVisible || settingsVisible || pairingVisible) &&
      millis() - lastRadioUpdateMs >= 1000) {
    lastRadioUpdateMs = millis();
    if (editorVisible) {
      uiNeedsRender = true;
    } else if (settingsRadioLabel) {
      const String keyboard = keyboardStatus();
      lv_label_set_text(settingsRadioLabel, keyboard.c_str());
      lv_obj_set_style_text_color(settingsRadioLabel,
          keyboard.startsWith("Connected:") ? lv_color_hex(0x8DE6B3)
                                              : lv_color_hex(0xD4DFE5), 0);
    } else if (pairingStatusLabel) {
      const String keyboard = keyboardStatus();
      lv_label_set_text(pairingStatusLabel, keyboard.c_str());
      lv_obj_set_style_text_color(pairingStatusLabel,
          keyboard.startsWith("Connected:") ? lv_color_hex(0x8DE6B3)
                                              : lv_color_hex(0xD4DFE5), 0);
    }
  }
  if (editorVisible && uiNeedsRender) {
    updateEditorLines();
    updateStatusBar();
    uiNeedsRender = false;
  }
  // Blink the caret cursor independently of full-screen redraws.
  if (editorVisible && cursorBlock && millis() - lastCursorBlinkMs >= 480) {
    lastCursorBlinkMs = millis();
    cursorBlinkOn = !cursorBlinkOn;
    lv_obj_set_style_bg_opa(cursorBlock,
        cursorBlinkOn ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
  }
  if (dirty && cardAvailable && millis() - lastEditMs > 5000) {
    saveDocument();
  }
  if (!backlightDimmed && millis() - lastInputMs > 60000) {
    display.setBrightness(48);
    backlightDimmed = true;
  }
  lv_timer_handler();
  delay(8);
}
