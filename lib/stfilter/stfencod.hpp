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




#ifndef STFENCOD_HPP_SENTRY
#define STFENCOD_HPP_SENTRY

#include "stfilter.hpp"

class StreamFilterExtAsciiToUtf8 : public StreamFilter {
    const int *the_table;
public:
    StreamFilterExtAsciiToUtf8(const int *table, StreamFilter *next)
        : StreamFilter(next), the_table(table) {}
    void FeedChar(int c);

    static const int *FindTable(const char *encname);
    static const int *GetTable(int encoding);
};

class StreamFilterUtf8ToExtAscii : public StreamFilter {
    const int * const *the_table;
    int current_code, current_err;
    char expected, received;
public:
    StreamFilterUtf8ToExtAscii(const int * const *table, StreamFilter *next)
        : StreamFilter(next), the_table(table), expected(0), received(0) {}
    void FeedChar(int c);
    void FeedEnd();
    void Reset();

    virtual void DecodingError(int err);
    virtual void UnknownCode(int code);

private:
    void HandleMultibyte(int code);

public:
    static const int * const *FindTable(const char *encname);
    static const int * const *GetTable(int encoding);
};

class StreamFilterKoi8rToUtf8 : public StreamFilterExtAsciiToUtf8 {
public:
    StreamFilterKoi8rToUtf8(StreamFilter *next);
};


// if this enum ever changes, the constant char_encoding_names
// in the .cpp file must be changed accordingly, in sync
enum streamfilter_char_encodings {
    streamfilter_enc_unknown = -1,

    streamfilter_enc_utf8 = 0,
    streamfilter_enc_ascii,
    streamfilter_enc_koi8r,
    streamfilter_enc_cp1251,

    streamfilter_enc_lastknown = streamfilter_enc_cp1251
};

int streamfilter_find_encoding(const char *enc);
const char *streamfilter_encoding_canonical_name(const char *enc);
int streamfilter_encoding_name_count();
const char *streamfilter_encoding_name_by_index(int index);


#endif
