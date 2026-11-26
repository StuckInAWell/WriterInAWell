#!/usr/bin/env sh
set -eu

# WriterInAWell fonts:
#   Elms Sans Bold       -> UI text (labels, buttons, breadcrumb, titles)
#   Elms Sans Bold Italic-> unsaved filenames in the breadcrumb
#   Space Mono           -> editor text and the UI icons
#
# The generated fonts are RLE-compressed, so include/lv_conf.h must define
# LV_USE_FONT_COMPRESSED 1 (it does).

UI_RANGES="-r 0x20-0x7E -r 0xA0-0xFF -r 0x2000-0x206F -r 0x2190-0x21FF"
# Nerd Font icon codepoints (Font Awesome set): F00B bars, F013 gear,
# F07B folder, F0C7 floppy/save, F0C9 grid, F01E redo (circular arrow, the
# typing RETRY button), F030C keyboard (Material md-keyboard: solid, with
# key rows and a spacebar), F1EB wifi, F293 bluetooth.
ICON_RANGES="-r 0xF00B -r 0xF013 -r 0xF07B -r 0xF0C7 -r 0xF0C9 -r 0xF01E -r 0xF030C -r 0xF1EB -r 0xF293"

# Elms Sans Bold (UI)
npx --yes lv_font_conv --size 12 --bpp 4 --format lvgl \
  --font assets/fonts/ElmsSans-Bold.ttf $UI_RANGES \
  --lv-fallback lv_font_montserrat_14 \
  --lv-font-name elms_bold_12 -o src/fonts/elms_bold_12.c

npx --yes lv_font_conv --size 14 --bpp 4 --format lvgl \
  --font assets/fonts/ElmsSans-Bold.ttf $UI_RANGES \
  --lv-fallback lv_font_montserrat_14 \
  --lv-font-name elms_bold_14 -o src/fonts/elms_bold_14.c

npx --yes lv_font_conv --size 16 --bpp 4 --format lvgl \
  --font assets/fonts/ElmsSans-Bold.ttf $UI_RANGES \
  --lv-fallback lv_font_montserrat_14 \
  --lv-font-name elms_bold_16 -o src/fonts/elms_bold_16.c

npx --yes lv_font_conv --size 14 --bpp 4 --format lvgl \
  --font assets/fonts/ElmsSans-BoldItalic.ttf $UI_RANGES \
  --lv-fallback lv_font_montserrat_14 \
  --lv-font-name elms_bold_italic_14 -o src/fonts/elms_bold_italic_14.c

# Space Mono (editor text, anti-aliased "smooth" variant). The 14 px cut is
# the default; 10/12 are the smaller picks and 16 the larger one, all four
# share the same glyph ranges so a document reads identically at any size.
npx --yes lv_font_conv --size 14 --bpp 4 --format lvgl \
  --font assets/fonts/SpaceMono-Regular.ttf $UI_RANGES $ICON_RANGES \
  --lv-fallback lv_font_montserrat_14 \
  --lv-font-name space_mono_14 -o src/fonts/space_mono_14.c

npx --yes lv_font_conv --size 10 --bpp 4 --format lvgl \
  --font assets/fonts/SpaceMono-Regular.ttf $UI_RANGES $ICON_RANGES \
  --lv-fallback lv_font_montserrat_14 \
  --lv-font-name space_mono_10 -o src/fonts/space_mono_10.c

npx --yes lv_font_conv --size 12 --bpp 4 --format lvgl \
  --font assets/fonts/SpaceMono-Regular.ttf $UI_RANGES $ICON_RANGES \
  --lv-fallback lv_font_montserrat_14 \
  --lv-font-name space_mono_12 -o src/fonts/space_mono_12.c

npx --yes lv_font_conv --size 16 --bpp 4 --format lvgl \
  --font assets/fonts/SpaceMono-Regular.ttf $UI_RANGES $ICON_RANGES \
  --lv-fallback lv_font_montserrat_14 \
  --lv-font-name space_mono_16 -o src/fonts/space_mono_16.c

# Space Mono (larger cut for the top-bar icons, e.g. the bluetooth logo)
npx --yes lv_font_conv --size 20 --bpp 4 --format lvgl \
  --font assets/fonts/SpaceMono-Regular.ttf $UI_RANGES $ICON_RANGES \
  --lv-fallback lv_font_montserrat_14 \
  --lv-font-name space_mono_20 -o src/fonts/space_mono_20.c

# Space Mono (icon-only 28 px cut for the main-menu grid tiles). No
# UI_RANGES: these glyphs are only ever rendered as icons, so the text
# ranges would just burn flash.
npx --yes lv_font_conv --size 28 --bpp 4 --format lvgl \
  --font assets/fonts/SpaceMono-Regular.ttf $ICON_RANGES \
  --lv-fallback lv_font_montserrat_14 \
  --lv-font-name space_mono_28 -o src/fonts/space_mono_28.c

# Space Mono (icon-only 40 px oversize cut for main-menu tiles whose glyph
# ink is short: md-keyboard's ink is 0.43 em tall, so at 40 px it matches
# the 0.61 em ink of a full-height icon at 28 px. Only the TYPING PRACTICE
# tile uses it today.)
npx --yes lv_font_conv --size 40 --bpp 4 --format lvgl \
  --font assets/fonts/SpaceMono-Regular.ttf $ICON_RANGES \
  --lv-fallback lv_font_montserrat_14 \
  --lv-font-name space_mono_40 -o src/fonts/space_mono_40.c

# Space Mono (editor text, 1-bpp "crisp" variant, no anti-aliasing).
# Thin stems render fully black instead of faint gray. Every size needs its
# own cut so the smoothing toggle works across the whole size ladder.
npx --yes lv_font_conv --size 14 --bpp 1 --no-compress --format lvgl \
  --font assets/fonts/SpaceMono-Regular.ttf $UI_RANGES $ICON_RANGES \
  --lv-fallback lv_font_montserrat_14 \
  --lv-font-name space_mono_14_cr -o src/fonts/space_mono_14_cr.c

npx --yes lv_font_conv --size 10 --bpp 1 --no-compress --format lvgl \
  --font assets/fonts/SpaceMono-Regular.ttf $UI_RANGES $ICON_RANGES \
  --lv-fallback lv_font_montserrat_14 \
  --lv-font-name space_mono_10_cr -o src/fonts/space_mono_10_cr.c

npx --yes lv_font_conv --size 12 --bpp 1 --no-compress --format lvgl \
  --font assets/fonts/SpaceMono-Regular.ttf $UI_RANGES $ICON_RANGES \
  --lv-fallback lv_font_montserrat_14 \
  --lv-font-name space_mono_12_cr -o src/fonts/space_mono_12_cr.c

npx --yes lv_font_conv --size 16 --bpp 1 --no-compress --format lvgl \
  --font assets/fonts/SpaceMono-Regular.ttf $UI_RANGES $ICON_RANGES \
  --lv-fallback lv_font_montserrat_14 \
  --lv-font-name space_mono_16_cr -o src/fonts/space_mono_16_cr.c
