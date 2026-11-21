# Fonts

WriterInAWell uses two font families:

- **Elms Sans Bold** — UI text: labels, buttons, the breadcrumb path, and
  screen titles. Generated at 12, 14, and 16 px from `ElmsSans-Bold.ttf`
  (plus a bold-italic cut for unsaved filenames).
- **Space Mono (Nerd Font)** — the editor text (monospace, for exact caret
  alignment) and the UI icons, generated at 14 px in two switchable variants:
  - `space_mono_14.c` — 4-bpp anti-aliased ("smooth"), RLE-compressed
  - `space_mono_14_cr.c` — 1-bpp crisp, uncompressed; thin stems stay solid
  - `space_mono_20.c` — 4-bpp anti-aliased 20 px cut for the larger top-bar
    icons (bluetooth logo), RLE-compressed

The Settings screen's TEXT SMOOTH toggle selects between them at runtime.

Source files (with their SIL Open Font License texts) live here; the
LVGL-compatible, 4-bpp, RLE-compressed subsets are generated into
`src/fonts/`.

Regenerate them with:

```sh
sh tools/build-fonts.sh
```

Glyph coverage: ASCII, Latin-1, general punctuation, arrows, and (for Space
Mono) the UI icons — hamburger `U+F0C9`, gear `U+F013`, bluetooth `U+F293`.
Anything outside that falls back to LVGL's Montserrat.

The generated fonts are RLE-compressed, so `include/lv_conf.h` must define
`LV_USE_FONT_COMPRESSED 1` (it does). Without it, LVGL returns `NULL` for every
glyph and all text renders blank.
