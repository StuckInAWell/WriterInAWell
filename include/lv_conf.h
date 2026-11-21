#ifndef LV_CONF_H
#define LV_CONF_H

#define LV_COLOR_DEPTH 16
#define LV_MEM_CUSTOM 0
#define LV_MEM_SIZE (48U * 1024U)
#define LV_TICK_CUSTOM 1
#define LV_TICK_CUSTOM_INCLUDE "Arduino.h"
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (millis())
#define LV_DPI_DEF 160

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
#define LV_USE_FONT_PLACEHOLDER 1
#define LV_USE_FONT_COMPRESSED 1

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
  extern const lv_font_t space_mono_14_cr;
#define LV_FONT_DEFAULT &elms_bold_12

#endif
