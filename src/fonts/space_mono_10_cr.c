/*******************************************************************************
 * Size: 10 px
 * Bpp: 1
 * Opts: --size 10 --bpp 1 --no-compress --format lvgl --font assets/fonts/SpaceMono-Regular.ttf -r 0x20-0x7E -r 0xA0-0xFF -r 0x2000-0x206F -r 0x2190-0x21FF -r 0xF00B -r 0xF013 -r 0xF07B -r 0xF0C7 -r 0xF0C9 -r 0xF01E -r 0xF030C -r 0xF1EB -r 0xF293 --lv-fallback lv_font_montserrat_14 --lv-font-name space_mono_10_cr -o src/fonts/space_mono_10_cr.c
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#ifndef SPACE_MONO_10_CR
#define SPACE_MONO_10_CR 1
#endif

#if SPACE_MONO_10_CR

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */
    0x0,

    /* U+0021 "!" */
    0x55, 0x4c,

    /* U+0022 "\"" */
    0xb4,

    /* U+0023 "#" */
    0x57, 0xd5, 0xf5, 0x0,

    /* U+0024 "$" */
    0x27, 0xd8, 0x76, 0x98,

    /* U+0025 "%" */
    0xe5, 0x3c, 0x4f, 0x94, 0xe0,

    /* U+0026 "&" */
    0xe4, 0x24, 0xf9, 0x49, 0xc0,

    /* U+0027 "'" */
    0xc0,

    /* U+0028 "(" */
    0x34, 0x88, 0x88, 0x88, 0x43,

    /* U+0029 ")" */
    0xc2, 0x11, 0x11, 0x11, 0x2c,

    /* U+002A "*" */
    0x23, 0xbe, 0xe2, 0x0,

    /* U+002B "+" */
    0x27, 0xc8, 0x40,

    /* U+002C "," */
    0xc0,

    /* U+002D "-" */
    0xe0,

    /* U+002E "." */
    0xc0,

    /* U+002F "/" */
    0x12, 0x22, 0x44, 0x48, 0x80,

    /* U+0030 "0" */
    0x74, 0x63, 0xd8, 0xc5, 0xc0,

    /* U+0031 "1" */
    0x63, 0x28, 0x42, 0x13, 0xe0,

    /* U+0032 "2" */
    0x74, 0x62, 0x26, 0x43, 0xe0,

    /* U+0033 "3" */
    0xf8, 0xc8, 0x60, 0xc5, 0xc0,

    /* U+0034 "4" */
    0x31, 0x94, 0xa9, 0x7c, 0x40,

    /* U+0035 "5" */
    0xf4, 0x2d, 0x90, 0xc5, 0xc0,

    /* U+0036 "6" */
    0x74, 0x6d, 0x98, 0xc5, 0xc0,

    /* U+0037 "7" */
    0xf8, 0x44, 0x42, 0x22, 0x0,

    /* U+0038 "8" */
    0x74, 0x62, 0xe8, 0xc5, 0xc0,

    /* U+0039 "9" */
    0x74, 0x63, 0x17, 0xc5, 0xc0,

    /* U+003A ":" */
    0xc0, 0xc0,

    /* U+003B ";" */
    0xc0, 0x50,

    /* U+003C "<" */
    0x7, 0x8e, 0x10,

    /* U+003D "=" */
    0xff,

    /* U+003E ">" */
    0x7, 0x82, 0xe8, 0x0,

    /* U+003F "?" */
    0x74, 0x62, 0x72, 0x1, 0x80,

    /* U+0040 "@" */
    0x7b, 0xe7, 0x39, 0xbc,

    /* U+0041 "A" */
    0x30, 0xc4, 0x92, 0x7a, 0x18, 0x40,

    /* U+0042 "B" */
    0xf2, 0x52, 0xe4, 0xa7, 0xc0,

    /* U+0043 "C" */
    0x74, 0x63, 0x8, 0xc5, 0xc0,

    /* U+0044 "D" */
    0xf2, 0x52, 0x94, 0xa7, 0xc0,

    /* U+0045 "E" */
    0xf8, 0x8f, 0x88, 0xf0,

    /* U+0046 "F" */
    0xf8, 0x8f, 0x88, 0x80,

    /* U+0047 "G" */
    0x74, 0x61, 0x78, 0xc5, 0xe0,

    /* U+0048 "H" */
    0x8c, 0x63, 0xf8, 0xc6, 0x20,

    /* U+0049 "I" */
    0xf9, 0x8, 0x42, 0x13, 0xe0,

    /* U+004A "J" */
    0x38, 0x84, 0x29, 0x49, 0x80,

    /* U+004B "K" */
    0x9c, 0xa9, 0x8a, 0x4a, 0x20,

    /* U+004C "L" */
    0x84, 0x21, 0x8, 0x43, 0xe0,

    /* U+004D "M" */
    0xde, 0xf7, 0xbe, 0xf6, 0xa0,

    /* U+004E "N" */
    0xce, 0x6b, 0x5a, 0xce, 0x60,

    /* U+004F "O" */
    0x74, 0x63, 0x18, 0xc5, 0xc0,

    /* U+0050 "P" */
    0xf4, 0x63, 0xe8, 0x42, 0x0,

    /* U+0051 "Q" */
    0x74, 0x63, 0x18, 0xc5, 0x44, 0x30,

    /* U+0052 "R" */
    0xf4, 0x63, 0xe9, 0x4a, 0x40,

    /* U+0053 "S" */
    0x74, 0x62, 0xe8, 0xc5, 0xc0,

    /* U+0054 "T" */
    0xf9, 0x8, 0x42, 0x10, 0x80,

    /* U+0055 "U" */
    0x8c, 0x63, 0x18, 0xc5, 0xc0,

    /* U+0056 "V" */
    0x85, 0x24, 0x92, 0x28, 0xc3, 0x0,

    /* U+0057 "W" */
    0xb6, 0xdb, 0x6d, 0xb6, 0xe4, 0x80,

    /* U+0058 "X" */
    0x8a, 0x98, 0x46, 0x2a, 0x40,

    /* U+0059 "Y" */
    0x8c, 0x54, 0xa2, 0x10, 0x80,

    /* U+005A "Z" */
    0xf8, 0x44, 0x44, 0x43, 0xe0,

    /* U+005B "[" */
    0xea, 0xaa, 0xb0,

    /* U+005C "\\" */
    0x88, 0x44, 0x42, 0x22, 0x10,

    /* U+005D "]" */
    0xd5, 0x55, 0x70,

    /* U+005E "^" */
    0x67, 0x99,

    /* U+005F "_" */
    0xf0,

    /* U+0060 "`" */
    0x40,

    /* U+0061 "a" */
    0x7a, 0x28, 0xa2, 0x7c,

    /* U+0062 "b" */
    0x84, 0x3d, 0x18, 0xc7, 0xc0,

    /* U+0063 "c" */
    0x74, 0x61, 0x17, 0x0,

    /* U+0064 "d" */
    0x8, 0x5f, 0x18, 0xc5, 0xe0,

    /* U+0065 "e" */
    0x74, 0x7f, 0x17, 0x0,

    /* U+0066 "f" */
    0x39, 0x3e, 0x42, 0x10, 0x80,

    /* U+0067 "g" */
    0x7c, 0x63, 0x17, 0x85, 0xe0,

    /* U+0068 "h" */
    0x88, 0xf9, 0x99, 0x90,

    /* U+0069 "i" */
    0x60, 0x38, 0x42, 0x13, 0xe0,

    /* U+006A "j" */
    0x23, 0x92, 0x49, 0xe0,

    /* U+006B "k" */
    0x88, 0x9a, 0xca, 0x90,

    /* U+006C "l" */
    0xe1, 0x8, 0x42, 0x13, 0xe0,

    /* U+006D "m" */
    0xfd, 0x6b, 0x5a, 0x80,

    /* U+006E "n" */
    0xf9, 0x99, 0x90,

    /* U+006F "o" */
    0x74, 0x63, 0x17, 0x0,

    /* U+0070 "p" */
    0xf4, 0x63, 0x1f, 0x42, 0x0,

    /* U+0071 "q" */
    0x7c, 0x63, 0x17, 0x84, 0x20,

    /* U+0072 "r" */
    0xfa, 0x50, 0x8e, 0x0,

    /* U+0073 "s" */
    0xe9, 0x69, 0xf0,

    /* U+0074 "t" */
    0x21, 0x3e, 0x42, 0x10, 0xe0,

    /* U+0075 "u" */
    0x99, 0x99, 0xf0,

    /* U+0076 "v" */
    0x94, 0xa4, 0xc6, 0x0,

    /* U+0077 "w" */
    0xb6, 0xdb, 0x5e, 0x48,

    /* U+0078 "x" */
    0x52, 0x88, 0xa5, 0x0,

    /* U+0079 "y" */
    0x99, 0x99, 0xf1, 0x70,

    /* U+007A "z" */
    0xf1, 0x68, 0xf0,

    /* U+007B "{" */
    0x69, 0x25, 0x12, 0x4c,

    /* U+007C "|" */
    0xff, 0xc0,

    /* U+007D "}" */
    0xc9, 0x24, 0x52, 0x58,

    /* U+007E "~" */
    0x5b,

    /* U+00A0 " " */
    0x0,

    /* U+00A1 "¡" */
    0xc5, 0x54,

    /* U+00A2 "¢" */
    0x27, 0xe9, 0x4f, 0x90,

    /* U+00A3 "£" */
    0x72, 0x38, 0x8f, 0x0,

    /* U+00A4 "¤" */
    0xfc, 0x63, 0x1f, 0x80,

    /* U+00A5 "¥" */
    0x96, 0xff, 0x40,

    /* U+00A6 "¦" */
    0xf3, 0xc0,

    /* U+00A7 "§" */
    0xd3, 0x6c,

    /* U+00A8 "¨" */
    0xf0,

    /* U+00A9 "©" */
    0x77, 0x73, 0xd7, 0x0,

    /* U+00AA "ª" */
    0xf7, 0x80,

    /* U+00AB "«" */
    0x2b, 0xa9, 0xe2, 0x80,

    /* U+00AC "¬" */
    0xf1,

    /* U+00AE "®" */
    0x77, 0x7b, 0x97, 0x0,

    /* U+00AF "¯" */
    0xc0,

    /* U+00B0 "°" */
    0xf7, 0x80,

    /* U+00B1 "±" */
    0x27, 0xc8, 0x4f, 0x80,

    /* U+00B2 "²" */
    0xdc,

    /* U+00B3 "³" */
    0xdc,

    /* U+00B4 "´" */
    0x80,

    /* U+00B5 "µ" */
    0x99, 0x99, 0xf8, 0x80,

    /* U+00B6 "¶" */
    0xff, 0xb6,

    /* U+00B7 "·" */
    0xc0,

    /* U+00B8 "¸" */
    0xf0,

    /* U+00B9 "¹" */
    0x54,

    /* U+00BA "º" */
    0xf7, 0x80,

    /* U+00BB "»" */
    0xa2, 0x8a, 0xfa, 0x0,

    /* U+00BC "¼" */
    0xc2, 0x3a, 0x6d, 0x8c, 0x20,

    /* U+00BD "½" */
    0xc2, 0x32, 0x65, 0x88, 0x60,

    /* U+00BE "¾" */
    0xc4, 0x31, 0xb6, 0x4c, 0x61,

    /* U+00BF "¿" */
    0x20, 0x9, 0xc8, 0xc5, 0xc0,

    /* U+00C0 "À" */
    0x20, 0x3, 0xc, 0x49, 0x27, 0xa1, 0x84,

    /* U+00C1 "Á" */
    0x10, 0x3, 0xc, 0x49, 0x27, 0xa1, 0x84,

    /* U+00C2 "Â" */
    0x30, 0xc3, 0xc, 0x49, 0x27, 0xa1, 0x84,

    /* U+00C3 "Ã" */
    0x20, 0x43, 0xc, 0x49, 0x27, 0xa1, 0x84,

    /* U+00C4 "Ä" */
    0x78, 0x3, 0xc, 0x49, 0x27, 0xa1, 0x84,

    /* U+00C5 "Å" */
    0x30, 0xc3, 0xc, 0x31, 0x24, 0x9e, 0x8a, 0x10,

    /* U+00C6 "Æ" */
    0x3c, 0xc5, 0x17, 0x72, 0x49, 0xc0,

    /* U+00C7 "Ç" */
    0x74, 0x63, 0x8, 0xc5, 0xc4, 0x20,

    /* U+00C8 "È" */
    0x40, 0xf8, 0x8f, 0x88, 0xf0,

    /* U+00C9 "É" */
    0x20, 0xf8, 0x8f, 0x88, 0xf0,

    /* U+00CA "Ê" */
    0x65, 0xf8, 0x8f, 0x88, 0xf0,

    /* U+00CB "Ë" */
    0xf0, 0xf8, 0x8f, 0x88, 0xf0,

    /* U+00CC "Ì" */
    0x20, 0x3e, 0x42, 0x10, 0x84, 0xf8,

    /* U+00CD "Í" */
    0x20, 0x3e, 0x42, 0x10, 0x84, 0xf8,

    /* U+00CE "Î" */
    0x22, 0xbe, 0x42, 0x10, 0x84, 0xf8,

    /* U+00CF "Ï" */
    0xf0, 0x3e, 0x42, 0x10, 0x84, 0xf8,

    /* U+00D0 "Ð" */
    0xf2, 0x53, 0xd4, 0xa7, 0xc0,

    /* U+00D1 "Ñ" */
    0x62, 0xb3, 0x9a, 0xd6, 0xb3, 0x98,

    /* U+00D2 "Ò" */
    0x20, 0x1d, 0x18, 0xc6, 0x31, 0x70,

    /* U+00D3 "Ó" */
    0x20, 0x1d, 0x18, 0xc6, 0x31, 0x70,

    /* U+00D4 "Ô" */
    0x22, 0x9d, 0x18, 0xc6, 0x31, 0x70,

    /* U+00D5 "Õ" */
    0x73, 0x9d, 0x18, 0xc6, 0x31, 0x70,

    /* U+00D6 "Ö" */
    0xf0, 0x1d, 0x18, 0xc6, 0x31, 0x70,

    /* U+00D7 "×" */
    0x96, 0x70,

    /* U+00D8 "Ø" */
    0x13, 0xb7, 0x5a, 0xd7, 0x2e, 0x40,

    /* U+00D9 "Ù" */
    0x20, 0x23, 0x18, 0xc6, 0x31, 0x70,

    /* U+00DA "Ú" */
    0x20, 0x23, 0x18, 0xc6, 0x31, 0x70,

    /* U+00DB "Û" */
    0x22, 0xa3, 0x18, 0xc6, 0x31, 0x70,

    /* U+00DC "Ü" */
    0xf0, 0x23, 0x18, 0xc6, 0x31, 0x70,

    /* U+00DD "Ý" */
    0x20, 0x23, 0x15, 0x28, 0x84, 0x20,

    /* U+00DE "Þ" */
    0x87, 0xa3, 0x18, 0xfa, 0x0,

    /* U+00DF "ß" */
    0x39, 0x2d, 0x96, 0x45, 0x95, 0x80,

    /* U+00E0 "à" */
    0x20, 0x7, 0xa2, 0x8a, 0x27, 0xc0,

    /* U+00E1 "á" */
    0x20, 0x7, 0xa2, 0x8a, 0x27, 0xc0,

    /* U+00E2 "â" */
    0x21, 0x47, 0xa2, 0x8a, 0x27, 0xc0,

    /* U+00E3 "ã" */
    0x71, 0xc7, 0xa2, 0x8a, 0x27, 0xc0,

    /* U+00E4 "ä" */
    0x78, 0x7, 0xa2, 0x8a, 0x27, 0xc0,

    /* U+00E5 "å" */
    0x20, 0x80, 0x1e, 0x8a, 0x28, 0x9f,

    /* U+00E6 "æ" */
    0x75, 0xbf, 0x4f, 0x0,

    /* U+00E7 "ç" */
    0x74, 0x61, 0x17, 0x10, 0x80,

    /* U+00E8 "è" */
    0x20, 0x1d, 0x1f, 0xc5, 0xc0,

    /* U+00E9 "é" */
    0x20, 0x1d, 0x1f, 0xc5, 0xc0,

    /* U+00EA "ê" */
    0x22, 0x9d, 0x1f, 0xc5, 0xc0,

    /* U+00EB "ë" */
    0xf0, 0x1d, 0x1f, 0xc5, 0xc0,

    /* U+00EC "ì" */
    0x40, 0x38, 0x42, 0x13, 0xe0,

    /* U+00ED "í" */
    0x20, 0x38, 0x42, 0x13, 0xe0,

    /* U+00EE "î" */
    0x62, 0xb8, 0x42, 0x13, 0xe0,

    /* U+00EF "ï" */
    0xf0, 0x38, 0x42, 0x13, 0xe0,

    /* U+00F0 "ð" */
    0x61, 0x9b, 0x38, 0xc5, 0xc0,

    /* U+00F1 "ñ" */
    0x42, 0xf9, 0x99, 0x90,

    /* U+00F2 "ò" */
    0x20, 0x1d, 0x18, 0xc5, 0xc0,

    /* U+00F3 "ó" */
    0x20, 0x1d, 0x18, 0xc5, 0xc0,

    /* U+00F4 "ô" */
    0x22, 0x9d, 0x18, 0xc5, 0xc0,

    /* U+00F5 "õ" */
    0x73, 0x9d, 0x18, 0xc5, 0xc0,

    /* U+00F6 "ö" */
    0x78, 0x1d, 0x18, 0xc5, 0xc0,

    /* U+00F7 "÷" */
    0x6f, 0x6,

    /* U+00F8 "ø" */
    0x75, 0x6b, 0x57, 0x0,

    /* U+00F9 "ù" */
    0x40, 0x99, 0x99, 0xf0,

    /* U+00FA "ú" */
    0x20, 0x99, 0x99, 0xf0,

    /* U+00FB "û" */
    0x66, 0x99, 0x99, 0xf0,

    /* U+00FC "ü" */
    0xf0, 0x99, 0x99, 0xf0,

    /* U+00FD "ý" */
    0x20, 0x99, 0x99, 0xf1, 0x70,

    /* U+00FE "þ" */
    0x84, 0x3d, 0x18, 0xc7, 0xd0, 0x80,

    /* U+00FF "ÿ" */
    0xf0, 0x99, 0x99, 0xf1, 0x70,

    /* U+2013 "–" */
    0xf0,

    /* U+2014 "—" */
    0xfc,

    /* U+2018 "‘" */
    0xe0,

    /* U+2019 "’" */
    0xe0,

    /* U+201A "‚" */
    0xc0,

    /* U+201C "“" */
    0xb6, 0x80,

    /* U+201D "”" */
    0xb6, 0x80,

    /* U+201E "„" */
    0xb4,

    /* U+2020 "†" */
    0x5d, 0x24,

    /* U+2021 "‡" */
    0x5d, 0x74,

    /* U+2022 "•" */
    0xf0,

    /* U+2026 "…" */
    0xf8,

    /* U+2030 "‰" */
    0xe5, 0x3c, 0x4e, 0x9c, 0xa0,

    /* U+2032 "′" */
    0xe0,

    /* U+2033 "″" */
    0x7a,

    /* U+2039 "‹" */
    0x3a, 0x62,

    /* U+203A "›" */
    0x88, 0xb8,

    /* U+2044 "⁄" */
    0x1, 0xb0,

    /* U+2190 "←" */
    0x44, 0x3f, 0x4, 0x0,

    /* U+2191 "↑" */
    0x75, 0x48, 0x42, 0x0,

    /* U+2192 "→" */
    0x10, 0x7e, 0x11, 0x0,

    /* U+2193 "↓" */
    0x21, 0x9, 0x57, 0x0,

    /* U+2194 "↔" */
    0x54, 0x7f, 0x15, 0x0,

    /* U+2195 "↕" */
    0x75, 0x49, 0x57, 0x0,

    /* U+2196 "↖" */
    0xe6, 0x28, 0x20, 0x0,

    /* U+2197 "↗" */
    0x38, 0xca, 0x80, 0x0,

    /* U+2198 "↘" */
    0x2, 0xa, 0x33, 0x80,

    /* U+2199 "↙" */
    0x0, 0xa9, 0x8e, 0x0,

    /* U+F00B "" */
    0x7b, 0x1f, 0xf1, 0xff, 0xf0,

    /* U+F013 "" */
    0x31, 0xe4, 0x92, 0x78, 0xc0,

    /* U+F01E "" */
    0x35, 0x39, 0xe0, 0xc9, 0xc0,

    /* U+F07B "" */
    0xe3, 0xff, 0xff, 0xfc,

    /* U+F0C7 "" */
    0xfb, 0x5f, 0x65, 0xa7, 0xf0,

    /* U+F0C9 "" */
    0xfc, 0xf, 0xc0, 0xfc,

    /* U+F1EB "" */
    0x7a, 0x17, 0x8c,

    /* U+F293 "" */
    0x7b, 0xbb, 0x73, 0xce, 0xde, 0xde,

    /* U+F030C "󰌌" */
    0x7d, 0xf7, 0xff, 0xdc, 0xff, 0x80
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 98, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 98, .box_w = 2, .box_h = 7, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 3, .adv_w = 98, .box_w = 3, .box_h = 2, .ofs_x = 2, .ofs_y = 5},
    {.bitmap_index = 4, .adv_w = 98, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 8, .adv_w = 98, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 12, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 17, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 22, .adv_w = 98, .box_w = 1, .box_h = 2, .ofs_x = 3, .ofs_y = 5},
    {.bitmap_index = 23, .adv_w = 98, .box_w = 4, .box_h = 10, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 28, .adv_w = 98, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 33, .adv_w = 98, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 37, .adv_w = 98, .box_w = 5, .box_h = 4, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 40, .adv_w = 98, .box_w = 1, .box_h = 2, .ofs_x = 3, .ofs_y = -1},
    {.bitmap_index = 41, .adv_w = 98, .box_w = 3, .box_h = 1, .ofs_x = 2, .ofs_y = 2},
    {.bitmap_index = 42, .adv_w = 98, .box_w = 2, .box_h = 1, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 43, .adv_w = 98, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 48, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 53, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 58, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 63, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 68, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 73, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 78, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 83, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 88, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 93, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 98, .adv_w = 98, .box_w = 2, .box_h = 5, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 100, .adv_w = 98, .box_w = 2, .box_h = 6, .ofs_x = 2, .ofs_y = -1},
    {.bitmap_index = 102, .adv_w = 98, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 105, .adv_w = 98, .box_w = 4, .box_h = 2, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 106, .adv_w = 98, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 110, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 115, .adv_w = 98, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 119, .adv_w = 98, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 125, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 130, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 135, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 140, .adv_w = 98, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 144, .adv_w = 98, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 148, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 153, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 158, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 163, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 168, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 173, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 178, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 183, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 188, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 193, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 198, .adv_w = 98, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 204, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 209, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 214, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 219, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 224, .adv_w = 98, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 230, .adv_w = 98, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 236, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 241, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 246, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 251, .adv_w = 98, .box_w = 2, .box_h = 10, .ofs_x = 3, .ofs_y = -2},
    {.bitmap_index = 254, .adv_w = 98, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 259, .adv_w = 98, .box_w = 2, .box_h = 10, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 262, .adv_w = 98, .box_w = 4, .box_h = 4, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 264, .adv_w = 98, .box_w = 4, .box_h = 1, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 265, .adv_w = 98, .box_w = 2, .box_h = 1, .ofs_x = 2, .ofs_y = 6},
    {.bitmap_index = 266, .adv_w = 98, .box_w = 6, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 270, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 275, .adv_w = 98, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 279, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 284, .adv_w = 98, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 288, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 293, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 298, .adv_w = 98, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 302, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 307, .adv_w = 98, .box_w = 3, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 311, .adv_w = 98, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 315, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 320, .adv_w = 98, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 324, .adv_w = 98, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 327, .adv_w = 98, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 331, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 336, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 341, .adv_w = 98, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 345, .adv_w = 98, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 348, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 353, .adv_w = 98, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 356, .adv_w = 98, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 360, .adv_w = 98, .box_w = 6, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 364, .adv_w = 98, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 368, .adv_w = 98, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 372, .adv_w = 98, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 375, .adv_w = 98, .box_w = 3, .box_h = 10, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 379, .adv_w = 98, .box_w = 1, .box_h = 10, .ofs_x = 3, .ofs_y = -2},
    {.bitmap_index = 381, .adv_w = 98, .box_w = 3, .box_h = 10, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 385, .adv_w = 98, .box_w = 4, .box_h = 2, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 386, .adv_w = 98, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 387, .adv_w = 98, .box_w = 2, .box_h = 7, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 389, .adv_w = 98, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 393, .adv_w = 98, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 397, .adv_w = 98, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 401, .adv_w = 98, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 404, .adv_w = 98, .box_w = 1, .box_h = 10, .ofs_x = 3, .ofs_y = -2},
    {.bitmap_index = 406, .adv_w = 98, .box_w = 3, .box_h = 5, .ofs_x = 2, .ofs_y = 2},
    {.bitmap_index = 408, .adv_w = 98, .box_w = 4, .box_h = 1, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 409, .adv_w = 98, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 413, .adv_w = 98, .box_w = 3, .box_h = 3, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 415, .adv_w = 98, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 419, .adv_w = 98, .box_w = 4, .box_h = 2, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 420, .adv_w = 98, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 424, .adv_w = 98, .box_w = 2, .box_h = 1, .ofs_x = 2, .ofs_y = 5},
    {.bitmap_index = 425, .adv_w = 98, .box_w = 3, .box_h = 3, .ofs_x = 2, .ofs_y = 4},
    {.bitmap_index = 427, .adv_w = 98, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 431, .adv_w = 98, .box_w = 2, .box_h = 3, .ofs_x = 2, .ofs_y = 4},
    {.bitmap_index = 432, .adv_w = 98, .box_w = 2, .box_h = 3, .ofs_x = 2, .ofs_y = 4},
    {.bitmap_index = 433, .adv_w = 98, .box_w = 2, .box_h = 1, .ofs_x = 2, .ofs_y = 6},
    {.bitmap_index = 434, .adv_w = 98, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 438, .adv_w = 98, .box_w = 3, .box_h = 5, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 440, .adv_w = 98, .box_w = 2, .box_h = 1, .ofs_x = 2, .ofs_y = 3},
    {.bitmap_index = 441, .adv_w = 98, .box_w = 2, .box_h = 2, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 442, .adv_w = 98, .box_w = 2, .box_h = 3, .ofs_x = 2, .ofs_y = 4},
    {.bitmap_index = 443, .adv_w = 98, .box_w = 3, .box_h = 3, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 445, .adv_w = 98, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 449, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 454, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 459, .adv_w = 98, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 464, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 469, .adv_w = 98, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 476, .adv_w = 98, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 483, .adv_w = 98, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 490, .adv_w = 98, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 497, .adv_w = 98, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 504, .adv_w = 98, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 512, .adv_w = 98, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 518, .adv_w = 98, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 524, .adv_w = 98, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 529, .adv_w = 98, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 534, .adv_w = 98, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 539, .adv_w = 98, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 544, .adv_w = 98, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 550, .adv_w = 98, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 556, .adv_w = 98, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 562, .adv_w = 98, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 568, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 573, .adv_w = 98, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 579, .adv_w = 98, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 585, .adv_w = 98, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 591, .adv_w = 98, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 597, .adv_w = 98, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 603, .adv_w = 98, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 609, .adv_w = 98, .box_w = 4, .box_h = 4, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 611, .adv_w = 98, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 617, .adv_w = 98, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 623, .adv_w = 98, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 629, .adv_w = 98, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 635, .adv_w = 98, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 641, .adv_w = 98, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 647, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 652, .adv_w = 98, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 658, .adv_w = 98, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 664, .adv_w = 98, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 670, .adv_w = 98, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 676, .adv_w = 98, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 682, .adv_w = 98, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 688, .adv_w = 98, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 694, .adv_w = 98, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 698, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 703, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 708, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 713, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 718, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 723, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 728, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 733, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 738, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 743, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 748, .adv_w = 98, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 752, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 757, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 762, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 767, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 772, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 777, .adv_w = 98, .box_w = 4, .box_h = 4, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 779, .adv_w = 98, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 783, .adv_w = 98, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 787, .adv_w = 98, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 791, .adv_w = 98, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 795, .adv_w = 98, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 799, .adv_w = 98, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 804, .adv_w = 98, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 810, .adv_w = 98, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 815, .adv_w = 98, .box_w = 4, .box_h = 1, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 816, .adv_w = 98, .box_w = 6, .box_h = 1, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 817, .adv_w = 98, .box_w = 1, .box_h = 3, .ofs_x = 2, .ofs_y = 4},
    {.bitmap_index = 818, .adv_w = 98, .box_w = 1, .box_h = 3, .ofs_x = 3, .ofs_y = 4},
    {.bitmap_index = 819, .adv_w = 98, .box_w = 1, .box_h = 2, .ofs_x = 3, .ofs_y = -1},
    {.bitmap_index = 820, .adv_w = 98, .box_w = 3, .box_h = 3, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 822, .adv_w = 98, .box_w = 3, .box_h = 3, .ofs_x = 2, .ofs_y = 4},
    {.bitmap_index = 824, .adv_w = 98, .box_w = 3, .box_h = 2, .ofs_x = 2, .ofs_y = -1},
    {.bitmap_index = 825, .adv_w = 98, .box_w = 3, .box_h = 5, .ofs_x = 2, .ofs_y = 2},
    {.bitmap_index = 827, .adv_w = 98, .box_w = 3, .box_h = 5, .ofs_x = 2, .ofs_y = 2},
    {.bitmap_index = 829, .adv_w = 98, .box_w = 2, .box_h = 2, .ofs_x = 2, .ofs_y = 2},
    {.bitmap_index = 830, .adv_w = 98, .box_w = 5, .box_h = 1, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 831, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 836, .adv_w = 98, .box_w = 2, .box_h = 2, .ofs_x = 2, .ofs_y = 5},
    {.bitmap_index = 837, .adv_w = 98, .box_w = 4, .box_h = 2, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 838, .adv_w = 98, .box_w = 3, .box_h = 5, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 840, .adv_w = 98, .box_w = 3, .box_h = 5, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 842, .adv_w = 98, .box_w = 5, .box_h = 3, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 844, .adv_w = 98, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 848, .adv_w = 98, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 852, .adv_w = 98, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 856, .adv_w = 98, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 860, .adv_w = 98, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 864, .adv_w = 98, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 868, .adv_w = 98, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 872, .adv_w = 98, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 876, .adv_w = 98, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 880, .adv_w = 98, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 884, .adv_w = 98, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 889, .adv_w = 98, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 894, .adv_w = 98, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 899, .adv_w = 98, .box_w = 6, .box_h = 5, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 903, .adv_w = 98, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 908, .adv_w = 98, .box_w = 6, .box_h = 5, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 912, .adv_w = 98, .box_w = 6, .box_h = 4, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 915, .adv_w = 98, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 921, .adv_w = 98, .box_w = 7, .box_h = 6, .ofs_x = 0, .ofs_y = 1}
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
const lv_font_t space_mono_10_cr = {
#else
lv_font_t space_mono_10_cr = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 12,          /*The maximum line height required by the font*/
    .base_line = 2,             /*Baseline measured from the bottom of the line*/
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



#endif /*#if SPACE_MONO_10_CR*/

