/*******************************************************************************
 * Size: 12 px
 * Bpp: 1
 * Opts: --size 12 --bpp 1 --no-compress --format lvgl --font assets/fonts/SpaceMono-Regular.ttf -r 0x20-0x7E -r 0xA0-0xFF -r 0x2000-0x206F -r 0x2190-0x21FF -r 0xF00B -r 0xF013 -r 0xF07B -r 0xF0C7 -r 0xF0C9 -r 0xF01E -r 0xF030C -r 0xF1EB -r 0xF293 --lv-fallback lv_font_montserrat_14 --lv-font-name space_mono_12_cr -o src/fonts/space_mono_12_cr.c
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#ifndef SPACE_MONO_12_CR
#define SPACE_MONO_12_CR 1
#endif

#if SPACE_MONO_12_CR

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */
    0x0,

    /* U+0021 "!" */
    0xfd,

    /* U+0022 "\"" */
    0x99, 0x90,

    /* U+0023 "#" */
    0x57, 0xd5, 0xf5, 0x28,

    /* U+0024 "$" */
    0x27, 0xaa, 0xea, 0xfc, 0x80,

    /* U+0025 "%" */
    0xf2, 0x4f, 0x8c, 0x58, 0x92, 0x46,

    /* U+0026 "&" */
    0x72, 0x8, 0x9f, 0x8a, 0x28, 0x9e,

    /* U+0027 "'" */
    0xe0,

    /* U+0028 "(" */
    0x3b, 0x31, 0x8, 0x42, 0x10, 0x86, 0x18, 0x70,

    /* U+0029 ")" */
    0xe1, 0x86, 0x10, 0x84, 0x21, 0x8, 0xcd, 0xc0,

    /* U+002A "*" */
    0x10, 0xa8, 0xe7, 0xf3, 0x82, 0x0,

    /* U+002B "+" */
    0x21, 0x3e, 0x42, 0x0,

    /* U+002C "," */
    0xf0,

    /* U+002D "-" */
    0xe0,

    /* U+002E "." */
    0x80,

    /* U+002F "/" */
    0x8, 0x84, 0x22, 0x10, 0x88, 0x42, 0x0,

    /* U+0030 "0" */
    0x7b, 0x38, 0x69, 0x86, 0x1c, 0xde,

    /* U+0031 "1" */
    0x31, 0xc5, 0x14, 0x10, 0x41, 0x1f,

    /* U+0032 "2" */
    0x7b, 0x18, 0x41, 0x3b, 0x8, 0x3f,

    /* U+0033 "3" */
    0xfc, 0x33, 0xe, 0x6, 0x1c, 0xde,

    /* U+0034 "4" */
    0x18, 0xa6, 0x92, 0x8b, 0xf0, 0x82,

    /* U+0035 "5" */
    0xfa, 0xb, 0xb3, 0x6, 0x1c, 0xde,

    /* U+0036 "6" */
    0x7a, 0x1b, 0xb3, 0x86, 0x1c, 0xde,

    /* U+0037 "7" */
    0xfc, 0x10, 0x86, 0x10, 0x84, 0x10,

    /* U+0038 "8" */
    0x7a, 0x18, 0x5e, 0x86, 0x18, 0x5e,

    /* U+0039 "9" */
    0x7a, 0x18, 0x61, 0x7e, 0x18, 0xde,

    /* U+003A ":" */
    0x84,

    /* U+003B ";" */
    0x86,

    /* U+003C "<" */
    0xb, 0xb1, 0x83, 0x80,

    /* U+003D "=" */
    0xf8, 0x3e,

    /* U+003E ">" */
    0x81, 0xc0, 0xc7, 0xf0,

    /* U+003F "?" */
    0x7a, 0x10, 0x4e, 0x20, 0x0, 0x8,

    /* U+0040 "@" */
    0x78, 0x17, 0x65, 0x96, 0x57, 0xc0,

    /* U+0041 "A" */
    0x18, 0x50, 0xa1, 0x24, 0x4f, 0x90, 0xc1,

    /* U+0042 "B" */
    0xf9, 0x14, 0x5e, 0x45, 0x14, 0x7e,

    /* U+0043 "C" */
    0x7b, 0x38, 0x60, 0x82, 0x1c, 0xde,

    /* U+0044 "D" */
    0xf9, 0x14, 0x51, 0x45, 0x14, 0x7e,

    /* U+0045 "E" */
    0xfc, 0x21, 0xf8, 0x42, 0x1f,

    /* U+0046 "F" */
    0xfc, 0x21, 0xf8, 0x42, 0x10,

    /* U+0047 "G" */
    0x7b, 0x38, 0x60, 0xbe, 0x1c, 0xdd,

    /* U+0048 "H" */
    0x86, 0x18, 0x7f, 0x86, 0x18, 0x61,

    /* U+0049 "I" */
    0xf9, 0x8, 0x42, 0x10, 0x9f,

    /* U+004A "J" */
    0x1c, 0x20, 0x82, 0xa, 0x28, 0x9c,

    /* U+004B "K" */
    0x8e, 0x6a, 0x38, 0xa2, 0x48, 0xa3,

    /* U+004C "L" */
    0x84, 0x21, 0x8, 0x42, 0x1f,

    /* U+004D "M" */
    0xcf, 0x3c, 0xf3, 0xb6, 0xdb, 0x6d,

    /* U+004E "N" */
    0xc7, 0x9a, 0x69, 0x96, 0x59, 0xe3,

    /* U+004F "O" */
    0x7b, 0x38, 0x61, 0x86, 0x1c, 0xde,

    /* U+0050 "P" */
    0xfa, 0x18, 0x61, 0xfa, 0x8, 0x20,

    /* U+0051 "Q" */
    0x7b, 0x38, 0x61, 0x86, 0x18, 0x5e, 0x20, 0x60,

    /* U+0052 "R" */
    0xfa, 0x18, 0x61, 0xfa, 0x28, 0xa2,

    /* U+0053 "S" */
    0x7a, 0x38, 0x58, 0x1e, 0x1c, 0x5e,

    /* U+0054 "T" */
    0xf9, 0x8, 0x42, 0x10, 0x84,

    /* U+0055 "U" */
    0x86, 0x18, 0x61, 0x86, 0x1c, 0xde,

    /* U+0056 "V" */
    0xc2, 0x8d, 0x13, 0x22, 0x45, 0xa, 0xc,

    /* U+0057 "W" */
    0x9b, 0x36, 0xad, 0x56, 0xad, 0x5a, 0xb6,

    /* U+0058 "X" */
    0x85, 0x27, 0x8c, 0x31, 0x24, 0xa1,

    /* U+0059 "Y" */
    0xc6, 0x89, 0x11, 0x42, 0x82, 0x4, 0x8,

    /* U+005A "Z" */
    0xfc, 0x10, 0x84, 0x21, 0x8, 0x3f,

    /* U+005B "[" */
    0xea, 0xaa, 0xab,

    /* U+005C "\\" */
    0x42, 0x10, 0x42, 0x10, 0x42, 0x10, 0x40,

    /* U+005D "]" */
    0xd5, 0x55, 0x57,

    /* U+005E "^" */
    0x32, 0x93, 0x10,

    /* U+005F "_" */
    0xf8,

    /* U+0060 "`" */
    0x80,

    /* U+0061 "a" */
    0x75, 0x9a, 0x14, 0x2c, 0xce, 0xc0,

    /* U+0062 "b" */
    0x82, 0xb, 0xb3, 0x86, 0x1c, 0xee,

    /* U+0063 "c" */
    0x7b, 0x38, 0x20, 0xcd, 0xe0,

    /* U+0064 "d" */
    0x4, 0x17, 0x73, 0x86, 0x1c, 0xdd,

    /* U+0065 "e" */
    0x7a, 0x2f, 0xe0, 0xc9, 0xc0,

    /* U+0066 "f" */
    0x39, 0x3e, 0x42, 0x10, 0x84,

    /* U+0067 "g" */
    0x77, 0x38, 0x61, 0xcd, 0xd0, 0x5f,

    /* U+0068 "h" */
    0x84, 0x2d, 0x98, 0xc6, 0x31,

    /* U+0069 "i" */
    0x20, 0x18, 0x42, 0x10, 0x9f,

    /* U+006A "j" */
    0x23, 0x92, 0x49, 0x3c,

    /* U+006B "k" */
    0x84, 0x25, 0x6a, 0x62, 0x92,

    /* U+006C "l" */
    0xe1, 0x8, 0x42, 0x10, 0x9f,

    /* U+006D "m" */
    0xef, 0x26, 0x4c, 0x99, 0x32, 0x40,

    /* U+006E "n" */
    0xb6, 0x63, 0x18, 0xc4,

    /* U+006F "o" */
    0x7b, 0x38, 0x61, 0xcd, 0xe0,

    /* U+0070 "p" */
    0xbb, 0x38, 0x61, 0xce, 0xe8, 0x20,

    /* U+0071 "q" */
    0x77, 0x38, 0x61, 0xcd, 0xd0, 0x41,

    /* U+0072 "r" */
    0xf9, 0x14, 0x10, 0x43, 0xc0,

    /* U+0073 "s" */
    0x74, 0x70, 0x78, 0xb8,

    /* U+0074 "t" */
    0x21, 0x3e, 0x42, 0x10, 0x87,

    /* U+0075 "u" */
    0x8c, 0x63, 0x19, 0xb4,

    /* U+0076 "v" */
    0x8a, 0x24, 0x94, 0x50, 0xc0,

    /* U+0077 "w" */
    0x9b, 0x35, 0xab, 0x56, 0xcd, 0x80,

    /* U+0078 "x" */
    0x4d, 0x23, 0xc, 0x4b, 0x20,

    /* U+0079 "y" */
    0x8c, 0x63, 0x19, 0xb4, 0x2f,

    /* U+007A "z" */
    0xf8, 0x4c, 0xc8, 0x7c,

    /* U+007B "{" */
    0x69, 0x24, 0xa2, 0x49, 0x30,

    /* U+007C "|" */
    0xff, 0xf0,

    /* U+007D "}" */
    0xc9, 0x24, 0x8a, 0x49, 0x60,

    /* U+007E "~" */
    0x6a, 0xc0,

    /* U+00A0 " " */
    0x0,

    /* U+00A1 "¡" */
    0xbf,

    /* U+00A2 "¢" */
    0x11, 0xf9, 0x64, 0x95, 0xf1, 0x0,

    /* U+00A3 "£" */
    0x32, 0x51, 0xc4, 0x7c,

    /* U+00A4 "¤" */
    0xff, 0x38, 0x61, 0xcf, 0xf0,

    /* U+00A5 "¥" */
    0x8a, 0x94, 0xf7, 0x90,

    /* U+00A6 "¦" */
    0xf9, 0xf0,

    /* U+00A7 "§" */
    0xf7, 0xfb, 0xc0,

    /* U+00A8 "¨" */
    0x90,

    /* U+00A9 "©" */
    0x7b, 0xfa, 0x69, 0xfd, 0xe0,

    /* U+00AA "ª" */
    0xf9, 0xf0,

    /* U+00AB "«" */
    0x5, 0xef, 0x2c, 0x78, 0x90,

    /* U+00AC "¬" */
    0xf8, 0x40,

    /* U+00AE "®" */
    0x7b, 0xfb, 0x6d, 0xfd, 0xe0,

    /* U+00AF "¯" */
    0xe0,

    /* U+00B0 "°" */
    0x69, 0x60,

    /* U+00B1 "±" */
    0x21, 0x3e, 0x42, 0x3, 0xe0,

    /* U+00B2 "²" */
    0xcb, 0x0,

    /* U+00B3 "³" */
    0xeb, 0x0,

    /* U+00B4 "´" */
    0x80,

    /* U+00B5 "µ" */
    0x8c, 0x63, 0x18, 0xfe, 0x10,

    /* U+00B6 "¶" */
    0xfb, 0x73, 0x33,

    /* U+00B7 "·" */
    0x80,

    /* U+00B8 "¸" */
    0xf0,

    /* U+00B9 "¹" */
    0xcb, 0x80,

    /* U+00BA "º" */
    0xf9, 0xf0,

    /* U+00BB "»" */
    0x81, 0xe3, 0xcd, 0x7a, 0x40,

    /* U+00BC "¼" */
    0xc0, 0x83, 0xb1, 0x8c, 0xc2, 0x83, 0x2,

    /* U+00BD "½" */
    0xc1, 0xe, 0xcc, 0x4c, 0x50, 0x87,

    /* U+00BE "¾" */
    0xc0, 0x83, 0x6, 0x63, 0x19, 0x85, 0xe, 0x4,

    /* U+00BF "¿" */
    0x10, 0x1, 0x1c, 0x82, 0x1c, 0xde,

    /* U+00C0 "À" */
    0x10, 0x0, 0x61, 0x42, 0x84, 0x91, 0x3e, 0x43,
    0x4,

    /* U+00C1 "Á" */
    0x8, 0x0, 0x61, 0x42, 0x84, 0x91, 0x3e, 0x43,
    0x4,

    /* U+00C2 "Â" */
    0x18, 0x50, 0x0, 0xc2, 0x85, 0x9, 0x22, 0x7c,
    0x86, 0x8,

    /* U+00C3 "Ã" */
    0x30, 0x30, 0x0, 0xc2, 0x85, 0x9, 0x22, 0x7c,
    0x86, 0x8,

    /* U+00C4 "Ä" */
    0x48, 0x3, 0x1e, 0x49, 0x2c, 0xff, 0x86, 0x10,

    /* U+00C5 "Å" */
    0x30, 0x50, 0xc0, 0x3, 0x5, 0x1a, 0x24, 0x44,
    0xfa, 0x14, 0x10,

    /* U+00C6 "Æ" */
    0x3c, 0xc5, 0x17, 0x53, 0xc9, 0x27,

    /* U+00C7 "Ç" */
    0x7b, 0x38, 0x60, 0x82, 0x1c, 0xde, 0x30, 0xc0,

    /* U+00C8 "È" */
    0x20, 0x3f, 0x8, 0x7e, 0x10, 0x87, 0xc0,

    /* U+00C9 "É" */
    0x10, 0x3f, 0x8, 0x7e, 0x10, 0x87, 0xc0,

    /* U+00CA "Ê" */
    0x32, 0x81, 0xf8, 0x43, 0xf0, 0x84, 0x3e,

    /* U+00CB "Ë" */
    0x50, 0x3f, 0x8, 0x7a, 0x10, 0x87, 0xc0,

    /* U+00CC "Ì" */
    0x20, 0x3e, 0x42, 0x10, 0x84, 0x27, 0xc0,

    /* U+00CD "Í" */
    0x10, 0x3e, 0x42, 0x10, 0x84, 0x27, 0xc0,

    /* U+00CE "Î" */
    0x32, 0x81, 0xf2, 0x10, 0x84, 0x21, 0x3e,

    /* U+00CF "Ï" */
    0x68, 0xf, 0xc4, 0x10, 0x41, 0x4, 0x13, 0xf0,

    /* U+00D0 "Ð" */
    0xf9, 0x14, 0x79, 0x45, 0x14, 0x7e,

    /* U+00D1 "Ñ" */
    0x21, 0x60, 0x31, 0xe6, 0x9a, 0x65, 0x96, 0x78,
    0xc0,

    /* U+00D2 "Ò" */
    0x20, 0x7, 0xb3, 0x86, 0x18, 0x61, 0xcd, 0xe0,

    /* U+00D3 "Ó" */
    0x10, 0x7, 0xb3, 0x86, 0x18, 0x61, 0xcd, 0xe0,

    /* U+00D4 "Ô" */
    0x30, 0xc0, 0x1e, 0xce, 0x18, 0x61, 0x87, 0x37,
    0x80,

    /* U+00D5 "Õ" */
    0x69, 0x60, 0x1e, 0xce, 0x18, 0x61, 0x87, 0x37,
    0x80,

    /* U+00D6 "Ö" */
    0x48, 0x0, 0x1e, 0xce, 0x18, 0x61, 0x87, 0x37,
    0x80,

    /* U+00D7 "×" */
    0x59, 0x9d, 0x90,

    /* U+00D8 "Ø" */
    0x9, 0xed, 0xe5, 0x96, 0x9a, 0x6b, 0x79, 0x0,

    /* U+00D9 "Ù" */
    0x20, 0x8, 0x61, 0x86, 0x18, 0x61, 0xcd, 0xe0,

    /* U+00DA "Ú" */
    0x10, 0x8, 0x61, 0x86, 0x18, 0x61, 0xcd, 0xe0,

    /* U+00DB "Û" */
    0x31, 0xe0, 0x21, 0x86, 0x18, 0x61, 0x87, 0x37,
    0x80,

    /* U+00DC "Ü" */
    0x48, 0x0, 0x21, 0x86, 0x18, 0x61, 0x87, 0x37,
    0x80,

    /* U+00DD "Ý" */
    0x10, 0x3, 0x1a, 0x24, 0x45, 0xa, 0x8, 0x10,
    0x20,

    /* U+00DE "Þ" */
    0x83, 0xe8, 0x61, 0x87, 0xe8, 0x20,

    /* U+00DF "ß" */
    0x38, 0x8b, 0x12, 0xc5, 0xc8, 0x58, 0xae,

    /* U+00E0 "à" */
    0x20, 0x1, 0xd6, 0x68, 0x50, 0xb3, 0x3b,

    /* U+00E1 "á" */
    0x10, 0x1, 0xd6, 0x68, 0x50, 0xb3, 0x3b,

    /* U+00E2 "â" */
    0x30, 0x71, 0xd6, 0x68, 0x50, 0xb3, 0x3b,

    /* U+00E3 "ã" */
    0x78, 0x1, 0xd6, 0x68, 0x50, 0xb3, 0x3b,

    /* U+00E4 "ä" */
    0x48, 0x1, 0xd6, 0x68, 0x50, 0xb3, 0x3b,

    /* U+00E5 "å" */
    0x30, 0x60, 0x3, 0xac, 0xd0, 0xa1, 0x66, 0x76,

    /* U+00E6 "æ" */
    0x75, 0x5f, 0x4a, 0xb8,

    /* U+00E7 "ç" */
    0x7b, 0x38, 0x20, 0xcd, 0xe3, 0xc,

    /* U+00E8 "è" */
    0x20, 0x7, 0xa2, 0xfe, 0xc, 0x9c,

    /* U+00E9 "é" */
    0x10, 0x7, 0xa2, 0xfe, 0xc, 0x9c,

    /* U+00EA "ê" */
    0x31, 0x47, 0xa2, 0xfe, 0xc, 0x9c,

    /* U+00EB "ë" */
    0x48, 0x7, 0xa2, 0xfe, 0xc, 0x9c,

    /* U+00EC "ì" */
    0x40, 0x38, 0x42, 0x10, 0x9f,

    /* U+00ED "í" */
    0x20, 0x38, 0x42, 0x10, 0x9f,

    /* U+00EE "î" */
    0x62, 0xb8, 0x42, 0x10, 0x9f,

    /* U+00EF "ï" */
    0x90, 0x38, 0x42, 0x10, 0x9f,

    /* U+00F0 "ð" */
    0x70, 0x27, 0x73, 0x86, 0x1c, 0xde,

    /* U+00F1 "ñ" */
    0x62, 0xad, 0x98, 0xc6, 0x31,

    /* U+00F2 "ò" */
    0x20, 0x7, 0xb3, 0x86, 0x1c, 0xde,

    /* U+00F3 "ó" */
    0x10, 0x7, 0xb3, 0x86, 0x1c, 0xde,

    /* U+00F4 "ô" */
    0x30, 0xc7, 0xb3, 0x86, 0x1c, 0xde,

    /* U+00F5 "õ" */
    0x78, 0x7, 0xb3, 0x86, 0x1c, 0xde,

    /* U+00F6 "ö" */
    0x48, 0x7, 0xb3, 0x86, 0x1c, 0xde,

    /* U+00F7 "÷" */
    0x20, 0x3e, 0x2, 0x0,

    /* U+00F8 "ø" */
    0x1, 0xed, 0xe5, 0xa6, 0xb7, 0x80,

    /* U+00F9 "ù" */
    0x20, 0x23, 0x18, 0xc6, 0x6d,

    /* U+00FA "ú" */
    0x20, 0x23, 0x18, 0xc6, 0x6d,

    /* U+00FB "û" */
    0x22, 0xa3, 0x18, 0xc6, 0x6d,

    /* U+00FC "ü" */
    0x50, 0x23, 0x18, 0xc6, 0x6d,

    /* U+00FD "ý" */
    0x20, 0x23, 0x18, 0xc6, 0x6d, 0xb, 0xc0,

    /* U+00FE "þ" */
    0x82, 0xb, 0xb3, 0x86, 0x1c, 0xee, 0x82, 0x0,

    /* U+00FF "ÿ" */
    0x50, 0x23, 0x18, 0xc6, 0x6d, 0xb, 0xc0,

    /* U+2013 "–" */
    0xf8,

    /* U+2014 "—" */
    0xfe,

    /* U+2018 "‘" */
    0xec,

    /* U+2019 "’" */
    0xdc,

    /* U+201A "‚" */
    0xf0,

    /* U+201C "“" */
    0xba, 0xf0,

    /* U+201D "”" */
    0xf5, 0xd0,

    /* U+201E "„" */
    0xfd,

    /* U+2020 "†" */
    0x5d, 0x24, 0x80,

    /* U+2021 "‡" */
    0x5d, 0x2e, 0x80,

    /* U+2022 "•" */
    0xff, 0x80,

    /* U+2026 "…" */
    0xa8,

    /* U+2030 "‰" */
    0xf2, 0x4f, 0x88, 0x54, 0x71, 0xc5,

    /* U+2032 "′" */
    0xd8,

    /* U+2033 "″" */
    0xf5, 0x80,

    /* U+2039 "‹" */
    0xf, 0x46, 0x40,

    /* U+203A "›" */
    0x86, 0x33, 0x68,

    /* U+2044 "⁄" */
    0x4, 0x66, 0x20,

    /* U+2190 "←" */
    0x42, 0xf, 0xe0, 0x40,

    /* U+2191 "↑" */
    0x75, 0x48, 0x42, 0x10,

    /* U+2192 "→" */
    0x8, 0x1f, 0xc1, 0x8,

    /* U+2193 "↓" */
    0x21, 0x8, 0x4a, 0xb8,

    /* U+2194 "↔" */
    0x4a, 0x1f, 0xe1, 0x48,

    /* U+2195 "↕" */
    0x75, 0x48, 0x4a, 0xb8,

    /* U+2196 "↖" */
    0xf6, 0x29, 0x20, 0x80,

    /* U+2197 "↗" */
    0x78, 0xca, 0x98, 0x0,

    /* U+2198 "↘" */
    0x82, 0x4a, 0x37, 0x80,

    /* U+2199 "↙" */
    0xc, 0xa9, 0x8f, 0x0,

    /* U+F00B "" */
    0xff, 0x47, 0xfd, 0x1f, 0xf4, 0x7f, 0x80,

    /* U+F013 "" */
    0x18, 0x7e, 0xe7, 0x66, 0xe7, 0x7e, 0x18,

    /* U+F01E "" */
    0x3a, 0x8e, 0x3c, 0x8, 0x8, 0x8e, 0x0,

    /* U+F07B "" */
    0xe1, 0xff, 0xff, 0xff, 0xff, 0xc0,

    /* U+F0C7 "" */
    0xfe, 0xc7, 0xbd, 0x81, 0x99, 0x81, 0xff,

    /* U+F0C9 "" */
    0xfe, 0x0, 0x7, 0xf0, 0x0, 0x3f, 0x80,

    /* U+F1EB "" */
    0x39, 0x8c, 0xe2, 0x21, 0x0,

    /* U+F293 "" */
    0x38, 0xdb, 0x9f, 0x7c, 0x7d, 0xf3, 0xf3, 0x6c,
    0x78,

    /* U+F030C "󰌌" */
    0xff, 0xdd, 0xff, 0xdd, 0xe7, 0xff
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 118, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 118, .box_w = 1, .box_h = 8, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 2, .adv_w = 118, .box_w = 4, .box_h = 3, .ofs_x = 2, .ofs_y = 5},
    {.bitmap_index = 4, .adv_w = 118, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 8, .adv_w = 118, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 13, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 19, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 25, .adv_w = 118, .box_w = 1, .box_h = 3, .ofs_x = 3, .ofs_y = 5},
    {.bitmap_index = 26, .adv_w = 118, .box_w = 5, .box_h = 12, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 34, .adv_w = 118, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 42, .adv_w = 118, .box_w = 7, .box_h = 6, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 48, .adv_w = 118, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 52, .adv_w = 118, .box_w = 2, .box_h = 2, .ofs_x = 3, .ofs_y = -1},
    {.bitmap_index = 53, .adv_w = 118, .box_w = 3, .box_h = 1, .ofs_x = 2, .ofs_y = 3},
    {.bitmap_index = 54, .adv_w = 118, .box_w = 1, .box_h = 1, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 55, .adv_w = 118, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 62, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 68, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 74, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 80, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 86, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 92, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 98, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 104, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 110, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 116, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 122, .adv_w = 118, .box_w = 1, .box_h = 6, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 123, .adv_w = 118, .box_w = 1, .box_h = 7, .ofs_x = 3, .ofs_y = -1},
    {.bitmap_index = 124, .adv_w = 118, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 128, .adv_w = 118, .box_w = 5, .box_h = 3, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 130, .adv_w = 118, .box_w = 6, .box_h = 5, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 134, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 140, .adv_w = 118, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 146, .adv_w = 118, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 153, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 159, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 165, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 171, .adv_w = 118, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 176, .adv_w = 118, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 181, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 187, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 193, .adv_w = 118, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 198, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 204, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 210, .adv_w = 118, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 215, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 221, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 227, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 233, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 239, .adv_w = 118, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 247, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 253, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 259, .adv_w = 118, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 264, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 270, .adv_w = 118, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 277, .adv_w = 118, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 284, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 290, .adv_w = 118, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 297, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 303, .adv_w = 118, .box_w = 2, .box_h = 12, .ofs_x = 3, .ofs_y = -2},
    {.bitmap_index = 306, .adv_w = 118, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 313, .adv_w = 118, .box_w = 2, .box_h = 12, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 316, .adv_w = 118, .box_w = 5, .box_h = 4, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 319, .adv_w = 118, .box_w = 5, .box_h = 1, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 320, .adv_w = 118, .box_w = 2, .box_h = 1, .ofs_x = 3, .ofs_y = 7},
    {.bitmap_index = 321, .adv_w = 118, .box_w = 7, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 327, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 333, .adv_w = 118, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 338, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 344, .adv_w = 118, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 349, .adv_w = 118, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 354, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 360, .adv_w = 118, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 365, .adv_w = 118, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 370, .adv_w = 118, .box_w = 3, .box_h = 10, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 374, .adv_w = 118, .box_w = 5, .box_h = 8, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 379, .adv_w = 118, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 384, .adv_w = 118, .box_w = 7, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 390, .adv_w = 118, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 394, .adv_w = 118, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 399, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 405, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 411, .adv_w = 118, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 416, .adv_w = 118, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 420, .adv_w = 118, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 425, .adv_w = 118, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 429, .adv_w = 118, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 434, .adv_w = 118, .box_w = 7, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 440, .adv_w = 118, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 445, .adv_w = 118, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 450, .adv_w = 118, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 454, .adv_w = 118, .box_w = 3, .box_h = 12, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 459, .adv_w = 118, .box_w = 1, .box_h = 12, .ofs_x = 3, .ofs_y = -2},
    {.bitmap_index = 461, .adv_w = 118, .box_w = 3, .box_h = 12, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 466, .adv_w = 118, .box_w = 6, .box_h = 2, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 468, .adv_w = 118, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 469, .adv_w = 118, .box_w = 1, .box_h = 8, .ofs_x = 3, .ofs_y = -2},
    {.bitmap_index = 470, .adv_w = 118, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 476, .adv_w = 118, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 480, .adv_w = 118, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 485, .adv_w = 118, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 489, .adv_w = 118, .box_w = 1, .box_h = 12, .ofs_x = 3, .ofs_y = -2},
    {.bitmap_index = 491, .adv_w = 118, .box_w = 3, .box_h = 6, .ofs_x = 2, .ofs_y = 2},
    {.bitmap_index = 494, .adv_w = 118, .box_w = 4, .box_h = 1, .ofs_x = 2, .ofs_y = 7},
    {.bitmap_index = 495, .adv_w = 118, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 500, .adv_w = 118, .box_w = 4, .box_h = 3, .ofs_x = 2, .ofs_y = 5},
    {.bitmap_index = 502, .adv_w = 118, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 507, .adv_w = 118, .box_w = 5, .box_h = 2, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 509, .adv_w = 118, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 514, .adv_w = 118, .box_w = 3, .box_h = 1, .ofs_x = 2, .ofs_y = 7},
    {.bitmap_index = 515, .adv_w = 118, .box_w = 4, .box_h = 3, .ofs_x = 2, .ofs_y = 5},
    {.bitmap_index = 517, .adv_w = 118, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 522, .adv_w = 118, .box_w = 3, .box_h = 3, .ofs_x = 3, .ofs_y = 5},
    {.bitmap_index = 524, .adv_w = 118, .box_w = 3, .box_h = 3, .ofs_x = 2, .ofs_y = 5},
    {.bitmap_index = 526, .adv_w = 118, .box_w = 2, .box_h = 1, .ofs_x = 3, .ofs_y = 7},
    {.bitmap_index = 527, .adv_w = 118, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 532, .adv_w = 118, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 535, .adv_w = 118, .box_w = 1, .box_h = 1, .ofs_x = 3, .ofs_y = 4},
    {.bitmap_index = 536, .adv_w = 118, .box_w = 2, .box_h = 2, .ofs_x = 3, .ofs_y = -3},
    {.bitmap_index = 537, .adv_w = 118, .box_w = 3, .box_h = 3, .ofs_x = 3, .ofs_y = 5},
    {.bitmap_index = 539, .adv_w = 118, .box_w = 4, .box_h = 3, .ofs_x = 2, .ofs_y = 5},
    {.bitmap_index = 541, .adv_w = 118, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 546, .adv_w = 118, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 553, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 559, .adv_w = 118, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 567, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 573, .adv_w = 118, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 582, .adv_w = 118, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 591, .adv_w = 118, .box_w = 7, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 601, .adv_w = 118, .box_w = 7, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 611, .adv_w = 118, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 619, .adv_w = 118, .box_w = 7, .box_h = 12, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 630, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 636, .adv_w = 118, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 644, .adv_w = 118, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 651, .adv_w = 118, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 658, .adv_w = 118, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 665, .adv_w = 118, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 672, .adv_w = 118, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 679, .adv_w = 118, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 686, .adv_w = 118, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 693, .adv_w = 118, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 701, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 707, .adv_w = 118, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 716, .adv_w = 118, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 724, .adv_w = 118, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 732, .adv_w = 118, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 741, .adv_w = 118, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 750, .adv_w = 118, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 759, .adv_w = 118, .box_w = 5, .box_h = 4, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 762, .adv_w = 118, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 770, .adv_w = 118, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 778, .adv_w = 118, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 786, .adv_w = 118, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 795, .adv_w = 118, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 804, .adv_w = 118, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 813, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 819, .adv_w = 118, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 826, .adv_w = 118, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 833, .adv_w = 118, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 840, .adv_w = 118, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 847, .adv_w = 118, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 854, .adv_w = 118, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 861, .adv_w = 118, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 869, .adv_w = 118, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 873, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 879, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 885, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 891, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 897, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 903, .adv_w = 118, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 908, .adv_w = 118, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 913, .adv_w = 118, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 918, .adv_w = 118, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 923, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 929, .adv_w = 118, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 934, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 940, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 946, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 952, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 958, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 964, .adv_w = 118, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 968, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 974, .adv_w = 118, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 979, .adv_w = 118, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 984, .adv_w = 118, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 989, .adv_w = 118, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 994, .adv_w = 118, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1001, .adv_w = 118, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1009, .adv_w = 118, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1016, .adv_w = 118, .box_w = 5, .box_h = 1, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 1017, .adv_w = 118, .box_w = 7, .box_h = 1, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 1018, .adv_w = 118, .box_w = 2, .box_h = 3, .ofs_x = 3, .ofs_y = 5},
    {.bitmap_index = 1019, .adv_w = 118, .box_w = 2, .box_h = 3, .ofs_x = 3, .ofs_y = 5},
    {.bitmap_index = 1020, .adv_w = 118, .box_w = 2, .box_h = 2, .ofs_x = 3, .ofs_y = -1},
    {.bitmap_index = 1021, .adv_w = 118, .box_w = 4, .box_h = 3, .ofs_x = 2, .ofs_y = 5},
    {.bitmap_index = 1023, .adv_w = 118, .box_w = 4, .box_h = 3, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 1025, .adv_w = 118, .box_w = 4, .box_h = 2, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 1026, .adv_w = 118, .box_w = 3, .box_h = 6, .ofs_x = 2, .ofs_y = 2},
    {.bitmap_index = 1029, .adv_w = 118, .box_w = 3, .box_h = 6, .ofs_x = 2, .ofs_y = 2},
    {.bitmap_index = 1032, .adv_w = 118, .box_w = 3, .box_h = 3, .ofs_x = 2, .ofs_y = 3},
    {.bitmap_index = 1034, .adv_w = 118, .box_w = 5, .box_h = 1, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1035, .adv_w = 118, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 1041, .adv_w = 118, .box_w = 2, .box_h = 3, .ofs_x = 3, .ofs_y = 5},
    {.bitmap_index = 1042, .adv_w = 118, .box_w = 4, .box_h = 3, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 1044, .adv_w = 118, .box_w = 3, .box_h = 6, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 1047, .adv_w = 118, .box_w = 4, .box_h = 6, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 1050, .adv_w = 118, .box_w = 6, .box_h = 4, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 1053, .adv_w = 118, .box_w = 6, .box_h = 5, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 1057, .adv_w = 118, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 1061, .adv_w = 118, .box_w = 6, .box_h = 5, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 1065, .adv_w = 118, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 1069, .adv_w = 118, .box_w = 6, .box_h = 5, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 1073, .adv_w = 118, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 1077, .adv_w = 118, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 1081, .adv_w = 118, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 1085, .adv_w = 118, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 1089, .adv_w = 118, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 1093, .adv_w = 118, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 1100, .adv_w = 118, .box_w = 8, .box_h = 7, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 1107, .adv_w = 118, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 1114, .adv_w = 118, .box_w = 7, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 1120, .adv_w = 118, .box_w = 8, .box_h = 7, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 1127, .adv_w = 118, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 1134, .adv_w = 118, .box_w = 7, .box_h = 5, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 1139, .adv_w = 118, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1148, .adv_w = 118, .box_w = 8, .box_h = 6, .ofs_x = 0, .ofs_y = 2}
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
const lv_font_t space_mono_12_cr = {
#else
lv_font_t space_mono_12_cr = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 15,          /*The maximum line height required by the font*/
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



#endif /*#if SPACE_MONO_12_CR*/

