#ifndef LV_CONF_H
#define LV_CONF_H

#define LV_COLOR_DEPTH 16
#define LV_MEM_CUSTOM 0
#define LV_MEM_SIZE (48U * 1024U)
#define LV_TICK_CUSTOM 1
#define LV_TICK_CUSTOM_INCLUDE "Arduino.h"
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (millis())
#define LV_DPI_DEF 160

/* The panel is flushed synchronously over a 40 MHz 8-bit parallel bus, so a
 * 480x32 band costs well under a millisecond; a 30 ms refresh period (the
 * LVGL default) would therefore cap every screen change at ~33 fps for no
 * reason. 16 ms matches the draw-buffer work and makes a keystroke's repaint
 * land on the next frame instead of up to 30 ms later. Touch polls a little
 * faster than the default too, for the same reason. */
#define LV_DISP_DEF_REFR_PERIOD 16
#define LV_INDEV_DEF_READ_PERIOD 20

#define LV_USE_LOG 0
#define LV_USE_ASSERT_NULL 1
#define LV_USE_ASSERT_MALLOC 1
#define LV_USE_PERF_MONITOR 0
#define LV_USE_MEM_MONITOR 0
#define LV_USE_USER_DATA 1

#define LV_USE_LABEL 1
#define LV_USE_BTN 1
#define LV_USE_LIST 1
#define LV_USE_TEXTAREA 1
#define LV_USE_THEME_DEFAULT 1
#define LV_THEME_DEFAULT_DARK 1
/* No style transitions. Settings rebuilds its popup on every arrow key, so the
 * theme's 80 ms indicator/border fades replay on each rebuild and all green
 * switches appear to flash together. Nothing in the app animates deliberately
 * (sliders are set with LV_ANIM_OFF), so an instant UI is both calmer and
 * cheaper. */
#define LV_THEME_DEFAULT_TRANSITION_TIME 0
#define LV_USE_FONT_PLACEHOLDER 1
#define LV_USE_FONT_COMPRESSED 1

/* Break only at spaces. LVGL's default set (", . ; : - _") also breaks after
 * punctuation, which can split an "#RRGGBB text#" span across two lines. The
 * draw parser restarts at each line start, so the closing '#' of a split span
 * is read as a new command and the following span's hex code is printed as
 * text (the typing word stream then shows "5F707A word"). The app's only
 * wrapping recolor label is the word stream, whose spans are single tokens
 * with no spaces, so space-only breaks keep every span on one line. */
#define LV_TXT_BREAK_CHARS " "

#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_16 0
#define LV_FONT_CUSTOM_DECLARE \
  extern const lv_font_t elms_bold_12; \
  extern const lv_font_t elms_bold_14; \
  extern const lv_font_t elms_bold_16; \
  extern const lv_font_t elms_bold_italic_14; \
  extern const lv_font_t space_mono_14; \
  extern const lv_font_t space_mono_20; \
  extern const lv_font_t space_mono_28; \
  extern const lv_font_t space_mono_40; \
  extern const lv_font_t space_mono_14_cr; \
  extern const lv_font_t space_mono_10; \
  extern const lv_font_t space_mono_10_cr; \
  extern const lv_font_t space_mono_12; \
  extern const lv_font_t space_mono_12_cr; \
  extern const lv_font_t space_mono_16; \
  extern const lv_font_t space_mono_16_cr;
#define LV_FONT_DEFAULT &elms_bold_12

#endif
