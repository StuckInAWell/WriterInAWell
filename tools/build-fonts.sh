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
ICON_RANGES="-r 0xF00B -r 0xF013 -r 0xF07B -r 0xF0C9 -r 0xF293"

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

# Space Mono (editor text, anti-aliased "smooth" variant)
npx --yes lv_font_conv --size 14 --bpp 4 --format lvgl \
  --font assets/fonts/SpaceMono-Regular.ttf $UI_RANGES $ICON_RANGES \
  --lv-fallback lv_font_montserrat_14 \
  --lv-font-name space_mono_14 -o src/fonts/space_mono_14.c

# Space Mono (larger cut for the top-bar icons, e.g. the bluetooth logo)
npx --yes lv_font_conv --size 20 --bpp 4 --format lvgl \
  --font assets/fonts/SpaceMono-Regular.ttf $UI_RANGES $ICON_RANGES \
  --lv-fallback lv_font_montserrat_14 \
  --lv-font-name space_mono_20 -o src/fonts/space_mono_20.c

# Space Mono (editor text, 1-bpp "crisp" variant, no anti-aliasing).
# Thin stems render fully black instead of faint gray.
npx --yes lv_font_conv --size 14 --bpp 1 --no-compress --format lvgl \
  --font assets/fonts/SpaceMono-Regular.ttf $UI_RANGES $ICON_RANGES \
  --lv-fallback lv_font_montserrat_14 \
  --lv-font-name space_mono_14_cr -o src/fonts/space_mono_14_cr.c
