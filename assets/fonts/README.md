# Fonts

WriterInAWell uses three font roles:

- **Elms Sans Bold** — UI text: labels, buttons, the breadcrumb path, and
  screen titles. Generated at 12, 14, and 16 px from `ElmsSans-Bold.ttf`
  (plus a bold-italic cut for unsaved filenames).
- **Terminess (Terminus Nerd Font Mono)** — editor text and every monospace
  text label (breadcrumb path, mode pill). Generated at 10, 12, 14 and 16 px,
  each in two switchable variants (14 px is the default):
  - `terminess_{10,12,14,16}.c` — 4-bpp anti-aliased ("smooth"),
    RLE-compressed
  - `terminess_{10,12,14,16}_cr.c` — 1-bpp crisp, uncompressed
  - `terminess_20.c` — 4-bpp anti-aliased 20 px cut for the typing-practice
    word stream
- **Space Mono (Nerd Font)** — icon glyphs only, never text:
  - `space_mono_14_icons.c` — 4-bpp 14 px toolbar icons (menu, gear, retry)
  - `space_mono_20_icons.c` — 4-bpp 20 px Bluetooth logo
  - `space_mono_28.c` — 4-bpp 28 px icon-only cut for the main-menu grid
  - `space_mono_40.c` — 4-bpp 40 px icon-only cut; the TYPING PRACTICE tile
    renders its keyboard glyph from here so its short ink (0.43 em) matches
    the other tiles' height at 28 px

## Why the editor ladder stays at 10/12/14/16

Terminus is a pixel font and the TTF carries native bitmap strikes at 12, 14,
16, 18, 20, 22, 24, 28 and 32 px. `lv_font_conv` uses those strikes, so an
odd size does not scale evenly: 13 px produced an 8 px-tall `0` while 12 px
produced a 12 px one, and 11/15 px showed the same mismatch. The editor
therefore keeps the native even sizes. 10 px reuses the smallest available
strike as the dense option, so it renders smaller ink than 12 px but is still
crisp and useful for fitting the most text.

The editor's header A−/A+ pair picks the text size; the Settings screen's
TEXT SMOOTH toggle selects smooth versus crisp at that size.

Source files (with their SIL Open Font License texts) live here; the
LVGL-compatible subsets are generated into `src/fonts/`.

Regenerate them with:

```sh
sh tools/build-fonts.sh
```

Glyph coverage: ASCII, Latin-1, general punctuation, arrows, and the UI icons.
The text cuts include the folder glyph `U+F07B` because the breadcrumb embeds
it inside a Terminess label; every other icon comes from a Space Mono
icon-only cut. Anything outside the generated ranges falls back to LVGL's
Montserrat.

The generated fonts are RLE-compressed except the four Terminess `_cr` cuts,
so `include/lv_conf.h` must define `LV_USE_FONT_COMPRESSED 1` (it does).
Without it, LVGL returns `NULL` for every glyph and all text renders blank.
