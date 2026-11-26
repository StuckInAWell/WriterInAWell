# Fonts

WriterInAWell uses two font families:

- **Elms Sans Bold** — UI text: labels, buttons, the breadcrumb path, and
  screen titles. Generated at 12, 14, and 16 px from `ElmsSans-Bold.ttf`
  (plus a bold-italic cut for unsaved filenames).
- **Space Mono (Nerd Font)** — the editor text (monospace, for exact caret
  alignment) and the UI icons. The editor text is generated at 10, 12, 14
  and 16 px, each in two switchable variants (14 px is the default):
  - `space_mono_{10,12,14,16}.c` — 4-bpp anti-aliased ("smooth"),
    RLE-compressed
  - `space_mono_{10,12,14,16}_cr.c` — 1-bpp crisp, uncompressed; thin stems
    stay solid
  - `space_mono_20.c` — 4-bpp anti-aliased 20 px cut for the larger top-bar
    icons (bluetooth logo), RLE-compressed
  - `space_mono_28.c` — 4-bpp anti-aliased 28 px icon-only cut for the
    main-menu grid tiles (text, Wi-Fi, gear), RLE-compressed
  - `space_mono_40.c` — 4-bpp anti-aliased 40 px icon-only cut; the
    TYPING PRACTICE tile renders its keyboard glyph from here so its
    short ink (0.43 em) matches the other tiles' height at 28 px,
    RLE-compressed

The editor's header A−/A+ pair picks the text size; the Settings screen's
TEXT SMOOTH toggle selects smooth versus crisp at that size.

Source files (with their SIL Open Font License texts) live here; the
LVGL-compatible, 4-bpp, RLE-compressed subsets are generated into
`src/fonts/`.

Regenerate them with:

```sh
sh tools/build-fonts.sh
```

Glyph coverage: ASCII, Latin-1, general punctuation, arrows, and (for Space
Mono) the UI icons — hamburger `U+F0C9`, gear `U+F013`, folder `U+F07B`,
floppy-disk save `U+F0C7`, keyboard `U+F030C`, wifi `U+F1EB`, bluetooth
`U+F293`. Anything outside that falls back to LVGL's Montserrat.

The generated fonts are RLE-compressed, so `include/lv_conf.h` must define
`LV_USE_FONT_COMPRESSED 1` (it does). Without it, LVGL returns `NULL` for every
glyph and all text renders blank.
