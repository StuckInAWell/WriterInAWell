/*******************************************************************************
 * Size: 16 px
 * Bpp: 1
 * Opts: --size 16 --bpp 1 --no-compress --format lvgl --font assets/fonts/SpaceMono-Regular.ttf -r 0x20-0x7E -r 0xA0-0xFF -r 0x2000-0x206F -r 0x2190-0x21FF -r 0xF00B -r 0xF013 -r 0xF07B -r 0xF0C7 -r 0xF0C9 -r 0xF01E -r 0xF030C -r 0xF1EB -r 0xF293 --lv-fallback lv_font_montserrat_14 --lv-font-name space_mono_16_cr -o src/fonts/space_mono_16_cr.c
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#ifndef SPACE_MONO_16_CR
#define SPACE_MONO_16_CR 1
#endif

#if SPACE_MONO_16_CR

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */
    0x0,

    /* U+0021 "!" */
    0x55, 0x55, 0x3c,

    /* U+0022 "\"" */
    0xde, 0xd2, 0x90,

    /* U+0023 "#" */
    0x24, 0x24, 0xff, 0x24, 0x24, 0xff, 0x24, 0x24,

    /* U+0024 "$" */
    0x11, 0xe9, 0x65, 0x70, 0x79, 0x75, 0x78, 0x40,

    /* U+0025 "%" */
    0x70, 0x88, 0x88, 0x89, 0x76, 0x18, 0x6e, 0x11,
    0x11, 0x11, 0xe,

    /* U+0026 "&" */
    0x7c, 0x60, 0x20, 0x10, 0x4c, 0x21, 0xff, 0x9,
    0x4, 0x82, 0x61, 0x1f, 0x80,

    /* U+0027 "'" */
    0xf5,

    /* U+0028 "(" */
    0x1c, 0xc4, 0x30, 0x82, 0x8, 0x20, 0x82, 0x8,
    0x20, 0xc1, 0x3, 0x7,

    /* U+0029 ")" */
    0xe0, 0xc0, 0x83, 0x4, 0x10, 0x41, 0x4, 0x10,
    0x41, 0xc, 0x23, 0x38,

    /* U+002A "*" */
    0x10, 0x52, 0x74, 0x38, 0xff, 0x3c, 0x56, 0x10,

    /* U+002B "+" */
    0x10, 0x23, 0xf8, 0x81, 0x2, 0x0,

    /* U+002C "," */
    0xf7,

    /* U+002D "-" */
    0xf8,

    /* U+002E "." */
    0xf0,

    /* U+002F "/" */
    0x0, 0x30, 0x82, 0x18, 0x41, 0xc, 0x20, 0x86,
    0x10, 0x43, 0x0, 0x0,

    /* U+0030 "0" */
    0x3c, 0x42, 0x81, 0x81, 0x99, 0x99, 0x81, 0x81,
    0x81, 0x42, 0x3c,

    /* U+0031 "1" */
    0x30, 0x70, 0x50, 0xd0, 0x90, 0x10, 0x10, 0x10,
    0x10, 0x10, 0xff,

    /* U+0032 "2" */
    0x3c, 0x42, 0x81, 0x81, 0x1, 0x6, 0x3c, 0xe0,
    0x80, 0x80, 0xff,

    /* U+0033 "3" */
    0xff, 0x3, 0xe, 0x18, 0x1e, 0x3, 0x81, 0x81,
    0x81, 0x42, 0x3c,

    /* U+0034 "4" */
    0x1c, 0x1c, 0x34, 0x24, 0x64, 0x44, 0xc4, 0x84,
    0xff, 0x4, 0x4,

    /* U+0035 "5" */
    0xfe, 0x80, 0x80, 0x9c, 0xa2, 0xe1, 0x1, 0x81,
    0x81, 0x42, 0x3c,

    /* U+0036 "6" */
    0x38, 0x8a, 0xc, 0xb, 0xd8, 0xe0, 0xc1, 0x82,
    0x88, 0xe0,

    /* U+0037 "7" */
    0xff, 0x1, 0x2, 0x6, 0x4, 0x8, 0x18, 0x30,
    0x20, 0x40, 0x40,

    /* U+0038 "8" */
    0x7c, 0xc6, 0x82, 0x82, 0xc6, 0x7c, 0xc2, 0x81,
    0x81, 0xc3, 0x3c,

    /* U+0039 "9" */
    0x38, 0x8a, 0xc, 0x18, 0x38, 0xde, 0x81, 0x82,
    0x89, 0xe0,

    /* U+003A ":" */
    0xf0, 0xf,

    /* U+003B ";" */
    0xf0, 0xf, 0x70,

    /* U+003C "<" */
    0x6, 0x7b, 0x84, 0xe, 0x7, 0x81, 0x80,

    /* U+003D "=" */
    0xfe, 0x3, 0xf8,

    /* U+003E ">" */
    0x81, 0xf0, 0x78, 0x11, 0xff, 0x20, 0x0,

    /* U+003F "?" */
    0x3d, 0x8e, 0xc, 0x10, 0x23, 0x84, 0x8, 0x0,
    0x60, 0xc0,

    /* U+0040 "@" */
    0x7e, 0x3, 0x75, 0xcd, 0x85, 0x85, 0x85, 0xcd,
    0x7f,

    /* U+0041 "A" */
    0x1c, 0x7, 0x81, 0x60, 0x48, 0x33, 0x8, 0xc2,
    0x11, 0xfc, 0x41, 0x90, 0x2c, 0x8,

    /* U+0042 "B" */
    0xfe, 0x43, 0x41, 0x41, 0x43, 0x7c, 0x43, 0x41,
    0x41, 0x43, 0xfe,

    /* U+0043 "C" */
    0x3c, 0x42, 0x81, 0x81, 0x80, 0x80, 0x80, 0x81,
    0x81, 0x42, 0x3c,

    /* U+0044 "D" */
    0xfc, 0x42, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
    0x41, 0x42, 0xfc,

    /* U+0045 "E" */
    0xfd, 0x2, 0x4, 0x8, 0x1f, 0xa0, 0x40, 0x81,
    0x3, 0xf8,

    /* U+0046 "F" */
    0xff, 0x2, 0x4, 0x8, 0x1f, 0xa0, 0x40, 0x81,
    0x2, 0x0,

    /* U+0047 "G" */
    0x3c, 0x42, 0x81, 0x81, 0x80, 0x80, 0x9f, 0x81,
    0x81, 0x43, 0x3d,

    /* U+0048 "H" */
    0x83, 0x6, 0xc, 0x18, 0x3f, 0xe0, 0xc1, 0x83,
    0x6, 0x8,

    /* U+0049 "I" */
    0xfe, 0x20, 0x40, 0x81, 0x2, 0x4, 0x8, 0x10,
    0x23, 0xf8,

    /* U+004A "J" */
    0x7, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x82,
    0x82, 0xc6, 0x7c,

    /* U+004B "K" */
    0x82, 0x84, 0x8c, 0x98, 0xb0, 0xe0, 0xb0, 0x98,
    0x8c, 0x86, 0x82,

    /* U+004C "L" */
    0x81, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x81,
    0x3, 0xf8,

    /* U+004D "M" */
    0xc3, 0xe7, 0xe7, 0xa5, 0xa5, 0xa5, 0xa5, 0xbd,
    0xbd, 0x99, 0x99,

    /* U+004E "N" */
    0xe3, 0xc6, 0x8d, 0x9b, 0x32, 0x66, 0xcd, 0x8b,
    0x1e, 0x38,

    /* U+004F "O" */
    0x3c, 0x42, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81,
    0x81, 0x42, 0x3c,

    /* U+0050 "P" */
    0xfd, 0xe, 0xc, 0x18, 0x7f, 0xa0, 0x40, 0x81,
    0x2, 0x0,

    /* U+0051 "Q" */
    0x38, 0x8a, 0xc, 0x18, 0x30, 0x60, 0xc1, 0x82,
    0x88, 0xe0, 0x81, 0xc0,

    /* U+0052 "R" */
    0xfd, 0xe, 0xc, 0x18, 0x7f, 0xa0, 0xc1, 0x83,
    0x6, 0x8,

    /* U+0053 "S" */
    0x3c, 0x63, 0x41, 0x41, 0x70, 0x1e, 0x3, 0x81,
    0x81, 0x43, 0x3e,

    /* U+0054 "T" */
    0xff, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
    0x10, 0x10, 0x10,

    /* U+0055 "U" */
    0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81,
    0x81, 0x42, 0x3c,

    /* U+0056 "V" */
    0x40, 0xa0, 0xd8, 0x6c, 0x22, 0x11, 0x18, 0xcc,
    0x24, 0x16, 0xf, 0x7, 0x0,

    /* U+0057 "W" */
    0x8c, 0xf7, 0x2d, 0xc9, 0x72, 0x56, 0x95, 0xa5,
    0x29, 0x4a, 0x53, 0x94, 0xe7, 0x38,

    /* U+0058 "X" */
    0xc1, 0xa0, 0x98, 0xc6, 0xc1, 0x40, 0xe0, 0x50,
    0x6c, 0x63, 0x20, 0xb0, 0x60,

    /* U+0059 "Y" */
    0xc1, 0xa0, 0x98, 0xc4, 0x43, 0x60, 0xa0, 0x70,
    0x10, 0x8, 0x4, 0x2, 0x0,

    /* U+005A "Z" */
    0xff, 0x1, 0x2, 0x6, 0xc, 0x18, 0x30, 0x60,
    0x40, 0x80, 0xff,

    /* U+005B "[" */
    0xf2, 0x49, 0x24, 0x92, 0x49, 0x27,

    /* U+005C "\\" */
    0x3, 0x4, 0x10, 0x60, 0x82, 0xc, 0x10, 0x41,
    0x82, 0x8, 0x30, 0x0,

    /* U+005D "]" */
    0xe4, 0x92, 0x49, 0x24, 0x92, 0x4f,

    /* U+005E "^" */
    0x38, 0x3c, 0x64, 0x46, 0xc2,

    /* U+005F "_" */
    0xfe,

    /* U+0060 "`" */
    0x90,

    /* U+0061 "a" */
    0x3d, 0x21, 0xa0, 0x50, 0x28, 0x14, 0x9, 0xc,
    0x7b,

    /* U+0062 "b" */
    0x80, 0x80, 0x80, 0xbc, 0xc2, 0x81, 0x81, 0x81,
    0x81, 0xc2, 0xbc,

    /* U+0063 "c" */
    0x3c, 0x46, 0x82, 0x80, 0x80, 0x83, 0x46, 0x3c,

    /* U+0064 "d" */
    0x1, 0x1, 0x1, 0x3d, 0x43, 0x81, 0x81, 0x81,
    0x81, 0x43, 0x3d,

    /* U+0065 "e" */
    0x3c, 0x42, 0x81, 0xff, 0x80, 0x81, 0x42, 0x3c,

    /* U+0066 "f" */
    0x1e, 0x20, 0x47, 0xf1, 0x2, 0x4, 0x8, 0x10,
    0x20, 0x40,

    /* U+0067 "g" */
    0x3d, 0x43, 0x81, 0x81, 0x81, 0x81, 0x43, 0x3d,
    0x1, 0x1, 0x7f,

    /* U+0068 "h" */
    0x81, 0x2, 0x5, 0xec, 0x70, 0x60, 0xc1, 0x83,
    0x6, 0x8,

    /* U+0069 "i" */
    0x30, 0x60, 0x7, 0x81, 0x2, 0x4, 0x8, 0x10,
    0x23, 0xf8,

    /* U+006A "j" */
    0x18, 0xc1, 0xf0, 0x84, 0x21, 0x8, 0x42, 0x10,
    0xfc,

    /* U+006B "k" */
    0x81, 0x2, 0x4, 0x69, 0x92, 0x2c, 0x70, 0xb1,
    0x32, 0x10,

    /* U+006C "l" */
    0xf0, 0x20, 0x40, 0x81, 0x2, 0x4, 0x8, 0x10,
    0x23, 0xf8,

    /* U+006D "m" */
    0xb7, 0x64, 0x62, 0x31, 0x18, 0x8c, 0x46, 0x23,
    0x11,

    /* U+006E "n" */
    0xbd, 0x8e, 0xc, 0x18, 0x30, 0x60, 0xc1,

    /* U+006F "o" */
    0x3c, 0x42, 0x81, 0x81, 0x81, 0x81, 0x42, 0x3c,

    /* U+0070 "p" */
    0xbc, 0xc2, 0x81, 0x81, 0x81, 0x81, 0xc2, 0xbc,
    0x80, 0x80, 0x80,

    /* U+0071 "q" */
    0x3d, 0x43, 0x81, 0x81, 0x81, 0x81, 0x43, 0x3d,
    0x1, 0x1, 0x1,

    /* U+0072 "r" */
    0xee, 0x33, 0x21, 0x20, 0x20, 0x20, 0x20, 0xf8,

    /* U+0073 "s" */
    0x7a, 0x38, 0x3c, 0x1e, 0x18, 0x5e,

    /* U+0074 "t" */
    0x10, 0x20, 0x47, 0xf1, 0x2, 0x4, 0x8, 0x10,
    0x20, 0x78,

    /* U+0075 "u" */
    0x83, 0x6, 0xc, 0x18, 0x30, 0x71, 0xbd,

    /* U+0076 "v" */
    0xc3, 0xc2, 0x42, 0x66, 0x64, 0x2c, 0x3c, 0x38,

    /* U+0077 "w" */
    0xcc, 0x93, 0x25, 0xc9, 0x5a, 0x56, 0x94, 0xe7,
    0x31, 0xcc,

    /* U+0078 "x" */
    0x42, 0x66, 0x24, 0x1c, 0x18, 0x3c, 0x24, 0x42,

    /* U+0079 "y" */
    0x83, 0x6, 0xc, 0x18, 0x30, 0x71, 0xbd, 0x2,
    0x5, 0xf8,

    /* U+007A "z" */
    0xfe, 0x4, 0x30, 0xc3, 0xc, 0x20, 0x7f,

    /* U+007B "{" */
    0x74, 0x44, 0x44, 0x44, 0x84, 0x44, 0x44, 0x47,

    /* U+007C "|" */
    0xff, 0xff,

    /* U+007D "}" */
    0xe1, 0x8, 0x42, 0x10, 0x84, 0x19, 0x8, 0x42,
    0x10, 0x9c,

    /* U+007E "~" */
    0x31, 0x56, 0x8c,

    /* U+00A0 " " */
    0x0,

    /* U+00A1 "¡" */
    0xf1, 0x55, 0x54,

    /* U+00A2 "¢" */
    0x8, 0x3e, 0x6b, 0x89, 0x88, 0x88, 0x89, 0x6b,
    0x3e, 0x8,

    /* U+00A3 "£" */
    0x39, 0x14, 0x50, 0xf1, 0x4, 0x3f,

    /* U+00A4 "¤" */
    0xbd, 0x42, 0x81, 0x81, 0x81, 0x81, 0x42, 0xbd,

    /* U+00A5 "¥" */
    0x42, 0x46, 0x2c, 0x28, 0x7e, 0x10, 0x7e, 0x10,

    /* U+00A6 "¦" */
    0xfe, 0x7f,

    /* U+00A7 "§" */
    0x69, 0xcb, 0xf3, 0x96,

    /* U+00A8 "¨" */
    0xde, 0xc0,

    /* U+00A9 "©" */
    0x3c, 0x42, 0xb9, 0xa9, 0xa1, 0xa9, 0x7a, 0x3c,

    /* U+00AA "ª" */
    0x7c, 0x62, 0xf0,

    /* U+00AB "«" */
    0x1, 0x37, 0x6c, 0xd8, 0x90, 0xfc, 0x37, 0x11,

    /* U+00AC "¬" */
    0xfe, 0x4, 0x8,

    /* U+00AE "®" */
    0x3c, 0x42, 0x9d, 0x95, 0x9d, 0x95, 0x42, 0x3c,

    /* U+00AF "¯" */
    0xf0,

    /* U+00B0 "°" */
    0x74, 0x62, 0xe0,

    /* U+00B1 "±" */
    0x10, 0x23, 0xf8, 0x81, 0x2, 0x0, 0x7f,

    /* U+00B2 "²" */
    0xf7, 0x70,

    /* U+00B3 "³" */
    0x72, 0x97,

    /* U+00B4 "´" */
    0xe0,

    /* U+00B5 "µ" */
    0x83, 0x6, 0xc, 0x18, 0x30, 0x71, 0xdd, 0x81,
    0x2, 0x0,

    /* U+00B6 "¶" */
    0x7d, 0x6a, 0xd2, 0x94, 0xa0,

    /* U+00B7 "·" */
    0xf0,

    /* U+00B8 "¸" */
    0xf7, 0x80,

    /* U+00B9 "¹" */
    0xd9, 0x70,

    /* U+00BA "º" */
    0x74, 0x62, 0xe0,

    /* U+00BB "»" */
    0x80, 0xec, 0x36, 0x1b, 0x9, 0x3f, 0xec, 0x88,

    /* U+00BC "¼" */
    0xc0, 0xc0, 0x40, 0xe2, 0xc, 0x10, 0x60, 0x86,
    0xa, 0xf, 0x2,

    /* U+00BD "½" */
    0xc0, 0xc0, 0x40, 0x40, 0xe6, 0x18, 0x66, 0x89,
    0x2, 0xc, 0xe,

    /* U+00BE "¾" */
    0x70, 0x18, 0xc, 0x2, 0x7, 0x30, 0x60, 0xc0,
    0x86, 0x5, 0x3, 0xc0, 0x40,

    /* U+00BF "¿" */
    0x18, 0x30, 0x0, 0x47, 0x98, 0x20, 0xc1, 0x82,
    0x88, 0xe0,

    /* U+00C0 "À" */
    0x18, 0x2, 0x0, 0x0, 0x70, 0x1e, 0x5, 0x81,
    0x20, 0xcc, 0x23, 0x8, 0x47, 0xf1, 0x6, 0x40,
    0xb0, 0x20,

    /* U+00C1 "Á" */
    0x6, 0x1, 0x0, 0x0, 0x70, 0x1e, 0x5, 0x81,
    0x20, 0xcc, 0x23, 0x8, 0x47, 0xf1, 0x6, 0x40,
    0xb0, 0x20,

    /* U+00C2 "Â" */
    0xc, 0x5, 0x80, 0x0, 0x70, 0x1e, 0x5, 0x81,
    0x20, 0xcc, 0x23, 0x8, 0x47, 0xf1, 0x6, 0x40,
    0xb0, 0x20,

    /* U+00C3 "Ã" */
    0x18, 0x1, 0x80, 0x0, 0x70, 0x1e, 0x5, 0x81,
    0x20, 0xcc, 0x23, 0x8, 0x47, 0xf1, 0x6, 0x40,
    0xb0, 0x20,

    /* U+00C4 "Ä" */
    0x36, 0x1b, 0x0, 0x3, 0x81, 0x40, 0xa0, 0xd8,
    0x44, 0x22, 0x31, 0x9f, 0xc8, 0x2c, 0x1e, 0xc,

    /* U+00C5 "Å" */
    0x1c, 0x4, 0x81, 0xc0, 0x0, 0x1c, 0x5, 0x81,
    0x60, 0xc8, 0x23, 0x8, 0xc6, 0x11, 0xfc, 0x41,
    0xb0, 0x2c, 0x8,

    /* U+00C6 "Æ" */
    0xf, 0x8e, 0x5, 0x2, 0x83, 0x41, 0xbc, 0x90,
    0x78, 0x64, 0x22, 0x11, 0xe0,

    /* U+00C7 "Ç" */
    0x3c, 0x42, 0x81, 0x81, 0x80, 0x80, 0x80, 0x81,
    0x81, 0x42, 0x3c, 0x1c, 0x14, 0x1c,

    /* U+00C8 "È" */
    0x60, 0x40, 0x7, 0xe8, 0x10, 0x20, 0x40, 0xfd,
    0x2, 0x4, 0x8, 0x1f, 0xc0,

    /* U+00C9 "É" */
    0x18, 0x20, 0x7, 0xe8, 0x10, 0x20, 0x40, 0xfd,
    0x2, 0x4, 0x8, 0x1f, 0xc0,

    /* U+00CA "Ê" */
    0x30, 0xd0, 0x7, 0xe8, 0x10, 0x20, 0x40, 0xfd,
    0x2, 0x4, 0x8, 0x1f, 0xc0,

    /* U+00CB "Ë" */
    0x6c, 0xd8, 0x7, 0xf8, 0x10, 0x20, 0x40, 0xff,
    0x2, 0x4, 0x8, 0x1f, 0xc0,

    /* U+00CC "Ì" */
    0x20, 0x20, 0x7, 0xf1, 0x2, 0x4, 0x8, 0x10,
    0x20, 0x40, 0x81, 0x1f, 0xc0,

    /* U+00CD "Í" */
    0x8, 0x20, 0x7, 0xf1, 0x2, 0x4, 0x8, 0x10,
    0x20, 0x40, 0x81, 0x1f, 0xc0,

    /* U+00CE "Î" */
    0x38, 0x50, 0x7, 0xf1, 0x2, 0x4, 0x8, 0x10,
    0x20, 0x40, 0x81, 0x1f, 0xc0,

    /* U+00CF "Ï" */
    0x6c, 0xd8, 0x7, 0xf1, 0x2, 0x4, 0x8, 0x10,
    0x20, 0x40, 0x81, 0x1f, 0xc0,

    /* U+00D0 "Ð" */
    0xfc, 0x42, 0x41, 0x41, 0x41, 0xf1, 0x41, 0x41,
    0x41, 0x42, 0xfc,

    /* U+00D1 "Ñ" */
    0x34, 0xb8, 0x7, 0x1e, 0x34, 0x6c, 0xd9, 0x93,
    0x36, 0x6c, 0x58, 0xf1, 0xc0,

    /* U+00D2 "Ò" */
    0x30, 0x10, 0x0, 0x3c, 0x42, 0x81, 0x81, 0x81,
    0x81, 0x81, 0x81, 0x81, 0x42, 0x3c,

    /* U+00D3 "Ó" */
    0xc, 0x8, 0x0, 0x3c, 0x42, 0x81, 0x81, 0x81,
    0x81, 0x81, 0x81, 0x81, 0x42, 0x3c,

    /* U+00D4 "Ô" */
    0x18, 0x24, 0x0, 0x3c, 0x42, 0x81, 0x81, 0x81,
    0x81, 0x81, 0x81, 0x81, 0x42, 0x3c,

    /* U+00D5 "Õ" */
    0x32, 0x4c, 0x0, 0x3c, 0x42, 0x81, 0x81, 0x81,
    0x81, 0x81, 0x81, 0x81, 0x42, 0x3c,

    /* U+00D6 "Ö" */
    0x66, 0x66, 0x0, 0x0, 0x3c, 0x42, 0x81, 0x81,
    0x81, 0x81, 0x81, 0x81, 0x81, 0x42, 0x3c,

    /* U+00D7 "×" */
    0x83, 0x37, 0x8c, 0x7b, 0x38, 0x0,

    /* U+00D8 "Ø" */
    0x2, 0x3e, 0x4e, 0x8d, 0x8d, 0x99, 0x99, 0x91,
    0xb1, 0xb1, 0x62, 0x7c, 0x40,

    /* U+00D9 "Ù" */
    0x30, 0x10, 0x0, 0x81, 0x81, 0x81, 0x81, 0x81,
    0x81, 0x81, 0x81, 0x81, 0x42, 0x3c,

    /* U+00DA "Ú" */
    0xc, 0x8, 0x0, 0x81, 0x81, 0x81, 0x81, 0x81,
    0x81, 0x81, 0x81, 0x81, 0x42, 0x3c,

    /* U+00DB "Û" */
    0x3c, 0x24, 0x0, 0x81, 0x81, 0x81, 0x81, 0x81,
    0x81, 0x81, 0x81, 0x81, 0x42, 0x3c,

    /* U+00DC "Ü" */
    0x6c, 0x6c, 0x0, 0x0, 0x81, 0x81, 0x81, 0x81,
    0x81, 0x81, 0x81, 0x81, 0x81, 0x42, 0x3c,

    /* U+00DD "Ý" */
    0x4, 0x4, 0x0, 0x18, 0x34, 0x13, 0x18, 0x88,
    0x6c, 0x14, 0xe, 0x2, 0x1, 0x0, 0x80, 0x40,

    /* U+00DE "Þ" */
    0x81, 0x3, 0xf4, 0x38, 0x30, 0x60, 0xc3, 0xfd,
    0x2, 0x0,

    /* U+00DF "ß" */
    0x3c, 0x62, 0x42, 0xc6, 0x4c, 0x4e, 0x43, 0x41,
    0x51, 0x51, 0x4e,

    /* U+00E0 "à" */
    0x30, 0x8, 0x0, 0x7, 0xa4, 0x34, 0xa, 0x5,
    0x2, 0x81, 0x21, 0x8f, 0x60,

    /* U+00E1 "á" */
    0xc, 0x4, 0x0, 0x7, 0xa4, 0x34, 0xa, 0x5,
    0x2, 0x81, 0x21, 0x8f, 0x60,

    /* U+00E2 "â" */
    0x1c, 0x1a, 0x0, 0x7, 0xa4, 0x34, 0xa, 0x5,
    0x2, 0x81, 0x21, 0x8f, 0x60,

    /* U+00E3 "ã" */
    0x32, 0x26, 0x0, 0x7, 0xa4, 0x34, 0xa, 0x5,
    0x2, 0x81, 0x21, 0x8f, 0x60,

    /* U+00E4 "ä" */
    0x6c, 0x36, 0x0, 0x7, 0xa4, 0x34, 0xa, 0x5,
    0x2, 0x81, 0x23, 0x8f, 0x60,

    /* U+00E5 "å" */
    0x18, 0x16, 0x6, 0x0, 0x3, 0xd2, 0x1a, 0x5,
    0x2, 0x81, 0x40, 0x90, 0xc7, 0xb0,

    /* U+00E6 "æ" */
    0x79, 0xa8, 0x4b, 0xf9, 0x12, 0x65, 0x3e,

    /* U+00E7 "ç" */
    0x3c, 0x42, 0x83, 0x80, 0x80, 0x83, 0x42, 0x3c,
    0x1c, 0x4, 0x1c,

    /* U+00E8 "è" */
    0x30, 0x10, 0x0, 0x3c, 0x42, 0x81, 0xff, 0x80,
    0x81, 0x42, 0x3c,

    /* U+00E9 "é" */
    0xc, 0x8, 0x0, 0x3c, 0x42, 0x81, 0xff, 0x80,
    0x81, 0x42, 0x3c,

    /* U+00EA "ê" */
    0x3c, 0x34, 0x0, 0x3c, 0x42, 0x81, 0xff, 0x80,
    0x81, 0x42, 0x3c,

    /* U+00EB "ë" */
    0x6c, 0x6c, 0x0, 0x3c, 0x42, 0x81, 0xff, 0x80,
    0x81, 0x42, 0x3c,

    /* U+00EC "ì" */
    0x20, 0x40, 0x7, 0x81, 0x2, 0x4, 0x8, 0x10,
    0x23, 0xf8,

    /* U+00ED "í" */
    0x18, 0x20, 0x7, 0x81, 0x2, 0x4, 0x8, 0x10,
    0x23, 0xf8,

    /* U+00EE "î" */
    0x30, 0xd0, 0x7, 0x81, 0x2, 0x4, 0x8, 0x10,
    0x23, 0xf8,

    /* U+00EF "ï" */
    0xcd, 0x98, 0x7, 0x81, 0x2, 0x4, 0x8, 0x10,
    0x23, 0xf8,

    /* U+00F0 "ð" */
    0x38, 0x1c, 0x6, 0x3e, 0x43, 0x81, 0x81, 0x81,
    0x81, 0x42, 0x3c,

    /* U+00F1 "ñ" */
    0x30, 0x98, 0x5, 0xec, 0x70, 0x60, 0xc1, 0x83,
    0x6, 0x8,

    /* U+00F2 "ò" */
    0x30, 0x10, 0x0, 0x3c, 0x42, 0x81, 0x81, 0x81,
    0x81, 0x42, 0x3c,

    /* U+00F3 "ó" */
    0xc, 0x8, 0x0, 0x3c, 0x42, 0x81, 0x81, 0x81,
    0x81, 0x42, 0x3c,

    /* U+00F4 "ô" */
    0x18, 0x3c, 0x0, 0x3c, 0x42, 0x81, 0x81, 0x81,
    0x81, 0x42, 0x3c,

    /* U+00F5 "õ" */
    0x32, 0x4c, 0x0, 0x3c, 0x42, 0x81, 0x81, 0x81,
    0x81, 0x42, 0x3c,

    /* U+00F6 "ö" */
    0x66, 0x66, 0x0, 0x3c, 0x42, 0x81, 0x81, 0x81,
    0x81, 0x42, 0x3c,

    /* U+00F7 "÷" */
    0x30, 0x60, 0x7, 0xf0, 0x6, 0xc, 0x0,

    /* U+00F8 "ø" */
    0x4, 0x3c, 0x4e, 0x89, 0x99, 0x99, 0x91, 0x72,
    0x3c, 0x20,

    /* U+00F9 "ù" */
    0x20, 0x60, 0x4, 0x18, 0x30, 0x60, 0xc1, 0x83,
    0x8d, 0xe8,

    /* U+00FA "ú" */
    0x8, 0x30, 0x4, 0x18, 0x30, 0x60, 0xc1, 0x83,
    0x8d, 0xe8,

    /* U+00FB "û" */
    0x38, 0xd8, 0x4, 0x18, 0x30, 0x60, 0xc1, 0x83,
    0x8d, 0xe8,

    /* U+00FC "ü" */
    0x6c, 0xd8, 0x4, 0x18, 0x30, 0x60, 0xc1, 0x83,
    0x8d, 0xe8,

    /* U+00FD "ý" */
    0x8, 0x20, 0x4, 0x18, 0x30, 0x60, 0xc1, 0x83,
    0x8d, 0xe8, 0x10, 0x2f, 0xc0,

    /* U+00FE "þ" */
    0x80, 0x80, 0x80, 0xbc, 0xc2, 0x81, 0x81, 0x81,
    0x81, 0xc2, 0xbc, 0x80, 0x80, 0x80,

    /* U+00FF "ÿ" */
    0x6c, 0xd8, 0x4, 0x18, 0x30, 0x60, 0xc1, 0x83,
    0x8d, 0xe8, 0x10, 0x2f, 0xc0,

    /* U+2013 "–" */
    0xfe,

    /* U+2014 "—" */
    0xff, 0x80,

    /* U+2018 "‘" */
    0xef,

    /* U+2019 "’" */
    0xf7,

    /* U+201A "‚" */
    0xf7,

    /* U+201C "“" */
    0xdc, 0xb7, 0xb0,

    /* U+201D "”" */
    0xdf, 0xdb, 0xb0,

    /* U+201E "„" */
    0xdf, 0xdb, 0xb0,

    /* U+2020 "†" */
    0x21, 0x3e, 0x42, 0x10, 0x84,

    /* U+2021 "‡" */
    0x21, 0x3e, 0x42, 0x7c, 0x84,

    /* U+2022 "•" */
    0x6f, 0x60,

    /* U+2026 "…" */
    0xdb, 0xdb,

    /* U+2030 "‰" */
    0x70, 0x88, 0x88, 0x88, 0x76, 0x18, 0x6d, 0xb,
    0xb, 0xb, 0xd,

    /* U+2032 "′" */
    0xfa,

    /* U+2033 "″" */
    0xde, 0xe5, 0x20,

    /* U+2039 "‹" */
    0x3, 0x6c, 0x8e, 0x31,

    /* U+203A "›" */
    0x87, 0xc, 0x30, 0x9f, 0x90,

    /* U+2044 "⁄" */
    0x1, 0x6, 0x18, 0x60, 0x80,

    /* U+2190 "←" */
    0x20, 0x30, 0x30, 0x1f, 0xfc, 0x3, 0x0, 0x80,

    /* U+2191 "↑" */
    0x38, 0xf9, 0x50, 0x81, 0x2, 0x4, 0x8,

    /* U+2192 "→" */
    0x2, 0x1, 0x80, 0x7f, 0xf0, 0x18, 0x18, 0x8,

    /* U+2193 "↓" */
    0x10, 0x20, 0x40, 0x81, 0xa, 0x9f, 0x1c,

    /* U+2194 "↔" */
    0x22, 0x31, 0xb0, 0x7f, 0xfc, 0x1b, 0x18, 0x88,

    /* U+2195 "↕" */
    0x38, 0xf9, 0x50, 0x81, 0xa, 0x9f, 0x1c,

    /* U+2196 "↖" */
    0xf1, 0x82, 0x84, 0x80, 0x80, 0x80, 0x80,

    /* U+2197 "↗" */
    0x1e, 0xc, 0x28, 0x92, 0x8, 0x20, 0x0,

    /* U+2198 "↘" */
    0x80, 0x80, 0x80, 0x90, 0xa0, 0xc7, 0x80,

    /* U+2199 "↙" */
    0x2, 0x8, 0x24, 0x8a, 0x18, 0x3c, 0x0,

    /* U+F00B "" */
    0xff, 0xe8, 0x1f, 0xfe, 0x81, 0xa0, 0x7f, 0xfa,
    0x7, 0xff,

    /* U+F013 "" */
    0x1c, 0x2e, 0xbf, 0xfe, 0xf6, 0x33, 0x1b, 0xdf,
    0xff, 0x5d, 0xe, 0x0,

    /* U+F01E "" */
    0x1c, 0x9f, 0x50, 0x70, 0xf8, 0x4, 0x2, 0x0,
    0xc6, 0x3e, 0x0,

    /* U+F07B "" */
    0xf8, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff,

    /* U+F0C7 "" */
    0xff, 0x30, 0xec, 0x2e, 0xf9, 0x80, 0x67, 0x19,
    0xc6, 0x71, 0x80, 0x7f, 0xf0,

    /* U+F0C9 "" */
    0xff, 0xc0, 0x0, 0x3, 0xff, 0x0, 0x0, 0x0,
    0x3, 0xff,

    /* U+F1EB "" */
    0x1e, 0x18, 0x68, 0x4, 0x78, 0x21, 0x3, 0x0,
    0xc0,

    /* U+F293 "" */
    0x1f, 0x1e, 0xe7, 0xbf, 0xe7, 0xca, 0xfc, 0x7f,
    0xbf, 0xc7, 0xea, 0xfe, 0x37, 0x9d, 0xee, 0x1f,
    0x0,

    /* U+F030C "󰌌" */
    0xff, 0xdb, 0x7f, 0xf6, 0xde, 0x9f, 0xfc
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 157, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 157, .box_w = 2, .box_h = 11, .ofs_x = 4, .ofs_y = 0},
    {.bitmap_index = 4, .adv_w = 157, .box_w = 5, .box_h = 4, .ofs_x = 2, .ofs_y = 7},
    {.bitmap_index = 7, .adv_w = 157, .box_w = 8, .box_h = 8, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 15, .adv_w = 157, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 23, .adv_w = 157, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 34, .adv_w = 157, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 47, .adv_w = 157, .box_w = 2, .box_h = 4, .ofs_x = 4, .ofs_y = 7},
    {.bitmap_index = 48, .adv_w = 157, .box_w = 6, .box_h = 16, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 60, .adv_w = 157, .box_w = 6, .box_h = 16, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 72, .adv_w = 157, .box_w = 8, .box_h = 8, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 80, .adv_w = 157, .box_w = 7, .box_h = 6, .ofs_x = 2, .ofs_y = 2},
    {.bitmap_index = 86, .adv_w = 157, .box_w = 2, .box_h = 4, .ofs_x = 4, .ofs_y = -2},
    {.bitmap_index = 87, .adv_w = 157, .box_w = 5, .box_h = 1, .ofs_x = 2, .ofs_y = 4},
    {.bitmap_index = 88, .adv_w = 157, .box_w = 2, .box_h = 2, .ofs_x = 4, .ofs_y = 0},
    {.bitmap_index = 89, .adv_w = 157, .box_w = 6, .box_h = 15, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 101, .adv_w = 157, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 112, .adv_w = 157, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 123, .adv_w = 157, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 134, .adv_w = 157, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 145, .adv_w = 157, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 156, .adv_w = 157, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 167, .adv_w = 157, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 177, .adv_w = 157, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 188, .adv_w = 157, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 199, .adv_w = 157, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 209, .adv_w = 157, .box_w = 2, .box_h = 8, .ofs_x = 4, .ofs_y = 0},
    {.bitmap_index = 211, .adv_w = 157, .box_w = 2, .box_h = 10, .ofs_x = 4, .ofs_y = -2},
    {.bitmap_index = 214, .adv_w = 157, .box_w = 7, .box_h = 7, .ofs_x = 2, .ofs_y = 2},
    {.bitmap_index = 221, .adv_w = 157, .box_w = 7, .box_h = 3, .ofs_x = 2, .ofs_y = 4},
    {.bitmap_index = 224, .adv_w = 157, .box_w = 7, .box_h = 7, .ofs_x = 2, .ofs_y = 2},
    {.bitmap_index = 231, .adv_w = 157, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 241, .adv_w = 157, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 250, .adv_w = 157, .box_w = 10, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 264, .adv_w = 157, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 275, .adv_w = 157, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 286, .adv_w = 157, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 297, .adv_w = 157, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 307, .adv_w = 157, .box_w = 7, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 317, .adv_w = 157, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 328, .adv_w = 157, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 338, .adv_w = 157, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 348, .adv_w = 157, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 359, .adv_w = 157, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 370, .adv_w = 157, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 380, .adv_w = 157, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 391, .adv_w = 157, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 401, .adv_w = 157, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 412, .adv_w = 157, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 422, .adv_w = 157, .box_w = 7, .box_h = 13, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 434, .adv_w = 157, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 444, .adv_w = 157, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 455, .adv_w = 157, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 466, .adv_w = 157, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 477, .adv_w = 157, .box_w = 9, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 490, .adv_w = 157, .box_w = 10, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 504, .adv_w = 157, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 517, .adv_w = 157, .box_w = 9, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 530, .adv_w = 157, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 541, .adv_w = 157, .box_w = 3, .box_h = 16, .ofs_x = 4, .ofs_y = -2},
    {.bitmap_index = 547, .adv_w = 157, .box_w = 6, .box_h = 15, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 559, .adv_w = 157, .box_w = 3, .box_h = 16, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 565, .adv_w = 157, .box_w = 8, .box_h = 5, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 570, .adv_w = 157, .box_w = 7, .box_h = 1, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 571, .adv_w = 157, .box_w = 2, .box_h = 2, .ofs_x = 4, .ofs_y = 9},
    {.bitmap_index = 572, .adv_w = 157, .box_w = 9, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 581, .adv_w = 157, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 592, .adv_w = 157, .box_w = 8, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 600, .adv_w = 157, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 611, .adv_w = 157, .box_w = 8, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 619, .adv_w = 157, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 629, .adv_w = 157, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 640, .adv_w = 157, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 650, .adv_w = 157, .box_w = 7, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 660, .adv_w = 157, .box_w = 5, .box_h = 14, .ofs_x = 2, .ofs_y = -3},
    {.bitmap_index = 669, .adv_w = 157, .box_w = 7, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 679, .adv_w = 157, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 689, .adv_w = 157, .box_w = 9, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 698, .adv_w = 157, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 705, .adv_w = 157, .box_w = 8, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 713, .adv_w = 157, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 724, .adv_w = 157, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 735, .adv_w = 157, .box_w = 8, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 743, .adv_w = 157, .box_w = 6, .box_h = 8, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 749, .adv_w = 157, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 759, .adv_w = 157, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 766, .adv_w = 157, .box_w = 8, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 774, .adv_w = 157, .box_w = 10, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 784, .adv_w = 157, .box_w = 8, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 792, .adv_w = 157, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 802, .adv_w = 157, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 809, .adv_w = 157, .box_w = 4, .box_h = 16, .ofs_x = 3, .ofs_y = -2},
    {.bitmap_index = 817, .adv_w = 157, .box_w = 1, .box_h = 16, .ofs_x = 4, .ofs_y = -2},
    {.bitmap_index = 819, .adv_w = 157, .box_w = 5, .box_h = 16, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 829, .adv_w = 157, .box_w = 8, .box_h = 3, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 832, .adv_w = 157, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 833, .adv_w = 157, .box_w = 2, .box_h = 11, .ofs_x = 4, .ofs_y = -3},
    {.bitmap_index = 836, .adv_w = 157, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 846, .adv_w = 157, .box_w = 6, .box_h = 8, .ofs_x = 2, .ofs_y = 2},
    {.bitmap_index = 852, .adv_w = 157, .box_w = 8, .box_h = 8, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 860, .adv_w = 157, .box_w = 8, .box_h = 8, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 868, .adv_w = 157, .box_w = 1, .box_h = 16, .ofs_x = 4, .ofs_y = -2},
    {.bitmap_index = 870, .adv_w = 157, .box_w = 4, .box_h = 8, .ofs_x = 3, .ofs_y = 3},
    {.bitmap_index = 874, .adv_w = 157, .box_w = 5, .box_h = 2, .ofs_x = 2, .ofs_y = 9},
    {.bitmap_index = 876, .adv_w = 157, .box_w = 8, .box_h = 8, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 884, .adv_w = 157, .box_w = 5, .box_h = 4, .ofs_x = 3, .ofs_y = 7},
    {.bitmap_index = 887, .adv_w = 157, .box_w = 8, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 895, .adv_w = 157, .box_w = 7, .box_h = 3, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 898, .adv_w = 157, .box_w = 8, .box_h = 8, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 906, .adv_w = 157, .box_w = 4, .box_h = 1, .ofs_x = 3, .ofs_y = 9},
    {.bitmap_index = 907, .adv_w = 157, .box_w = 5, .box_h = 4, .ofs_x = 3, .ofs_y = 7},
    {.bitmap_index = 910, .adv_w = 157, .box_w = 7, .box_h = 8, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 917, .adv_w = 157, .box_w = 3, .box_h = 4, .ofs_x = 3, .ofs_y = 7},
    {.bitmap_index = 919, .adv_w = 157, .box_w = 4, .box_h = 4, .ofs_x = 3, .ofs_y = 7},
    {.bitmap_index = 921, .adv_w = 157, .box_w = 2, .box_h = 2, .ofs_x = 4, .ofs_y = 9},
    {.bitmap_index = 922, .adv_w = 157, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 932, .adv_w = 157, .box_w = 5, .box_h = 7, .ofs_x = 2, .ofs_y = 4},
    {.bitmap_index = 937, .adv_w = 157, .box_w = 2, .box_h = 2, .ofs_x = 4, .ofs_y = 4},
    {.bitmap_index = 938, .adv_w = 157, .box_w = 3, .box_h = 3, .ofs_x = 4, .ofs_y = -3},
    {.bitmap_index = 940, .adv_w = 157, .box_w = 3, .box_h = 4, .ofs_x = 4, .ofs_y = 7},
    {.bitmap_index = 942, .adv_w = 157, .box_w = 5, .box_h = 4, .ofs_x = 3, .ofs_y = 7},
    {.bitmap_index = 945, .adv_w = 157, .box_w = 8, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 953, .adv_w = 157, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 964, .adv_w = 157, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 975, .adv_w = 157, .box_w = 9, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 988, .adv_w = 157, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 998, .adv_w = 157, .box_w = 10, .box_h = 14, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1016, .adv_w = 157, .box_w = 10, .box_h = 14, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1034, .adv_w = 157, .box_w = 10, .box_h = 14, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1052, .adv_w = 157, .box_w = 10, .box_h = 14, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1070, .adv_w = 157, .box_w = 9, .box_h = 14, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1086, .adv_w = 157, .box_w = 10, .box_h = 15, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1105, .adv_w = 157, .box_w = 9, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1118, .adv_w = 157, .box_w = 8, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1132, .adv_w = 157, .box_w = 7, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1145, .adv_w = 157, .box_w = 7, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1158, .adv_w = 157, .box_w = 7, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1171, .adv_w = 157, .box_w = 7, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1184, .adv_w = 157, .box_w = 7, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1197, .adv_w = 157, .box_w = 7, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1210, .adv_w = 157, .box_w = 7, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1223, .adv_w = 157, .box_w = 7, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1236, .adv_w = 157, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1247, .adv_w = 157, .box_w = 7, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1260, .adv_w = 157, .box_w = 8, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1274, .adv_w = 157, .box_w = 8, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1288, .adv_w = 157, .box_w = 8, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1302, .adv_w = 157, .box_w = 8, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1316, .adv_w = 157, .box_w = 8, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1331, .adv_w = 157, .box_w = 6, .box_h = 7, .ofs_x = 2, .ofs_y = 2},
    {.bitmap_index = 1337, .adv_w = 157, .box_w = 8, .box_h = 13, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1350, .adv_w = 157, .box_w = 8, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1364, .adv_w = 157, .box_w = 8, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1378, .adv_w = 157, .box_w = 8, .box_h = 14, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1392, .adv_w = 157, .box_w = 8, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1407, .adv_w = 157, .box_w = 9, .box_h = 14, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1423, .adv_w = 157, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1433, .adv_w = 157, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1444, .adv_w = 157, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1457, .adv_w = 157, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1470, .adv_w = 157, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1483, .adv_w = 157, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1496, .adv_w = 157, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1509, .adv_w = 157, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1523, .adv_w = 157, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1530, .adv_w = 157, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1541, .adv_w = 157, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1552, .adv_w = 157, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1563, .adv_w = 157, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1574, .adv_w = 157, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1585, .adv_w = 157, .box_w = 7, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 1595, .adv_w = 157, .box_w = 7, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 1605, .adv_w = 157, .box_w = 7, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 1615, .adv_w = 157, .box_w = 7, .box_h = 11, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 1625, .adv_w = 157, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1636, .adv_w = 157, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1646, .adv_w = 157, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1657, .adv_w = 157, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1668, .adv_w = 157, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1679, .adv_w = 157, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1690, .adv_w = 157, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1701, .adv_w = 157, .box_w = 7, .box_h = 7, .ofs_x = 2, .ofs_y = 2},
    {.bitmap_index = 1708, .adv_w = 157, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1718, .adv_w = 157, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1728, .adv_w = 157, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1738, .adv_w = 157, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1748, .adv_w = 157, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1758, .adv_w = 157, .box_w = 7, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1771, .adv_w = 157, .box_w = 8, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1785, .adv_w = 157, .box_w = 7, .box_h = 14, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1798, .adv_w = 157, .box_w = 7, .box_h = 1, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 1799, .adv_w = 157, .box_w = 9, .box_h = 1, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 1801, .adv_w = 157, .box_w = 2, .box_h = 4, .ofs_x = 4, .ofs_y = 7},
    {.bitmap_index = 1802, .adv_w = 157, .box_w = 2, .box_h = 4, .ofs_x = 4, .ofs_y = 7},
    {.bitmap_index = 1803, .adv_w = 157, .box_w = 2, .box_h = 4, .ofs_x = 4, .ofs_y = -2},
    {.bitmap_index = 1804, .adv_w = 157, .box_w = 5, .box_h = 4, .ofs_x = 2, .ofs_y = 7},
    {.bitmap_index = 1807, .adv_w = 157, .box_w = 5, .box_h = 4, .ofs_x = 2, .ofs_y = 7},
    {.bitmap_index = 1810, .adv_w = 157, .box_w = 5, .box_h = 4, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 1813, .adv_w = 157, .box_w = 5, .box_h = 8, .ofs_x = 2, .ofs_y = 3},
    {.bitmap_index = 1818, .adv_w = 157, .box_w = 5, .box_h = 8, .ofs_x = 2, .ofs_y = 3},
    {.bitmap_index = 1823, .adv_w = 157, .box_w = 4, .box_h = 3, .ofs_x = 3, .ofs_y = 4},
    {.bitmap_index = 1825, .adv_w = 157, .box_w = 8, .box_h = 2, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1827, .adv_w = 157, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1838, .adv_w = 157, .box_w = 2, .box_h = 4, .ofs_x = 4, .ofs_y = 7},
    {.bitmap_index = 1839, .adv_w = 157, .box_w = 5, .box_h = 4, .ofs_x = 2, .ofs_y = 7},
    {.bitmap_index = 1842, .adv_w = 157, .box_w = 4, .box_h = 8, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 1846, .adv_w = 157, .box_w = 5, .box_h = 8, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 1851, .adv_w = 157, .box_w = 8, .box_h = 5, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 1856, .adv_w = 157, .box_w = 9, .box_h = 7, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 1864, .adv_w = 157, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 1871, .adv_w = 157, .box_w = 9, .box_h = 7, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 1879, .adv_w = 157, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 1886, .adv_w = 157, .box_w = 9, .box_h = 7, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 1894, .adv_w = 157, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 1901, .adv_w = 157, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 1908, .adv_w = 157, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 1915, .adv_w = 157, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 1922, .adv_w = 157, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 1929, .adv_w = 157, .box_w = 10, .box_h = 8, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 1939, .adv_w = 157, .box_w = 9, .box_h = 10, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 1951, .adv_w = 157, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 1962, .adv_w = 157, .box_w = 10, .box_h = 8, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 1972, .adv_w = 157, .box_w = 10, .box_h = 10, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 1985, .adv_w = 157, .box_w = 10, .box_h = 8, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 1995, .adv_w = 157, .box_w = 10, .box_h = 7, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 2004, .adv_w = 157, .box_w = 10, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2021, .adv_w = 157, .box_w = 9, .box_h = 6, .ofs_x = 0, .ofs_y = 3}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_3[] = {
    0x0, 0x1, 0x5, 0x6, 0x7, 0x9, 0xa, 0xb,
    0xd, 0xe, 0xf, 0x13, 0x1d, 0x1f, 0x20, 0x26,
    0x27, 0x31, 0x17d, 0x17e, 0x17f, 0x180, 0x181, 0x182,
    0x183, 0x184, 0x185, 0x186, 0xcff8, 0xd000, 0xd00b, 0xd068,
    0xd0b4, 0xd0b6, 0xd1d8, 0xd280
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 95, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 160, .range_length = 13, .glyph_id_start = 96,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 174, .range_length = 82, .glyph_id_start = 109,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 8211, .range_length = 53889, .glyph_id_start = 191,
        .unicode_list = unicode_list_3, .glyph_id_ofs_list = NULL, .list_length = 36, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    },
    {
        .range_start = 983820, .range_length = 1, .glyph_id_start = 227,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 5,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};

extern const lv_font_t lv_font_montserrat_14;


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t space_mono_16_cr = {
#else
lv_font_t space_mono_16_cr = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 18,          /*The maximum line height required by the font*/
    .base_line = 3,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -1,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = &lv_font_montserrat_14,
#endif
    .user_data = NULL,
};



#endif /*#if SPACE_MONO_16_CR*/

