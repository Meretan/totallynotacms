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




#include "scrvect.hpp"
#include "scrsubs.hpp"

ScriptSubstitution::~ScriptSubstitution()
{
    while(first) {
        item *p = first;
        first = p->next;
        delete p->var;
        delete p;
    }
}

ScriptSubstitution::VarId ScriptSubstitution::AddVar(Var* v)
{
    item *p = new item;
    p->var = v;
    p->next = first;
    first = p;
    return first;
}

bool ScriptSubstitution::RemoveVar(ScriptSubstitution::VarId id)
{
    for(item **p = &first; *p; p = &(*p)->next) {
        if(*p == id) {
            item *tmp = *p;
            *p = tmp->next;
            delete tmp->var;
            delete tmp;
            return true;
        }
    }
    return false;
}

ScriptVariable ScriptSubstitution::Substitute(const ScriptVariable &src) const
{
    return DoSubstitution(src, first, recursion_limit);
}

ScriptVariable ScriptSubstitution::
Substitute(const ScriptVariable &src,
           const ScriptVector &argv, int idx, int count) const
{
    item it;
    it.next = first;
    it.var = new ScriptSubstitutionArgv(&argv, false, idx, count);
    ScriptVariable res = DoSubstitution(src, &it, recursion_limit);
    delete it.var;
    return res;
}

ScriptVariable ScriptSubstitution::
Substitute(const ScriptVariable &src, const Var *aux) const
{
    item it;
    it.next = first;
    it.var = aux;
    return DoSubstitution(src, &it, recursion_limit);
}

ScriptVariable ScriptSubstitution::
DoSubstitution(const ScriptVariable &src, item *itemlist, int reclim) const
{
    if(src.IsInvalid())
        return ScriptVariableInv();
    if(reclim <= 0)
        return src;
    ScriptVariable res("");
    const char *p = src.c_str();
    while(*p) {
        if(*p == esc_char) {
            if(p[1] == left_br) {
                int len;
                PerformNesting(p+2, itemlist, res, len);
                p += (len + 2);
            } else
            if(p[1] == left_lazy) {
                int len;
                PerformLazy(p+2, itemlist, res, len, reclim);
                p += (len + 2);
            } else
            if(p[1] == esc_char || p[1] == right_lazy) {
                res += p[1];
                p += 2;
            } else {
                int len;
                PerformSimple(p+1, itemlist, res, len);
                p += (len + 1);
            }
        } else {
            res += *p;
            p++;
        }
    }
    return res;
}

void ScriptSubstitution::
PerformSimple(const char *p, item *lst, ScriptVariable &res, int &len) const
{
    len = 0;
    while(p[len] && p[len] != esc_char && p[len] != '\n')
        len++;
    if(!p[len])
        return;
    if(p[len] == '\n') {
        len++;
        return;
    }
    if(len == 0) {
        res += esc_char;
        len++;
        return;
    } else {
        if(!AppendReplacement(p, len, lst, res))
            AppendUntouched(p-1, len+2, res);
        len++;
    }
}

void ScriptSubstitution::
PerformNesting(const char *p, item *lst, ScriptVariable &res, int &len) const
{
    ScriptVariable inner = "";
    len = 0;
    while(*p) {
        if(*p == esc_char) {
            int inlen;
            if(p[1] == left_br) {
                PerformNesting(p+2, lst, inner, inlen);
                inlen += 2;
            } else
            if(p[1] == left_lazy) {
                PerformLazy(p+2, lst, inner, inlen, recursion_limit);
                inlen += 2;
            } else {
                PerformSimple(p+1, lst, inner, inlen);
                inlen += 1;
            }
            p += inlen;
            len += inlen;
        } else
        if(*p == right_br) {
            // inner string construction finished
            len++;
            if(!AppendReplacement(inner.c_str(), inner.Length(), lst, res)) {
                res += esc_char;
                res += left_br;
                res += inner;
                res += right_br;
            }
            return;
        } else {
            inner += *p;
            p++;
            len++;
        }
    }
    // no construction ending found, leave everything as is
    res += esc_char;
    res += left_br;
    res += inner;
}

void ScriptSubstitution::PerformLazy(const char *p, item *lst,
                            ScriptVariable &res, int &len, int reclim) const
{
    int level = 1;
    len = 0;
    while(p[len]) {
        if(p[len] == esc_char) {
            if(!p[len+1]) {
                break;
            } else
            if(p[len+1] == left_lazy) {
                level++;
                len += 2;
            } else
            if(p[len+1] == right_lazy) {
                // ``escaped'' '}' doesn't affect levels
                len += 2;
            } else
            if(p[len+1] == esc_char) {
                // "%%" might be considered something special, but not now
                len += 2;
            } else {
                // nothing special, we're lazy!
                len += 1;
            }
        } else
        if(p[len] == right_lazy) {
            len++;
            level--;
            if(level == 0) {
                // here we go!
                ScriptVariable inner_res;
                if(!AppendReplacement(p, len-1, lst, inner_res)) {
                    AppendUntouched(p-2, len+3, inner_res);
                }
                res += DoSubstitution(inner_res, lst, reclim - 1);
                return;
            }
        } else {
            len++;
        }
    }
    res += esc_char;
    res += left_lazy;
    len = 2;
}

bool ScriptSubstitution::
AppendReplacement(const char *p, int len, item *lst, ScriptVariable &res)
{
    for(item *t = lst; t; t = t->next) {
        ScriptVariable v = t->var->Sub(p, len);
        if(!v.IsInvalid()) {
            res += v;
            return true;
        }
    }
    // just tell the parent to add everything back as it was
    return false;
}

void ScriptSubstitution::
AppendUntouched(const char *p, int len, ScriptVariable &res)
{
    int i;
    for(i = 0; i < len; i++)
        res += p[i];
}



ScriptSubstitutionSingleName::
ScriptSubstitutionSingleName(const ScriptVariable &n)
    : name(n)
{}

ScriptVariable
ScriptSubstitutionSingleName::Sub(const char *nm, int len) const
{
    if(name.Length() != len)
        return ScriptVariableInv();
    for(int i = 0; i < len && nm[i]; i++)
        if(nm[i] != name[i])
            return ScriptVariableInv();
    return Text();
}


ScriptSubstitutionConst::
ScriptSubstitutionConst(const ScriptVariable &n, const ScriptVariable &v)
    : ScriptSubstitutionSingleName(n), val(v)
{}

ScriptVariable ScriptSubstitutionConst::Text() const
{
    return val;
}


ScriptSubstitutionScrVar::
ScriptSubstitutionScrVar(const ScriptVariable &n, ScriptVariable *pv)
    : ScriptSubstitutionSingleName(n), val(pv)
{}

ScriptVariable ScriptSubstitutionScrVar::Text() const
{
    return *val;
}



ScriptSubstitutionPrefixRequest::
ScriptSubstitutionPrefixRequest(const ScriptVariable &p, const char *spec)
    : prefix(p + spec[0]), delim(spec[0]),
    esc_char(spec[1]), left_br(spec[2]), right_br(spec[3]),
    left_lazy(spec[4]), right_lazy(spec[5])
{
}

ScriptVariable
ScriptSubstitutionPrefixRequest::Sub(const char *name, int len) const
{
    int pl = prefix.Length();
    if(len < pl || !ScriptVariable(name).HasPrefix(prefix))
        return ScriptVariableInv();

    ScriptVector params;
    int npar = 0;
    params[npar] = "";  /* to make sure it is valid */
    int level = 0;
    for(int i = prefix.Length(); i < len; i++)
        if(name[i] == delim && level == 0) {
            npar++;
            params[npar] = "";
        } else
        if(name[i] == esc_char && i+1 < len) {
            params[npar] += name[i];
            if(name[i+1] == left_br || name[i+1] == left_lazy) {
                params[npar] += name[i+1];
                level++;
                i++;
            } else
            if(name[i+1] == right_lazy) {  // right_br can't be escaped
                params[npar] += name[i+1];
                i++;
            }
        } else
        if(name[i] == right_br || name[i] == right_lazy) {
            params[npar] += name[i];
            level--;
        } else {
            params[npar] += name[i];
        }
    return Handle(params);
}



ScriptSubstitutionArgv::
ScriptSubstitutionArgv(const ScriptVector *avec, bool copy, int aidx, int cnt)
    : vec_owned(copy), idx(aidx), count(cnt)
{
    if(copy) {
        vec = new ScriptVector(*avec, idx, count);
        idx = 0;
    } else {
        vec = avec;
    }
    if(count == -1 || count > (vec->Length() - idx))
        count = vec->Length() - idx;
}

ScriptSubstitutionArgv::~ScriptSubstitutionArgv()
{
    if(vec_owned)
        delete vec;
}

ScriptVariable ScriptSubstitutionArgv::Sub(const char *name, int len) const
{
    if(len == 1 && *name == '*') {
        ScriptVariable res("");
        for(int i = 1; i < count; i++) {
            if(i>1) res += ' ';
            res += (*vec)[i+idx];
        }
        return res;
    }
    int n = 0;
    for(int i = 0; i < len; i++) {
        if(name[i] > '9' || name[i] < '0')
            return ScriptVariableInv();
        n *= 10;
        n += name[i] - '0';
    }
    if(n >= count)
        return ScriptVariable("");
    else
        return (*vec)[n+idx];
}

ScriptSubstitutionDictionary::
ScriptSubstitutionDictionary(const ScriptVector &dict, bool copy)
    : vec_owned(copy)
{
    if(copy)
        the_dict = new ScriptVector(dict);
    else
        the_dict = &dict;
}

ScriptSubstitutionDictionary::~ScriptSubstitutionDictionary()
{
    if(vec_owned)
        delete the_dict;
}

ScriptVariable
ScriptSubstitutionDictionary::Sub(const char *name, int len) const
{
    int i;
    for(i = 0; i < the_dict->Length() - 1; i += 2) {
        if((*the_dict)[i].Length() == len) {
            bool ok = true;
            for(int k = 0; k < len; k++)
                if(name[k] != (*the_dict)[i][k]) {
                    ok = false;
                    break;
                }
            if(ok)
                return (*the_dict)[i+1];
        }
    }
    return ScriptVariableInv();
}

