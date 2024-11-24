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




#include <string.h> // for strlen()
#include <stdlib.h> // for free()
#include "scrvar.hpp"
#include "scrvect.hpp"

enum { scriptvector_default_size = 16 };

ScriptVector::ScriptVector()
    : vec(0), len(0), maxlen(0)
{
}

ScriptVector::ScriptVector(const ScriptVariable &sv,
                           const char *delims,
                           const char *trimspaces)
{
    vec = new ScriptVariable[scriptvector_default_size];
    len = 0;
    maxlen = scriptvector_default_size;
    if(!delims) {
        vec[0] = sv;
        len = 0;
        return;
    }
    ScriptVariable sv1(sv); // to avoid const problem
    ScriptVariable::Substring iter(sv1);
    ScriptVariable::Substring word;
    while(trimspaces ? iter.FetchToken(word, delims, trimspaces) :
                       iter.FetchWord(word, delims))
    {
        AddItem(word.Get());
    }
}

ScriptVector::ScriptVector(const char * const *v)
{
    vec = new ScriptVariable[scriptvector_default_size];
    len = 0;
    maxlen = scriptvector_default_size;
    while(*v) {
        AddItem(*v);
        v++;
    }
}

ScriptVector::ScriptVector(const ScriptVector &other, int aidx, int alen)
{
    if(aidx < 0)
        aidx = 0;
    else if(aidx > other.len)
        aidx = other.len;
    if(alen == -1 || alen > other.len - aidx)
        alen = other.len - aidx;
    len = alen;
    maxlen = scriptvector_default_size;
    while(maxlen<len) maxlen*=2;
    vec = new ScriptVariable[maxlen];
    for(int i = 0; i<len; i++)
        vec[i] = other.vec[i+aidx];
}

ScriptVector::~ScriptVector()
{
    if(vec)
        delete[] vec;
}

ScriptVariable& ScriptVector::operator[](int i)
{
    if(i<0)
        return vec[0];  // should be exception in fact... let it be.
    if(i>=maxlen)
        ProvideVectorLength(i+1);
    if(i>=len)
        len = i+1;
    return vec[i];
}

const ScriptVector& ScriptVector::operator=(const ScriptVector &other)
{
    ProvideVectorLength(other.len);
    if(len < other.len)
        len = other.len;
    while(len>other.len) {
        vec[len-1] = "";
        len--;
    }
    for(int i = 0; i<other.len; i++)
        vec[i] = other.vec[i];
    return *this;
}

ScriptVariable ScriptVector::operator[](int i) const
{
    return (i>=0 && i<len) ? vec[i] : ScriptVariable();
}

void ScriptVector::Insert(int idx, const ScriptVariable& sv)
{
    DoInsert(idx, &sv, 1);
}

void ScriptVector::Insert(int idx, const ScriptVector& svec)
{

    DoInsert(idx, svec.vec, svec.len);
}

void ScriptVector::Remove(int idx, int amount)
{
    if(idx>=len)
        return;
    if(idx+amount>len)
        amount = len-idx;
    int i;
    for(i=idx; i<len-amount; i++)
        vec[i] = vec[i+amount];
    for(;i<len;i++)
        vec[i] = "";
    len -= amount;
}

void ScriptVector::AddItem(const ScriptVariable& sv)
{
    ProvideVectorLength(len+1);
    vec[len++] = sv;
}

void ScriptVector::Clear()
{
    for(int i=0;i<len;i++)
        vec[i] = "";
    len = 0;
}

ScriptVariable ScriptVector::Join(const char *sep, int start, int cnt) const
{
    int totallen, i, j, k;
    int seplen = sep ? strlen(sep) : 0;
    int last = (cnt == -1) ? len-1 : start+cnt-1;
    if(last >= len)
        last = len-1;

    if(start > last)
        return "";

    totallen = 0;
    for(i=start; i<=last; i++)
        totallen += vec[i].length();
    if(sep)
        totallen += seplen * (last-start);

    ScriptVariableInv res;
    res.MakeRoom(totallen);
    int idx = 0;
    for(i=start; i<=last; i++) {
        for(k = 0; vec[i][k]; k++) {
            res[idx++] = vec[i][k];
        }
        if(sep && i<last) {
            for(j = 0; j<seplen; j++) {
                res[idx++] = sep[j];
            }
        }
    }
    res[idx] = 0;
    return res;
}

char** ScriptVector::MakeArgv() const
{
    char ** ret = new char*[len+1];
    for(int i=0; i<len; i++)
        ret[i] = strdup((*this)[i].c_str());
    ret[len] = 0;
    return ret;
}

void ScriptVector::DeleteArgv(char** argv)
{
    for(int i=0; argv[i]; i++)
        free((void*)(argv[i]));
    delete[] argv;
}

void ScriptVector::DoInsert(int idx, const ScriptVariable *vars, int n)
{
    ProvideVectorLength(len+n);
    int i;
    for(i=len+n-1; i>=idx+n; i--)
        vec[i] = vec[i-n];
    for(i=0; i<n; i++)
        vec[idx+i] = vars[i];
    len+=n;
}

void ScriptVector::ProvideVectorLength(int i)
{
    if(i<=maxlen)
        return;
    int newlen = maxlen ? maxlen * 2 : scriptvector_default_size;
    while(newlen<i)
        newlen *= 2;
    ScriptVariable *newvec = new ScriptVariable[newlen];
    for(int i=0; i<len; i++)
        newvec[i] = vec[i];
    if(vec)
        delete[] vec;
    vec = newvec;
    maxlen = newlen;
}
