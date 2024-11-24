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




#include <string.h>
#include <stdlib.h>
#include <stddef.h>

#include "scrvar.hpp"


enum { size_of_memblock_header = sizeof(int) * 2 };


static ScriptVariableImplementation TheEmptyString = { 1, 0, 0, "" };


ScriptVariable::ScriptVariable()
    : p(0)
{
    Assign(&TheEmptyString);
}

#if 0
ScriptVariable::ScriptVariable(int len)
    : p(0)
{
    if(len != -1) {
        Create(len);
        p->buf[0] = 0;
        p->len_cached = 0;
    }
}
#endif

ScriptVariable::ScriptVariable(const char *s, int len)
    : p(0)
{
    if(s && len != -1) {
        Create(len);
        memcpy(p->buf, s, len);
        // p->buf[len] = 0; // no need for this, done by Create()
        p->len_cached = len;
    }
}

ScriptVariable::ScriptVariable(const char *str)
    : p(0)
{
    if(str) {
        int len = strlen(str);
        Create(len);
        memcpy(p->buf, str, len);
        p->len_cached = len;
    }
}

ScriptVariable::ScriptVariable(const ScriptVariable& other)
    : p(0)
{
    Assign(other.p);
}

ScriptVariable::~ScriptVariable()
{
    Unlink();
}

void ScriptVariable::MakeRoom(int len)
{
    if(!p) {
        Create(len);
        return;
    }
    if(p->maxlen >= len)
        return;
    ScriptVariableInv res;
    res.Create(len);
    memcpy(res.p->buf, p->buf, len+1);
    Assign(res.p);
}

void ScriptVariable::Unlink()
{
    if(p) {
        if(--(p->refcount)<=0)
            free(p);
        p = 0;
    }
}

void ScriptVariable::Assign(ScriptVariableImplementation *q)
{
    Unlink();
    p = q;
    if(p)
        p->refcount++;
}

void ScriptVariable::Create(int len)
{
    Unlink();
    int efflen = 16;
    int hdrsize = offsetof(ScriptVariableImplementation, buf);
    int minsize = hdrsize + len + 1;
    while(efflen < minsize)
        efflen *= 2;
    p = reinterpret_cast<ScriptVariableImplementation*>(malloc(efflen));
    p->refcount = 1;
    p->maxlen = efflen - hdrsize - 1;
    p->len_cached = -1;
    p->buf[len] = 0;
}

void ScriptVariable::EnsureOwnCopy()
{
    if(p && p->refcount > 1) {
        int len = Length();
        ScriptVariableImplementation *tmp = p;
        Create(len);
        memcpy(p->buf, tmp->buf, len);
        p->len_cached = len;
    }
}

int ScriptVariable::Length() const
{
    if(!p)
        return 0;
    if(p->len_cached == -1)
        p->len_cached =  strlen(p->buf);
    return p->len_cached;
}

const char *ScriptVariable::c_str() const
{
    return p ? p->buf : 0;
}

char ScriptVariable::operator[](int i) const
{
    return p ? p->buf[i] : 0;
}

char& ScriptVariable::operator[](int i)
{
    if(p)
        EnsureOwnCopy();
    else
        Create(i+1);
    p->len_cached = -1;
        // because our caller can truncate the string assigning zero
    return p->buf[i];
}

////////////////////////////////////////

ScriptVariable ScriptVariable::operator+(const char *o2) const
{
    if(!p)
        return *this;
    int len1 = Length();
    int len2 = strlen(o2);
    int newlen = len1 + len2;
    ScriptVariableInv res;
    res.Create(newlen);
    memcpy(res.p->buf, p->buf, len1);
    memcpy(res.p->buf+len1, o2, len2);
    res.p->buf[newlen] = 0;
    res.p->len_cached = newlen;
    return res;
}

ScriptVariable& ScriptVariable::operator+=(const char *o2)
{
    if(!p)
        return *this;
    int len1 = Length();
    int len2 = strlen(o2);
    int newlen = len1 + len2;
    if(p && p->refcount == 1 && p->maxlen >= newlen) {
        // in this special case we can just copy the second string in
        memcpy(p->buf+len1, o2, len2);
        p->buf[newlen] = 0;
        p->len_cached = newlen;
    } else {
        // we have to create another one, so let's just make it sum...
        ScriptVariable res(*this + o2);
        Assign(res.p);
    }
    return *this;
}

ScriptVariable& ScriptVariable::operator=(const char *o2)
{
    int len2 = strlen(o2);
    Create(len2);
    memcpy(p->buf, o2, len2 + 1);
    p->len_cached = len2;
    return *this;
}

///////////////////////////

ScriptVariable ScriptVariable::operator+(char c) const
{
    // if(!p) return *this; // not needed, the subseq. call will do
    char bb[2];
    bb[0]=c; bb[1]=0;
    return (*this)+bb;
}

ScriptVariable& ScriptVariable::operator+=(char c)
{
    // if(!p) return *this; // not needed, the subseq. call will do
    char bb[2];
    bb[0]=c; bb[1]=0;
    return (*this)+=bb;
}

ScriptVariable& ScriptVariable::operator=(char c)
{
    char bb[2];
    bb[0]=c; bb[1]=0;
    return (*this)=bb;
}

///////////////////////////////////////////

ScriptVariable ScriptVariable::operator+(const ScriptVariable &o2) const
{
    if(!o2.p)
        return o2; // it is invalid, return the invalid str...
    return (*this) + o2.p->buf;
}

ScriptVariable& ScriptVariable::operator+=(const ScriptVariable &o2)
{
    if(!o2.p)
        Invalidate(); // it was invalid, we're now invalid, too
    else
        (*this) += o2.p->buf;
    return *this;
}

ScriptVariable& ScriptVariable::operator=(const ScriptVariable &o2)
{
    Assign(o2.p);
    return *this;
}


int ScriptVariable::Strcmp(const ScriptVariable &o2) const
{
    if(!p || !o2.p) {  // well, let's think Invalid is lesser than everything
        if(!p && !o2.p)
            return 0;   // both are invalid
        if(!p)
            return -1;  // the first is invalid, the second is not
        return 1;       // the first is valid
    }
    return strcmp(p->buf, o2.p->buf);
}

bool ScriptVariable::HasPrefix(const char *pr) const
{
    if(!p)
        return false;
    for(const char *qq = p->buf; *pr; pr++, qq++)
        if(*pr!=*qq)
            return false;
    return true;
}

bool ScriptVariable::HasPrefix(const ScriptVariable& prefix) const
{
    return HasPrefix(prefix.c_str());
}

bool ScriptVariable::HasSuffix(const char *suf) const
{
    if(!p)
        return false;
    int suflen = strlen(suf);
    for(const char *qq = p->buf + Length() - suflen; *suf; suf++, qq++)
        if(*suf!=*qq)
            return false;
    return true;
}

bool ScriptVariable::HasSuffix(const ScriptVariable& suffix) const
{
    if(!p || !suffix.p)
        return false;
    int l = suffix.Length();
    return (*const_cast<ScriptVariable*>(this)).Range(-l, l).Get() == suffix;
}

ScriptVariable& ScriptVariable::Trim(const char *spaces)
{
    if(!p)
        return *this;
    (*this) = Whole().Trim(spaces).Get();
    return *this;
}

const ScriptVariable& ScriptVariable::Toupper()
{
    if(!p)
        return *this;
    EnsureOwnCopy();
    int len = Length();
    for(int i=0; i<len; i++) {
        char c = p->buf[i];
        if(c >= 'a' && c <= 'z')
            p->buf[i] -= ('a' - 'A');
    }
    return *this;
}

const ScriptVariable& ScriptVariable::Tolower()
{
    if(!p)
        return *this;
    EnsureOwnCopy();
    int len = Length();
    for(int i=0; i<len; i++) {
        char c = p->buf[i];
        if(c >= 'A' && c <= 'Z')
            p->buf[i] += ('a' - 'A');
    }
    return *this;
}

ScriptVariable::Substring::
Substring(ScriptVariable &a_master, int a_pos, int a_len)
{
    master = &a_master;
    if(master->IsValid()) {
        pos = a_pos>=0 ? a_pos : (master->Length() + a_pos);
        if(pos<0)
            pos = 0;
        len = a_len;
        if(len == -1)
            len = master->Length() - pos;
        if(pos+len > master->p->maxlen)
            len = master->p->maxlen - pos;
    }
}

void ScriptVariable::Substring::Erase()
{
    if(IsInvalid())
        return;
    master->EnsureOwnCopy();
    // Isn't the string really shorter than it is needed?
    for(int i = pos; i < pos+len; i++) {
        if(!master->p->buf[i]) { // it is; just truncate it
            master->p->buf[pos] = 0;
            master->p->len_cached = -1;
            len = 0;
            return;
        }
    }
    for(char *p = master->p->buf + pos; (*p = *(p+len)); p++)
        ;
    master->p->len_cached = -1;
    len = 0;
}

void ScriptVariable::Substring::Replace(const char *what)
{
    if(IsInvalid())
        return;
    master->EnsureOwnCopy();
    int mlen = master->Length();
    if(pos+len > mlen)
        len = mlen - pos;
    int whatlen = strlen(what);
    if(whatlen > len) { // first move the rest forward
        // do we have enough room for it?
        int delta = whatlen - len;
        if(mlen+delta <= master->p->maxlen) {
            // yes, we've got enough room
            memmove(master->p->buf + pos + whatlen,
                    master->p->buf + pos + len,
                    mlen - pos - len + 1);
            master->p->buf[mlen+delta] = 0; // sanity
            master->p->len_cached = -1;
        } else {
            // not enough room, don't bother moving
            ScriptVariableInv tmp;
            tmp.Create(mlen+delta);
            memmove(tmp.p->buf, master->p->buf, pos);
            memmove(tmp.p->buf+whatlen, master->p->buf+len,
                    mlen - pos - len + 1);
            tmp.p->len_cached = -1;
            (*master) = tmp;
        }
    } else
    if(whatlen < len) { // first move the rest backward
        memmove(master->p->buf + pos + whatlen,
                master->p->buf + pos + len,
                mlen - pos - len + 1);
        master->p->len_cached = -1;
    }
    // now just replace
    memcpy(master->p->buf + pos, what, whatlen);
    master->p->len_cached = -1;
    len = whatlen;
}

ScriptVariable ScriptVariable::Substring::Get() const
{
    if(IsInvalid())
        return ScriptVariableInv();
    int llen = len>=0 ? len : master->Length()-pos;
    if(llen<=0)
        return ScriptVariable("");
    ScriptVariableInv res;
    res.Create(llen);
    memcpy(res.p->buf, master->p->buf + pos, llen);
    res.p->buf[llen] = 0;
    res.p->len_cached = llen;
    return res;
}

ScriptVariable::Substring ScriptVariable::Substring::Before() const
{
    if(IsInvalid())
        return Substring(); // invalid
    return Substring(*master, 0, pos);
}

ScriptVariable::Substring ScriptVariable::Substring::After() const
{
    if(IsInvalid())
        return Substring(); // invalid
    return Substring(*master, pos + len, master->Length() - pos - len);
}

bool ScriptVariable::Substring::FetchWord(Substring &word, const char *spaces)
{
    if(IsInvalid())
        return false;
    char *p = master->p->buf + pos;
    char *q = p;
    while(*p && strchr(spaces, *p))
        p++;
    if(!*p)
        return false;

    word.master = master;
    word.pos = pos + (p-q); // skip spaces

    q = p;
    while(*p && !strchr(spaces, *p))
        p++;
    word.len = p-q;

    int fetchedlen = word.pos + word.len - pos;
    pos += fetchedlen;
    len -= fetchedlen;

    return true;
}

bool ScriptVariable::Substring::
FetchToken(Substring &token,
           const char *delimiters,
           const char *trim_spaces)
{
    if(IsInvalid())
        return false;
    if(len<0) // this means nothing left in the string
        return false;
              // else, at least one token exists
    char *p = master->p->buf + pos;
    token.master = master;
    token.pos = pos;

    char *q = p;

    while(*p && !strchr(delimiters, *p))
        p++;

    if(*p) {
        // delimiter found; there will be another token...
        token.len = (p-q);
        pos += p-q+1;
        len -= p-q+1;
    } else {
        // no delimiters; this will be the last and the only token
        token.len = len;
        pos += len;
        len = -1; // no more tokens!
    }

    if(trim_spaces && *trim_spaces)
        token.Trim(trim_spaces);

    return true;
}

ScriptVariable::Substring
ScriptVariable::Substring::FetchWord(const char *spaces)
{
    Substring ret;
    FetchWord(ret, spaces);
    return ret;
}

ScriptVariable::Substring ScriptVariable::Substring::
FetchToken(const char *delimiters, const char *trim_spaces)
{
    Substring ret;
    FetchToken(ret, delimiters, trim_spaces);
    return ret;
}

ScriptVariable::Substring& ScriptVariable::Substring::Move(int d)
{
    if(IsInvalid())
        return *this;
    pos+=d;
    if(pos < 0) pos = 0;
    if(pos >= master->p->maxlen)
        pos = master->p->maxlen-1;
    if(pos+len > master->p->maxlen)
        len = master->p->maxlen - pos;
    return *this;
}

ScriptVariable::Substring& ScriptVariable::Substring::Resize(int d)
{
    if(IsInvalid())
        return *this;
    len+=d;
    if(len<0)
        len = 0;
    else
    if(pos+len > master->p->maxlen)
        len = master->p->maxlen - pos;
    return *this;
}

ScriptVariable::Substring& ScriptVariable::Substring::ExtendToBegin()
{
    if(IsInvalid())
        return *this;
    len += pos;
    pos = 0;
    return *this;
}

ScriptVariable::Substring& ScriptVariable::Substring::ExtendToEnd()
{
    if(IsInvalid())
        return *this;
    len = master->Length() - pos;
    return *this;
}

ScriptVariable::Substring& ScriptVariable::Substring::SetLength(int nl)
{
    if(IsInvalid())
        return *this;
    int maxlen = master->Length() - pos;
        // for negative nl, we just make the substring as long as possible
    len = (nl >= 0 && nl < maxlen) ? nl : maxlen;
    return *this;
}

ScriptVariable::Substring&
ScriptVariable::Substring::Trim(const char *spaces)
{
    if(IsInvalid())
        return *this;
    char *p = master->p->buf + pos;
    char *q = p;
    while(*p && strchr(spaces, *p))
        p++;
    pos+=(p-q);
    len-=(p-q);
    q=p;
    p+=len-1;
    while(p>q && strchr(spaces, *p))
        p--;
    len = p-q+1;
    return *this;
}

ScriptVariable::Substring ScriptVariable::Strchr(int c)
{
    if(!p)
        return ScriptVariable::Substring(); // invalid
    const char *t = strchr(c_str(), c);
    if(!t)
        return ScriptVariable::Substring(); // invalid
    return ScriptVariable::Substring(*this, t-c_str(), 1);
}

ScriptVariable::Substring ScriptVariable::Strrchr(int c)
{
    if(!p)
        return ScriptVariable::Substring(); // invalid
    const char *t = strrchr(c_str(), c);
    if(!t)
        return ScriptVariable::Substring(); // invalid
    return ScriptVariable::Substring(*this, t-c_str(), 1);
}

ScriptVariable::Substring ScriptVariable::Strstr(const char *str)
{
    if(!p)
        return ScriptVariable::Substring(); // invalid
    const char *t = strstr(c_str(), str);
    if(!t)
        return ScriptVariable::Substring(); // invalid
    return ScriptVariable::Substring(*this, t-c_str(), strlen(str));
}

ScriptVariable::Substring ScriptVariable::Strrstr(const char *str)
{
    if(!p)
        return ScriptVariable::Substring(); // invalid
    const char *t = strstr(c_str(), str);
    if(!t)
        return ScriptVariable::Substring(); // invalid
    const char *p;
    while((p = strstr(t+1, str)))
        t = p;
    return ScriptVariable::Substring(*this, t-c_str(), strlen(str));
}






bool ScriptVariable::Iterator::NextWord(const char *spaces)
{
    if(!master->IsValid())
        return false;
    pos += len; // begin from the next position
    char *p = master->p->buf + pos;
    char *q = p;
    while(*p && strchr(spaces, *p))
        p++;
    if(!*p)
        return false;
    pos += p - q;
    q = p;
    while(*p && !strchr(spaces, *p))
        p++;
    len = p - q;
    just_started = false; // this is just to keep the things consistent
                    // NextWord doesn't really use the just_started variable
    return true;
}

bool ScriptVariable::Iterator::NextToken(
                  const char *delimiters,
                  const char *trim_spaces)
{
    if(!master->IsValid())
        return false;
    char *p, *q;
    if(!just_started) { // not the starting situation
              // we need first to find the delimiter that finishes the
              // current token, and only in this case
        pos += len; // begin from the next position
        p = master->p->buf + pos;
        q = p;
        while(*p && !strchr(delimiters, *p))
            p++;
        if(!*p)
            return false; // that was the last token
        p++;
        pos += p - q; // now pos is the next position after the delimiter
    } else {
        p = master->p->buf;  // begin from the beginning
        just_started = false;
    }
    q = p;
    while(*p && !strchr(delimiters, *p))
        p++;
    p--;
    // q points at the begin and p points at the end
    if(trim_spaces && *trim_spaces) {
        // need trimming
        while(q<=p && strchr(trim_spaces, *q))
            q++;
        if(q<=p) while(strchr(trim_spaces, *p))
            p--;
        // if everything gets trim away, then p is actually less than q
    }
    pos = q - master->p->buf;
    len = p - q + 1;
    //return (len + pos > 0); // to prevent infinite loop on, say, empty string
    return true;
}



bool ScriptVariable::Iterator::PrevWord(const char *spaces)
{
    if(!master->IsValid())
        return false;
    if(just_started)
        pos = master->Length();
    pos -= 1; // begin from the previous position
    char *limit = master->p->buf;
    char *p = limit + pos;
    while(p>=limit && strchr(spaces, *p))
        p--;
    if(p<limit)
        return false;
    char *q = p;
    while(p>=limit && !strchr(spaces, *p))
        p--;
    pos = p - limit + 1;
    len = q - p;
    just_started = false;
    return true;
}

bool ScriptVariable::Iterator::PrevToken(
                  const char *delimiters,
                  const char *trim_spaces)
{
    if(!master->IsValid())
        return false;
    char *p, *q;
    char *limit = master->p->buf;
    if(!just_started) { // not the starting situation
              // we need first to find the delimiter that starts the
              // current token, and only in this case
        pos -= 1; // begin from the previous position
        p = limit + pos;
        q = p;
        while(p>=limit && !strchr(delimiters, *p))
            p--;
        if(p<limit)
            return false; // that was the first token
        p--; // jump over the delimiter
        pos -= p - q; // now pos is the next position before the delimiter
    } else {
        p = limit + master->Length() - 1;  // begin from the end
        just_started = false;
    }
    q = p;
    while(p>=limit && !strchr(delimiters, *p))
        p--;
    p++;
    // q points at the end and p points at the begin
    if(trim_spaces && *trim_spaces) {
        // need trimming
        while(q>=p && strchr(trim_spaces, *q))
            q--;
        while(q>p && strchr(trim_spaces, *p))
            p++;
        // if everything gets trim away, then q is actually less than p
    }
    pos = p - limit;
    len = q - p + 1;
    return true;
}


bool ScriptVariable::GetLong(long &l, int radix) const
{
    if(!p)
        return false;
    long ret;
    char *err;
    char *q = p->buf;
    ret = strtol(q, &err, radix);
    if(*q != '\0' && *err == '\0') {
        l = ret;
        return true;
    } else {
        return false;
    }
}

bool ScriptVariable::GetLongLong(long long &l, int radix) const
{
    if(!p)
        return false;
    long long ret;
    char *err;
    char *q = p->buf;
    ret = strtoll(q, &err, radix);
    if(*q != '\0' && *err == '\0') {
        l = ret;
        return true;
    } else {
        return false;
    }
}

bool ScriptVariable::GetDouble(double &d) const
{
    if(!p)
        return false;
    double ret;
    char *err;
    char *q = p->buf;
    ret = strtod(q, &err);
    if(*q != '\0' && *err == '\0') {
        d = ret;
        return true;
    } else {
        return false;
    }
}

bool ScriptVariable::GetRational(long &n, long &m) const
{
    if(!p)
        return false;
    long ret;
    char *err;
    char *q = p->buf;
    ret = strtol(q, &err, 10);
    if(*q == '\0' || (*err != '\0' && *err != '.'))
        return false;
    if(*err=='\0' || *(err+1)=='\0') {
         // no decimal dot or nothing right after the decimal dot
        n = ret;
        m = 1;
        return true;
    }
    // decimal dot found
    q = err+1;
    long ret2 = strtol(q, &err, 10);
    if(*err != '\0' || ret2<=0)
        return false;
    long x = 10;
    while(x<=ret2)
        x*=10;
    n = ret * x + ret2;
    m = x;
    return true;
}



template <class Int, int maxbuf>
const char *scriptpp_signed_to_str(Int i, const char *minstr)
{
    if(i == 0)
        return "0";
    static char buf[maxbuf];
    int idx = sizeof(buf)-1;
    buf[idx] = 0;
    bool negative = (i < 0);
    if(negative)
        i = -i;
    if(i <= 0)         // it's the minimum for the size
        return minstr;
    while(i) {
        idx--;
        buf[idx] = i % 10 + '0';
        i /= 10;
    }
    if(negative) {
        idx--;
        buf[idx] = '-';
    }
    return buf + idx;
}

template <class Int, int maxbuf>
const char *scriptpp_unsigned_to_str(Int i)
{
    if(i == 0)
        return "0";
    static char buf[maxbuf];
    int idx = sizeof(buf)-1;
    buf[idx] = 0;
    while(i) {
        idx--;
        buf[idx] = i % 10 + '0';
        i /= 10;
    }
    return buf + idx;
}

/*
    The following code assumes there are only 32-bit and 64-bit
    architectures, and that int is always 32-bit, long long is
    always 64-bit and long may be either the same as int or the
    same as long long.

    On 16-bit platforms, this code will not work correctly for
    minimal signed values of int and long.

    Should the world change one day so long long becomes 128 bit,
    this code will fail as well, but such a change will mean the
    world failed as a whole.
 */

static const char min_i32_str[] = "-2147483648";
static const char min_i64_str[] = "-9223372036854775808";
static const char *min_long_str =
    sizeof(int) == sizeof(long) ? min_i32_str : min_i64_str;

ScriptNumber::ScriptNumber(short int i)
    : ScriptVariable(scriptpp_signed_to_str<int, 16>(i, min_i32_str)) {}
ScriptNumber::ScriptNumber(unsigned short int i)
    : ScriptVariable(scriptpp_unsigned_to_str<unsigned int, 16>(i)) {}

ScriptNumber::ScriptNumber(int i)
    : ScriptVariable(scriptpp_signed_to_str<int, 16>(i, min_i32_str)) {}
ScriptNumber::ScriptNumber(unsigned int i)
    : ScriptVariable(scriptpp_unsigned_to_str<unsigned int, 16>(i)) {}

ScriptNumber::ScriptNumber(long i)
    : ScriptVariable(scriptpp_signed_to_str<long, 32>(i, min_long_str)) {}
ScriptNumber::ScriptNumber(unsigned long int i)
    : ScriptVariable(scriptpp_unsigned_to_str<unsigned long, 32>(i)) {}

ScriptNumber::ScriptNumber(long long int i)
    : ScriptVariable(scriptpp_signed_to_str<long long, 32>(i, min_i64_str)) {}
ScriptNumber::ScriptNumber(unsigned long long int i)
    : ScriptVariable(scriptpp_unsigned_to_str<unsigned long long, 32>(i)) {}
