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
#include "scrmacro.hpp"



/*
    TODO:  The method named FetchArgs actually scans AND EVALUATES the
           arguments (of an eager macro), so using it from within
           PerformSimple method (which evaluates things like %f:x:y%,
           where there can't be anything nested) seems to be overkill.

           Perhaps FetchArgs should be renamed into smth. like
           ProcessEagerArguments; for PerformSimple, there should be
           smth. simpler than current FetchArgs.  After that, there
           will only be ONE call to FetchArgs (or whatever we rename
           it to), so arguments for it can be reduced, and maybe its
           body can be simplified.

           Actually, that's no strange to encounter this crap.  Reusing
           methods in a recursive descent, well, /doubtful/.

 */

struct macro_item {
    const ScriptMacroprocessorMacro *macro;
    macro_item *next;
} *first;


/* These two structures are operated by ScriptMacroprocessor which is
   responsible for them; they don't own anything, including the list of macros
 */
struct ScriptMacroRealm {
    char esc_char, left_br, right_br, left_lazy, right_lazy;
    macro_item *first;
    const ScriptMacroRealm *parent;

    const ScriptVector *positionals;
    int pos_idx, pos_count;

    explicit ScriptMacroRealm(char ec)
        : esc_char(ec), left_br('['), right_br(']'),
        left_lazy('{'), right_lazy('}'),
        first(0), parent(0), positionals(0)
    {}
    explicit ScriptMacroRealm(const char *chrs)
        : esc_char(chrs[0]), left_br(chrs[1]), right_br(chrs[2]),
        left_lazy(chrs[3]), right_lazy(chrs[4]),
        first(0), parent(0), positionals(0)
    {}
    explicit ScriptMacroRealm(const ScriptMacroRealm *par) // this isn't copy!
        : esc_char(par->esc_char),
        left_br(par->left_br), right_br(par->right_br),
        left_lazy(par->left_lazy), right_lazy(par->right_lazy),
        first(0), parent(par), positionals(0)
    {}

    /* There's no explicit copy contstructor! the structure is copied
       byte to byte and this is exaclty what we want.
       There's also no destructor here.  The master is responsible for
       the destruction.
     */
};

struct ScriptMacroContext : public ScriptMacroRealm {
    int recursion_limit;
    const char *rest;
    ScriptVariable result;

    ScriptMacroContext(const ScriptMacroRealm &realm, const char *s, int lim)
        : ScriptMacroRealm(realm), recursion_limit(lim), rest(s) {}
    /* for the Realm, we use implicit copy-constructor here */

    void DoProcess();

private:
    void Next() { if(*rest) rest++; }
    void Scan(const char *stoppers = 0);

                // recursive descent is implemented here
    void PerformSimple();    // called for rest pointing after '%'
    void PerformNesting();   //                         ----   '%['
    void PerformLazy();      //                         ----   '%{'

    ScriptVariable FetchName();
    void FetchArgs(char delim, char closing, ScriptVector &args);

    const ScriptMacroprocessorMacro* FindMacro(const ScriptVariable &name)
                                                                       const;
    ScriptVariable GetValue(const ScriptVariable &name) const;
    void LazyComputation(const ScriptVariable &name,
                         const ScriptVector &args);

    void HandleError(const char *start_err_pos);

public:
    ScriptVariable GetValue(const ScriptVariable &name,
                            const ScriptVector &args, bool lazy = false) const;
};


static bool char_ok_in_name(int c)
{
    return
        (c >= 'A' && c <= 'Z') ||
        (c >= 'a' && c <= 'z') ||
        (c >= '0' && c <= '9') || c == '*' || c == '_';
}

ScriptVariable ScriptMacroContext::FetchName()
{
    ScriptVariable res = "";
    while(*rest && char_ok_in_name(*rest)) {
        res += *rest;
        Next();
    }
    return res;
}

void ScriptMacroContext::
FetchArgs(char delim, char closing, ScriptVector &args)
{
    char stoppers[3] = { delim, closing, 0 };

    ScriptVariable save_res = result;
    args.Clear();
    char c;

    do {
        result = "";
        Scan(stoppers);
        args.AddItem(result);
        c = *rest;
        Next();
    } while(*rest && c != closing);

    result = save_res;
}

void ScriptMacroContext::DoProcess()
{
    if(!rest) {
        result.Invalidate();
        return;
    }
    if(recursion_limit <= 0) {
        result = rest;
        return;
    }
    result = "";
    Scan();
}

static bool char_in_str(char c, const char *s)
{
    while(*s) {
        if(c == *s)
            return true;
        s++;
    }
    return false;
}

void ScriptMacroContext::Scan(const char *stoppers)
{
    while(*rest) {
        if(stoppers && char_in_str(*rest, stoppers))
            break;
        if(*rest == esc_char) {
            Next();
            char c = *rest;
            if(!c) {
                break;
            } else
            if(c == left_br) {
                Next();
                PerformNesting();
            } else
            if(c == left_lazy) {
                Next();
                PerformLazy();
            } else
            if(c == esc_char || c == right_br || c == right_lazy) {
                result += c;
                Next();
            } else {
                PerformSimple();
            }
        } else {
            result += *rest;
            Next();
        }
    }
}

void ScriptMacroContext::PerformSimple()
{
    const char *startpos = rest - 1;
    ScriptVariable name = FetchName();
    if(!*rest)   /* XXX error handling to be performed here */
        return;
    if(*rest == esc_char) {
        Next();
        ScriptVariable r = GetValue(name);
        if(r.IsInvalid()) {
            HandleError(startpos);
            return;
        }
        result += r;
        return;
    }
    char delim = *rest;
    Next();
    ScriptVector args;
    FetchArgs(delim, esc_char, args);
        // please note it is impossible to have anyting nested here
        // because esc_char is used as the closer
        // FetchArgs is 'too good' here, as it actually evaluates the
        // encountered macros, but here we can't have any of them!

    ScriptVariable r = GetValue(name, args);
    if(r.IsInvalid()) {
        HandleError(startpos);
        return;
    }
    result += r;
}

void ScriptMacroContext::PerformNesting()
{
    const char *startpos = rest - 2;
    ScriptVariable name = FetchName();
    if(!*rest)   /* XXX error handling to be performed here */
        return;
    if(*rest == right_br) {
        Next();
            // no nesting is allowed within the name
        ScriptVariable r = GetValue(name);
        if(r.IsInvalid()) {
            HandleError(startpos);
            return;
        }
        result += r;
        return;
    }
    char delim = *rest;
    Next();
    ScriptVector args;
    FetchArgs(delim, right_br, args);
#if 0
        // now we need to process every arg in a sub-context
        // then pass them to the macro
        // NO WE DON'T!!! FetchArgs already did that for us!
    int i;
    for(i = 0; i < args.Length(); i++) {
        if(args[i].IsInvalid())   // invalidated string remains invalidated
           continue;
        ScriptMacroContext cnt(*this, args[i].c_str(), recursion_limit-1);
        cnt.DoProcess();
        args[i] = cnt.result;
    }
#endif
    ScriptVariable r = GetValue(name, args);
    if(r.IsInvalid()) {
        HandleError(startpos);
        return;
    }
    result += r;
}

void ScriptMacroContext::PerformLazy()
{
    const char *startpos = rest - 2;
    ScriptVariable name = FetchName();
    if(!*rest) {
        HandleError(startpos);
        return;
    }
    if(*rest == right_lazy) {
        Next();
        ScriptVector dummy_args;  // lazy comp. always done with args
        LazyComputation(name, dummy_args);
        return;
    }
    char delim = *rest;
    Next();
    ScriptVector args;
    // we can't use Scan, it is for eager strategy

    ScriptVariable unclosed;
    unclosed += left_lazy;
    args[0] = "";
    while(*rest) {
        if(unclosed.length() == 1 && *rest == delim) {
            Next();
            args.AddItem("");
            continue;
        }
        if((*rest == right_br && unclosed[unclosed.length()-1] == left_br) ||
            (*rest == esc_char && unclosed[unclosed.length()-1] == esc_char))
        {
            args[args.Length()-1] += *rest;
            Next();
            unclosed.Range(-1, 1).Erase();
            continue;
        }
        if(*rest == right_lazy && unclosed[unclosed.length()-1] == left_lazy) {
            unclosed.Range(-1, 1).Erase();
            if(unclosed == "") {
                Next();
                LazyComputation(name, args);
                return;
            } else {
                args[args.Length()-1] += *rest;
                Next();
            }
            continue;
        }
        // all other chars go to the args!
        args[args.Length()-1] += *rest;
        if(*rest == esc_char) {
            Next();
            char c = *rest;
            if(!c)
                break;
            if(c == left_lazy || c == left_br) {
                unclosed += c;
                args[args.Length()-1] += c;
                Next();
            } else
            if(c == right_lazy || c == right_br) {
                // so they will not act as special chars this time
                args[args.Length()-1] += c;
                Next();
            } else {
                // otherwise, leave it where it is, don't consume!
                // it is a start of simple call like %macro%, so
                // add the % to the unclosed stack
                unclosed += esc_char;
            }
        } else {
            Next();
        }
    }
    /* unclosed lazy call */
    HandleError(startpos);
}

void ScriptMacroContext::LazyComputation(const ScriptVariable &name,
                                         const ScriptVector &args)
{
    ScriptVariable s = GetValue(name, args, true);
    if(s.IsInvalid()) // XXXXXXX not found, error?
        return;
    ScriptMacroContext cnt(*this, s.c_str(), recursion_limit-1);
    cnt.DoProcess();
    result += cnt.result;
}

const ScriptMacroprocessorMacro*
ScriptMacroContext::FindMacro(const ScriptVariable &name) const
{
    const ScriptMacroRealm *r;
    for(r = this; r; r = r->parent) {
        macro_item *p;
        for(p = r->first; p; p = p->next)
            if(p->macro->GetName() == name)
                return p->macro;
    }
    return 0;
}


ScriptVariable
ScriptMacroContext::GetValue(const ScriptVariable &name) const
{
    // this form is never called for lazy computation

    if(positionals) {
        long pos;
        if(name.GetLong(pos)) {
            if(pos_count != -1 && pos > pos_count)
                return ScriptVariable("");
            return (*positionals)[pos + pos_idx];
        }
        if(name == "*")
            return (*positionals).Join(" ", pos_idx+1,
                                pos_count == -1 ? -1 : pos_count-1);
    }

    const ScriptMacroprocessorMacro *m = FindMacro(name);
    if(!m)
        return ScriptVariableInv();
    return m->Expand();
}

ScriptVariable
ScriptMacroContext::GetValue(const ScriptVariable &name,
                             const ScriptVector &args, bool lazy) const
{
    // no check for positional here, don't waste the time
    // in future we might want to add %*:separator% here
    const ScriptMacroprocessorMacro *m = FindMacro(name);
    if(!m)
        return ScriptVariableInv();
    if(lazy && m->IsDirty())   // prohibited!
        return ScriptVariableInv();
    return m->Expand(args);
}

void ScriptMacroContext::HandleError(const char *start_err_pos)
{
    const char *p;
    for(p = start_err_pos; p < rest; p++)
        result += *p;
}

////////////////////////////////////////////////////////////////////////

ScriptMacroprocessor::ScriptMacroprocessor(char ec)
    : base_realm(new ScriptMacroRealm(ec)),
    recursion_limit(default_recursion_limit),
    positionals_owned(false)
{
}

ScriptMacroprocessor::ScriptMacroprocessor(const char *chrs)
    : base_realm(new ScriptMacroRealm(chrs)),
    recursion_limit(default_recursion_limit),
    positionals_owned(false)
{
}

ScriptMacroprocessor::ScriptMacroprocessor(const ScriptMacroprocessor *parent)
    : base_realm(new ScriptMacroRealm(parent->base_realm)), // not a copy!
    recursion_limit(default_recursion_limit),
    positionals_owned(false)
{
}

ScriptMacroprocessor::~ScriptMacroprocessor()
{
    while(base_realm->first) {
        macro_item *p = base_realm->first;
        base_realm->first = p->next;
        delete p->macro;
        delete p;
    }
    if(base_realm->positionals && positionals_owned)
        delete base_realm->positionals;
    delete base_realm;
}

ScriptMacroprocessor::MacroId
ScriptMacroprocessor::AddMacro(ScriptMacroprocessorMacro* v)
{
    macro_item *p = new macro_item;
    p->macro = v;
    p->next = base_realm->first;
    base_realm->first = p;
    return base_realm->first;
}

bool ScriptMacroprocessor::RemoveMacro(ScriptMacroprocessor::MacroId id)
{
    for(macro_item **p = &base_realm->first; *p; p = &(*p)->next) {
        if(*p == id) {
            macro_item *tmp = *p;
            *p = tmp->next;
            delete tmp->macro;
            delete tmp;
            return true;
        }
    }
    return false;
}

void ScriptMacroprocessor::SetPositionals(const ScriptVector &args, bool copy)
{
    SetPositionals(args, 0, -1, copy);
}

void ScriptMacroprocessor::SetPositionals(const ScriptVector &args,
                                          int idx, int cnt, bool copy)
{
    if(positionals_owned && base_realm->positionals)
        delete base_realm->positionals;
    if(copy) {
        base_realm->positionals = new ScriptVector(args, idx, cnt);
        positionals_owned = true;
        base_realm->pos_idx = 0;
        base_realm->pos_count = -1;
    } else {
        base_realm->positionals = &args;
        positionals_owned = false;
        base_realm->pos_idx = idx;
        base_realm->pos_count = cnt;
    }
}

ScriptVariable ScriptMacroprocessor::Process(const ScriptVariable &src) const
{
    ScriptMacroContext context(*base_realm, src.c_str(), recursion_limit);
    context.DoProcess();
    return context.result;
}

ScriptVariable
ScriptMacroprocessor::Process(const ScriptVariable &src,
                        const ScriptVector &argv, int idx, int count) const
{
    ScriptMacroContext context(*base_realm, src.c_str(), recursion_limit);
    context.positionals = &argv;
    context.pos_idx = idx;
    context.pos_count = count;
    context.DoProcess();
    return context.result;
}

ScriptVariable ScriptMacroprocessor::
Process(const ScriptVariable &src, const ScriptMacroprocessorMacro *aux) const
{
    ScriptMacroContext context(*base_realm, src.c_str(), recursion_limit);
    macro_item it;
    it.next = context.first;
    it.macro = aux;
    context.first = &it;
    context.DoProcess();
    return context.result;
}


ScriptVariable ScriptMacroprocessor::
Apply(const ScriptVariable &name, const ScriptVector &args,
                                  bool force_no_dirty) const
{
    ScriptMacroContext context(*base_realm, 0, recursion_limit);
    return context.GetValue(name, args, force_no_dirty);
}



ScriptVariable ScriptMacroprocessorMacro::Expand() const
{
    ScriptVector dummy;
    return Expand(dummy);
}





ScriptMacroDictionary::ScriptMacroDictionary(const ScriptVariable &name,
                                    const ScriptVector &dict, bool copy)
    : ScriptMacroprocessorMacro(name), vec_owned(copy)
{
    if(copy)
        the_dict = new ScriptVector(dict);
    else
        the_dict = &dict;
}

ScriptMacroDictionary::~ScriptMacroDictionary()
{
    if(vec_owned)
        delete the_dict;
}

ScriptVariable
ScriptMacroDictionary::Expand(const ScriptVector &args) const
{
    int i;
    for(i = 0; i < the_dict->Length() - 1; i += 2) {
        if((*the_dict)[i] == args[0])
            return (*the_dict)[i+1];
    }
    return "";
}
