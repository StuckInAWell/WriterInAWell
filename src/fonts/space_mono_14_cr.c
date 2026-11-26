/*******************************************************************************
 * Size: 14 px
 * Bpp: 1
 * Opts: --size 14 --bpp 1 --no-compress --format lvgl --font assets/fonts/SpaceMono-Regular.ttf -r 0x20-0x7E -r 0xA0-0xFF -r 0x2000-0x206F -r 0x2190-0x21FF -r 0xF00B -r 0xF013 -r 0xF07B -r 0xF0C7 -r 0xF0C9 -r 0xF01E -r 0xF030C -r 0xF1EB -r 0xF293 --lv-fallback lv_font_montserrat_14 --lv-font-name space_mono_14_cr -o src/fonts/space_mono_14_cr.c
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#ifndef SPACE_MONO_14_CR
#define SPACE_MONO_14_CR 1
#endif

#if SPACE_MONO_14_CR

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */
    0x0,

    /* U+0021 "!" */
    0xaa, 0xa8, 0xf0,

    /* U+0022 "\"" */
    0xdc, 0xa4,

    /* U+0023 "#" */
    0x24, 0x24, 0xff, 0x24, 0xff, 0x24, 0x24,

    /* U+0024 "$" */
    0x21, 0xea, 0xe8, 0x7a, 0x9a, 0x5e, 0x20,

    /* U+0025 "%" */
    0x61, 0x22, 0x43, 0x20, 0x86, 0x33, 0x9, 0x12,
    0x18,

    /* U+0026 "&" */
    0x78, 0xc0, 0x80, 0x84, 0xc4, 0x3f, 0xc4, 0x84,
    0x84, 0x7c,

    /* U+0027 "'" */
    0xe8,

    /* U+0028 "(" */
    0x1d, 0x84, 0x20, 0x82, 0x8, 0x20, 0x82, 0x8,
    0x10, 0x60, 0x70,

    /* U+0029 ")" */
    0xe0, 0x60, 0x81, 0x4, 0x10, 0x41, 0x4, 0x10,
    0x42, 0x1b, 0x80,

    /* U+002A "*" */
    0x10, 0xa8, 0xe7, 0xf3, 0x8a, 0x84, 0x0,

    /* U+002B "+" */
    0x21, 0x3e, 0x42, 0x0,

    /* U+002C "," */
    0xf7,

    /* U+002D "-" */
    0xf0,

    /* U+002E "." */
    0xf0,

    /* U+002F "/" */
    0x8, 0x84, 0x22, 0x10, 0x88, 0x42, 0x21, 0x0,

    /* U+0030 "0" */
    0x38, 0x8a, 0xc, 0x1b, 0x36, 0x60, 0xc1, 0x44,
    0x70,

    /* U+0031 "1" */
    0x30, 0xe1, 0x46, 0x89, 0x2, 0x4, 0x8, 0x11,
    0xfc,

    /* U+0032 "2" */
    0x3c, 0x8e, 0xc, 0x10, 0x67, 0x98, 0x40, 0x81,
    0xfc,

    /* U+0033 "3" */
    0xfe, 0xc, 0x71, 0x83, 0xc0, 0xe0, 0xc1, 0xc6,
    0x78,

    /* U+0034 "4" */
    0x1c, 0x14, 0x34, 0x24, 0x64, 0x44, 0x84, 0xff,
    0x4, 0x4,

    /* U+0035 "5" */
    0xfd, 0x2, 0x5, 0xee, 0x60, 0x60, 0xc1, 0x44,
    0x70,

    /* U+0036 "6" */
    0x3c, 0x8e, 0xd, 0xec, 0x70, 0x60, 0xc1, 0xc4,
    0x70,

    /* U+0037 "7" */
    0xfe, 0x4, 0x10, 0x60, 0x82, 0xc, 0x10, 0x40,
    0x80,

    /* U+0038 "8" */
    0x79, 0xa, 0x16, 0x67, 0x98, 0xa0, 0xc1, 0xc6,
    0xf8,

    /* U+0039 "9" */
    0x38, 0x8a, 0xc, 0x18, 0x38, 0xde, 0xc1, 0xc6,
    0xf0,

    /* U+003A ":" */
    0xf0, 0x3c,

    /* U+003B ";" */
    0xf0, 0x3d, 0xc0,

    /* U+003C "<" */
    0x4, 0xfe, 0x30, 0x70, 0x30,

    /* U+003D "=" */
    0xfc, 0xf, 0xc0,

    /* U+003E ">" */
    0xc0, 0xf0, 0x38, 0x33, 0xdc, 0x0,

    /* U+003F "?" */
    0x7b, 0x38, 0x61, 0x4, 0xe2, 0x0, 0x30, 0xc0,

    /* U+0040 "@" */
    0x7c, 0x5, 0xac, 0xd8, 0xb1, 0x62, 0xbf,

    /* U+0041 "A" */
    0x1c, 0x14, 0x34, 0x34, 0x26, 0x22, 0x62, 0x7f,
    0x41, 0xc1,

    /* U+0042 "B" */
    0xfc, 0x8d, 0xa, 0x14, 0x6f, 0x11, 0xa1, 0x43,
    0xf8,

    /* U+0043 "C" */
    0x38, 0x8a, 0xc, 0x18, 0x10, 0x20, 0xc1, 0x44,
    0x70,

    /* U+0044 "D" */
    0xfc, 0x8d, 0xa, 0x14, 0x28, 0x50, 0xa1, 0x47,
    0xf8,

    /* U+0045 "E" */
    0xfe, 0x8, 0x20, 0x83, 0xf8, 0x20, 0x83, 0xf0,

    /* U+0046 "F" */
    0xfe, 0x8, 0x20, 0x83, 0xf8, 0x20, 0x82, 0x0,

    /* U+0047 "G" */
    0x3c, 0x8a, 0xc, 0x18, 0x17, 0xe0, 0xc1, 0x46,
    0x74,

    /* U+0048 "H" */
    0x86, 0x18, 0x61, 0x87, 0xf8, 0x61, 0x86, 0x10,

    /* U+0049 "I" */
    0xfc, 0x41, 0x4, 0x10, 0x41, 0x4, 0x13, 0xf0,

    /* U+004A "J" */
    0xe, 0x8, 0x10, 0x20, 0x40, 0xa1, 0x42, 0xcc,
    0xf0,

    /* U+004B "K" */
    0x85, 0x1a, 0x65, 0x8a, 0x1c, 0x28, 0x48, 0x89,
    0x8,

    /* U+004C "L" */
    0x82, 0x8, 0x20, 0x82, 0x8, 0x20, 0x83, 0xf0,

    /* U+004D "M" */
    0xc7, 0x8f, 0x1d, 0x5a, 0xb5, 0x6a, 0xd5, 0xab,
    0x74,

    /* U+004E "N" */
    0xc7, 0x9a, 0x69, 0xa6, 0x59, 0x65, 0x9e, 0x30,

    /* U+004F "O" */
    0x38, 0x8a, 0xc, 0x18, 0x30, 0x60, 0xc1, 0x44,
    0x70,

    /* U+0050 "P" */
    0xfa, 0x38, 0x61, 0x87, 0xe8, 0x20, 0x82, 0x0,

    /* U+0051 "Q" */
    0x38, 0x8a, 0xc, 0x18, 0x30, 0x60, 0xc1, 0x44,
    0xf8, 0x40, 0xe0,

    /* U+0052 "R" */
    0xfa, 0x38, 0x61, 0x87, 0xe8, 0x61, 0x86, 0x10,

    /* U+0053 "S" */
    0x3c, 0x8d, 0xa, 0x3, 0x81, 0xe0, 0xc1, 0x46,
    0x78,

    /* U+0054 "T" */
    0xfe, 0x20, 0x40, 0x81, 0x2, 0x4, 0x8, 0x10,
    0x20,

    /* U+0055 "U" */
    0x83, 0x6, 0xc, 0x18, 0x30, 0x60, 0xc1, 0xc6,
    0xf8,

    /* U+0056 "V" */
    0x41, 0x43, 0x43, 0x62, 0x22, 0x26, 0x24, 0x34,
    0x14, 0x1c,

    /* U+0057 "W" */
    0x99, 0x9d, 0x95, 0x55, 0x55, 0x55, 0x55, 0x75,
    0x67, 0x66,

    /* U+0058 "X" */
    0x86, 0x14, 0x92, 0x30, 0xc7, 0x92, 0xce, 0x10,

    /* U+0059 "Y" */
    0x83, 0x8d, 0x12, 0x22, 0x85, 0x4, 0x8, 0x10,
    0x20,

    /* U+005A "Z" */
    0xfe, 0x4, 0x10, 0x61, 0x86, 0x18, 0x20, 0x81,
    0xfc,

    /* U+005B "[" */
    0xea, 0xaa, 0xaa, 0xb0,

    /* U+005C "\\" */
    0x84, 0x10, 0x84, 0x10, 0x84, 0x10, 0x84, 0x10,

    /* U+005D "]" */
    0xd5, 0x55, 0x55, 0x70,

    /* U+005E "^" */
    0x38, 0xd1, 0x36, 0x28, 0x60,

    /* U+005F "_" */
    0xfe,

    /* U+0060 "`" */
    0x40,

    /* U+0061 "a" */
    0x3a, 0xc6, 0x82, 0x82, 0x82, 0xc6, 0x3b,

    /* U+0062 "b" */
    0x81, 0x2, 0x5, 0xcc, 0x70, 0x60, 0xc1, 0xc7,
    0x78,

    /* U+0063 "c" */
    0x38, 0x8a, 0xc, 0x8, 0x28, 0x8e, 0x0,

    /* U+0064 "d" */
    0x2, 0x4, 0x9, 0xdc, 0x70, 0x60, 0xc1, 0xc6,
    0x74,

    /* U+0065 "e" */
    0x38, 0x8a, 0xf, 0xf8, 0x8, 0xcf, 0x0,

    /* U+0066 "f" */
    0x3c, 0x82, 0x3f, 0x20, 0x82, 0x8, 0x20, 0x80,

    /* U+0067 "g" */
    0x3a, 0x8e, 0xc, 0x18, 0x28, 0xce, 0x81, 0x2,
    0xfc,

    /* U+0068 "h" */
    0x82, 0x8, 0x2e, 0xce, 0x18, 0x61, 0x86, 0x10,

    /* U+0069 "i" */
    0x30, 0xc0, 0x38, 0x20, 0x82, 0x8, 0x23, 0xf0,

    /* U+006A "j" */
    0x18, 0xc0, 0xf0, 0x84, 0x21, 0x8, 0x42, 0x1f,
    0x80,

    /* U+006B "k" */
    0x82, 0x8, 0x22, 0x92, 0x8e, 0x28, 0x92, 0x20,

    /* U+006C "l" */
    0x70, 0x20, 0x40, 0x81, 0x2, 0x4, 0x8, 0x11,
    0xfc,

    /* U+006D "m" */
    0xef, 0x26, 0x4c, 0x99, 0x32, 0x64, 0x80,

    /* U+006E "n" */
    0xbb, 0x38, 0x61, 0x86, 0x18, 0x40,

    /* U+006F "o" */
    0x38, 0x8a, 0xc, 0x18, 0x28, 0x8e, 0x0,

    /* U+0070 "p" */
    0xb9, 0x8e, 0xc, 0x18, 0x38, 0xef, 0x40, 0x81,
    0x0,

    /* U+0071 "q" */
    0x3a, 0x8e, 0xc, 0x18, 0x38, 0xce, 0x81, 0x2,
    0x4,

    /* U+0072 "r" */
    0x7c, 0x44, 0x89, 0x2, 0x4, 0x3e, 0x0,

    /* U+0073 "s" */
    0x7a, 0x38, 0x1e, 0x86, 0x17, 0x80,

    /* U+0074 "t" */
    0x20, 0x82, 0x3f, 0x20, 0x82, 0x8, 0x20, 0xe0,

    /* U+0075 "u" */
    0x86, 0x18, 0x61, 0x86, 0x37, 0x40,

    /* U+0076 "v" */
    0x87, 0x89, 0x12, 0x66, 0x85, 0xe, 0x0,

    /* U+0077 "w" */
    0x99, 0xdd, 0x55, 0x55, 0x57, 0x56, 0x66,

    /* U+0078 "x" */
    0x85, 0x27, 0x8c, 0x79, 0x28, 0x40,

    /* U+0079 "y" */
    0x86, 0x18, 0x61, 0x86, 0x37, 0x41, 0x5, 0xf0,

    /* U+007A "z" */
    0xfc, 0x11, 0x8c, 0x62, 0xf, 0xc0,

    /* U+007B "{" */
    0x69, 0x24, 0x94, 0x49, 0x24, 0xc0,

    /* U+007C "|" */
    0xff, 0xfc,

    /* U+007D "}" */
    0xc9, 0x24, 0x91, 0x49, 0x25, 0x80,

    /* U+007E "~" */
    0x77, 0x30,

    /* U+00A0 " " */
    0x0,

    /* U+00A1 "¡" */
    0xf2, 0xaa, 0xa0,

    /* U+00A2 "¢" */
    0x10, 0xfb, 0x54, 0x99, 0x12, 0x75, 0xbe, 0x10,

    /* U+00A3 "£" */
    0x39, 0x14, 0x3c, 0x41, 0xf, 0xc0,

    /* U+00A4 "¤" */
    0xba, 0x8a, 0xc, 0x18, 0x28, 0xae, 0x80,

    /* U+00A5 "¥" */
    0x8e, 0xd5, 0xf2, 0x7c, 0x80,

    /* U+00A6 "¦" */
    0xfc, 0xfc,

    /* U+00A7 "§" */
    0xe8, 0xea, 0x6a, 0xe0,

    /* U+00A8 "¨" */
    0xde, 0xc0,

    /* U+00A9 "©" */
    0x38, 0xea, 0xad, 0x1a, 0xae, 0x8e, 0x0,

    /* U+00AA "ª" */
    0x79, 0x97,

    /* U+00AB "«" */
    0x2, 0x6f, 0xe4, 0x8d, 0x87, 0x84, 0x80,

    /* U+00AC "¬" */
    0xfc, 0x10,

    /* U+00AE "®" */
    0x38, 0xfa, 0xad, 0xda, 0xa8, 0x8e, 0x0,

    /* U+00AF "¯" */
    0xf0,

    /* U+00B0 "°" */
    0x69, 0x96,

    /* U+00B1 "±" */
    0x21, 0x3e, 0x42, 0x3, 0xe0,

    /* U+00B2 "²" */
    0xf7, 0x70,

    /* U+00B3 "³" */
    0xeb, 0x60,

    /* U+00B4 "´" */
    0x40,

    /* U+00B5 "µ" */
    0x86, 0x18, 0x61, 0x87, 0x3b, 0x60, 0x82, 0x0,

    /* U+00B6 "¶" */
    0x7d, 0x6a, 0xd2, 0x94, 0xa0,

    /* U+00B7 "·" */
    0xf0,

    /* U+00B8 "¸" */
    0xe5, 0x80,

    /* U+00B9 "¹" */
    0xd9, 0x70,

    /* U+00BA "º" */
    0x69, 0x96,

    /* U+00BB "»" */
    0x1, 0xb1, 0xf8, 0x93, 0x7b, 0x24, 0x0,

    /* U+00BC "¼" */
    0xc0, 0x81, 0x7, 0x21, 0x8c, 0x23, 0xa, 0xe,
    0x8,

    /* U+00BD "½" */
    0xc0, 0x81, 0xf, 0x21, 0x8d, 0xe2, 0x86, 0xe,

    /* U+00BE "¾" */
    0x60, 0x60, 0xa0, 0x62, 0xc, 0x30, 0x46, 0xa,
    0xf, 0x2,

    /* U+00BF "¿" */
    0x18, 0x60, 0x4, 0x72, 0x8, 0x61, 0xcd, 0xe0,

    /* U+00C0 "À" */
    0x10, 0x0, 0x1c, 0x14, 0x34, 0x34, 0x26, 0x22,
    0x62, 0x7f, 0x41, 0xc1,

    /* U+00C1 "Á" */
    0x8, 0x0, 0x1c, 0x14, 0x34, 0x34, 0x26, 0x22,
    0x62, 0x7f, 0x41, 0xc1,

    /* U+00C2 "Â" */
    0x18, 0x14, 0x0, 0x1c, 0x14, 0x34, 0x34, 0x26,
    0x22, 0x62, 0x7f, 0x41, 0xc1,

    /* U+00C3 "Ã" */
    0x18, 0x2c, 0x1c, 0x14, 0x34, 0x34, 0x26, 0x22,
    0x62, 0x7f, 0x41, 0xc1,

    /* U+00C4 "Ä" */
    0x36, 0x1b, 0x0, 0x3, 0x81, 0x40, 0xa0, 0xd8,
    0x44, 0x22, 0x31, 0x9f, 0xc8, 0x2c, 0x18,

    /* U+00C5 "Å" */
    0x1c, 0xa, 0x7, 0x0, 0x1, 0xc0, 0xa0, 0x50,
    0x6c, 0x22, 0x11, 0x18, 0xcf, 0xe4, 0x16, 0x8,

    /* U+00C6 "Æ" */
    0x1f, 0x18, 0x28, 0x28, 0x28, 0x6e, 0x48, 0x78,
    0xc8, 0x8f,

    /* U+00C7 "Ç" */
    0x38, 0x8a, 0xc, 0x18, 0x10, 0x20, 0xc1, 0x44,
    0xf8, 0xe0, 0x41, 0x80,

    /* U+00C8 "È" */
    0x20, 0xf, 0xe0, 0x82, 0x8, 0x3f, 0x82, 0x8,
    0x3f,

    /* U+00C9 "É" */
    0x10, 0xf, 0xe0, 0x82, 0x8, 0x3f, 0x82, 0x8,
    0x3f,

    /* U+00CA "Ê" */
    0x31, 0x40, 0x3f, 0x82, 0x8, 0x20, 0xfe, 0x8,
    0x20, 0xfc,

    /* U+00CB "Ë" */
    0x79, 0xe0, 0x3f, 0x82, 0x8, 0x20, 0xfe, 0x8,
    0x20, 0xfc,

    /* U+00CC "Ì" */
    0x30, 0xf, 0xc4, 0x10, 0x41, 0x4, 0x10, 0x41,
    0x3f,

    /* U+00CD "Í" */
    0x8, 0xf, 0xc4, 0x10, 0x41, 0x4, 0x10, 0x41,
    0x3f,

    /* U+00CE "Î" */
    0x30, 0xa0, 0x3f, 0x10, 0x41, 0x4, 0x10, 0x41,
    0x4, 0xfc,

    /* U+00CF "Ï" */
    0x6d, 0xb0, 0x3f, 0x10, 0x41, 0x4, 0x10, 0x41,
    0x4, 0xfc,

    /* U+00D0 "Ð" */
    0xfc, 0x8d, 0xa, 0x14, 0x3e, 0x50, 0xa1, 0x47,
    0xf8,

    /* U+00D1 "Ñ" */
    0x21, 0x6c, 0x79, 0xa6, 0x9a, 0x65, 0x96, 0x59,
    0xe3,

    /* U+00D2 "Ò" */
    0x30, 0x0, 0xe2, 0x28, 0x30, 0x60, 0xc1, 0x83,
    0x5, 0x11, 0xc0,

    /* U+00D3 "Ó" */
    0x8, 0x0, 0xe2, 0x28, 0x30, 0x60, 0xc1, 0x83,
    0x5, 0x11, 0xc0,

    /* U+00D4 "Ô" */
    0x38, 0x50, 0x1, 0xc4, 0x50, 0x60, 0xc1, 0x83,
    0x6, 0xa, 0x23, 0x80,

    /* U+00D5 "Õ" */
    0x34, 0xb0, 0x1, 0xc4, 0x50, 0x60, 0xc1, 0x83,
    0x6, 0xa, 0x23, 0x80,

    /* U+00D6 "Ö" */
    0x6c, 0xd8, 0x1, 0xc4, 0x50, 0x60, 0xc1, 0x83,
    0x6, 0xa, 0x23, 0x80,

    /* U+00D7 "×" */
    0x4, 0xdc, 0xcd, 0x44,

    /* U+00D8 "Ø" */
    0x4, 0x79, 0x34, 0x59, 0xb2, 0x64, 0xd9, 0xa3,
    0xc9, 0xe2, 0x0,

    /* U+00D9 "Ù" */
    0x30, 0x2, 0xc, 0x18, 0x30, 0x60, 0xc1, 0x83,
    0x7, 0x1b, 0xe0,

    /* U+00DA "Ú" */
    0x8, 0x2, 0xc, 0x18, 0x30, 0x60, 0xc1, 0x83,
    0x7, 0x1b, 0xe0,

    /* U+00DB "Û" */
    0x38, 0x50, 0x4, 0x18, 0x30, 0x60, 0xc1, 0x83,
    0x6, 0xe, 0x37, 0xc0,

    /* U+00DC "Ü" */
    0x6c, 0xd8, 0x4, 0x18, 0x30, 0x60, 0xc1, 0x83,
    0x6, 0xe, 0x37, 0xc0,

    /* U+00DD "Ý" */
    0x18, 0x2, 0xe, 0x34, 0x48, 0x8a, 0x14, 0x10,
    0x20, 0x40, 0x80,

    /* U+00DE "Þ" */
    0x82, 0xf, 0xa3, 0x86, 0x18, 0x7e, 0x82, 0x0,

    /* U+00DF "ß" */
    0x38, 0x89, 0x16, 0x24, 0x89, 0x90, 0xa1, 0x52,
    0x98,

    /* U+00E0 "à" */
    0x30, 0x0, 0x0, 0x3a, 0xc6, 0x82, 0x82, 0x82,
    0xc6, 0x3b,

    /* U+00E1 "á" */
    0x8, 0x0, 0x0, 0x3a, 0xc6, 0x82, 0x82, 0x82,
    0xc6, 0x3b,

    /* U+00E2 "â" */
    0x38, 0x28, 0x0, 0x3a, 0xc6, 0x82, 0x82, 0x82,
    0xc6, 0x3b,

    /* U+00E3 "ã" */
    0x34, 0x5c, 0x0, 0x3a, 0xc6, 0x82, 0x82, 0x82,
    0xc6, 0x3b,

    /* U+00E4 "ä" */
    0x6c, 0x6c, 0x0, 0x3a, 0xc6, 0x82, 0x82, 0x82,
    0xc6, 0x3b,

    /* U+00E5 "å" */
    0x38, 0x48, 0x38, 0x0, 0x3a, 0xc6, 0x82, 0x82,
    0x82, 0xc6, 0x3b,

    /* U+00E6 "æ" */
    0x79, 0xa8, 0x4b, 0xf9, 0x12, 0xdf, 0x0,

    /* U+00E7 "ç" */
    0x38, 0x8a, 0x1c, 0x8, 0x78, 0x9e, 0xc, 0x8,
    0x30,

    /* U+00E8 "è" */
    0x30, 0x0, 0x1, 0xc4, 0x50, 0x7f, 0xc0, 0x46,
    0x78,

    /* U+00E9 "é" */
    0x8, 0x0, 0x1, 0xc4, 0x50, 0x7f, 0xc0, 0x46,
    0x78,

    /* U+00EA "ê" */
    0x38, 0x50, 0x1, 0xc4, 0x50, 0x7f, 0xc0, 0x46,
    0x78,

    /* U+00EB "ë" */
    0x6c, 0xd8, 0x1, 0xc4, 0x50, 0x7f, 0xc0, 0x46,
    0x78,

    /* U+00EC "ì" */
    0x40, 0x1, 0xc2, 0x10, 0x84, 0x27, 0xc0,

    /* U+00ED "í" */
    0x20, 0x1, 0xc2, 0x10, 0x84, 0x27, 0xc0,

    /* U+00EE "î" */
    0x62, 0x81, 0xc2, 0x10, 0x84, 0x27, 0xc0,

    /* U+00EF "ï" */
    0xdb, 0x60, 0x38, 0x20, 0x82, 0x8, 0x23, 0xf0,

    /* U+00F0 "ð" */
    0x38, 0x30, 0x11, 0xf4, 0x70, 0x60, 0xc1, 0x44,
    0x70,

    /* U+00F1 "ñ" */
    0x71, 0x60, 0x2e, 0xce, 0x18, 0x61, 0x86, 0x10,

    /* U+00F2 "ò" */
    0x30, 0x0, 0x1, 0xc4, 0x50, 0x60, 0xc1, 0x44,
    0x70,

    /* U+00F3 "ó" */
    0x8, 0x0, 0x1, 0xc4, 0x50, 0x60, 0xc1, 0x44,
    0x70,

    /* U+00F4 "ô" */
    0x38, 0x50, 0x1, 0xc4, 0x50, 0x60, 0xc1, 0x44,
    0x70,

    /* U+00F5 "õ" */
    0x34, 0xb0, 0x1, 0xc4, 0x50, 0x60, 0xc1, 0x44,
    0x70,

    /* U+00F6 "ö" */
    0x6c, 0xd8, 0x1, 0xc4, 0x50, 0x60, 0xc1, 0x44,
    0x70,

    /* U+00F7 "÷" */
    0x30, 0xc0, 0x3f, 0x0, 0xc3, 0x0,

    /* U+00F8 "ø" */
    0x8, 0x71, 0x34, 0x99, 0x32, 0x79, 0x1c, 0x20,

    /* U+00F9 "ù" */
    0x20, 0x0, 0x21, 0x86, 0x18, 0x61, 0x8d, 0xd0,

    /* U+00FA "ú" */
    0x10, 0x0, 0x21, 0x86, 0x18, 0x61, 0x8d, 0xd0,

    /* U+00FB "û" */
    0x31, 0xe0, 0x21, 0x86, 0x18, 0x61, 0x8d, 0xd0,

    /* U+00FC "ü" */
    0x79, 0xe0, 0x21, 0x86, 0x18, 0x61, 0x8d, 0xd0,

    /* U+00FD "ý" */
    0x10, 0x0, 0x21, 0x86, 0x18, 0x61, 0x8d, 0xd0,
    0x41, 0x7c,

    /* U+00FE "þ" */
    0x81, 0x2, 0x5, 0xcc, 0x70, 0x60, 0xc1, 0xc7,
    0x7a, 0x4, 0x8, 0x0,

    /* U+00FF "ÿ" */
    0x79, 0xe0, 0x21, 0x86, 0x18, 0x61, 0x8d, 0xd0,
    0x41, 0x7c,

    /* U+2013 "–" */
    0xfe,

    /* U+2014 "—" */
    0xff,

    /* U+2018 "‘" */
    0xef,

    /* U+2019 "’" */
    0xf7,

    /* U+201A "‚" */
    0xf7,

    /* U+201C "“" */
    0xda, 0xfd,

    /* U+201D "”" */
    0xd6, 0xd3, 0xa0,

    /* U+201E "„" */
    0xd6, 0xd3, 0xa0,

    /* U+2020 "†" */
    0x21, 0x3e, 0x42, 0x10, 0x80,

    /* U+2021 "‡" */
    0x21, 0x3e, 0x4f, 0x90, 0x80,

    /* U+2022 "•" */
    0xff, 0x80,

    /* U+2026 "…" */
    0xff, 0xf0,

    /* U+2030 "‰" */
    0x61, 0x22, 0x43, 0x21, 0x84, 0x36, 0x8b, 0x16,
    0x34,

    /* U+2032 "′" */
    0xf8,

    /* U+2033 "″" */
    0xde, 0xe4,

    /* U+2039 "‹" */
    0x13, 0xc8, 0xc6, 0x10,

    /* U+203A "›" */
    0xc, 0x71, 0x3c, 0x80,

    /* U+2044 "⁄" */
    0x2, 0x18, 0xc6, 0x0,

    /* U+2190 "←" */
    0x60, 0xc0, 0xff, 0xc0, 0x60,

    /* U+2191 "↑" */
    0x77, 0xea, 0x42, 0x10, 0x80,

    /* U+2192 "→" */
    0x6, 0x3, 0xff, 0x3, 0x6,

    /* U+2193 "↓" */
    0x21, 0x8, 0x4a, 0xfd, 0xc0,

    /* U+2194 "↔" */
    0x66, 0xc3, 0xff, 0xc3, 0x66,

    /* U+2195 "↕" */
    0x77, 0xea, 0x4a, 0xfd, 0xc0,

    /* U+2196 "↖" */
    0xf3, 0xa, 0x24, 0x8, 0x10,

    /* U+2197 "↗" */
    0x3c, 0x31, 0x49, 0x42, 0x0,

    /* U+2198 "↘" */
    0x81, 0x2, 0x45, 0xc, 0xf0,

    /* U+2199 "↙" */
    0x4, 0x29, 0x28, 0xc3, 0xc0,

    /* U+F00B "" */
    0xff, 0xd0, 0x7f, 0xf4, 0x1f, 0xfd, 0x7, 0xfe,

    /* U+F013 "" */
    0x1c, 0xe, 0x3f, 0xfc, 0x76, 0x37, 0x1f, 0xfe,
    0xba, 0x1c, 0x0,

    /* U+F01E "" */
    0x0, 0x3d, 0x43, 0x87, 0x80, 0x80, 0x80, 0x46,
    0x3c,

    /* U+F07B "" */
    0xf0, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe,

    /* U+F0C7 "" */
    0xfe, 0x63, 0xb1, 0x77, 0x98, 0xc, 0xc6, 0x63,
    0x1, 0xff, 0x80,

    /* U+F0C9 "" */
    0xff, 0x80, 0x0, 0x1f, 0xf0, 0x0, 0x3, 0xfe,

    /* U+F1EB "" */
    0x3e, 0x60, 0xcf, 0x4, 0x40, 0x80,

    /* U+F293 "" */
    0x3e, 0x3b, 0xbc, 0xda, 0xae, 0x37, 0xbf, 0x8f,
    0xaa, 0xf3, 0x3b, 0x8f, 0x80,

    /* U+F030C "󰌌" */
    0xff, 0xbd, 0xff, 0xff, 0xe3, 0xff
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 137, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 137, .box_w = 2, .box_h = 10, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 4, .adv_w = 137, .box_w = 5, .box_h = 3, .ofs_x = 2, .ofs_y = 7},
    {.bitmap_index = 6, .adv_w = 137, .box_w = 8, .box_h = 7, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 13, .adv_w = 137, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 20, .adv_w = 137, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 29, .adv_w = 137, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 39, .adv_w = 137, .box_w = 2, .box_h = 3, .ofs_x = 3, .ofs_y = 7},
    {.bitmap_index = 40, .adv_w = 137, .box_w = 6, .box_h = 14, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 51, .adv_w = 137, .box_w = 6, .box_h = 14, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 62, .adv_w = 137, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 69, .adv_w = 137, .box_w = 5, .box_h = 5, .ofs_x = 2, .ofs_y = 2},
    {.bitmap_index = 73, .adv_w = 137, .box_w = 2, .box_h = 4, .ofs_x = 3, .ofs_y = -2},
    {.bitmap_index = 74, .adv_w = 137, .box_w = 4, .box_h = 1, .ofs_x = 2, .ofs_y = 3},
    {.bitmap_index = 75, .adv_w = 137, .box_w = 2, .box_h = 2, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 76, .adv_w = 137, .box_w = 5, .box_h = 12, .ofs_x = 2, .ofs_y = -1},
    {.bitmap_index = 84, .adv_w = 137, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 93, .adv_w = 137, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 102, .adv_w = 137, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 111, .adv_w = 137, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 120, .adv_w = 137, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 130, .adv_w = 137, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 139, .adv_w = 137, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 148, .adv_w = 137, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 157, .adv_w = 137, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 166, .adv_w = 137, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 175, .adv_w = 137, .box_w = 2, .box_h = 7, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 177, .adv_w = 137, .box_w = 2, .box_h = 9, .ofs_x = 3, .ofs_y = -2},
    {.bitmap_index = 180, .adv_w = 137, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 185, .adv_w = 137, .box_w = 6, .box_h = 3, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 188, .adv_w = 137, .box_w = 7, .box_h = 6, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 194, .adv_w = 137, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 202, .adv_w = 137, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 209, .adv_w = 137, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 219, .adv_w = 137, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 228, .adv_w = 137, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 237, .adv_w = 137, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 246, .adv_w = 137, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 254, .adv_w = 137, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 262, .adv_w = 137, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 271, .adv_w = 137, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 279, .adv_w = 137, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 287, .adv_w = 137, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 296, .adv_w = 137, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 305, .adv_w = 137, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 313, .adv_w = 137, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 322, .adv_w = 137, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 330, .adv_w = 137, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 339, .adv_w = 137, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 347, .adv_w = 137, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 358, .adv_w = 137, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 366, .adv_w = 137, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 375, .adv_w = 137, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 384, .adv_w = 137, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 393, .adv_w = 137, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 403, .adv_w = 137, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 413, .adv_w = 137, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 421, .adv_w = 137, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 430, .adv_w = 137, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 439, .adv_w = 137, .box_w = 2, .box_h = 14, .ofs_x = 4, .ofs_y = -2},
    {.bitmap_index = 443, .adv_w = 137, .box_w = 5, .box_h = 12, .ofs_x = 2, .ofs_y = -1},
    {.bitmap_index = 451, .adv_w = 137, .box_w = 2, .box_h = 14, .ofs_x = 3, .ofs_y = -2},
    {.bitmap_index = 455, .adv_w = 137, .box_w = 7, .box_h = 5, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 460, .adv_w = 137, .box_w = 7, .box_h = 1, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 461, .adv_w = 137, .box_w = 2, .box_h = 1, .ofs_x = 3, .ofs_y = 8},
    {.bitmap_index = 462, .adv_w = 137, .box_w = 8, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 469, .adv_w = 137, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 478, .adv_w = 137, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 485, .adv_w = 137, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 494, .adv_w = 137, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 501, .adv_w = 137, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 509, .adv_w = 137, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 518, .adv_w = 137, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 526, .adv_w = 137, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 534, .adv_w = 137, .box_w = 5, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 543, .adv_w = 137, .box_w = 6, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 551, .adv_w = 137, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 560, .adv_w = 137, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 567, .adv_w = 137, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 573, .adv_w = 137, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 580, .adv_w = 137, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 589, .adv_w = 137, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 598, .adv_w = 137, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 605, .adv_w = 137, .box_w = 6, .box_h = 7, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 611, .adv_w = 137, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 619, .adv_w = 137, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 625, .adv_w = 137, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 632, .adv_w = 137, .box_w = 8, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 639, .adv_w = 137, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 645, .adv_w = 137, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 653, .adv_w = 137, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 659, .adv_w = 137, .box_w = 3, .box_h = 14, .ofs_x = 3, .ofs_y = -2},
    {.bitmap_index = 665, .adv_w = 137, .box_w = 1, .box_h = 14, .ofs_x = 4, .ofs_y = -2},
    {.bitmap_index = 667, .adv_w = 137, .box_w = 3, .box_h = 14, .ofs_x = 3, .ofs_y = -2},
    {.bitmap_index = 673, .adv_w = 137, .box_w = 7, .box_h = 2, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 675, .adv_w = 137, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 676, .adv_w = 137, .box_w = 2, .box_h = 10, .ofs_x = 3, .ofs_y = -3},
    {.bitmap_index = 679, .adv_w = 137, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 687, .adv_w = 137, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 693, .adv_w = 137, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 700, .adv_w = 137, .box_w = 5, .box_h = 7, .ofs_x = 2, .ofs_y = 1},
    {.bitmap_index = 705, .adv_w = 137, .box_w = 1, .box_h = 14, .ofs_x = 4, .ofs_y = -2},
    {.bitmap_index = 707, .adv_w = 137, .box_w = 4, .box_h = 7, .ofs_x = 2, .ofs_y = 3},
    {.bitmap_index = 711, .adv_w = 137, .box_w = 5, .box_h = 2, .ofs_x = 2, .ofs_y = 8},
    {.bitmap_index = 713, .adv_w = 137, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 720, .adv_w = 137, .box_w = 4, .box_h = 4, .ofs_x = 2, .ofs_y = 6},
    {.bitmap_index = 722, .adv_w = 137, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 729, .adv_w = 137, .box_w = 6, .box_h = 2, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 731, .adv_w = 137, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 738, .adv_w = 137, .box_w = 4, .box_h = 1, .ofs_x = 2, .ofs_y = 8},
    {.bitmap_index = 739, .adv_w = 137, .box_w = 4, .box_h = 4, .ofs_x = 2, .ofs_y = 6},
    {.bitmap_index = 741, .adv_w = 137, .box_w = 5, .box_h = 7, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 746, .adv_w = 137, .box_w = 3, .box_h = 4, .ofs_x = 3, .ofs_y = 6},
    {.bitmap_index = 748, .adv_w = 137, .box_w = 3, .box_h = 4, .ofs_x = 3, .ofs_y = 6},
    {.bitmap_index = 750, .adv_w = 137, .box_w = 2, .box_h = 1, .ofs_x = 3, .ofs_y = 8},
    {.bitmap_index = 751, .adv_w = 137, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 759, .adv_w = 137, .box_w = 5, .box_h = 7, .ofs_x = 2, .ofs_y = 3},
    {.bitmap_index = 764, .adv_w = 137, .box_w = 2, .box_h = 2, .ofs_x = 3, .ofs_y = 4},
    {.bitmap_index = 765, .adv_w = 137, .box_w = 3, .box_h = 3, .ofs_x = 3, .ofs_y = -3},
    {.bitmap_index = 767, .adv_w = 137, .box_w = 3, .box_h = 4, .ofs_x = 3, .ofs_y = 6},
    {.bitmap_index = 769, .adv_w = 137, .box_w = 4, .box_h = 4, .ofs_x = 2, .ofs_y = 6},
    {.bitmap_index = 771, .adv_w = 137, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 778, .adv_w = 137, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 787, .adv_w = 137, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 795, .adv_w = 137, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 805, .adv_w = 137, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 813, .adv_w = 137, .box_w = 8, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 825, .adv_w = 137, .box_w = 8, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 837, .adv_w = 137, .box_w = 8, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 850, .adv_w = 137, .box_w = 8, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 862, .adv_w = 137, .box_w = 9, .box_h = 13, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 877, .adv_w = 137, .box_w = 9, .box_h = 14, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 893, .adv_w = 137, .box_w = 8, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 903, .adv_w = 137, .box_w = 7, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 915, .adv_w = 137, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 924, .adv_w = 137, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 933, .adv_w = 137, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 943, .adv_w = 137, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 953, .adv_w = 137, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 962, .adv_w = 137, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 971, .adv_w = 137, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 981, .adv_w = 137, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 991, .adv_w = 137, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1000, .adv_w = 137, .box_w = 6, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1009, .adv_w = 137, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1020, .adv_w = 137, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1031, .adv_w = 137, .box_w = 7, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1043, .adv_w = 137, .box_w = 7, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1055, .adv_w = 137, .box_w = 7, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1067, .adv_w = 137, .box_w = 5, .box_h = 6, .ofs_x = 2, .ofs_y = 2},
    {.bitmap_index = 1071, .adv_w = 137, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1082, .adv_w = 137, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1093, .adv_w = 137, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1104, .adv_w = 137, .box_w = 7, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1116, .adv_w = 137, .box_w = 7, .box_h = 13, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1128, .adv_w = 137, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1139, .adv_w = 137, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1147, .adv_w = 137, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1156, .adv_w = 137, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1166, .adv_w = 137, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1176, .adv_w = 137, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1186, .adv_w = 137, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1196, .adv_w = 137, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1206, .adv_w = 137, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1217, .adv_w = 137, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1224, .adv_w = 137, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1233, .adv_w = 137, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1242, .adv_w = 137, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1251, .adv_w = 137, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1260, .adv_w = 137, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1269, .adv_w = 137, .box_w = 5, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 1276, .adv_w = 137, .box_w = 5, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 1283, .adv_w = 137, .box_w = 5, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 1290, .adv_w = 137, .box_w = 6, .box_h = 10, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 1298, .adv_w = 137, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1307, .adv_w = 137, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1315, .adv_w = 137, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1324, .adv_w = 137, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1333, .adv_w = 137, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1342, .adv_w = 137, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1351, .adv_w = 137, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1360, .adv_w = 137, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 1366, .adv_w = 137, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1374, .adv_w = 137, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1382, .adv_w = 137, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1390, .adv_w = 137, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1398, .adv_w = 137, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1406, .adv_w = 137, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1416, .adv_w = 137, .box_w = 7, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1428, .adv_w = 137, .box_w = 6, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 1438, .adv_w = 137, .box_w = 7, .box_h = 1, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 1439, .adv_w = 137, .box_w = 8, .box_h = 1, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 1440, .adv_w = 137, .box_w = 2, .box_h = 4, .ofs_x = 3, .ofs_y = 6},
    {.bitmap_index = 1441, .adv_w = 137, .box_w = 2, .box_h = 4, .ofs_x = 3, .ofs_y = 6},
    {.bitmap_index = 1442, .adv_w = 137, .box_w = 2, .box_h = 4, .ofs_x = 3, .ofs_y = -2},
    {.bitmap_index = 1443, .adv_w = 137, .box_w = 4, .box_h = 4, .ofs_x = 2, .ofs_y = 6},
    {.bitmap_index = 1445, .adv_w = 137, .box_w = 5, .box_h = 4, .ofs_x = 2, .ofs_y = 6},
    {.bitmap_index = 1448, .adv_w = 137, .box_w = 5, .box_h = 4, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 1451, .adv_w = 137, .box_w = 5, .box_h = 7, .ofs_x = 2, .ofs_y = 3},
    {.bitmap_index = 1456, .adv_w = 137, .box_w = 5, .box_h = 7, .ofs_x = 2, .ofs_y = 3},
    {.bitmap_index = 1461, .adv_w = 137, .box_w = 3, .box_h = 3, .ofs_x = 3, .ofs_y = 3},
    {.bitmap_index = 1463, .adv_w = 137, .box_w = 6, .box_h = 2, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1465, .adv_w = 137, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1474, .adv_w = 137, .box_w = 2, .box_h = 3, .ofs_x = 3, .ofs_y = 7},
    {.bitmap_index = 1475, .adv_w = 137, .box_w = 5, .box_h = 3, .ofs_x = 2, .ofs_y = 7},
    {.bitmap_index = 1477, .adv_w = 137, .box_w = 4, .box_h = 7, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 1481, .adv_w = 137, .box_w = 4, .box_h = 7, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 1485, .adv_w = 137, .box_w = 7, .box_h = 4, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 1489, .adv_w = 137, .box_w = 8, .box_h = 5, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 1494, .adv_w = 137, .box_w = 5, .box_h = 7, .ofs_x = 2, .ofs_y = 1},
    {.bitmap_index = 1499, .adv_w = 137, .box_w = 8, .box_h = 5, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 1504, .adv_w = 137, .box_w = 5, .box_h = 7, .ofs_x = 2, .ofs_y = 1},
    {.bitmap_index = 1509, .adv_w = 137, .box_w = 8, .box_h = 5, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 1514, .adv_w = 137, .box_w = 5, .box_h = 7, .ofs_x = 2, .ofs_y = 1},
    {.bitmap_index = 1519, .adv_w = 137, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 1524, .adv_w = 137, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 1529, .adv_w = 137, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 1534, .adv_w = 137, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 1539, .adv_w = 137, .box_w = 9, .box_h = 7, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 1547, .adv_w = 137, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 1558, .adv_w = 137, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 1567, .adv_w = 137, .box_w = 9, .box_h = 7, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 1575, .adv_w = 137, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 1586, .adv_w = 137, .box_w = 9, .box_h = 7, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 1594, .adv_w = 137, .box_w = 9, .box_h = 5, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 1600, .adv_w = 137, .box_w = 9, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1613, .adv_w = 137, .box_w = 8, .box_h = 6, .ofs_x = 0, .ofs_y = 2}
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
const lv_font_t space_mono_14_cr = {
#else
lv_font_t space_mono_14_cr = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 17,          /*The maximum line height required by the font*/
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



#endif /*#if SPACE_MONO_14_CR*/

