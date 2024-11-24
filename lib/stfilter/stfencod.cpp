// +-------------------------------------------------------------------------+
// |                   StreamFilters library vers. 0.1.01                    |
// |  Copyright (c) Andrey V. Stolyarov <croco at croco dot net> 2022, 2023  |
// | ----------------------------------------------------------------------- |
// | This is free software.  Permission is granted to everyone to use, copy  |
// |        or modify this software under the terms and conditions of        |
// |                 GNU LESSER GENERAL PUBLIC LICENSE, v. 2.1               |
// |     as published by Free Software Foundation (see the file LGPL.txt)    |
// |                                                                         |
// | Please visit http://www.croco.net/software/stfilter to get a fresh copy |
// | ----------------------------------------------------------------------- |
// |   This code is provided strictly and exclusively on the "AS IS" basis.  |
// | !!! THERE IS NO WARRANTY OF ANY KIND, NEITHER EXPRESSED NOR IMPLIED !!! |
// +-------------------------------------------------------------------------+




#include "stfencod.hpp"



void StreamFilterExtAsciiToUtf8::FeedChar(int c)
{
    int c2;
    if(c < 0x80) {   /* ASCII symbol goes unchanged */
        PutChar(c);
        return;
    }
    c2 = the_table[c - 0x80];
#if 0
        /* it is assumed here that the table doesn't contain
           codes less than 0x80 (ascii codes) which is true
           for all ExtAscii encodings known to the author;
           if, however, you decide to apply this object to
           a table that contains at least a single code of
           the ASCII range, then replace #if 0 with #if 1
         */
    if(c2 < 0x80) {   /* ASCII symbol goes unchanged */
        PutChar(c2);
        return;
    }
#endif
    if(c2 < 0x800) {  /* 110xxxxx 10xxxxxx */
        PutChar(0xC0 | ((c2 >> 6) & 0x1F));
        PutChar(0x80 | (c2 & 0x3F));
        return;
    }
    if(c2 < 0x10000) {  /* 1110xxxx 10xxxxxx 10xxxxxx */
        PutChar(0xE0 | ((c2 >> 12) & 0x0F));
        PutChar(0x80 | ((c2 >> 6) & 0x3F));
        PutChar(0x80 | (c2 & 0x3F));
        return;
    }
    /* well, here perhaps should be a check for (c2 < 0x11000) which is
       the UTF8 upper limit, but we just drop the "extra" bits; finally,
       these codes come from the table supplied by the application, not
       retrieved from the external world, so we leave it to the programmer
       to make sure the codes are correct.
     */
        /* 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
    PutChar(0xF0 | ((c2 >> 18) & 0x03));
    PutChar(0x80 | ((c2 >> 12) & 0x3F));
    PutChar(0x80 | ((c2 >> 6) & 0x3F));
    PutChar(0x80 | (c2 & 0x3F));
}


void StreamFilterUtf8ToExtAscii::FeedChar(int c)
{
        // first of all, perform the self-synchronizing error check
    if(expected != 0 && (c & 0xC0) != 0x80) {
        // unexpected byte that (perhaps) starts another symbol
        // we report error, reset the state and let the byte go
        DecodingError(current_err);
        expected = 0;
    }

    if(expected == 0) {
        current_err = c & 0xFF;
        received = 0;
        if((c & 0x80) == 0) {     // single-byte (ASCII)
            PutChar(c);
        } else
        if((c & 0xE0) == 0xC0) {  // two-byte
            current_code = c & 0x1F;
            expected = 1;
        } else
        if((c & 0xF0) == 0xE0) {  // three-byte
            current_code = c & 0x0F;
            expected = 2;
        } else
        if((c & 0xF8) == 0xF0) {  // four-byte
            current_code = c & 0x07;
            expected = 3;
        } else {
            DecodingError(current_err);
        }
    } else {
        // okay, we're sure it's a continuation byte, because the case
        // of expected > 0 and not a 10xxxxxx byte is handled already
        static const int min_lims[3] = { 0x80, 0x800, 0x10000 };

        current_err = (current_err << 8) | (c & 0xFF);
        current_code = (current_code << 6) | (c & 0x3F);
        received++;
        if(received >= expected) {
            if(current_code<min_lims[expected-1] || current_code>0x10FFFF)
                DecodingError(current_err);
            else
                HandleMultibyte(current_code);
            expected = 0;
        }
    }
}

void StreamFilterUtf8ToExtAscii::FeedEnd()
{
    if(expected != 0) {
        DecodingError(current_err);
        expected = 0;
    }
}

void StreamFilterUtf8ToExtAscii::Reset()
{
    expected = 0;
    received = 0;
}

    // by default, we output smth. like ``#err:C09F ''
void StreamFilterUtf8ToExtAscii::DecodingError(int err)
{
    PutStr("#err:");
    PutHex(err, 0, true);
    PutChar(' ');
}

    // by default, we output smth. like ``#[AF23]''
void StreamFilterUtf8ToExtAscii::UnknownCode(int code)
{
    PutStr("#[");
    PutHex(code, 5, true);
    PutChar(']');
}

void StreamFilterUtf8ToExtAscii::HandleMultibyte(int code)
{
    int rescode = -1;

    // first, try to find it in the table
    // remember, each ``line'' of the table starts with
    //   the first codepoint and the length
    const int * const *p;
    for(p = the_table; *p; p++) {
        if(code >= (*p)[0] && code < (*p)[0] + (*p)[1]) {  // found?
            rescode = (*p)[code - (*p)[0] + 2];
            break;
        }
    }

    if(rescode == -1)    // remains unknown
        UnknownCode(code);
    else
        PutChar(rescode);
}

/////////////////////////////////////////////////////////////////////
// boring tables follow

// koi8r ////////////////////////////////////////////////////////////

static const int koi8r_to_unicode[128] = {   /* offset 0x80 */
    0x2500, 0x2502, 0x250C, 0x2510, 0x2514, 0x2518, 0x251C, 0x2524,
    0x252C, 0x2534, 0x253C, 0x2580, 0x2584, 0x2588, 0x258C, 0x2590,
    0x2591, 0x2592, 0x2593, 0x2320, 0x25A0, 0x2219, 0x221A, 0x2248,
    0x2264, 0x2265, 0x00A0, 0x2321, 0x00B0, 0x00B2, 0x00B7, 0x00F7,
    0x2550, 0x2551, 0x2552, 0x0451, 0x2553, 0x2554, 0x2555, 0x2556,
    0x2557, 0x2558, 0x2559, 0x255A, 0x255B, 0x255C, 0x255D, 0x255E,
    0x255F, 0x2560, 0x2561, 0x0401, 0x2562, 0x2563, 0x2564, 0x2565,
    0x2566, 0x2567, 0x2568, 0x2569, 0x256A, 0x256B, 0x256C, 0x00A9,
    0x044E, 0x0430, 0x0431, 0x0446, 0x0434, 0x0435, 0x0444, 0x0433,
    0x0445, 0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E,
    0x043F, 0x044F, 0x0440, 0x0441, 0x0442, 0x0443, 0x0436, 0x0432,
    0x044C, 0x044B, 0x0437, 0x0448, 0x044D, 0x0449, 0x0447, 0x044A,
    0x042E, 0x0410, 0x0411, 0x0426, 0x0414, 0x0415, 0x0424, 0x0413,
    0x0425, 0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E,
    0x041F, 0x042F, 0x0420, 0x0421, 0x0422, 0x0423, 0x0416, 0x0412,
    0x042C, 0x042B, 0x0417, 0x0428, 0x042D, 0x0429, 0x0427, 0x042A
};

static const int unicode_to_koi8r_401[83] = { 0x00401,  81,
    0xb3,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1, 0xe1,
    0xe2, 0xf7, 0xe7, 0xe4, 0xe5, 0xf6, 0xfa, 0xe9,
    0xea, 0xeb, 0xec, 0xed, 0xee, 0xef, 0xf0, 0xf2,
    0xf3, 0xf4, 0xf5, 0xe6, 0xe8, 0xe3, 0xfe, 0xfb,
    0xfd, 0xff, 0xf9, 0xf8, 0xfc, 0xe0, 0xf1, 0xc1,
    0xc2, 0xd7, 0xc7, 0xc4, 0xc5, 0xd6, 0xda, 0xc9,
    0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 0xd0, 0xd2,
    0xd3, 0xd4, 0xd5, 0xc6, 0xc8, 0xc3, 0xde, 0xdb,
    0xdd, 0xdf, 0xd9, 0xd8, 0xdc, 0xc0, 0xd1,   -1,
    0xa3
};
static const int unicode_to_koi8r_2500[163] = { 0x02500, 161,
    0x80,   -1, 0x81,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1, 0x82,   -1,   -1,   -1,
    0x83,   -1,   -1,   -1, 0x84,   -1,   -1,   -1,
    0x85,   -1,   -1,   -1, 0x86,   -1,   -1,   -1,
      -1,   -1,   -1,   -1, 0x87,   -1,   -1,   -1,
      -1,   -1,   -1,   -1, 0x88,   -1,   -1,   -1,
      -1,   -1,   -1,   -1, 0x89,   -1,   -1,   -1,
      -1,   -1,   -1,   -1, 0x8a,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
    0xa0, 0xa1, 0xa2, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8,
    0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0,
    0xb1, 0xb2, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9,
    0xba, 0xbb, 0xbc, 0xbd, 0xbe,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
    0x8b,   -1,   -1,   -1, 0x8c,   -1,   -1,   -1,
    0x8d,   -1,   -1,   -1, 0x8e,   -1,   -1,   -1,
    0x8f, 0x90, 0x91, 0x92,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
    0x94
};
static const int unicode_to_koi8r_2219[267] = { 0x02219, 265,
    0x95, 0x96,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1, 0x97,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1, 0x98, 0x99,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1, 0x93,
    0x9b
};
static const int unicode_to_koi8r_a0[90] = { 0x000a0,  88,
    0x9a,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1, 0xbf,   -1,   -1,   -1,   -1,   -1,   -1,
    0x9c,   -1, 0x9d,   -1,   -1,   -1,   -1, 0x9e,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1, 0x9f
};
static const int * const unicode_to_koi8r[] = {
    unicode_to_koi8r_401,
    unicode_to_koi8r_2500,
    unicode_to_koi8r_2219,
    unicode_to_koi8r_a0,
    0
};


// cp1251 ////////////////////////////////////////////////////////////

static const int cp1251_to_unicode[128] = {   /* offset 0x80 */
        /* NOTE undefined position at 0x98 filled with 0xFFFD */
    0x0402, 0x0403, 0x201A, 0x0453, 0x201E, 0x2026, 0x2020, 0x2021,
    0x20AC, 0x2030, 0x0409, 0x2039, 0x040A, 0x040C, 0x040B, 0x040F,
    0x0452, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
    0xFFFD, 0x2122, 0x0459, 0x203A, 0x045A, 0x045C, 0x045B, 0x045F,
    0x00A0, 0x040E, 0x045E, 0x0408, 0x00A4, 0x0490, 0x00A6, 0x00A7,
    0x0401, 0x00A9, 0x0404, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x0407,
    0x00B0, 0x00B1, 0x0406, 0x0456, 0x0491, 0x00B5, 0x00B6, 0x00B7,
    0x0451, 0x2116, 0x0454, 0x00BB, 0x0458, 0x0405, 0x0455, 0x0457,
    0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417,
    0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F,
    0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427,
    0x0428, 0x0429, 0x042A, 0x042B, 0x042C, 0x042D, 0x042E, 0x042F,
    0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437,
    0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E, 0x043F,
    0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447,
    0x0448, 0x0449, 0x044A, 0x044B, 0x044C, 0x044D, 0x044E, 0x044F
};
static const int unicode_to_cp1251_401[147] = { 0x00401, 145,
    0xa8, 0x80, 0x81, 0xaa, 0xbd, 0xb2, 0xaf, 0xa3,
    0x8a, 0x8c, 0x8e, 0x8d,   -1, 0xa1, 0x8f, 0xc0,
    0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8,
    0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 0xd0,
    0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8,
    0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, 0xe0,
    0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8,
    0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef, 0xf0,
    0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
    0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff,   -1,
    0xb8, 0x90, 0x83, 0xba, 0xbe, 0xb3, 0xbf, 0xbc,
    0x9a, 0x9c, 0x9e, 0x9d,   -1, 0xa2, 0x9f,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1, 0xa5,
    0xb4
};
static const int unicode_to_cp1251_a0[30] = { 0x000a0,  28,
    0xa0,   -1,   -1,   -1, 0xa4,   -1, 0xa6, 0xa7,
      -1, 0xa9,   -1, 0xab, 0xac, 0xad, 0xae,   -1,
    0xb0, 0xb1,   -1,   -1,   -1, 0xb5, 0xb6, 0xb7,
      -1,   -1,   -1, 0xbb
};
static const int unicode_to_cp1251_2013[274] = { 0x02013, 272,
    0x96, 0x97,   -1,   -1,   -1, 0x91, 0x92, 0x82,
      -1, 0x93, 0x94, 0x84,   -1, 0x86, 0x87, 0x95,
      -1,   -1,   -1, 0x85,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1, 0x89,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1, 0x8b, 0x9b,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1, 0x88,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
      -1,   -1,   -1, 0xb9,   -1,   -1,   -1,   -1,
      -1,   -1,   -1,   -1,   -1,   -1,   -1, 0x99
};
const int * const unicode_to_cp1251[] = {
    unicode_to_cp1251_401,
    unicode_to_cp1251_a0,
    unicode_to_cp1251_2013,
    0
};


// ascii ////////////////////////////////////////////////////////////

static const int * const unicode_to_ascii[] = {
    0
};


// encoding directory ///////////////////////////////////////////////

#if 0
struct encoding_info {
    const char *name;
    const int *to_unicode;
    const int * const *from_unicode;
};

static const encoding_info encoding_info_base[] = {
    { "koi8-r", koi8r_to_unicode,  unicode_to_koi8r },
    { "koi8r",  koi8r_to_unicode,  unicode_to_koi8r },
    { "cp1251", cp1251_to_unicode, unicode_to_cp1251 },

    { "koi8",   koi8r_to_unicode,  unicode_to_koi8r },
    { "koi-8",  koi8r_to_unicode,  unicode_to_koi8r },
    { "1251",   cp1251_to_unicode, unicode_to_cp1251 },
    { "windows-1251",  cp1251_to_unicode, unicode_to_cp1251 },

    { "ascii",  0, unicode_to_ascii },
    { "us-ascii",  0, unicode_to_ascii },

    { 0, 0, 0 }
};
#endif

    // we don't use strcasecmp in order to avoid linking locale support
static bool string_eq_nc(const char *freestr, const char *lowcasestr)
{
    int i;
    for(i = 0; ; i++) {
        if(!freestr[i] && !lowcasestr[i])    // both empty
            return true;
        if(!freestr[i] || !lowcasestr[i])    // one empty, not both
            return false;
        if(freestr[i] == lowcasestr[i])
            continue;
        if(freestr[i] >= 'A' && freestr[i] <= 'Z' &&
            freestr[i]-'A'+'a' == lowcasestr[i])
        {
            continue;
        }
        return false;
    }
    // we'll never get here
}

#if 0
static const encoding_info *find_encinfo(const char *name)
{
    const encoding_info *p;
    for(p = encoding_info_base; p->name; p++)
        if(string_eq_nc(name, p->name))
            return p;
    return 0;
}

const int *StreamFilterExtAsciiToUtf8::FindTable(const char *name)
{
    const encoding_info *p = find_encinfo(name);
    return p ? p->to_unicode : 0;
}

const int * const *StreamFilterUtf8ToExtAscii::FindTable(const char *name)
{
    const encoding_info *p = find_encinfo(name);
    return p ? p->from_unicode : 0;
}
#endif

const int *StreamFilterExtAsciiToUtf8::FindTable(const char *name)
{
    int n = streamfilter_find_encoding(name);
    return GetTable(n);
}
const int * const *StreamFilterUtf8ToExtAscii::FindTable(const char *name)
{
    int n = streamfilter_find_encoding(name);
    return GetTable(n);
}


const int *StreamFilterExtAsciiToUtf8::GetTable(int enc)
{
    switch(enc) {
    case streamfilter_enc_ascii:
        return 0;
    case streamfilter_enc_koi8r:
        return koi8r_to_unicode;
    case streamfilter_enc_cp1251:
        return cp1251_to_unicode;
    default:
        return 0;
    }
}

const int * const *StreamFilterUtf8ToExtAscii::GetTable(int enc)
{
    switch(enc) {
    case streamfilter_enc_ascii:
        return unicode_to_ascii;
    case streamfilter_enc_koi8r:
        return unicode_to_koi8r;
    case streamfilter_enc_cp1251:
        return unicode_to_cp1251;
    default:
        return 0;
    }
}



StreamFilterKoi8rToUtf8::StreamFilterKoi8rToUtf8(StreamFilter *next)
    : StreamFilterExtAsciiToUtf8(koi8r_to_unicode, next)
{}


struct encentry {
    const char *name;
    int code;
};
static const struct encentry encs[] = {
    { "utf8", streamfilter_enc_utf8 },
    { "ascii", streamfilter_enc_ascii },
    { "koi8-r", streamfilter_enc_koi8r },
    { "cp1251", streamfilter_enc_cp1251 },

    { "utf-8", streamfilter_enc_utf8 },

    { "us-ascii", streamfilter_enc_ascii },

    { "koi8r", streamfilter_enc_koi8r },
    { "koi8", streamfilter_enc_koi8r },

    { "1251", streamfilter_enc_cp1251 },
    { "win1251", streamfilter_enc_cp1251 },
    { "win-1251", streamfilter_enc_cp1251 },
    { "windows-1251", streamfilter_enc_cp1251 },

    { 0, 0 }
};

int streamfilter_find_encoding(const char *enc)
{
    int i;
    for(i = 0; encs[i].name; i++)
        if(string_eq_nc(enc, encs[i].name))
            return encs[i].code;

    return streamfilter_enc_unknown;
}

int streamfilter_encoding_name_count()
{
    return sizeof(encs) / sizeof(*encs) - 1;
}

const char *streamfilter_encoding_name_by_index(int index)
{
    return encs[index].name;
}

// ! must be in sync with enum streamfilter_char_encodings, starting with 0
static const char * const char_encoding_names[streamfilter_enc_lastknown+1] = {
    "utf8", "ascii", "koi8-r", "cp1251"
};

const char *streamfilter_encoding_canonical_name(const char *enc)
{
    int c = streamfilter_find_encoding(enc);
    if(c < 0 || c > streamfilter_enc_lastknown)
        return 0;
    return char_encoding_names[c];
}
