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




#ifndef SCRIPTPP_SCRSUBS_HPP_SENTRY
#define SCRIPTPP_SCRSUBS_HPP_SENTRY

/*! \file scrsubs.hpp
    \brief This file invents the ScriptSubstitution class which substitutes
    (pseudo)-variables in a given string

    PLEASE DON'T USE THIS MODULE IN NEW PROJECTS, IT HAS PROVEN TO
    HAVE ILL-DEFINED SEMANTICS!

    Consider using ScriptMacroprocessor class instead (see scrmacro.[ch]pp)
 */

#include "scrvar.hpp"
#include "scrvect.hpp"

//! Script Substitution substitutes escaped 'vars' in a string
/*! Variables have a form of either %var%, %[var] or ${var}; you can also
    use any other 'escape' chars instead of %,[,],{,} passing them to the
    constructor (as a string of exactly 5 chars).  In all examples we
    assume the default chars are used.

    The special case %0%, %1%, ..., %120%, etc., can (and perhaps should)
    be used to access elements of a certain ScriptVector, as ``arguments''.
    Another important special case is ``%%'', which produces a single '%'
    char.

    The ScriptSubstitution class object owns its collection of
    ``variables'' represented by objects of classes derived from
    ScriptSubstitution::Var; the master simply finds variables in the
    given string and, once a variable entry found, it queries its slaves
    one by one, whether they have a substitution for this var, or not.

    If no suitable variable object is found, the entry is simply replaced
    by empty string; however, you can use your own 'last resort'
    variable object, which, at your option, can replace the unknown
    variable with an error message, or with the name of the var as is,
    or even throw an exception.  To do so, create a ``variable'' class
    which accepts any name (perhaps just ignoring the name) and does what
    you want, then add this object FIRST, before any other vars.

    Any of the names (including these 0, 1, ...) can be used in any of the
    three ways -- as %name%, %[name] or %{name}.  The difference between
    %name% and %[name] is that in the former case variable references can
    be nested, such as %[foo%[bar]bur] or even %[foo%bar%bur].  The %[]
    var references are performed in 'eager' manner, that is, the inner
    reference is processed first, so you can use this to organize a kind
    of indexation, like %[myarray.%[myindex]]: if ``myindex'' is replaced
    with 120, then the whole construct will be replaced just like
    ``%myarray.120%''.

    The %{} references are processed in 'lazy' manner: first, the line is
    searched to match the '{' with '}', ignoring 'escaped' instances
    (``%}''), and then all the chars between ``%{'' and the matching ``}''
    is offered to the vars; afterwards, the resulting (substituted) string
    is processed again.  You might want to take a look at the
    ScriptSubstitutionPrefixRequest class to understand how to make some
    use out of these 'lazy' variables.

    Please be WARNED that if your variable is replaced with some 'external'
    data, such as received from the Net or read from a file, SUCH VARIABLE
    MUST NEVER BE USED with %{}!  Such usage would lead to obvious
    injection vulnerability.  From the other hand, there's nothing to worry
    about if such a variable is used like %var% or %[var], even if it is
    inside of %{}.

    Note that %x% generally performs faster than %[x] so if you don't
    need them nested, then perhaps you should prefer the first form.  %{x}
    is the slowest thing; avoid it if possible.
 */
class ScriptSubstitution {
public:
    //! The substitution variable interface
    /*! To create a variable, or a set of them, simply derive your class
        from this one, implementing the Sub() virtual method, which does
        all the job.
     */
    class Var {
    public:
            //! The method to be implemented
            /*! If the Substitution encounters, e.g., %MYVAR%, then name
                will point to the `M' in MYVAR, and len will be 5.  If the
                name is not interesting for you, simply return
                ScriptVariableInv(); if you return a valid ScriptVariable,
                then the master assumes that this is exacly the text to be
                substituted, and doesn't query more vars.
                \param name is the pointer to the var's name's first char
                \len is the length of the var's name
             */
        virtual ScriptVariable Sub(const char *name, int len) const = 0;
        virtual ~Var() {}
    };

    typedef void *VarId;

    enum { default_recursion_limit = 20 };

private:
    char esc_char, left_br, right_br, left_lazy, right_lazy;
    int recursion_limit;
    struct item {
        const Var *var;
        item *next;
    } *first;
public:
    ScriptSubstitution(char ec = '%')
        : esc_char(ec), left_br('['), right_br(']'),
        left_lazy('{'), right_lazy('}'),
        recursion_limit(default_recursion_limit), first(0) {}
    ScriptSubstitution(const char *chrs) // e.g., "%[]{}"
        : esc_char(chrs[0]), left_br(chrs[1]), right_br(chrs[2]),
        left_lazy(chrs[3]), right_lazy(chrs[4]),
        recursion_limit(default_recursion_limit), first(0) {}

    void SetRecursionLimit(int lim) { recursion_limit = lim; }

    ~ScriptSubstitution();

        /*! \note The class OWNS all the 'Var' instances!
            \note To increase performans, add the most often used
                  entries last, and the rarely used entries first,
                  as they are added to the front of a single-linked
                  list and are queried in the order of the list.
                  If you want to have a 'last resort' variable, which
                  accepts any name, add it first.
         */
    VarId AddVar(Var* v);
        /*! \note The 'var' object is deleted, so don't try to reuse it */
    bool RemoveVar(VarId id);
    ScriptVariable Substitute(const ScriptVariable &src) const;
    ScriptVariable Substitute(const ScriptVariable &src,
                              const ScriptVector &argv,
                              int idx = 0, int count = -1) const;
    ScriptVariable Substitute(const ScriptVariable &src, const Var *aux) const;

    ScriptVariable operator()(const ScriptVariable &src) const
        { return Substitute(src); }
    ScriptVariable operator()(const ScriptVariable &src,
                              const ScriptVector &argv,
                              int idx = 0, int count = -1) const
        { return Substitute(src, argv, idx, count); }
private:
    ScriptVariable
    DoSubstitution(const ScriptVariable &src, item *lsti, int levlimit) const;
        /*! \note substitution result is ADDED to the res buffer */
    void PerformSimple(const char *p, item *lst,
                       ScriptVariable &res, int &eaten_len) const;
    void PerformNesting(const char *p, item *lst,
                        ScriptVariable &res, int &eaten_len) const;
    void PerformLazy(const char *p, item *lst,
                     ScriptVariable &res, int &eaten_len, int levlim) const;
    static bool AppendReplacement(const char *p, int len, item *lst,
                                  ScriptVariable &res);
    static void AppendUntouched(const char *p, int len, ScriptVariable &res);
};

//! A single substitution variable of the given (fixed) name
/*! The name of the substitution variable is given to the constructor,
    e.g., to arrange a substitution for '%MYVAR%', give the string 'MYVAR'.
    The method Text() must be implemented by your derived class; it
    returns the string to be substituted, which can be different each time.
 */
class ScriptSubstitutionSingleName : public ScriptSubstitution::Var {
    ScriptVariable name;
public:
    ScriptSubstitutionSingleName(const ScriptVariable &name);
    ~ScriptSubstitutionSingleName() {}
    ScriptVariable Sub(const char *name, int len) const;
    virtual ScriptVariable Text() const = 0;
};

//! The simplest possible case: fixed name, fixed value
class ScriptSubstitutionConst : public ScriptSubstitutionSingleName {
    ScriptVariable val;
public:
    ScriptSubstitutionConst(const ScriptVariable &name,
                            const ScriptVariable &value);
    ~ScriptSubstitutionConst() {}
private:
    virtual ScriptVariable Text() const;
};

//! Substitute a var of given name with the value of a ScriptVariable
/*! The ScriptVariable you give here can change whenever you want, so
    the value to be substituted will change, too.
    \warning Make sure the object of ScriptVariable that you use here
    remains existing all the time the object of ScriptSubstitutionScrVar
    exists, or else full degree of chaos is guaranteed.
 */
class ScriptSubstitutionScrVar : public ScriptSubstitutionSingleName {
    ScriptVariable *val;
public:
    ScriptSubstitutionScrVar(const ScriptVariable &name, ScriptVariable *pval);
    ~ScriptSubstitutionScrVar() {}
private:
    virtual ScriptVariable Text() const;
};

//! A substitution family identified by fixed prefix
/*! The prefix is given to the constructor; the ``variable'' name
    consists of the prefix and an arbitrary count of parameters,
    delimited by the given char, by default ':'.  The parameters are
    passed to the (pure virtual) Handle method as a prepared ScriptVector.
    E.g. if the prefix is ``contact'', then the object will handle
    requests like %contact:John:Smith:homephone%, and the Handle method
    will be called with { "John", "Smith", "homephone" } incapsulated in
    a ScriptVector object.

    Objects of this class are aware of %[] and %{} variable references,
    as well as of the %} escape; the delimiter char is only active outside
    of nested variable references, if any.  In case you use modified
    set of active chars, make sure you pass the same chars both to the
    constructor of ScriptSubstitutionPrefixRequest and of
    ScriptSubstitution (e.g. if "$<>()" is passed there, smth. like
    "|$<>()" must be passed here.)
 */
class ScriptSubstitutionPrefixRequest : public ScriptSubstitution::Var {
    ScriptVariable prefix;
    char delim, esc_char, left_br, right_br, left_lazy, right_lazy;
public:
    ScriptSubstitutionPrefixRequest(const ScriptVariable &pref,
                                    const char *spec = ":%[]{}");
    ~ScriptSubstitutionPrefixRequest() {}
    ScriptVariable Sub(const char *name, int len) const;
    virtual ScriptVariable Handle(const ScriptVector &params) const = 0;
};

//! The substitution for %0%, %1%, ..., %17%, ...
/*! Arguments are numbered from zero.  The special case %*% returns
    all args starting from 1 (not 0!) joined by space.

    To organize your 'arguments' for the substitutiuon, use a ScriptVector
    object.  The ScriptSubstitutionArgv object can either make a copy of
    your vector, or (if you wish so) assume the vector remains existing
    somewhere else, and there's no need for a copy.  You can use a part
    (a segment) of an existing vector as well (i.e., only 12 'arguments',
    from 17th to 28th element of the vector, etc.)

    Typically you don't create objects of this type yourself, instead
    simply use the appropriate method of ScriptSubstitution; however,
    it IS possible to use this class directly.
 */
class ScriptSubstitutionArgv : public ScriptSubstitution::Var {
    const class ScriptVector *vec;
    bool vec_owned;
    int idx, count;
public:
    ScriptSubstitutionArgv(const ScriptVector *vec, bool copy = true,
                           int idx = 0, int count = -1);
    ~ScriptSubstitutionArgv();
    ScriptVariable Sub(const char *name, int len) const;
};

//! The 'dictionary' substitution
/*! The dictionary is represented by a ScriptVector of even length,
    the 0th, 2nd, 4th... items being names to substitute, and the
    1st, 3rd, 5th... to be the text to substitute.

    The ScriptSubstitutionDictionary object can either make a copy of
    your vector, or (if you wish so) assume the vector remains existing
    somewhere else, and there's no need for a copy.
*/
class ScriptSubstitutionDictionary : public ScriptSubstitution::Var {
    const class ScriptVector *the_dict;
    bool vec_owned;
public:
    ScriptSubstitutionDictionary(const ScriptVector &dict, bool copy = true);
    ~ScriptSubstitutionDictionary();
    ScriptVariable Sub(const char *name, int len) const;
};


#endif
