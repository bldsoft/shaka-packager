// Copyright 2020 Google LLC. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "packager/media/formats/dvb/dvb_teletext_charset.h"

#include <cstring>

namespace shaka {
namespace media {

//-----------------------------------------------------------------------------
// Static parity checking table.
//-----------------------------------------------------------------------------

namespace {
const uint8_t PARITY_8[256] = {
    0x00, 0x01, 0x01, 0x00, 0x01, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01,
    0x00, 0x01, 0x01, 0x00, 0x01, 0x00, 0x00, 0x01, 0x00, 0x01, 0x01, 0x00,
    0x00, 0x01, 0x01, 0x00, 0x01, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01,
    0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01, 0x00, 0x01, 0x00, 0x00, 0x01,
    0x00, 0x01, 0x01, 0x00, 0x01, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01,
    0x00, 0x01, 0x01, 0x00, 0x01, 0x00, 0x00, 0x01, 0x00, 0x01, 0x01, 0x00,
    0x00, 0x01, 0x01, 0x00, 0x01, 0x00, 0x00, 0x01, 0x00, 0x01, 0x01, 0x00,
    0x01, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x00, 0x01, 0x01, 0x00,
    0x00, 0x01, 0x01, 0x00, 0x01, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01,
    0x00, 0x01, 0x01, 0x00, 0x01, 0x00, 0x00, 0x01, 0x00, 0x01, 0x01, 0x00,
    0x00, 0x01, 0x01, 0x00, 0x01, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01,
    0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01, 0x00, 0x01, 0x00, 0x00, 0x01,
    0x00, 0x01, 0x01, 0x00, 0x01, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01,
    0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01, 0x00, 0x01, 0x00, 0x00, 0x01,
    0x01, 0x00, 0x00, 0x01, 0x00, 0x01, 0x01, 0x00, 0x01, 0x00, 0x00, 0x01,
    0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01, 0x00, 0x01, 0x00, 0x00, 0x01,
    0x00, 0x01, 0x01, 0x00, 0x01, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01,
    0x00, 0x01, 0x01, 0x00, 0x01, 0x00, 0x00, 0x01, 0x00, 0x01, 0x01, 0x00,
    0x00, 0x01, 0x01, 0x00, 0x01, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01,
    0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 0x01, 0x00, 0x01, 0x00, 0x00, 0x01,
    0x00, 0x01, 0x01, 0x00, 0x01, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01,
    0x00, 0x01, 0x01, 0x00};
}

//-----------------------------------------------------------------------------
// Teletext character set tables. All characters are encoded in UCS-2.
//-----------------------------------------------------------------------------

//
// Initial base content of a charset.
//
const DvbTeletextCharset::G0CharsetData DvbTeletextCharset::G0Base = {
    {// Latin G0 Primary Set
     0x0020, 0x0021, 0x0022, 0x00a3, 0x0024, 0x0025, 0x0026, 0x0027, 0x0028,
     0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f, 0x0030, 0x0031,
     0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003a,
     0x003b, 0x003c, 0x003d, 0x003e, 0x003f, 0x0040, 0x0041, 0x0042, 0x0043,
     0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004a, 0x004b, 0x004c,
     0x004d, 0x004e, 0x004f, 0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055,
     0x0056, 0x0057, 0x0058, 0x0059, 0x005a, 0x00ab, 0x00bd, 0x00bb, 0x005e,
     0x0023, 0x002d, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
     0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f, 0x0070,
     0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078, 0x0079,
     0x007a, 0x00bc, 0x00a6, 0x00be, 0x00f7, 0x007f},
    {// Cyrillic G0 Primary Set - Option 1 - Serbian/Croatian
     0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x044b, 0x0027, 0x0028,
     0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f, 0x0030, 0x0031,
     0x3200, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003a,
     0x003b, 0x003c, 0x003d, 0x003e, 0x003f, 0x0427, 0x0410, 0x0411, 0x0426,
     0x0414, 0x0415, 0x0424, 0x0413, 0x0425, 0x0418, 0x0408, 0x041a, 0x041b,
     0x041c, 0x041d, 0x041e, 0x041f, 0x040c, 0x0420, 0x0421, 0x0422, 0x0423,
     0x0412, 0x0403, 0x0409, 0x040a, 0x0417, 0x040b, 0x0416, 0x0402, 0x0428,
     0x040f, 0x0447, 0x0430, 0x0431, 0x0446, 0x0434, 0x0435, 0x0444, 0x0433,
     0x0445, 0x0438, 0x0428, 0x043a, 0x043b, 0x043c, 0x043d, 0x043e, 0x043f,
     0x042c, 0x0440, 0x0441, 0x0442, 0x0443, 0x0432, 0x0423, 0x0429, 0x042a,
     0x0437, 0x042b, 0x0436, 0x0422, 0x0448, 0x042f},
    {// Cyrillic G0 Primary Set - Option 2 - Russian/Bulgarian
     0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x044b, 0x0027, 0x0028,
     0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f, 0x0030, 0x0031,
     0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003a,
     0x003b, 0x003c, 0x003d, 0x003e, 0x003f, 0x042e, 0x0410, 0x0411, 0x0426,
     0x0414, 0x0415, 0x0424, 0x0413, 0x0425, 0x0418, 0x0419, 0x041a, 0x041b,
     0x041c, 0x041d, 0x041e, 0x041f, 0x042f, 0x0420, 0x0421, 0x0422, 0x0423,
     0x0416, 0x0412, 0x042c, 0x042a, 0x0417, 0x0428, 0x042d, 0x0429, 0x0427,
     0x042b, 0x044e, 0x0430, 0x0431, 0x0446, 0x0434, 0x0435, 0x0444, 0x0433,
     0x0445, 0x0438, 0x0439, 0x043a, 0x043b, 0x043c, 0x043d, 0x043e, 0x043f,
     0x044f, 0x0440, 0x0441, 0x0442, 0x0443, 0x0436, 0x0432, 0x044c, 0x044a,
     0x0437, 0x0448, 0x044d, 0x0449, 0x0447, 0x044b},
    {// Cyrillic G0 Primary Set - Option 3 - Ukrainian
     0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x00ef, 0x0027, 0x0028,
     0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f, 0x0030, 0x0031,
     0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003a,
     0x003b, 0x003c, 0x003d, 0x003e, 0x003f, 0x042e, 0x0410, 0x0411, 0x0426,
     0x0414, 0x0415, 0x0424, 0x0413, 0x0425, 0x0418, 0x0419, 0x041a, 0x041b,
     0x041c, 0x041d, 0x041e, 0x041f, 0x042f, 0x0420, 0x0421, 0x0422, 0x0423,
     0x0416, 0x0412, 0x042c, 0x0049, 0x0417, 0x0428, 0x042d, 0x0429, 0x0427,
     0x00cf, 0x044e, 0x0430, 0x0431, 0x0446, 0x0434, 0x0435, 0x0444, 0x0433,
     0x0445, 0x0438, 0x0439, 0x043a, 0x043b, 0x043c, 0x043d, 0x043e, 0x043f,
     0x044f, 0x0440, 0x0441, 0x0442, 0x0443, 0x0436, 0x0432, 0x044c, 0x0069,
     0x0437, 0x0448, 0x044d, 0x0449, 0x0447, 0x00ff},
    {// Greek G0 Primary Set
     0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 0x0028,
     0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f, 0x0030, 0x0031,
     0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003a,
     0x003b, 0x003c, 0x003d, 0x003e, 0x003f, 0x0390, 0x0391, 0x0392, 0x0393,
     0x0394, 0x0395, 0x0396, 0x0397, 0x0398, 0x0399, 0x039a, 0x039b, 0x039c,
     0x039d, 0x039e, 0x039f, 0x03a0, 0x03a1, 0x03a2, 0x03a3, 0x03a4, 0x03a5,
     0x03a6, 0x03a7, 0x03a8, 0x03a9, 0x03aa, 0x03ab, 0x03ac, 0x03ad, 0x03ae,
     0x03af, 0x03b0, 0x03b1, 0x03b2, 0x03b3, 0x03b4, 0x03b5, 0x03b6, 0x03b7,
     0x03b8, 0x03b9, 0x03ba, 0x03bb, 0x03bc, 0x03bd, 0x03be, 0x03bf, 0x03c0,
     0x03c1, 0x03c2, 0x03c3, 0x03c4, 0x03c5, 0x03c6, 0x03c7, 0x03c8, 0x03c9,
     0x03ca, 0x03cb, 0x03cc, 0x03cd, 0x03ce, 0x03cf},
    {
        // Arabic G0 Primary Set (not yet implemented)
        0x0000,
    },
    {
        // Hebrew G0 Primary Set (not yet implemented)
        0x0000,
    }};

namespace {
// Array positions where chars from G0_LATIN_NATIONAL_SUBSETS are injected into
// G0[LATIN]
const uint8_t G0_LATIN_NATIONAL_SUBSETS_POSITIONS[13] = {
    0x03, 0x04, 0x20, 0x3b, 0x3c, 0x3d, 0x3e,
    0x3f, 0x40, 0x5b, 0x5c, 0x5d, 0x5e};

// ETS 300 706, chapter 15.2, table 32: Function of Default G0 and G2 Character
// Set Designation
// and National Option Selection bits in packets X/28/0 Format 1, X/28/4, M/29/0
// and M/29/4

// Latin National Option Sub-sets

const uint16_t G0_LATIN_NATIONAL_SUBSETS[13][13] = {
    {// 0, English
     0x00a3, 0x0024, 0x0040, 0x00ab, 0x00bd, 0x00bb, 0x005e, 0x0023, 0x002d,
     0x00bc, 0x00a6, 0x00be, 0x00f7},
    {// 1, French
     0x00e9, 0x00ef, 0x00e0, 0x00eb, 0x00ea, 0x00f9, 0x00ee, 0x0023, 0x00e8,
     0x00e2, 0x00f4, 0x00fb, 0x00e7},
    {// 2, Swedish, Finnish, Hungarian
     0x0023, 0x00a4, 0x00c9, 0x00c4, 0x00d6, 0x00c5, 0x00dc, 0x005f, 0x00e9,
     0x00e4, 0x00f6, 0x00e5, 0x00fc},
    {// 3, Czech, Slovak
     0x0023, 0x016f, 0x010d, 0x0165, 0x017e, 0x00fd, 0x00ed, 0x0159, 0x00e9,
     0x00e1, 0x011b, 0x00fa, 0x0161},
    {// 4, German
     0x0023, 0x0024, 0x00a7, 0x00c4, 0x00d6, 0x00dc, 0x005e, 0x005f, 0x00b0,
     0x00e4, 0x00f6, 0x00fc, 0x00df},
    {// 5, Portuguese, Spanish
     0x00e7, 0x0024, 0x00a1, 0x00e1, 0x00e9, 0x00ed, 0x00f3, 0x00fa, 0x00bf,
     0x00fc, 0x00f1, 0x00e8, 0x00e0},
    {// 6, Italian
     0x00a3, 0x0024, 0x00e9, 0x00b0, 0x00e7, 0x00bb, 0x005e, 0x0023, 0x00f9,
     0x00e0, 0x00f2, 0x00e8, 0x00ec},
    {// 7, Romanian
     0x0023, 0x00a4, 0x0162, 0x00c2, 0x015e, 0x0102, 0x00ce, 0x0131, 0x0163,
     0x00e2, 0x015f, 0x0103, 0x00ee},
    {// 8, Polish
     0x0023, 0x0144, 0x0105, 0x017b, 0x015a, 0x0141, 0x0107, 0x00f3, 0x0119,
     0x017c, 0x015b, 0x0142, 0x017a},
    {// 9, Turkish
     0x0054, 0x011f, 0x0130, 0x015e, 0x00d6, 0x00c7, 0x00dc, 0x011e, 0x0131,
     0x015f, 0x00f6, 0x00e7, 0x00fc},
    {// 10, Serbian, Croatian, Slovenian
     0x0023, 0x00cb, 0x010c, 0x0106, 0x017d, 0x0110, 0x0160, 0x00eb, 0x010d,
     0x0107, 0x017e, 0x0111, 0x0161},
    {// 11, Estonian
     0x0023, 0x00f5, 0x0160, 0x00c4, 0x00d6, 0x017e, 0x00dc, 0x00d5, 0x0161,
     0x00e4, 0x00f6, 0x017e, 0x00fc},
    {// 12, Lettish, Lithuanian
     0x0023, 0x0024, 0x0160, 0x0117, 0x0119, 0x017d, 0x010d, 0x016b, 0x0161,
     0x0105, 0x0173, 0x017e, 0x012f}};

// References to the G0_LATIN_NATIONAL_SUBSETS array
const uint8_t G0_LATIN_NATIONAL_SUBSETS_MAP[56] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x01, 0x02, 0x03,
    0x04, 0xff, 0x06, 0xff, 0x00, 0x01, 0x02, 0x09, 0x04, 0x05, 0x06, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0x0a, 0xff, 0x07, 0xff, 0xff, 0x0b, 0x03,
    0x04, 0xff, 0x0c, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0x09, 0xff, 0xff, 0xff, 0xff};

// --- G2 ----------------------------------------------------------------------

const char16_t G2[3][DvbTeletextCharset::CHAR_COUNT] = {
    {// Latin G2 Supplementary Set
     0x0020, 0x00a1, 0x00a2, 0x00a3, 0x0024, 0x00a5, 0x0023, 0x00a7, 0x00a4,
     0x2018, 0x201c, 0x00ab, 0x2190, 0x2191, 0x2192, 0x2193, 0x00b0, 0x00b1,
     0x00b2, 0x00b3, 0x00d7, 0x00b5, 0x00b6, 0x00b7, 0x00f7, 0x2019, 0x201d,
     0x00bb, 0x00bc, 0x00bd, 0x00be, 0x00bf, 0x0020, 0x0300, 0x0301, 0x0302,
     0x0303, 0x0304, 0x0306, 0x0307, 0x0308, 0x0000, 0x030a, 0x0327, 0x005f,
     0x030b, 0x0328, 0x030c, 0x2015, 0x00b9, 0x00ae, 0x00a9, 0x2122, 0x266a,
     0x20ac, 0x2030, 0x03B1, 0x0000, 0x0000, 0x0000, 0x215b, 0x215c, 0x215d,
     0x215e, 0x03a9, 0x00c6, 0x0110, 0x00aa, 0x0126, 0x0000, 0x0132, 0x013f,
     0x0141, 0x00d8, 0x0152, 0x00ba, 0x00de, 0x0166, 0x014a, 0x0149, 0x0138,
     0x00e6, 0x0111, 0x00f0, 0x0127, 0x0131, 0x0133, 0x0140, 0x0142, 0x00f8,
     0x0153, 0x00df, 0x00fe, 0x0167, 0x014b, 0x0020},
    {// Cyrillic G2 Supplementary Set
     0x00A0, 0x00A1, 0x00A2, 0x00A3, 0x0020, 0x00A5, 0x0023, 0x00A7, 0x0020,
     0x2018, 0x201C, 0x00AB, 0x2190, 0x2191, 0x2192, 0x2193, 0x00B0, 0x00B1,
     0x00B2, 0x00B3, 0x00D7, 0x00B5, 0x00B6, 0x00B7, 0x00F7, 0x2019, 0x201D,
     0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x00BF, 0x0020, 0x02CB, 0x02CA, 0x02C6,
     0x02DC, 0x02C9, 0x02D8, 0x02D9, 0x00A8, 0x002E, 0x02DA, 0x02CF, 0x02CD,
     0x02DD, 0x02DB, 0x02C7, 0x2014, 0x00B9, 0x00AE, 0x00A9, 0x2122, 0x266A,
     0x20A0, 0x2030, 0x0251, 0x0141, 0x0142, 0x00DF, 0x215B, 0x215C, 0x215D,
     0x215E, 0x0044, 0x0045, 0x0046, 0x0047, 0x0049, 0x004A, 0x004B, 0x004C,
     0x004E, 0x0051, 0x0052, 0x0053, 0x0055, 0x0056, 0x0057, 0x005A, 0x0064,
     0x0065, 0x0066, 0x0067, 0x0069, 0x006A, 0x006B, 0x006C, 0x006E, 0x0071,
     0x0072, 0x0073, 0x0075, 0x0076, 0x0077, 0x007A},
    {// Greek G2 Supplementary Set
     0x00A0, 0x0061, 0x0062, 0x00A3, 0x0065, 0x0068, 0x0069, 0x00A7, 0x003A,
     0x2018, 0x201C, 0x006B, 0x2190, 0x2191, 0x2192, 0x2193, 0x00B0, 0x00B1,
     0x00B2, 0x00B3, 0x0078, 0x006D, 0x006E, 0x0070, 0x00F7, 0x2019, 0x201D,
     0x0074, 0x00BC, 0x00BD, 0x00BE, 0x0078, 0x0020, 0x02CB, 0x02CA, 0x02C6,
     0x02DC, 0x02C9, 0x02D8, 0x02D9, 0x00A8, 0x002E, 0x02DA, 0x02CF, 0x02CD,
     0x02DD, 0x02DB, 0x02C7, 0x003F, 0x00B9, 0x00AE, 0x00A9, 0x2122, 0x266A,
     0x20A0, 0x2030, 0x0251, 0x038A, 0x038E, 0x038F, 0x215B, 0x215C, 0x215D,
     0x215E, 0x0043, 0x0044, 0x0046, 0x0047, 0x004A, 0x004C, 0x0051, 0x0052,
     0x0053, 0x0055, 0x0056, 0x0057, 0x0059, 0x005A, 0x0386, 0x0389, 0x0063,
     0x0064, 0x0066, 0x0067, 0x006A, 0x006C, 0x0071, 0x0072, 0x0073, 0x0075,
     0x0076, 0x0077, 0x0079, 0x007A, 0x0388, 0x25A0},
    // Unimplemented character sets:
    // { // Arabic G2 Supplementary Set
    // }
};

const size_t ACCENT_MODE_COUNT = 15;
const size_t ACCENT_LETTER_COUNT = 52;  // A-Z a-z

const char16_t G2_ACCENTS[ACCENT_MODE_COUNT][ACCENT_LETTER_COUNT] = {
    // A B C D E F G H I J K L M N O P Q R S T U V W X Y Z a b c d e f g h i j k
    // l m n o p q r s t u v w x y z
    {// grave
     0x00c0, 0x0000, 0x0000, 0x0000, 0x00c8, 0x0000, 0x0000, 0x0000, 0x00cc,
     0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x00d2, 0x0000, 0x0000, 0x0000,
     0x0000, 0x0000, 0x00d9, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x00e0,
     0x0000, 0x0000, 0x0000, 0x00e8, 0x0000, 0x0000, 0x0000, 0x00ec, 0x0000,
     0x0000, 0x0000, 0x0000, 0x0000, 0x00f2, 0x0000, 0x0000, 0x0000, 0x0000,
     0x0000, 0x00f9, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
    {// acute
     0x00c1, 0x0000, 0x0106, 0x0000, 0x00c9, 0x0000, 0x0000, 0x0000, 0x00cd,
     0x0000, 0x0000, 0x0139, 0x0000, 0x0143, 0x00d3, 0x0000, 0x0000, 0x0154,
     0x015a, 0x0000, 0x00da, 0x0000, 0x0000, 0x0000, 0x00dd, 0x0179, 0x00e1,
     0x0000, 0x0107, 0x0000, 0x00e9, 0x0000, 0x0123, 0x0000, 0x00ed, 0x0000,
     0x0000, 0x013a, 0x0000, 0x0144, 0x00f3, 0x0000, 0x0000, 0x0155, 0x015b,
     0x0000, 0x00fa, 0x0000, 0x0000, 0x0000, 0x00fd, 0x017a},
    {// circumflex
     0x00c2, 0x0000, 0x0108, 0x0000, 0x00ca, 0x0000, 0x011c, 0x0124, 0x00ce,
     0x0134, 0x0000, 0x0000, 0x0000, 0x0000, 0x00d4, 0x0000, 0x0000, 0x0000,
     0x015c, 0x0000, 0x00db, 0x0000, 0x0174, 0x0000, 0x0176, 0x0000, 0x00e2,
     0x0000, 0x0109, 0x0000, 0x00ea, 0x0000, 0x011d, 0x0125, 0x00ee, 0x0135,
     0x0000, 0x0000, 0x0000, 0x0000, 0x00f4, 0x0000, 0x0000, 0x0000, 0x015d,
     0x0000, 0x00fb, 0x0000, 0x0175, 0x0000, 0x0177, 0x0000},
    {// tilde
     0x00c3, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0128,
     0x0000, 0x0000, 0x0000, 0x0000, 0x00d1, 0x00d5, 0x0000, 0x0000, 0x0000,
     0x0000, 0x0000, 0x0168, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x00e3,
     0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0129, 0x0000,
     0x0000, 0x0000, 0x0000, 0x00f1, 0x00f5, 0x0000, 0x0000, 0x0000, 0x0000,
     0x0000, 0x0169, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
    {// macron
     0x0100, 0x0000, 0x0000, 0x0000, 0x0112, 0x0000, 0x0000, 0x0000, 0x012a,
     0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x014c, 0x0000, 0x0000, 0x0000,
     0x0000, 0x0000, 0x016a, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0101,
     0x0000, 0x0000, 0x0000, 0x0113, 0x0000, 0x0000, 0x0000, 0x012b, 0x0000,
     0x0000, 0x0000, 0x0000, 0x0000, 0x014d, 0x0000, 0x0000, 0x0000, 0x0000,
     0x0000, 0x016b, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
    {// breve
     0x0102, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x011e, 0x0000, 0x0000,
     0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
     0x0000, 0x0000, 0x016c, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0103,
     0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x011f, 0x0000, 0x0000, 0x0000,
     0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
     0x0000, 0x016d, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
    {// dot
     0x0000, 0x0000, 0x010a, 0x0000, 0x0116, 0x0000, 0x0120, 0x0000, 0x0130,
     0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
     0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x017b, 0x0000,
     0x0000, 0x010b, 0x0000, 0x0117, 0x0000, 0x0121, 0x0000, 0x0000, 0x0000,
     0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
     0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x017c},
    {// umlaut
     0x00c4, 0x0000, 0x0000, 0x0000, 0x00cb, 0x0000, 0x0000, 0x0000, 0x00cf,
     0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x00d6, 0x0000, 0x0000, 0x0000,
     0x0000, 0x0000, 0x00dc, 0x0000, 0x0000, 0x0000, 0x0178, 0x0000, 0x00e4,
     0x0000, 0x0000, 0x0000, 0x00eb, 0x0000, 0x0000, 0x0000, 0x00ef, 0x0000,
     0x0000, 0x0000, 0x0000, 0x0000, 0x00f6, 0x0000, 0x0000, 0x0000, 0x0000,
     0x0000, 0x00fc, 0x0000, 0x0000, 0x0000, 0x00ff, 0x0000},
    {0},
    {// ring
     0x00c5, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
     0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
     0x0000, 0x0000, 0x016e, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x00e5,
     0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
     0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
     0x0000, 0x016f, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
    {// cedilla
     0x0000, 0x0000, 0x00c7, 0x0000, 0x0000, 0x0000, 0x0122, 0x0000, 0x0000,
     0x0000, 0x0136, 0x013b, 0x0000, 0x0145, 0x0000, 0x0000, 0x0000, 0x0156,
     0x015e, 0x0162, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
     0x0000, 0x00e7, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
     0x0137, 0x013c, 0x0000, 0x0146, 0x0000, 0x0000, 0x0000, 0x0157, 0x015f,
     0x0163, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
    {0},
    {// double acute
     0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
     0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0150, 0x0000, 0x0000, 0x0000,
     0x0000, 0x0000, 0x0170, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
     0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
     0x0000, 0x0000, 0x0000, 0x0000, 0x0151, 0x0000, 0x0000, 0x0000, 0x0000,
     0x0000, 0x0171, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
    {// ogonek
     0x0104, 0x0000, 0x0000, 0x0000, 0x0118, 0x0000, 0x0000, 0x0000, 0x012e,
     0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
     0x0000, 0x0000, 0x0172, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0105,
     0x0000, 0x0000, 0x0000, 0x0119, 0x0000, 0x0000, 0x0000, 0x012f, 0x0000,
     0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
     0x0000, 0x0173, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
    {// caron
     0x0000, 0x0000, 0x010c, 0x010e, 0x011a, 0x0000, 0x0000, 0x0000, 0x0000,
     0x0000, 0x0000, 0x013d, 0x0000, 0x0147, 0x0000, 0x0000, 0x0000, 0x0158,
     0x0160, 0x0164, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x017d, 0x0000,
     0x0000, 0x010d, 0x010f, 0x011b, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
     0x0000, 0x013e, 0x0000, 0x0148, 0x0000, 0x0000, 0x0000, 0x0159, 0x0161,
     0x0165, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x017e}};
}

//----------------------------------------------------------------------------
// Constructor.
//----------------------------------------------------------------------------

DvbTeletextCharset::DvbTeletextCharset()
    : _current(0x00), _g0m29(UNDEFINED), _g0x28(UNDEFINED), _g0Default(LATIN) {
  ::memcpy(&_G0, G0Base, sizeof(_G0));
}

//-----------------------------------------------------------------------------
// Check parity and translate any reasonable Teletext character into UCS-2.
//-----------------------------------------------------------------------------

char16_t DvbTeletextCharset::teletextToUcs2(uint8_t c) const {
  if (PARITY_8[c] == 0) {
    // Unrecoverable parity data error.
    return 0x20;
  } else {
    const char16_t r = c & 0x7F;
    if (r < 0x20) {
      return r;
    } else if (_g0Default == LATIN) {
      return _G0[LATIN][r - 0x20];
    } else {
      return G0Base[_g0Default][r - 0x20];
    }
  }
}

//-----------------------------------------------------------------------------
// Translate a G2 character into UCS-2.
//-----------------------------------------------------------------------------

char16_t DvbTeletextCharset::g2ToUcs2(uint8_t c) const {
  return c >= 0x20 && c < 0x20 + CHAR_COUNT ? G2[0][c - 0x20] : 0x00;
}

char16_t DvbTeletextCharset::g2AccentToUcs2(uint8_t c, uint8_t accent) const {
  if (c >= 65 && c <= 90 && accent < ACCENT_MODE_COUNT) {  // A - Z
    return G2_ACCENTS[accent][c - 65];
  } else if (c >= 97 && c <= 122 && accent < ACCENT_MODE_COUNT) {  // a - z
    return G2_ACCENTS[accent][c - 71];
  } else {  // other
    return teletextToUcs2(c);
  }
}

//-----------------------------------------------------------------------------
// Change character sets.
//-----------------------------------------------------------------------------

void DvbTeletextCharset::setG0Charset(uint32_t triplet) {
  // ETS 300 706, Table 32
  if ((triplet & 0x3c00) != 0x1000) {
    _g0Default = LATIN;
  } else if ((triplet & 0x0380) == 0x0000) {
    _g0Default = CYRILLIC1;
  } else if ((triplet & 0x0380) == 0x0200) {
    _g0Default = CYRILLIC2;
  } else if ((triplet & 0x0380) == 0x0280) {
    _g0Default = CYRILLIC3;
  } else {
    _g0Default = LATIN;
  }
}

void DvbTeletextCharset::setX28(uint8_t charset) {
  if (_g0Default == LATIN) {
    remapG0(_g0x28 = charset);
  }
}

void DvbTeletextCharset::setM29(uint8_t charset) {
  if (_g0Default == LATIN) {
    _g0m29 = charset;
    if (_g0x28 == UNDEFINED) {
      remapG0(_g0m29);
    }
  }
}

void DvbTeletextCharset::resetX28(uint8_t fallback) {
  if (_g0Default == LATIN) {
    _g0x28 = UNDEFINED;
    remapG0(_g0m29 != UNDEFINED ? _g0m29 : fallback);
  }
}

//-----------------------------------------------------------------------------
// Remap the GO character set.
//-----------------------------------------------------------------------------

void DvbTeletextCharset::remapG0(uint8_t charset) {
  if (charset != _current && charset < sizeof(G0_LATIN_NATIONAL_SUBSETS_MAP)) {
    const uint8_t m = G0_LATIN_NATIONAL_SUBSETS_MAP[charset];
    if (m != 0xff) {
      for (uint8_t j = 0; j < 13; j++) {
        _G0[LATIN][G0_LATIN_NATIONAL_SUBSETS_POSITIONS[j]] =
            G0_LATIN_NATIONAL_SUBSETS[m][j];
      }
      _current = charset;
    }
  }
}
}
}