# AGENTS.md — WriterInAWell

Operating instructions for AI coding sessions in this repository. Read this, then
`README.md` (product overview) and `docs/USAGE.md` (user-facing keymap and flows).

## What this is

An offline-first Markdown writing appliance for the **WT32-SC01 Plus** board:
ESP32-S3, 480×320 ST7796 parallel TFT, FT6336U touch, microSD (the canonical
document store), BLE HID keyboard input, and an optional temporary Wi-Fi AP for
file transfer. Single LVGL 8.3 app, Arduino framework, PlatformIO build.

`src/main.cpp` (~2,200 lines, file-locals in an anonymous namespace) is the whole
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
| `src/main.cpp` | Whole app: display/touch config, LVGL plumbing, document model, Markdown highlighter, editor/Vim logic, Wi-Fi transfer server, settings/browser/pairing screens, NVS prefs |
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
  AP (SSID `WriterInAWell`, WPA2 password `well-XXXXXX` derived from the eFuse
  MAC, `http://192.168.4.1/`, max 1 client).
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
- Fixed screen geometry: 480×320, editor shows 13 lines × 17 px
  (`SCREEN_W/H`, `VISIBLE_LINES`, `LINE_HEIGHT`).

## Behaviors to preserve

- **Storage:** `.md`/`.markdown` only; 512 KiB cap with PSRAM (32 KiB without) —
  larger files are refused, not truncated. Saves write `.tmp`, keep a `.bak`, then
  rename; `recoverSaveBackups()` restores orphaned `.bak` at boot. Don't add code
  that blindly deletes `.tmp`/`.bak`/`.incoming.tmp` files.
- **Editor:** Vim-flavored subset (Normal/Insert/Visual, motions, `dd/yy/p/u`,
  `:w :q :e` command line). Search is forward-only and literal, no wrap. Editor
  lines render truncated at 64 columns (caret math shares this) — a known
  limitation, not a bug to silently "fix".
- **Dirty guard:** opening/creating another file is blocked while unsaved
  changes exist. Respect it or save first.
- **Auto-save** runs 5 s after the first edit; `Ctrl-S`/`:w` also saves.
- **Markdown highlighting** is an LVGL recolor string from `markdownMarkup()`:
  spans must stay balanced and literal `#` must be escaped (`##`), otherwise
  text vanishes from the UI.
- **BLE keyboard parsing** assumes the standard 8-byte boot report (optionally
  prefixed with report ID 1) and a US layout. Only `Ctrl-S`/`Ctrl-O` are
  special-cased. Descriptor-aware decoding / Unicode is known follow-up work.
- **NVS:** namespace `"wiaw"`, keys `"bl"` (backlight) and `"smooth"`
  (text smoothing). Backlight range 24–255, default 170, dims to 48 after 60 s.

## Fonts and icons

- Seven fonts in `src/fonts/` are generated by `tools/build-fonts.sh` from
  `assets/fonts/` TTFs. Each file records its exact `lv_font_conv` command.
- `LV_USE_FONT_COMPRESSED` must stay `1` (six of seven fonts are RLE-compressed;
  disabling it renders all text blank). The exception, `space_mono_14_cr.c`, is
  deliberately 1-bpp **uncompressed** for the crisp mode.
- New Nerd Font icons require adding their codepoint to `ICON_RANGES` in
  `tools/build-fonts.sh` and regenerating, or the glyph renders blank/fallback.

## Conventions

- Everything in English (comments, identifiers, UI labels, docs). UI buttons in
  UPPERCASE; status messages sentence case.
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
