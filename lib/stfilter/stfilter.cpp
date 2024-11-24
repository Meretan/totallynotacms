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




#include "stfilter.hpp"


void StreamFilter::AddToEnd(StreamFilter *p)
{
    if(dest)
        dest->AddToEnd(p);
    else
        dest = p;
}

void StreamFilter::InsertAfter(StreamFilter *p)
{
    p->dest = dest;
    dest = p;
}

void StreamFilter::DeleteChain()
{
    if(dest)
        dest->DeleteChain();
    delete this;
}

void StreamFilter::ChainReset()
{
    Reset();
    if(dest)
        dest->ChainReset();
}

void StreamFilter::PutStr(const char *s)
{
    const char *p = s;
    while(*p) {
        PutChar(*p);
        p++;
    }
}

void StreamFilter::PutHex(unsigned long long n, int minsigns, bool uppercase)
{
    unsigned long long k = n;
    int signs = 0;
    while(k) {
        signs++;
        k >>= 4;
    }
    if(minsigns > 0 && signs < minsigns)
        signs = minsigns;
    int i;
    for(i = signs-1; i >= 0; i--) {
        int dig = (n >> (4*i)) & 0x0f;
        PutChar(dig + (dig < 10 ? '0' : (uppercase ? 'A' : 'a') - 10));
    }
}


StreamFilterDestination::StreamFilterDestination()
    : StreamFilter(0), buf(0), buflen(0), buf_used(0)
{
}

StreamFilterDestination::~StreamFilterDestination()
{
    if(buf)
        delete[] buf;
}

char *StreamFilterDestination::ReleaseBuffer()
{
    char *ret = buf;
    buf = 0;
    buflen = 0;
    buf_used = 0;
    return ret;
}

void StreamFilterDestination::FeedChar(int c)
{
    if(buflen - buf_used < 2)
        More();
    buf[buf_used-1] = c;
    buf[buf_used] = 0;
    buf_used++;
}

void StreamFilterDestination::Reset()
{
    buf_used = 0;
}

void StreamFilterDestination::More()
{
    if(!buf) {
        buflen = start_bufsize;
        buf = new char[start_bufsize];
        *buf = 0;
        buf_used = 0;
        return;
    }
    int newlen = bufsize_step>0 ? buflen + bufsize_step : 2*buflen;
    char *nbuf = new char[newlen];
    int i;
    for(i = 0; i <= buf_used; i++)   /* note <= because of term. '\0' */
        nbuf[i] = buf[i];
    delete[] buf;
    buf = nbuf;
    buflen = newlen;
}
