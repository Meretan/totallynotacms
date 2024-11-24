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




#ifndef SCRIPTPP_SCRMACRO_HPP_SENTRY
#define SCRIPTPP_SCRMACRO_HPP_SENTRY

/*! \file scrmacro.hpp
    \brief This file invents the ScriptMacroprocessor class which
    expand predefined macros in a given string
 */

#include "scrvar.hpp"
#include "scrvect.hpp"

//! The Macro interface
/*! To create a macro, simply derive your class from this one,
    implementing the Expand() virtual method, which does all the job.
 */
class ScriptMacroprocessorMacro {
    ScriptVariable name;
    bool dirty;
public:
    ScriptMacroprocessorMacro(const ScriptVariable &n, bool d = false)
        : name(n), dirty(d) {}
    virtual ~ScriptMacroprocessorMacro() {}
    const ScriptVariable &GetName() const { return name; }
    bool IsDirty() const { return dirty; }

        //! The method to be implemented
        /*!
         */
    virtual ScriptVariable Expand(const ScriptVector &args) const = 0;

        //! The simplified case
        /*! This version is called when the macro call specifies
            no arguments (e.g., if it is a simple macrovariable).
            By default, this method calls the previous form with
            empty Vector but you can override it to gain efficiecny
         */
    virtual ScriptVariable Expand() const;
};

//! Script Macroprocessor processes macros.  Surprize, heh.
/*! Macro calls have a form of either %var%, %[var] or ${var}, where name
    can consist of alphanumeric chars ('a'..'z', 'A'..'Z', '0'..'9'
    ranges), and the characters '_' and '*'.  Macro calls can have
    parameters like $[func:arg1:arg2]; the ':' here can actually be any char
    which is not allowed in names (not alnum, '_' nor '*').  Please note
    that the library is (as a matter of principle) i18n-agnostic, so please
    use ASCII symbols in macro names and punctuation, or else problems are
    almost guaranteed.  For each macro call its argument-delimiting char is
    set to the first _byte_ inside the macro call which is outside of the
    abovementioned ranges and is not the closing char for the particular
    call form (%, ], }); for example, you can use $[func/arg1/arg2], or
    $[func@arg1@arg2], or $[func#arg1#arg2].

    You can also replace all the 'special' chars to anything else than
    %,[,],{,} passing them to the constructor (as a string of exactly 5
    chars).  In all examples we assume the default chars are used.

    The special case %0%, %1%, ..., %120%, etc., can (and perhaps should)
    be used to access elements of a certain ScriptVector, as ``arguments''.
    %*% gives all ``args'' except for the zeroth, joined with one space.
    Another important special case is ``%%'', which produces a single '%'
    char; %} and %] can be used to 'escape' the closing character, should
    it happen to be needed within the call.

    The ScriptMacroprocessor class object owns its collection of
    ``macros'' represented by objects of classes derived from
    ScriptMacroprocessor::Macro, each having its name.  In the present
    version, the name search is done in a simple linear way; this works
    perfectly until you've got a lot of macros (say, 100 and more of them).
    The Macro class has a pure virtual method which takes ScriptVector
    (formed out of all params except the macro name itself) and returns
    the resulting string in form of ScriptVariable.

    Any macro (including these 0, 1, ...) can be used in any of the
    three ways -- as %name%, %[name] or %{name}.  The difference between
    %name% and %[name] is that in the former case variable references can
    be nested, such as %[foo:%[bar]:bur].  The %[] macro expansions are
    performed in 'eager' manner, that is, the inner macro call is expanded
    first.  However, the call is broken down to delimited parts _before_
    any more calls are processed, and each part is processed separately.

    The %{} references are processed in 'lazy' manner: first, the line is
    searched to match the '{' with '}', ignoring 'escaped' instances
    (``%}'', ``%]''), broken down to delimited arguments, and then the
    arguments are passed to the Macro object without any processing;
    afterwards, the resulting (substituted) string is processed again.

    Please be WARNED that if your macro is replaced with some 'external'
    data, such as received from the Net or read from a file, SUCH MACRO
    MUST NEVER BE USED with %{}!  Such usage would lead to obvious
    injection vulnerability.  From the other hand, there's nothing to worry
    about if such a macro is used like %var% or %[var], even if it is
    inside of %{}.  Such macros must be marked as ``dirty'' so that the
    macroprocessor will refuse to expand them in lazy manner.

    Note that %x% generally performs faster than %[x] so if you don't
    need them nested, then perhaps you should prefer the first form.  %{x}
    is the slowest thing; avoid it if possible.

    If no suitable Macro object is found, the entry is silently left
    untouched by default, except for the lazy calls, which are replaced
    with empty strings.  The same thing happens when your macro returns
    ScriptVariableInv.  In the present version this behavior can't
    be changed; if you really need to, contact the author (or see the
    ScriptMacroContext::HandleError method which is to be made more
    flexible).  It is also impossible to determine what caused the error
    (unknown macro name or existing macro returning invalidated string).
 */
class ScriptMacroprocessor {
public:

    typedef void *MacroId;

    enum { default_recursion_limit = 20 };

private:
    struct ScriptMacroRealm *base_realm;
    int recursion_limit;
    bool positionals_owned;
        // do we own the positionals (the pointer is inside the base_realm)
public:
    ScriptMacroprocessor(char ec = '%');
    ScriptMacroprocessor(const char *chrs);
        /*! \param parent is a (kinda) 'outer scope', local vars shadow
                   the names from the parent; the parent is checked
                   in case no match is found locally
            \note  make sure the parent object remains existing
                   until you stop using all objects that depend on it
         */
    ScriptMacroprocessor(const ScriptMacroprocessor *parent);
    ~ScriptMacroprocessor();

    void SetRecursionLimit(int lim) { recursion_limit = lim; }

        /*! \note The class OWNS all the 'Macro' instances!
            \note To increase performans, add the most often used
                  entries last, and the rarely used entries first,
                  as they are added to the front of a single-linked
                  list and are queried in the order of the list.
         */
    MacroId AddMacro(ScriptMacroprocessorMacro* v);
        /*! \note The Macro object is deleted, so don't try to reuse it */
    bool RemoveMacro(MacroId id);

    void SetPositionals(const ScriptVector &args, bool copy);
    void SetPositionals(const ScriptVector &args, int idx, int cnt, bool copy);

    ScriptVariable Process(const ScriptVariable &src) const;
    ScriptVariable Process(const ScriptVariable &src,
                           const ScriptVector &argv,
                           int idx = 0, int count = -1) const;

        /*! \note the *aux object is not deleted, it remains yours */
    ScriptVariable Process(const ScriptVariable &src,
                           const ScriptMacroprocessorMacro *aux) const;

    ScriptVariable operator()(const ScriptVariable &src) const
        { return Process(src); }
    ScriptVariable operator()(const ScriptVariable &src,
                              const ScriptVector &argv,
                              int idx = 0, int count = -1) const
        { return Process(src, argv, idx, count); }

        //! Apply a macro to the given arguments
        /*! \warning Be sure to set force_no_dirty to true in case
            you're going to process the result again (like for
            lazy evaluations); however there's no reason to bother
            in case you've got no ``dirty'' macros in your realm.
            See the ScriptMacroprocessor documentation for details.
         */
    ScriptVariable Apply(const ScriptVariable &name,
                         const ScriptVector &args,
                         bool force_no_dirty = false) const;
};

//! A single macro 'variable' without parameters
/*! The name of the substitution variable is given to the constructor,
    e.g., to arrange a substitution for '%MYVAR%', give the string 'MYVAR'.
    The method Text() must be implemented by your derived class; it
    returns the string to be substituted, which can be different each time.
 */
class ScriptMacroVariable : public ScriptMacroprocessorMacro {
public:
    ScriptMacroVariable(const ScriptVariable &a_name)
        : ScriptMacroprocessorMacro(a_name) {}
    ~ScriptMacroVariable() {}
    ScriptVariable Expand(const ScriptVector &args) const
        { return args.Length() == 0 ? Expand() : ScriptVariable(""); }
    ScriptVariable Expand() const { return Text(); }
    virtual ScriptVariable Text() const = 0;
};

//! The simplest possible case: fixed name, fixed value
class ScriptMacroConst : public ScriptMacroVariable {
    ScriptVariable val;
public:
    ScriptMacroConst(const ScriptVariable &name,
                     const ScriptVariable &value)
        : ScriptMacroVariable(name), val(value) {}
    ~ScriptMacroConst() {}
private:
    virtual ScriptVariable Text() const { return val; }
};

//! Substitute a var of given name with the value of a ScriptVariable
/*! The ScriptVariable you give here can change whenever you want, so
    the value to be substituted will change, too.
    \warning Make sure the object of ScriptVariable that you use here
    remains existing all the time the object of ScriptMacroScrVar
    exists, or else full degree of chaos is guaranteed.
 */
class ScriptMacroScrVar : public ScriptMacroVariable {
    ScriptVariable *val;
public:
    ScriptMacroScrVar(const ScriptVariable &name, ScriptVariable *pval)
        : ScriptMacroVariable(name), val(pval) {}
    ~ScriptMacroScrVar() {}
private:
    virtual ScriptVariable Text() const { return *val; }
};

//! The 'dictionary' substitution
/*! The dictionary is represented by a ScriptVector of even length,
    the 0th, 2nd, 4th... items being names to substitute, and the
    1st, 3rd, 5th... to be the text to substitute.  The macro call
    should have the form %name:key%, %[name:key] or ${name:key}.

    The ScriptMacroDictionary object can either make a copy of
    your vector, or (if you wish so) assume the vector remains existing
    somewhere else, and there's no need for a copy.
*/
class ScriptMacroDictionary : public ScriptMacroprocessorMacro {
    const class ScriptVector *the_dict;
    bool vec_owned;
public:
    ScriptMacroDictionary(const ScriptVariable &name,
                          const ScriptVector &dict, bool copy = true);
    ~ScriptMacroDictionary();
    ScriptVariable Expand(const ScriptVector &args) const;
};


#endif
