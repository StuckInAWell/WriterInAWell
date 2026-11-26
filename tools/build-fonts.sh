#!/usr/bin/env sh
set -eu

# WriterInAWell fonts:
#   Elms Sans Bold        -> UI text (labels, buttons, breadcrumb, titles)
#   Elms Sans Bold Italic -> unsaved filenames in the breadcrumb
#   Terminess (Terminus)  -> editor text and every monospace text label
#   Space Mono (Nerd Font)-> icon glyphs only
#
# The generated fonts are RLE-compressed unless noted, so include/lv_conf.h
# must define LV_USE_FONT_COMPRESSED 1 (it does).

UI_RANGES="-r 0x20-0x7E -r 0xA0-0xFF -r 0x2000-0x206F -r 0x2190-0x21FF"
# Nerd Font icon codepoints (Font Awesome set): F00B bars, F013 gear,
# F07B folder, F0C7 floppy/save, F0C9 grid, F01E redo (circular arrow, the
# typing RETRY button), F030C keyboard (Material md-keyboard: solid, with
# key rows and a spacebar), F1EB wifi, F293 bluetooth.
ICON_RANGES="-r 0xF00B -r 0xF013 -r 0xF07B -r 0xF0C7 -r 0xF0C9 -r 0xF01E -r 0xF030C -r 0xF1EB -r 0xF293"
# The breadcrumb path embeds ICON_FOLDER inside a text label, so the folder
# glyph has to exist in the text font. Every other icon is rendered from a
# Space Mono icon-only cut.
TEXT_RANGES="$UI_RANGES -r 0xF07B"

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

# Terminess (Terminus Nerd Font Mono) editor cuts. Terminus ships native
# bitmap strikes at 12/14/16/18/20/22/24/28/32 px; at 11/13/15 the strike
# selection renders smaller, mismatched ink (13 px produced an 8 px-tall '0'
# while 12 px produced a 12 px one), so the ladder stays on the native even
# sizes. 10 px reuses the smallest available strike and is the dense option.
# The 14 px cut is the default. Each size has a 4-bpp anti-aliased ("smooth")
# cut and a 1-bpp uncompressed ("crisp") cut for the smoothing toggle.
for size in 10 12 14 16; do
  npx --yes lv_font_conv --size "$size" --bpp 4 --format lvgl \
    --font assets/fonts/TerminessNerdFontMono-Regular.ttf $TEXT_RANGES \
    --lv-fallback lv_font_montserrat_14 \
    --lv-font-name "terminess_${size}" -o "src/fonts/terminess_${size}.c"

  npx --yes lv_font_conv --size "$size" --bpp 1 --no-compress --format lvgl \
    --font assets/fonts/TerminessNerdFontMono-Regular.ttf $TEXT_RANGES \
    --lv-fallback lv_font_montserrat_14 \
    --lv-font-name "terminess_${size}_cr" -o "src/fonts/terminess_${size}_cr.c"
done

# Terminess 20 px for the typing-practice word stream. Text only: the word
# stream never carries icons.
npx --yes lv_font_conv --size 20 --bpp 4 --format lvgl \
  --font assets/fonts/TerminessNerdFontMono-Regular.ttf $UI_RANGES \
  --lv-fallback lv_font_montserrat_14 \
  --lv-font-name terminess_20 -o src/fonts/terminess_20.c

# Space Mono icon-only cuts. Text labels use Terminess; these four cuts exist
# so the UI icons keep their original Nerd Font shapes and metrics.
npx --yes lv_font_conv --size 14 --bpp 4 --format lvgl \
  --font assets/fonts/SpaceMono-Regular.ttf $ICON_RANGES \
  --lv-fallback lv_font_montserrat_14 \
  --lv-font-name space_mono_14_icons -o src/fonts/space_mono_14_icons.c

npx --yes lv_font_conv --size 20 --bpp 4 --format lvgl \
  --font assets/fonts/SpaceMono-Regular.ttf $ICON_RANGES \
  --lv-fallback lv_font_montserrat_14 \
  --lv-font-name space_mono_20_icons -o src/fonts/space_mono_20_icons.c

# Space Mono (icon-only 28 px cut for the main-menu grid tiles).
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
