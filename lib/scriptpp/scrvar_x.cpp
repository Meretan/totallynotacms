// +-------------------------------------------------------------------------+
// |                     Script Plus Plus vers. 0.3.70                       |
// | Copyright (c) Andrey V. Stolyarov  <croco at croco dot net>  2003--2023 |
// | ----------------------------------------------------------------------- |
// | This is free software.  Permission is granted to everyone to use, copy  |
// |        or modify this software under the terms and conditions of        |
// |                 GNU LESSER GENERAL PUBLIC LICENSE, v. 2.1               |
// |     as published by Free Software Foundation (see the file LGPL.txt)    |
// |                                                                         |
// | Please visit http://www.croco.net/software/scriptpp to get a fresh copy |
// | ----------------------------------------------------------------------- |
// |   This code is provided strictly and exclusively on the "AS IS" basis.  |
// | !!! THERE IS NO WARRANTY OF ANY KIND, NEITHER EXPRESSED NOR IMPLIED !!! |
// +-------------------------------------------------------------------------+




#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <strings.h>

#include "scrvar.hpp"

ScriptVariable::ScriptVariable(int len, const char *format, ...)
    : p(0)
{
    if(len < 1)
        len = 16;
    /* the following code is shamelessly stolen from man (3) vsnprintf */
    for(;;) {
        Create(len);
         /* Try to print in the allocated space. */
        va_list ap;
        va_start(ap, format);
        int n = vsnprintf(p->buf, len+1, format, ap);
        va_end(ap);
        /* If that worked, we're done */
        if (n > -1 && n <= len) {
            p->len_cached = n;
            break;
        }
        /* Else try again with more space. */
        if (n > -1)    /* glibc 2.1 */
            len = n+1; /* precisely what is needed */
        else           /* glibc 2.0 */
            len *= 2;  /* twice the old size */
    }
}

const ScriptVariable& ScriptVariable::ToupperLC()
{
    if(!p)
        return *this;
    EnsureOwnCopy();
    int len = Length();
    for(int i=0; i<len; i++)
        p->buf[i] = toupper(p->buf[i]);
    return *this;
}

const ScriptVariable& ScriptVariable::TolowerLC()
{
    if(!p)
        return *this;
    EnsureOwnCopy();
    int len = Length();
    for(int i=0; i<len; i++)
        p->buf[i] = tolower(p->buf[i]);
    return *this;
}

int ScriptVariable::Strcasecmp(const ScriptVariable &o2) const
{
    if(!p || !o2.p) {  // well, let's think Invalid is lesser than everything
        if(!p && !o2.p)
            return 0;   // both are invalid
        if(!p)
            return -1;  // the first is invalid, the second is not
        return 1;       // the first is valid
    }
    return strcasecmp(p->buf, o2.p->buf);
}


ScriptNumber::ScriptNumber(float f)
    : ScriptVariable(0, "%g", f) {}
ScriptNumber::ScriptNumber(double f)
    : ScriptVariable(0, "%g", f) {}
ScriptNumber::ScriptNumber(long double f)
    : ScriptVariable(0, "%Lg", f) {}
ScriptNumber::ScriptNumber(float f, int pre)
    : ScriptVariable(0, "%.*f", pre, f) {}
ScriptNumber::ScriptNumber(double f, int pre)
    : ScriptVariable(0, "%.*f", pre, f) {}
ScriptNumber::ScriptNumber(long double f, int pre)
    : ScriptVariable(0, "%.*Lf", pre, f) {}
