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




#include "stfbs64.hpp"


static int b64_digit(int value)
{
    value &= 0x3f;
    if(value < 26)
        return value + 'A';
    if(value < 52)
        return value + ('a' - 26);
    if(value < 62)
        return value - (52 - '0');
    return value == 62 ? '+' : '/';
}


void StreamFilterBase64encode::FeedChar(int c)
{
    src[(int)count] = c;
    if(count < 2)
        count++;
    else {
        PutChar(b64_digit((src[0] >> 2) & 0x3f));
        PutChar(b64_digit(((src[0] << 4) & 0x30) | ((src[1] >> 4) & 0x0f)));
        PutChar(b64_digit(((src[1] << 2) & 0x3c) | ((src[2] >> 6) & 0x03)));
        PutChar(b64_digit(src[2] & 0x3f));
        count = 0;
    }
}

void StreamFilterBase64encode::FeedEnd()
{
    if(count == 0)
        return;
    // now count may be either 1 or 2
    if(count == 1)
        src[1] = 0;
    src[2] = 0;
    PutChar(b64_digit((src[0] >> 2) & 0x3f));
    PutChar(b64_digit(((src[0] << 4) & 0x30) | ((src[1] >> 4) & 0x0f)));
    if(count == 1)
        PutChar('=');
    else
        PutChar(b64_digit((src[1] << 2) & 0x3c));
    PutChar('=');
}



#ifdef BS64TEST

#include <stdio.h>

class StreamFilterOutput : public StreamFilter {
public:
    StreamFilterOutput() : StreamFilter(0) {}
    virtual void FeedChar(int c) { putchar(c); }
};

int main()
{
    StreamFilterOutput out;
    StreamFilterBase64encode enc(&out);
    int c;
    while((c = getchar()) != EOF) {
        enc.FeedChar(c);
    }
    enc.FeedEnd();
    putchar('\n');
    return 0;
}

#endif
