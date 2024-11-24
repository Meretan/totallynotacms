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




#ifndef STFBS64_HPP_SENTRY
#define STFBS64_HPP_SENTRY

#include "stfilter.hpp"

class StreamFilterBase64encode : public StreamFilter {
    char count;    // 0..2
    char src[3];
public:
    StreamFilterBase64encode(StreamFilter *next)
        : StreamFilter(next), count(0) {}
    void FeedChar(int c);
    void FeedEnd();
    void Reset() { count = 0; }
};

#endif
