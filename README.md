# WriterInAWell

An offline-first Markdown writing appliance for the **WT32-SC01 Plus**.
The project targets its ESP32-S3, 480×320 ST7796 parallel display, FT6336U
touchscreen, microSD slot, and BLE radio.

## Hardware facts that shape the design

- The ESP32-S3 supports **Bluetooth Low Energy (BLE), not Bluetooth Classic**.
  Use a BLE HID keyboard. Classic-only keyboards cannot pair with this board.
- The board has no documented battery gauge or battery input, so the UI
  reports USB/external power (`5V`) rather than inventing a battery
  percentage. An opt-in `-DWIAW_BATTERY_SENSE` build flag turns the status
  indicator into a real gauge once a 100 kΩ/100 kΩ + 100 nF divider is wired
  to EXT_IO1 (GPIO10); see docs/USAGE.md.
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

- Dark, compact UI with green/blue accents and keyboard-first navigation.
  Boots to a four-tile main menu (Text Editor, Typing Practice, Share Over
  Wi-Fi, Settings) with no save tile and no status line; `:w` saves. A
  low-profile breadcrumb header (hamburger menu on the left, Bluetooth
  picker shortcut and gear on the right) keeps the editor as large as
  possible.
- SD-card text browser and editor; `Ctrl-S` / `:w` save (up to
  512 KiB with PSRAM, enough for many long-form drafts). A blinking caret
  marks the cursor — a light blue cell that reverses the character under it,
  alternating with a 1 px outline of the same cell so the character stays
  visible. The main menu's tiles print their `1`–`4` shortcut in the corner.
  Long lines word-wrap by default — folded rows keep the
  number of the line they started from and no newlines are added; switch
  the setting off for sideways scrolling that keeps the caret on screen.
- Adjustable editor text size from the header's A−/A+ pair, `+`/`-` in Normal
  mode, `:font 16`, or the Text size row in Settings → Editor: 10/12/14/16 px,
  with the line pitch and visible rows following (18 down to 12 rows); the
  choice is saved and applies to the rendered preview too.
- LazyVim-style editor start screen (`n` new, `f` filtered find, `r` recent
  files, `q` menu), with recent documents persisted in NVS; `q`/`:q` closes a
  clean buffer back to the menu. Recent entries remember their caret offset,
  so reopening a draft resumes where you stopped.
- Editing speed: `Ctrl-R` redo, `:42` goto-line, `n`/`N` repeat the last
  search, relative line numbers, a live word count in the footer,
  focus-line (typewriter) scrolling, and a Zen mode (`Ctrl-Z` or `:zen`) that
  hides the header, status bar and hints and expands the page. An opt-in
  Start in editor setting resumes the newest draft at power-on.
- Opt-in version snapshots keep the previous 3/5/10 saves in a hidden
  `.wiaw-versions` folder, with a `:history` screen to restore one as an
  unsaved edit. Idle power adds a configurable screen-off timeout and an
  experimental opt-in light sleep.
- Optional password lock: boots to a minimalist login screen and re-locks after
  light sleep, with `Ctrl-L` / `:lock` / Settings → Security → Lock now for an
  immediate lock. The password can come from the BLE keyboard or the on-screen
  4×10 keypad; setting, changing or removing it requires the current password.
  Only a salted hash is stored — it is a UI gate, not disk encryption.
- Syntax highlighting for Markdown, HTML, CSS, Python, C/C++, JavaScript,
  JSON, shell, YAML and plain text in one warm palette, with balanced inline
  spans that never vanish; Markdown gets per-level heading colours, links,
  task boxes, strikethrough and tables. Insert mode indents on Enter, inserts
  2-space tabs, and auto-pairs brackets and quotes. Opt-in Markdown list
  continuation keeps bullets/tasks/numbered items going on Enter,
  `Ctrl-Enter` flips a task box, and `:tpl` inserts a journal, meeting, blog,
  bug or daily-note skeleton. Opt-in prose punctuation types curly quotes,
  em dashes and ellipses; the editor counts columns in UTF-8 codepoints, so
  uploaded Markdown and those glyphs keep the caret and wrapping aligned.
- Read-only rendered preview (`:p`): headings without `#`, quote bars, tinted
  code blocks and wrapped lines, rendered from a PSRAM block table with the
  same fonts as the rest of the UI.
- Typing practice (main menu tile `2`): a monkeytype-style word
  sprint — a 20-word Short run or a Long run that fills the word area with
  however many words fit, live WPM/accuracy over a red/green word
  stream, and personal bests persisted to NVS. A `Ctrl-O` settings submenu
  picks word complexity, maximum word length, symbols, numbers,
  capitalization, and punctuation; the stats sit in a segmented,
  color-coded bar.
- Password-protected Wi-Fi sharing (download/upload text files from a phone or
  computer browser) from a dedicated screen with an on/off toggle, editable
  hotspot name and password, and the live address. The page is a file-browser
  view with size, modified date, and SD usage; off by default, opened from the
  main menu or `:wifi`.
- Settings as a seven-section list (Display, Editor, Writing, Files & Data,
  Power, Keyboard, Security) with live summary readouts, each opening a popup
  panel in the File Type picker's style. The popups hold the backlight slider,
  the editor and writing toggles, auto-save, idle dimming, light sleep, the BLE
  keyboard status/pairing rows, the file-type picker that remembers the default
  grammar for new buffers, and the password controls. The section layout keeps
  all seven rows on screen without a scrolling column.
- Vim-style Normal/Insert/Visual modes, motions, line operations, and command
  line basics. This is a focused embedded subset, **not a complete Neovim
  implementation**. Press `?` in Normal for the built-in cheat sheet of the
  keys that are actually implemented.
- BLE HID discovery and explicit keyboard selection for devices exposing the
  standard 8-byte boot input report. HID report layouts vary; some keyboards
  will need descriptor-aware decoding to work.
- Wi-Fi is off by default to preserve power. File transfer is via the local
  password-protected Wi-Fi page; Bluetooth is used for the keyboard, not file
  transfer. Richer Vim operators/macros, robust Unicode/input-method support,
  and battery charging-state telemetry are follow-up work.

The code uses the board’s microSD card as the canonical document store.
Documents are plain text (Markdown by default) and can also be transferred through the temporary
Wi-Fi access point (`WriterInAWell` by default, password shown on the Wi-Fi
screen; the live address appears there in bold soft green) at
`http://192.168.4.1/`. The clock is not yet implemented; the board has no RTC,
and Wi-Fi remains off unless sharing is enabled. UI text uses bold Elms Sans;
the editor text and icons use the Space Mono Nerd Font (see
`assets/fonts/README.md`). The header breadcrumb shows `~/` for the SD root,
folder icons for subfolders, and the current filename in reddish-pink italics
when unsaved or pastel mint when saved.

See [`docs/USAGE.md`](docs/USAGE.md) for the current keymap, file-transfer flow,
and power behavior.
