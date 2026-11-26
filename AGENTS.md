# AGENTS.md — WriterInAWell

Operating instructions for AI coding sessions in this repository. Read this, then
`README.md` (product overview) and `docs/USAGE.md` (user-facing keymap and flows).

## What this is

An offline-first Markdown writing appliance for the **WT32-SC01 Plus** board:
ESP32-S3, 480×320 ST7796 parallel TFT, FT6336U touch, microSD (the canonical
document store), BLE HID keyboard input, and an optional temporary Wi-Fi AP for
file transfer. Single LVGL 8.3 app, Arduino framework, PlatformIO build.

`src/main.cpp` (~3,900 lines, file-locals in an anonymous namespace) is the whole
application. `src/ble_keyboard.cpp` is the NimBLE HID keyboard client.
`include/` holds only `ble_keyboard.h` and `lv_conf.h` — there is no app header.

## Build / verify

```sh
pio run                                          # build — the only automated check
pio run -t upload --upload-port /dev/ttyACM0     # flash (overwrites firmware!)
pio device monitor --port /dev/ttyACM0 -b 115200 # serial console (native USB CDC)
```

- One environment: `writerinawell` (platform `espressif32`, board
  `esp32-s3-devkitc-1`, framework `arduino`). The board ID is only a convenient
  PlatformIO target — the real board is the WT32-SC01 Plus and **all pins are
  hardcoded in `WriterDisplay` in `src/main.cpp`**. There is no custom board JSON.
- There are **no tests and no CI**. Verification = `pio run` plus on-hardware
  behavior. A session cannot run the hardware, so at minimum every change must
  build cleanly.
- `Serial` output goes over native USB-Serial/JTAG (`ARDUINO_USB_CDC_ON_BOOT=1`)
  at 115200 baud.

### Build config that must not be "cleaned up"

- `board_build.partitions = default_16MB.csv` overrides the board JSON default
  (`default_8MB.csv`). Removing it breaks the 16 MiB flash layout.
- PSRAM requires all of: `board_build.psram = enabled`,
  `board_build.psram_type = qspi`, `board_build.arduino.memory_type = qio_qspi`,
  and `-DBOARD_HAS_PSRAM`. Without PSRAM the app still runs, but the document cap
  drops 512 KiB → 32 KiB and the LVGL draw buffer falls back to internal RAM.
- `-DLV_CONF_INCLUDE_SIMPLE` selects the custom `include/lv_conf.h`.

## Source map

| Path | Role |
| --- | --- |
| `src/main.cpp` | Whole app: display/touch config, LVGL plumbing, document model, syntax highlighter and language tables, editor/Vim logic, Wi-Fi transfer server, settings/browser/pairing screens, NVS prefs |
| `src/ble_keyboard.cpp` | NimBLE client: scan, pairing list (max 6), 8-byte boot-report parser, auto-repeat |
| `include/ble_keyboard.h` | `KeyKind` enum, `KeyEvent`, public BLE functions |
| `include/lv_conf.h` | LVGL 8 config: 16-bit color, 48 KiB internal LVGL heap, compressed fonts on |
| `src/fonts/*.c` | **Generated** LVGL fonts — do not hand-edit; regenerate via `tools/build-fonts.sh` |
| `tools/build-fonts.sh` | Font regeneration (needs Node.js + network; `npx lv_font_conv`) |
| `assets/fonts/` | TTF sources + SIL OFL license texts |
| `docs/USAGE.md` | Canonical keymap and user flows — keep in sync with code |
| `backups/` | Original 16 MiB factory flash dump + manifest; restore docs inside `backups/README.md` |

## Hardware facts that shape the design

- **BLE only** — ESP32-S3 has no Bluetooth Classic. A Classic-only keyboard can
  never work; don't try to add support.
- **No battery gauge, no RTC.** The UI deliberately reports `5V` and no clock.
  Do not fabricate a battery percentage or timestamp; that needs new hardware.
- Wi-Fi is **off by default** to save power. It exists only for the file-transfer
  AP (default SSID `WriterInAWell` and passphrase `well-XXXXXX`, both seeded
  from the eFuse MAC on first boot, editable on the Wi-Fi screen, and saved in
  NVS; the live address is shown on that screen, normally
  `http://192.168.4.1/`; max 1 client). The name/password fields lock while
  sharing is on. The served page is a self-contained file-browser view in the
  device palette (name, size, SD-modified time, card usage; timestamps are
  hidden when the card clock is unusable).
- Pin assignments (do not change without the WT32-SC01 Plus schematic):
  - Display: 8-bit parallel ST7796, WR=47, RS=0, D0..D7 = 9, 46, 3, 8, 18, 17,
    16, 15, RST=4, 40 MHz. Backlight PWM on GPIO 45 (44.1 kHz).
  - Touch: FT6336U, I²C port 1, addr 0x38, SDA=6, SCL=5, INT=7. Reports are
    offset empirically by (+10, +10) — see `TOUCH_OFFSET_X/Y`; don't remove.
  - microSD: SPI at 20 MHz, SCK=39, MISO=38, MOSI=40, CS=41. Canonical dir
    `/WriterInAWell` (auto-created).
  - Strapping/JTAG overlap: GPIO 0, 45, 46 are strapping pins used by the
    display/backlight; GPIO 39/40/41 are JTAG pins used by SD. Hardware JTAG
    debugging conflicts — the supported debug path is the USB-Serial/JTAG console.
- Fixed screen geometry: 480×320; the editor's band count and row pitch come
  from the chosen 10/12/14/16 px font (13 lines × 17 px at the 14 px default)
  via `refreshEditorMetrics()` and `MAX_VISIBLE_LINES`. The **editor** header is
  36 px tall with 28 px buttons (`makeToolbarButton`), while every other screen
  keeps the 30 px / 22 px bar (`makeButton`). `editorCanvasY()/editorCanvasH()`
  therefore give the page y 41..270 — keep `229 - 2 - 6 = 221` px of text area
  or the 14 px default silently drops to 12 lines.

## Behaviors to preserve

- **Screen flow:** with no password, boot lands on the four-tile main menu
  (Text Editor, Typing Practice, Share Over Wi-Fi, Settings; no save tile, no
  status line — `:w` saves); with one set, boot stops at the lock screen
  first. Each tile carries a dim `1`–`4` badge in its top-right corner (the
  `shortcut` argument of `makeMenuTileButton`) matching the `1`–`4` jump keys.
  Text Editor resumes an open buffer, or shows the start screen (`n`
  new, `f` filtered find, `r` recents, `q` menu) when none is open. `q`/`:q`
  closes a clean buffer back to the menu; `:menu` returns without closing it.
  Share Over Wi-Fi opens a dedicated Wi-Fi screen (toggle, editable name and
  password, live address) rather than toggling the AP directly. Sub-screens'
  BACK uses `backTarget` (menu / editor / start screen).
- **Lock screen:** a password set in Settings → Security makes the login screen
  the boot screen and the long-idle light-sleep wake screen; `Ctrl-L`, `:lock`
  and Security → Lock now lock on demand. `lockActive` makes `handleText()` and
  `handleSpecial()` route every key to `lockHandleText()`/`lockHandleSpecial()`
  before any other screen logic; the on-screen 4×10 keypad calls the same
  append/backspace/submit helpers. The hash is salted FNV-1a 64 over 4–24
  printable ASCII chars, stored as `lockhlo`/`lockhhi`/`locksalt` with
  `lockon` as the authority — never store the plaintext. This is a UI gate, not
  disk encryption; do not claim otherwise.
- **Storage:** any extension the editor has a grammar for (Markdown, plain text,
  HTML, CSS, Python, C/C++, JavaScript, JSON, shell, YAML — see
  `docLangForName()`); the browser lists only those, and an unknown extension
  opens as plain text. CRLF is normalised to LF on load. 512 KiB cap with PSRAM
  (32 KiB without) — larger files are refused, not truncated. Saves write
  `.tmp`, keep a `.bak`, then rename; `recoverSaveBackups()` restores orphaned
  `.bak` at boot. Don't add code that blindly deletes `.tmp`/`.bak`/
  `.incoming.tmp` files.
- **Editor:** Vim-flavored subset (Normal/Insert/Visual, motions, `dd/yy/p/u`,
  `:w :q :e` command line). Search is literal with smart case (an all-lowercase
  pattern ignores case, any capital makes it exact); `/pattern` starts just
  after the caret and wraps to the top when the only match is above it (the
  prompt reads `find: `), while `n`/`N` wrap only when Search wrap is on. Long
  lines follow the **Word Wrap** setting (`ww`, default on): wrap folds them
  into extra screen bands with no newlines inserted — a line's bands share its
  gutter number, and the base heading/quote/code style carries across folds;
  wrap off scrolls horizontally instead, jumping ~80% of the band width once
  the caret reaches ~95% (it parks ~15% in when moving back left). The column
  window is `editorViewportCols()` = (label width − 4 px pad) ÷ measured glyph
  advance: 49 columns at 14 px (74/64/44 at 10/12/16 px), 46 with the gutter
  at 14 px; wrap width, scroll window, and caret math all share it. The
  header's A−/A+ pair steps the size and saves it under `"esize"`. Viewport
  anchors: `viewportStart` plus `viewportWrapCol` (wrap: top band's soft-break
  column) or `viewportScrollCol` (no wrap). A span's colour now carries across
  a fold: the highlighter re-parses the whole line and writes only the band's
  columns, and
  `Markup` closes its run at the end, so every band stays balanced. The caret
  is `cursorBlock` (a filled cell) plus `cursorGlyph` (one reversed-out
  character in the band's own bg); `setCursorPhase()` alternates them every
  480 ms between the solid light-blue block and a 1 px outline of the same
  cell, so the cell size never changes. Normal-mode `+`/`=` and `-` step the
  text size (the `:font` command's keyboard twin). In Settings, the Editor
  popup's row 0 is the **Text size** `Value` row — `SETTINGS_EDITOR_ROWS` is
  index-checked by number in `settingsToggleValue`/`settingsToggleMessage`/
  `settingsApplyToggle`, so a row inserted above the toggles renumbers all
  three switches.
- **Insert mode** indents with spaces: `Tab` goes to the next 2-space stop
  (`EDITOR_TAB_SPACES`, not configurable), `Enter` keeps the line's indent and
  adds one level after `(`/`[`/`{` or a Python `:`, and splits `{|}` onto three
  lines; `o`/`O` copy the neighbouring indent. `(`, `[`, `{` auto-pair (quotes
  only in code languages), a typed closer skips over its partner, and Backspace
  removes an empty pair as a unit. Anything that changes this must keep
  `insertTypedChar()`/`backspaceInEditor()`/`insertNewlineInEditor()` in sync.
- **Dirty guard:** opening, creating, or closing another document is blocked
  while unsaved changes exist. Respect it or save first.
- **Auto-save** runs 5 s after the first edit; `Ctrl-S`/`:w` also saves.
- **Syntax highlighting** is one LVGL recolor string per visible band, built by
  `lexLine()`/`syntaxRender()` into a shared 1 KiB `Markup` buffer (no
  allocation on the keystroke path). Grammar is picked by `DocLang`
  (`editorFileType`); the extension decides it on open and the Settings picker
  overrides it. Multiline constructs (block comments, fences, `<script>`,
  triple quotes) are carried by a one-byte state; `syntaxStateAt()` rebuilds the
  state at the viewport by scanning from the document start, bounded by
  `SYNTAX_SCAN_BYTES`. Spans must stay balanced and literal `#` must be escaped
  (`##`), otherwise text vanishes from the UI. A span must also never be split
  by a line break: LVGL restarts its recolor parser at every line and would read
  the closing `#` as a new command, printing the next span's hex as text.
  `lv_conf.h` therefore sets `LV_TXT_BREAK_CHARS " "`, and the only wrapping
  recolor label (the typing word stream) keeps its spans to one space-free
  token each. Every column in a band must reach the builder — a lexer branch
  that skips characters silently deletes them from the screen.
- **Typing practice** generates a sentence-aware stream: clauses of 3–7 words,
  capitalization only at sentence starts and in curated acronyms, punctuation
  chosen by clause position, curated contractions/compounds/acronyms, numbers
  as standalone tokens, and only bracket/asterisk/hyphen symbols. The stats
  area is a segmented bar of tinted LVGL cells (live row and two-row summary),
  not a recolor label — LVGL needs a literal space after `#RRGGBB`, so
  `#RRGGBB|#` separators leak the hex text into the UI. The run info line sits
  in the strip above the hint row, not inside the panel.
- **BLE keyboard parsing** assumes the standard 8-byte boot report (optionally
  prefixed with report ID 1) and a US layout. `Ctrl-S`, `Ctrl-O`, `Ctrl-R`,
  `Ctrl-Z`, `Ctrl-L`, `Ctrl-Enter` and `Shift-Tab` are special-cased; the rest
  arrives as text. Descriptor-aware decoding / Unicode is known follow-up work.
- **NVS:** namespace `"wiaw"`, keys `"bl"` (backlight), `"smooth"` (text
  smoothing), `"hl"` (text colors / syntax highlight), `"as"` (auto-save),
  `"dim"` (idle dimming), `"ln"` (line numbers), `"ww"` (word wrap),
  `"esize"` (editor font size index, default 2 = 14 px, range-checked),
  `"ftype"` (default `DocLang`, one byte, range-checked on load),
  `"ssid"` (Wi-Fi hotspot name, max 32 chars), `"wpass"` (Wi-Fi passphrase,
  8–63 chars), `"lockon"` (bool; password is set), `"locksalt"`, `"lockhlo"`,
  `"lockhhi"` (salted 64-bit FNV-1a password hash, never plaintext),
  `"recent"` (newline-joined recent-file paths, newest first, max 5), plus
  typing-practice keys: `"tb20"`/`"tblong"` (personal-best WPM for the 20-word
  Short run and the Long run, written immediately when a run sets a new
  record) and
  `"tcomp"`/`"tmax"`/`"tsym"`/`"tnum"`/`"tcap"`/`"tpunc"` (complexity, max
  length, symbols, numbers, capitalization, punctuation); bools default on
  except smoothing and line numbers. Backlight range 24–255, default 170,
  dims to 48 after 60 s unless the dim toggle is off.

## Fonts and icons

- Fifteen fonts in `src/fonts/` are generated by `tools/build-fonts.sh` from
  `assets/fonts/` TTFs. Each file records its exact `lv_font_conv` command.
- `LV_USE_FONT_COMPRESSED` must stay `1` (eleven of fifteen fonts are
  RLE-compressed; disabling it renders all text blank). The four
  `space_mono_*_cr.c` cuts (10/12/14/16 px) are deliberately 1-bpp
  **uncompressed** for the crisp mode.
- New Nerd Font icons require adding their codepoint to `ICON_RANGES` in
  `tools/build-fonts.sh` and regenerating, or the glyph renders blank/fallback.

## Conventions

- Everything in English (comments, identifiers, UI labels, docs). UI buttons in
  UPPERCASE; settings toggle names are mixed case with the shortcut letter
  underlined; status messages sentence case.
- 2-space indent, braces on the same line, `camelCase` functions/variables,
  `PascalCase` classes, `constexpr` `UPPER_SNAKE_CASE` constants. Comments
  explain *why* (hardware quirks, pixel math), not *what*.
- Types: `DocumentString` (PSRAM-backed `DocumentAllocator`) for document, undo,
  and yank buffers; `std::string` for filenames/UI text; Arduino `String` only at
  SD/Wi-Fi API boundaries; `snprintf` into LVGL labels.
- UI: imperative LVGL **v8** API (`lv_disp_drv_t`, `lv_scr_act()`,
  `lv_obj_clean()` full-screen rebuilds, recolor labels), global widget
  pointers, absolute pixel layout on 480×320, hardcoded hex palette. Do not
  introduce LVGL v9 APIs.
- Text labels that must not bleed into a neighbour need **both** a bounded
  width and a bounded height: `LV_LABEL_LONG_DOT` only elides when the text
  overflows the label's own box. With `LV_SIZE_CONTENT` height it grows to a
  second line and prints over the row below (this was the `?` help sheet bug;
  `renderHelpColumn` now sets one line of `elms_bold_12` = 14 px).
- Memory: LVGL heap is 48 KiB internal; document/undo buffers reserve 512 KiB
  PSRAM each. Keep allocations out of `loop()`.
- After changing keybindings or UI behavior, update `docs/USAGE.md`, the in-UI
  hint strings, and `README.md` in the same change.

## Hard rules

1. **Never edit anything under `.pio/`** (build output + vendored library
   sources, ~377 MB). Project code lives in `src/` and `include/`; change
   dependency versions via `platformio.ini` instead.
2. **Do not reassign GPIO pins** without checking the WT32-SC01 Plus schematic —
   several are strapping/JTAG pins (see above).
3. **Do not remove `TOUCH_OFFSET_X/Y`, the 16 MB partition override, the PSRAM
   flags, or `LV_USE_FONT_COMPRESSED`.**
4. **Do not invent battery/RTC features** — the board has no gauge circuitry and
   no RTC.
5. **Flashing replaces firmware.** The original factory image is
   `backups/wt32-sc01-plus-original-flash.bin` — but it is **gitignored**, so
   don't assume it exists; verify its SHA-256 against `backups/README.md` before
   any restore.
6. Preserve font licenses: Elms Sans and Space Mono are SIL OFL 1.1 (keep the
   `*-OFL.txt` files); LovyanGFX/LVGL/NimBLE-Arduino are MIT/BSD/Apache-2.0.
   The project source itself is MIT-licensed — see `LICENSE`.

## Repo state notes

- **This directory is not a git repository** (no `.git`; a `.gitignore` exists
  for when it becomes one). There is no history — the working tree is the source
  of truth, and there are no branches, remotes, or CI to consult.
- `.gitignore` excludes `.pio/`, `.vscode/`, and `backups/*.bin`/`*.sha256`, so
  a fresh checkout lacks build artifacts and the flash dump.
- Orientation order for a new session: `README.md` → `docs/USAGE.md` →
  `platformio.ini` → `src/main.cpp` → `src/ble_keyboard.cpp` → `include/lv_conf.h`.
