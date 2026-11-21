# WriterInAWell

An offline-first Markdown writing appliance for the **WT32-SC01 Plus**.
The project targets its ESP32-S3, 480×320 ST7796 parallel display, FT6336U
touchscreen, microSD slot, and BLE radio.

## Hardware facts that shape the design

- The ESP32-S3 supports **Bluetooth Low Energy (BLE), not Bluetooth Classic**.
  Use a BLE HID keyboard. Classic-only keyboards cannot pair with this board.
- The board has no documented battery gauge or battery input. The UI therefore
  reports USB/external power rather than inventing a battery percentage. A
  future battery pack needs a voltage-divider/ADC or fuel-gauge circuit.
- Display pins and SD pins are board-specific; see `src/main.cpp`.

## Build

Install PlatformIO, then from this directory run:

```sh
pio run
pio run -t upload --upload-port /dev/ttyACM0
```

Flashing replaces the existing firmware. The full original 16 MiB image is
backed up in `backups/wt32-sc01-plus-original-flash.bin`; verify its hash in
`backups/README.md` before flashing.

## First implementation

- Dark, compact UI with green/blue accents and keyboard-first navigation. A
  low-profile breadcrumb header (hamburger menu on the left, gear on the right)
  keeps the editor as large as possible.
- SD-card Markdown browser and editor; `Ctrl-S` / `:w` save to `.md` (up to
  512 KiB with PSRAM, enough for many long-form drafts). A blinking caret marks
  the cursor.
- Markdown syntax highlighting in a warm palette (orange headings, purple bold,
  peach italics, yellow bullets) with balanced inline spans that don't vanish.
- Password-protected Wi-Fi sharing (download/upload `.md` files from a phone or
  computer browser); off by default, toggled from the file menu or `:wifi`.
- Settings screen for backlight levels and the BLE keyboard picker.
- Vim-style Normal/Insert/Visual modes, motions, line operations, and command
  line basics. This is a focused embedded subset, **not a complete Neovim
  implementation**.
- BLE HID discovery and explicit keyboard selection for devices exposing the
  standard 8-byte boot input report. HID report layouts vary; some keyboards
  will need descriptor-aware decoding to work.
- Wi-Fi is off by default to preserve power. File transfer is via the local
  password-protected Wi-Fi page; Bluetooth is used for the keyboard, not file
  transfer. Richer Vim operators/macros, robust Unicode/input-method support,
  and battery telemetry are follow-up work.

The code uses the board’s microSD card as the canonical document store.
Documents are plain Markdown and can also be transferred through the temporary
Wi-Fi access point (`WriterInAWell`, password shown on the status bar) at
`http://192.168.4.1/`. The clock is not yet implemented; the board has no RTC,
and Wi-Fi remains off unless sharing is enabled. UI text uses bold Elms Sans;
the editor text and icons use the Space Mono Nerd Font (see
`assets/fonts/README.md`). The header breadcrumb shows `~/` for the SD root,
folder icons for subfolders, and the current filename in italics when unsaved.

See [`docs/USAGE.md`](docs/USAGE.md) for the current keymap, file-transfer flow,
and power behavior.
