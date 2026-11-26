# WriterInAWell quick reference

## Main menu

If a password has been set, boot stops at the lock screen first (see Lock
screen); without one the board boots straight to the menu. The menu is a
four-tile screen headed **WriterInAWell**. In reading
order the tiles are **TEXT EDITOR**, **TYPING PRACTICE**, **SHARE OVER WI-FI**
and **SETTINGS**; each tile prints its number in the top-right corner. Move the
highlight with the arrow keys or `h`/`j`/`k`/`l`,
press `Enter` to open a tile, click it, or jump straight to one with `1`–`4`.
There is no save tile and no status line: `:w` (or `Ctrl-S`) saves a document,
and the tiles themselves report state. The Bluetooth icon in the header is a
button: tap it to open the BLE keyboard picker (the same screen as `p` in
Settings or `:pair`), and BACK returns to the menu. Its color shows
scan/connected status; scanning starts automatically at boot.

## Storage and sharing

Put text files in `/WriterInAWell` on the microSD card. The file browser lists
files the editor knows how to open: Markdown (`.md`, `.markdown`), plain text
(`.txt`, `.log`, `.ini`, `.conf`, `.toml`), HTML (`.html`, `.htm`, `.xml`,
`.php`), CSS, Python, C/C++ (`.c`, `.h`, `.cpp`, `.hpp`, `.cc`, `.hh`),
JavaScript/TypeScript, JSON, shell, and YAML. New drafts auto-save every five
seconds after the first edit; use `:w name.ext` to choose a name (`:w notes`
appends the current language's extension). With the board's 2 MiB PSRAM, the
editor supports documents up to 512 KiB; without PSRAM it falls back to 32 KiB.
Larger files are left untouched rather than truncated. Files with Windows
(CRLF) line endings are opened with those endings normalised to LF.

With **Snapshots** on (FILES & DATA), every overwrite keeps the previous file
in `/WriterInAWell/.wiaw-versions/`, 3 versions by default and adjustable to
5 or 10. `:history` lists them, newest first; `↑`/`↓` select and `Enter`
loads one into the buffer as an unsaved edit (`:w` then overwrites the
current file, and `u` undoes the restore). Version files stay hidden from the
file browser and the Wi-Fi page.

Wi-Fi sharing is intentionally off at boot. Choose **SHARE OVER WI-FI** in the
main menu to open the Wi-Fi screen. It shows a live on/off toggle, the hotspot
name, the unique `well-XXXXXX` password, and the address once sharing is
running. From the editor, `:wifi` starts sharing directly and `:wifi-off` stops
it.

Both the name and the password are editable while sharing is off: tap a field
(or press `n` for the name, `p` for the password), type with the paired
Bluetooth keyboard (there is no on-screen keyboard), then press `Enter` or
`Esc` to finish. The password must be 8–63 characters (WPA2). Both values are
saved in NVS. Turn the toggle on (or press `Enter`) to start the WPA2 access
point; the name and password fields then lock, and the address appears in bold
soft green, e.g. `http://192.168.4.1`. Connect a phone or computer with the shown password, open
that address in a browser, and download or upload Markdown files. The page is a
file-browser view in the device's palette: each row shows the document name,
size, and SD modification time, with total count, total size, and card usage
above. Dates show as `—` when the card has no usable clock (the board has no
RTC, so on-device files normally have no meaningful timestamp). Documents live
in the single `/WriterInAWell` folder. Turn the toggle off (or press `Enter`),
press the screen's BACK, or use `:wifi-off` when finished. Only one client can
connect at a time.

## Text editor

**TEXT EDITOR** resumes the document already open. When no document has been
created or opened yet it shows a LazyVim-style start screen instead: `n` new
file, `f` find a file, `r` recent files, `q` quit back to the menu. The right
half lists recent documents (newest first, persisted across reboots); move
with `↑`/`↓`, `1`–`5` jumps straight to one, `Enter` opens it. `f` opens the
file browser with its filter already armed.

The browser filters as you type: `f` or `/` starts a query, the header echoes
it, and the list narrows case-insensitively. `Enter` opens the highlighted
file, `n` starts a new one, `Esc` cancels the filter (press it again to go
back), and `q`/BACK leave.

In the editor the header is a low-profile breadcrumb. `~/` stands for the
`WriterInAWell` root on the SD card; any subfolders appear with a folder icon,
followed by the current filename. Unsaved documents (including a new
`Untitled.md`) show their name in italics, in a reddish-pink color; a saved
file shows the name in a pastel mint. The hamburger button on the left opens
the main menu without closing the buffer, the Bluetooth square between the
power indicator and the gear opens the keyboard picker (BACK returns to the
document), and the gear on the right opens Settings. Between the filename and
the power readout, the **A−** and **A+** buttons step the editor text through
10, 12, 14 and 16 px; the status line reports the new size and visible line
count.

Recent documents remember their caret position, so reopening a draft resumes
where you stopped. The footer shows `line:column` by default, or a live word
count (with the net gain since opening) when Word count is on. `u` undoes and
`Ctrl-R` redoes; `:42` jumps to line 42. `/pattern` opens a `find: ` prompt
and searches from the caret, wrapping to the top when the text only appears
above it; matching is literal with smart case, so an all-lowercase pattern
finds any capitalization while one with a capital is exact. `n`/`N` repeat the
last search forward or backward. `Ctrl-Z`, `:zen`,
or a two-finger tap enters Zen mode, which hides the chrome and expands the
page; `Esc`, a two-finger tap, or the bottom helper exits. The **esc or two-finger tap to
exit** helper sits at the very bottom for the first 10 seconds, and a single
tap anywhere brings it back for a few seconds; the helper is also a button, so
a tap on it exits. Zen always starts off at boot, so a reboot always returns
to the normal chrome. The editor counts display columns in Unicode codepoints,
so curly
quotes, em dashes and Markdown uploaded from a phone keep the caret, wrapping
and `line:column` readout aligned; Backspace and the arrow keys never split a
multi-byte glyph.

## Settings

Open Settings with the Settings tile, the gear button in the editor, or
`:settings`. BACK returns to wherever you came from (the menu or the editor).

Settings is a short section list rather than one flat wall of switches. Seven
rows — **DISPLAY**, **EDITOR**, **WRITING**, **FILES & DATA**, **POWER**,
**KEYBOARD** and **SECURITY** — each carry a live summary (brightness, text
size, writing tools, current language, pairing state or lock state). Move with
`↑`/`↓` or `j`/`k`, press `Enter` or tap a row to open its popup panel, or
press `1`–`7` to jump straight to one. `Esc` or BACK leaves Settings. The underlined letter in a row
name is its keyboard shortcut.

Each section opens a popup over the list. Inside a popup, `↑`/`↓` move between
rows, `Enter` (or the row's shortcut letter) activates the selection, and
`←`/`→` adjust a value such as brightness. `Esc`, the popup's **CLOSE** button
or BACK closes the popup and returns to the section list; BACK again leaves
Settings. A nested picker (File Type) returns to its parent section.

- **DISPLAY** — the backlight slider. Drag it, use `←`/`→` for fine steps, or
  `1`–`4` for presets; `:bright` still steps the brightness from the editor.
- **EDITOR** — **Text s**ize (`i`) is the first row: the live text size in
  pixels, adjusted with `←`/`→` between 10, 12, 14 and 16 px (tapping or
  `Enter` steps to the next one). It is the same setting as the editor header's
  **A−**/**A+** pair and `:font`. **Text smoothing** (`t`) switches editor-font
  anti-aliasing:
  off (crisp) keeps thin stems solid black, on (smooth) softens glyph edges.
  **Text colors** (`c`) chooses the colored syntax view or a plain
  high-contrast one. **Line numbers** (`n`) adds the gutter; the wrap width
  narrows by the fixed 34 px rail (five columns at the two smallest sizes,
  three at the two largest) while it is on, and wrapped rows keep the number
  of the line they started from. **Word wrap** (`w`) is on by default: long
  lines fold onto the next screen row for display, without adding newlines to
  the file. Off, each line keeps its own row and the editor scrolls sideways
  to keep the caret visible: when text reaches about 95% of the row width,
  the view jumps about 80% of the width at a time. **Relative numbers** (`r`)
  keeps the caret's line absolute and shows every other line's distance from
  it. **Focus line** (`f`) keeps the caret's band vertically centered as you
  type. **Zen mode** (`z`) hides the header, status bar and hint strip and
  expands the page to the full screen; `Ctrl-Z` toggles it from the editor
  and `Esc` leaves it. **Search wrap** (`s`) makes `n`/`N` wrap around the
  document.
- **WRITING** — **List continuation** (`l`) is on by default: `Enter`
  continues a Markdown bullet, task or numbered item with the same indent, a
  continued task starts unchecked, and pressing `Enter` on an empty item
  removes its marker (and outdents one level for a nested item) instead of
  adding another bullet. `Tab` nests the current item one 2-space step and
  `Shift-Tab` outdents it, like Obsidian. `Ctrl-Enter` flips the current
  line's task box whether or not continuation is on. **Prose punctuation**
  (`p`) types curly quotes, `--` as an em dash and `...` as an ellipsis in
  Markdown and plain text; Markdown `---` rules and code files are left as
  typed. It is off by default.
- **FILES & DATA** — **Auto-save** (`a`) saves 5 s after your last edit; off
  means saving only via `:w` / Ctrl-S. **File Type** (`t`) opens the language
  picker: `j`/`k` and `↑`/`↓` move, `Enter` or a digit applies, `Esc`/`q`
  closes. Picking a type also sets the extension for a new untitled buffer,
  and the choice is remembered as the default for the next new file.
  **Word count** (`w`) replaces the footer's line:column readout with a live
  word total and the net gain since the file was opened. **Snapshots** (`s`)
  keeps a copy of the previous save in a hidden `.wiaw-versions` folder;
  **Keep versions** (`k`) cycles 3, 5 or 10 per file. `:history` lists them
  and restores one into the buffer as an unsaved edit.
- **POWER** — **Dim when idle** (`d`): off stops the backlight from dropping
  after 60 s without input. **Start in editor** (`b`) makes power-on resume
  the newest recent file straight into the editor instead of the menu.
  **Off after** (`o`) turns the backlight fully off after 2, 5, 10 or 15 idle
  minutes (off by default); any key wakes it, and on a blank screen the first
  tap only wakes it (see Power). **Light sleep** (`l`) is
  an opt-in, timer-wake sleep that starts only after screen-off and 15 minutes
  of idle; it needs on-device validation because a BLE keyboard may disconnect
  and reconnect.
- **KEYBOARD** — the live pairing state (`paired`, `searching` or
  `not paired`), a **Pair Keyboard** row (`p`) that opens the BLE picker, and
  a **DISCONNECT** row (`d`) while a keyboard is attached.
- **SECURITY** — **Set / change password** (`s`), **Remove password** (`r`)
  and **Lock now** (`l`). The row summary reads `on` or `off`. Changing or
  removing a password first asks for the current one; a wrong entry returns to
  the same prompt. Lock now, `Ctrl-L` and `:lock` all show the lock screen
  immediately. See Lock screen for the password rules and the keypad.

All settings, brightness, the file type and the password hash are saved to NVS
and restored on boot.

## File types and highlighting

The editor colours ten languages; the extension picks one when a file is
opened, and an unknown extension opens as plain text with no styling.

| Type | Extensions |
| --- | --- |
| Markdown | `.md`, `.markdown` |
| Plain Text | `.txt`, `.log`, `.ini`, `.conf`, `.toml` |
| HTML | `.html`, `.htm`, `.xml`, `.php` |
| CSS | `.css` |
| Python | `.py`, `.pyi` |
| C/C++ | `.c`, `.h`, `.cpp`, `.hpp`, `.cc`, `.hh`, `.cxx`, `.hxx` |
| JavaScript | `.js`, `.mjs`, `.ts`, `.tsx`, `.jsx` |
| JSON | `.json` |
| Shell | `.sh`, `.bash`, `.zsh` |
| YAML | `.yml`, `.yaml` |

Highlighting covers comments, strings, keywords, numbers, types, attributes
and punctuation, and follows constructs across lines: `/* */` blocks,
`<!-- -->` comments, `<script>`/`<style>` bodies, Python triple-quoted
strings, and fenced Markdown code. Only the visible rows are lexed; the
multi-line state behind them is rebuilt from the top of the file with a fixed
scan budget, so a long document does not make typing sluggish.

Markdown styling is richer than the other grammars: headings take a colour per
level (`#`, `##`, `###` and deeper), blockquote text is purple, list markers
and task boxes (`- [ ]` / `- [x]`) tint, `**bold**`, `*italic*`,
`~~strikethrough~~`, inline `` `code` `` and `[links](target)` all take their
own ink, tables get dim pipes, and `---` rules dim out. A span's colour
carries across a wrapped fold instead of being lost at the band edge.

Insert mode adds two language-aware conveniences:

- **Indent** — `Tab` inserts spaces up to the next 2-space stop. `Enter`
  keeps the current line's indentation and adds one level (two spaces) after
  an opener (`(`, `[`, `{`) or, in Python, a `:`; pressing `Enter` between
  `{|}` splits them onto three lines with the caret indented in the middle.
  `o` and `O` copy the neighbouring line's indentation.
- **Auto-pairing** — `(`, `[` and `{` insert their closing partner and leave
  the caret inside; typing the closer when it is already next steps over it
  instead of doubling it, and Backspace inside an empty pair removes both.
  Quotes auto-pair in code languages but not in Markdown or plain text.

## Keyboard

The firmware scans for BLE HID keyboards; Bluetooth Classic-only keyboards are
not supported by the ESP32-S3. The current HID parser targets the standard
boot-keyboard report and a US key layout. The last keyboard that connected is
remembered in NVS, and the firmware tries to reconnect to it once at boot, so
a normally powered-off device comes back with its keyboard attached. If it is
not found, the normal scan and the picker are still there.

The editor has a deliberately small Vim-like modal keymap (it is not Neovim):

- Normal movement: `h j k l`, `w`, `b`, `0`, `^`, `$`, `G`; arrow keys also
  move the cursor.
- Insert: `i`, `a`, `A`, `I`, `o`, `O`; `Esc` returns to Normal.
- Editing: `x`, `dd`, `yy`, `cc`, `dw`, `d$`, `d0`, `cw`, `p`, `u`;
  `Ctrl-R` redoes and `Ctrl-Enter` toggles the current Markdown task box.
- Text size: `+` (or `=`) steps up and `-` steps down through 10/12/14/16 px,
  like the header's **A−**/**A+** pair; `:font 16` sets an exact size.
- Selection: `v`, then move and use `y`, `d`, or `x`.
- Search: `/pattern` (`find: ` prompt) then Enter, wrapping past the top;
  smart case (all-lowercase ignores case); `n`/`N` repeat forward/backward.
- Go to line: `:42` jumps to line 42.
- Zen: `Ctrl-Z` or `:zen` toggles the chrome-free page; `Esc` leaves it.
- Help: `?` in Normal opens an on-device cheat sheet of these keys.
- Commands: `:w [name.ext]`, `:wq`, `:q`, `:e`, `:new`, `:p`, `:history`,
  `:wifi`, `:wifi-off`, `:bright`, `:zen`, `:lock`, `:settings`, `:pair`,
  `:font [10|12|14|16]`, and `:tpl [journal|meeting|blog|bug|daily]` (no
  name lists the templates; a name inserts the skeleton and enters Insert
  mode); `Ctrl-S` saves, `Ctrl-O` opens the file browser, and `Ctrl-L` locks
  the device (it is a no-op until a password has been set).
- Rendered preview: `:p` (or `:preview`) opens a read-only view of the
  document — headings grow and lose their `#`, quotes get a bar, code blocks
  tint, and long lines wrap. `j`/`k` or the arrows scroll, `PgUp`/`PgDn`
  page, `Home`/`End` jump to the ends; `q`, `p`, `Esc` or BACK returns to
  editing. Extremely long lines clip at 200 characters and the block table
  caps at 16384 lines (the header then says TRUNCATED).

The hint line under the status bar tracks the current mode: in Normal it
reads `i` input, `v` visual, `:` commands, `?` help, with the key letters
drawn as keycaps on a dark grey background; in Insert and Visual it
collapses to `esc` normal. While a `:` command or `/` search is being typed
it becomes the command line, echoing what you type (a search is prefixed with
`find: `). The right end of the
line holds two more keycaps, outlined in blue to mark them as menu
shortcuts: `:menu` for the main menu and `:settings` for Settings. They share
the Normal-mode visibility of the other chips.

Pressing `?` in Normal opens a full-screen cheat sheet of the editor's Vim
keys — movement, insert, edit, visual, search, and the text-size commands —
each with a one-line description. It is view-only, so it opens even with
unsaved changes, and it closes on `Esc`, `?`, `q`, or the BACK button.

Unsaved changes are protected from opening, creating, or closing another
document — save first. In Normal mode `q` closes the buffer and returns to the
main menu (it refuses while there are unsaved changes); `:wq` saves and then
quits, and `:q` mirrors `q`. `:menu` shows the main menu without closing the
buffer, so **TEXT EDITOR** picks it up again. Touch actions are the four
main-menu tiles, the editor's menu/settings buttons, the start-screen commands
and recent list, the filtered file list, keyboard selection, backlight
presets, and typing practice (RETRY/SETTINGS/BACK).

## Typing practice

Open **TYPING PRACTICE** from the main menu (or `:menu` in the editor, then
tile `2`) for a monkeytype-style word sprint with your BLE keyboard. It never
touches the document, so it opens even with unsaved changes, and `Esc`/BACK
always returns to the main menu.

- Runs are word sprints: `1` = **Short** (20 words) and `2` = **Long** (as
  many words as fit the word area, so the count follows the generated word
  lengths). Pressing a length key starts a fresh run immediately (when
  Numbers is on, the same digit is typed instead if it is the next expected
  character).
- The word stream uses the 20 px Space Mono cut. The active word renders in
  color bands: the correctly typed prefix bright,
  mistakes red, the untyped tail light. Finished words turn mint when clean,
  pink when anything went wrong; letters typed past a word's end appear as
  red extras, capped at eight.
- The clock arms on your first keystroke. `Space` advances to the next word
  and counts as a correct character when the word was clean; a leading space
  before any typing is ignored. The last word can end without a trailing
  space. `Backspace` erases within the current word.
- `Tab` or the RETRY button restarts; `Esc` or BACK leaves. `Ctrl-O` — pinned
  as a blue command chip at the bottom right — or the SETTINGS header button
  opens the practice settings.
- The stats bar is a segmented strip: tinted cells with a dim caption and a
  bold value, divided by hairlines rather than characters. The live run shows
  WPM, accuracy, elapsed time, and your personal best for the current option;
  the end-of-run summary swaps in a two-row bar that adds raw WPM and the
  correct-word count. WPM is monkeytype's formula: correct characters
  (including spaces) ÷ 5 per minute. The run info ("Long run, 54 words — type
  the bright word") sits in the strip above the hint row.
- Personal bests persist across reboots (NVS keys `tb20` and `tblong` under
  the existing `wiaw` namespace). The retired `tb10`/`tb15`/`tb25`/`tb50`
  records are carried over on first boot.

### Typing settings

**TYPING SETTINGS** uses the same header, panel, underlined shortcut letters,
and pill switches as Settings. Two choice rows cycle their value when tapped
or when their letter is pressed; the four toggles mirror the Settings rows.
Returning to practice starts a fresh run with the new rules. The decorations
are deliberately sparse and only appear where they read as language, so
typing stays about the words rather than about fighting stray symbols.

- `Complexity` (`c`) — Easy limits the common word bank to four letters,
  Normal uses the whole common bank, and Hard mixes 7–12 letter words from
  the long-word bank with common words (about three long draws in five) so
  short words still keep the word count up.
- `Max Length` (`m`) — caps the base word length: Any (default), 8, 10, or
  12 letters. Easy and Normal words are already shorter, so the cap mainly
  narrows Hard runs.
- `Symbols` (`s`) — real hyphenated compounds (`well-known`, `long-term`),
  plus one pair of parentheses, brackets, or asterisks around a mid-sentence
  word. No symbol is ever inserted into the middle of a word.
- `Numbers` (`n`) — occasionally replaces a word with a standalone number
  (1–999), never glued to letters.
- `Capitalization` (`a`) — capitalizes the first word of each sentence, and
  occasionally draws an acronym (`API`, `USB`, `PDF`) so Shift gets practice.
- `Punctuation` (`p`) — sentences end with `.`, `?`, or `!`; clauses can join
  with `;` (the next word stays lower case); commas follow introductory
  adverbs (`however,`, `later,`, `instead,`); contractions bring their own
  apostrophe (`don't`, `it's`, `they're`).

`#` is never generated: the Markdown recolor parser reserves it. The rules
persist in NVS under `tcomp`, `tmax`, `tsym`, `tnum`, `tcap`, and `tpunc`.

## Power

The backlight dims after one minute without keyboard input. With **Off after**
set in POWER it goes fully dark after the chosen 2/5/10/15 idle minutes; any
key restores it instantly. A tap on the dark screen only wakes it: the backlight
fades up over about a second with touches ignored, so the wake tap cannot press
the button that happens to sit under your finger, and a finger held down is
still ignored until it lifts. After the fade the screen takes touch input
normally. **Light sleep** is opt-in and begins only after
screen-off plus 15 minutes of idle, waking on a 1 s timer to poll BLE and
touch. Treat it as experimental: a BLE keyboard may disconnect and need to
reconnect, so leave it off unless you are testing that path. With a password
set, that same idle point also locks the device, and the wake tap lands on the
login screen (see Lock screen).

Wi-Fi stays disabled unless sharing is started. By default the status bar says
`5V`: the board has no battery gauge, so nothing is measured and no percentage
is invented. A real readout is available for hardware you add yourself:

- Wire a divider at the battery terminal (before the charger/boost, so it
  reads the cell, not the 5 V rail): battery+ through 100 kΩ to EXT_IO1
  (GPIO10), GPIO10 through 100 kΩ to GND, and 100 nF from GPIO10 to GND.
  GPIO10 and GPIO11 are the only ADC1 pins on the expansion connector; the
  rest are ADC2, which cannot be read while Wi-Fi runs.
- Enable `-DWIAW_BATTERY_SENSE` in `platformio.ini` (commented out by
  default). The top-right indicator then shows an approximate charge percent,
  refreshed every 10 s from a 16-sample average.
- Treat the percent as a coarse indicator: a LiPo's voltage is flat through
  the mid-range and sags under load, and while a charger is attached the cell
  sits at ~4.2 V so the readout pins at 100%. Charger-state sensing is
  follow-up work. If a multimeter disagrees with the displayed value, trim
  `BATTERY_CALIBRATION_MV` in `src/main.cpp`.

There is no real-time clock on the board, so a clock is not displayed.

## Lock screen

Set a password in Settings → **SECURITY**. With one set, the board boots to a
minimalist login screen and returns to it after light sleep (the 15-minute
idle point that also starts the timer-wake sleep). `Ctrl-L`, `:lock`, or the
Security panel's **Lock now** locks on demand; unlocking returns to the editor
when that is where you locked from, otherwise to the menu.

The password can be typed on the paired BLE keyboard or tapped on the on-screen
4×10 keypad, so no keyboard is required to get back in. `SHIFT` capitalizes
one letter and resets after use, `SYM` switches the letter rows to
punctuation, `DEL` erases, and `ENTER` submits. `Esc` clears the field while
unlocking; during Set / Remove it cancels and returns to Settings. Passwords
are 4–24 printable ASCII characters, and the field shows one `*` per character
typed.

Only a salted 64-bit hash is stored in NVS, never the password itself. This is
a UI gate, not disk encryption: the microSD card stays readable in a card
reader, and a forgotten password means clearing the device's NVS (which also
resets the other preferences). Changing or removing the password requires the
current one. Locking does not stop an active Wi-Fi share; turn sharing off
first if you want the access point to go away.

## Fonts

UI text uses bold **Elms Sans** (12/14/16 px); unsaved filenames use Elms Sans
Bold Italic. The editor text uses the **Space Mono Nerd Font** at 10, 12, 14
(default) or 16 px, in two switchable variants: anti-aliased (smooth) and 1-bpp
crisp — see the TEXT SMOOTH toggle in Settings. The size changes the line pitch
and band count: 18, 14, 13 or 12 visible lines of 74, 64, 49 or 44 columns
(69, 59, 46 or 41 while line numbers are on). The typing-practice word stream
uses the smooth 20 px Space Mono cut (not part of the editor size ladder). UI
icons are Space Mono
Nerd Font glyphs: 14 px (toolbar buttons), 20 px (Bluetooth logo), 28 px
(action-menu grid tiles), and 40 px (the menu's keyboard tile, sized so it
matches its neighbors' height). See `assets/fonts/README.md`. The Markdown
editor colors syntax with a
warm palette: headings orange, bold purple, italics peach, list bullets
yellow, and inline code a soft orange-peach. A blinking block caret marks the
cursor and is sized to exactly one character cell: the solid phase is a very
light blue block with the character reversed out of it, and the other phase is
a 1 px box of the same cell, so the character under the caret stays readable
either way.
