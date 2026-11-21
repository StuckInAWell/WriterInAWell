# WriterInAWell quick reference

## Storage and sharing

Put Markdown files in `/WriterInAWell` on the microSD card. The file browser
lists `.md` and `.markdown` files. New drafts auto-save every five seconds
after the first edit; use `:w filename.md` to choose a name. With the board's
2 MiB PSRAM, the editor supports documents up to 512 KiB; without PSRAM it
falls back to 32 KiB. Larger files are left untouched rather than truncated.

Wi-Fi sharing is intentionally off at boot. Open the file actions menu (the
hamburger button at the top-left) and choose **SHARE OVER WI-FI**, or use
`:wifi`. This starts the `WriterInAWell` WPA2 access point; the unique password
is shown in the bottom status bar. Connect a phone or computer, visit
`http://192.168.4.1`, and download or upload Markdown files. Use `:wifi-off`
when finished.

The header is a low-profile breadcrumb. `~/` stands for the `WriterInAWell`
root on the SD card; any subfolders appear with a folder icon, followed by the
current filename. Unsaved documents (including new `untitled.md`) show their
name in italics. The hamburger button on the left opens the file-actions menu
(browser, save, Wi-Fi share); the gear on the right opens Settings. The
Bluetooth icon shows scan/connected status; scanning starts automatically at
boot.

## Settings

Open Settings with the gear button or `:settings`:

- **Backlight** — a slider you can drag, or adjust with the `←`/`→` keys.
  Keys `1`–`4` jump to presets; `:bright` steps the brightness.
- **Text smooth** — toggles editor-font anti-aliasing. Off (crisp) keeps thin
  stems solid black; on (smooth) softens glyph edges.
- **Keyboards** — the BLE keyboard picker (`p`), with a disconnect option when
  a keyboard is connected.

Brightness and the smoothing choice are saved to NVS and restored on boot.

## Keyboard

The firmware scans for BLE HID keyboards; Bluetooth Classic-only keyboards are
not supported by the ESP32-S3. The current HID parser targets the standard
boot-keyboard report and a US key layout.

The editor has a deliberately small Vim-like modal keymap (it is not Neovim):

- Normal movement: `h j k l`, `w`, `b`, `0`, `^`, `$`, `G`; arrow keys also
  move the cursor.
- Insert: `i`, `a`, `A`, `I`, `o`, `O`; `Esc` returns to Normal.
- Editing: `x`, `dd`, `yy`, `cc`, `dw`, `d$`, `d0`, `cw`, `p`, `u`.
- Selection: `v`, then move and use `y`, `d`, or `x`.
- Search: `/pattern` then Enter.
- Commands: `:w [filename.md]`, `:wq`, `:q`, `:e`, `:new`, `:wifi`,
  `:wifi-off`, `:bright`, `:settings`, `:pair`; `Ctrl-S` saves and `Ctrl-O`
  opens the file browser.

The hint line under the status bar tracks the current mode: it reads
`i write  v visual` in Normal, `esc normal` in Insert and Visual, and while
a `:` command or `/` search is being typed it becomes the command line,
echoing what you type.

Unsaved changes are protected from opening or creating another document; save
first. Touch actions are Files, Save, Share, Settings, keyboard selection, and
backlight presets.

## Power

The backlight dims after one minute without keyboard input. Wi-Fi stays disabled
unless sharing is started. The status bar says `5V` to indicate the board's
normal supply; this board has no documented battery gauge and reports no
battery percentage. Add a
properly designed voltage-sense or fuel-gauge circuit before attaching a
battery. There is no real-time clock on the board, so a clock is not displayed.

## Fonts

UI text uses bold **Elms Sans** (12/14/16 px); unsaved filenames use Elms Sans
Bold Italic. The editor text uses the **Space Mono Nerd Font** at 14 px (13
visible lines), in two switchable variants: anti-aliased (smooth) and 1-bpp
crisp — see the TEXT SMOOTH toggle in Settings. UI icons use Space Mono at
14 px. See `assets/fonts/README.md`. The Markdown editor colors syntax with a
warm palette: headings orange, bold purple, italics peach, list bullets
yellow, and inline code a soft orange-peach. A blinking block caret marks the
cursor and is sized to exactly one character cell.
