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




#ifndef SCRIPTPP_SCRVAR_HPP_SENTRY
#define SCRIPTPP_SCRVAR_HPP_SENTRY

/*! \file scrvar.hpp
    \brief This file introduces the ScriptVariable class
 */


//! Script Variable (a universal string)
/*! This class implements a common notion of a string.
    In fact, it could be a good replacement for the 'standard' string class,
    and it even has some methods invented for compatibility reasons only.
    You shouldn't, however, expect it to be totally compatible with the
    string class; there was no intention to maintain such a compatibility.
   \par
    The copy-on-write technology is implemented. The class itself has
    only one data member (a pointer) so it is Ok to pass ScriptVariable
    objects by value.
   \par
    The main architectural difference is that the class itself doesn't have
    any methods to manipulate substrings (like erase(), replace() etc).
    Instead, there is a nested class named ScriptVariable::Substring, which
    provides the appropriate methods.
   \par
    The author's intention was to provide the operations required
    more often than the others. These are (1) various tokenization services
    (including iteration by tokens and/or by words) and (2) conversions
    to/from numbers, both floats and ints.
   \note
    There are two terms, namely word and token, which are widely used in
    this document. It is important to understand the difference. If a string
    is broken by some delimiters, and everything between any two
    delimiters is to be extracted, it is called TOKEN. For example,
    colon-separated lines such as in /etc/passwd are tokenized lines.
    If, by contrast, two items within a string can be delimited not by
    exactly one, but with any number of delimiting characters (typically
    whitespace), then such items are called WORDS. Consider the string
    "aaa::bbb::ccc::ddd". If ":" is the only delimiter, then the string
    has 7 tokens ("aaa", "", "bbb", "", "ccc", "", "ddd") and only 4
    words ("aaa", "bbb", "ccc", "ddd").
   \note
    There's also a notion of an 'invalid' ScriptVariable object
    (internally it is represented by NULL pointer).
 */
class ScriptVariable {
    struct ScriptVariableImplementation *p;
public:
        //! Default constructor
        /*! Creates an empty string */
    ScriptVariable();
        //! Constructor from a raw memory (not 0-terminated)
        /*!
         */
    ScriptVariable(const char *p, int len);
        //! The copy constructor
        /*! \note
            The string itself is not copied until one of its users tries
            to modify it.
         */
    ScriptVariable(const ScriptVariable &other);
        //! Cast constructor
        /*! This constructor allows to create a ScriptVariable having a
            traditional C zero-terminated string.  This form of the
            constructor is also able to create an 'invalid' object;
            pass the NULL pointer to do so.
         */
    ScriptVariable(const char *s);
        //! Sprintf constructor
        /*! Allows to create a ScriptVariable representing a result of
            processing a format string just like with sprintf(3).
            In fact, vsnprintf(3) is used to fill the string.
           \param len is a length hint. The main goal of this parameter
            is, however, just to allow to tell this constructor call from
            the ScriptVariable(const char *) one. Just pass zero len if
            you don't want to think.
           \param format is a format string. See printf(3) for the format
            string documentation.
           \warning Using this form of the constructor will suck in a lot
            of glibc code which may be not what you want.  The
            implementation of this constructor resides in a separate
            file (scrvar_x.cpp) so if you don't use it, the printf family
            implementation will hopefully stay away from your binary.
         */
    ScriptVariable(int len, const char *format, ...);

    ~ScriptVariable();

    bool IsValid() const { return p; }
    bool IsInvalid() const { return !p; }
    void Invalidate() { Unlink(); }

        //! Make room for at least len chars
        /*! This method makes sure the underlying array has at least
            len+1 element, so it can hold strings up to len chars
            long.  If the object is 'invalid', it becomes an empty
            string.  If the object already has len+1 or more elements,
            it is left untouched.  Otherwise, the string will be copied
            to a newly-allocated underlying storage.
         */
    void MakeRoom(int len);

        //! Current string length
    int Length() const;
        //! Current string length
        /*! string class compatibility name */
    int length() const { return Length(); }

        //! Convert to a traditional C string.
    const char *c_str() const;

        //! Access the particular character (read only)
    char operator[](int i) const;
        //! Access the particular character (assignment is allowed)
    char& operator[](int i);

        //! strcmp(3) functionality
        /*! \warning returns 0 if either of strings is invalid */
    int Strcmp(const ScriptVariable &o2) const;
        //! strcasecmp(3) functionality
        /*! \warning returns 0 if either of strings is invalid */
    int Strcasecmp(const ScriptVariable &o2) const;

        //! Convert ascii latin letters to upper case
    const ScriptVariable& Toupper();
        //! Convert ascii latin letters to lower case
    const ScriptVariable& Tolower();

        //! Convert all chars to upper case using locale
    const ScriptVariable& ToupperLC();
        //! Convert all chars to lower case using locale
    const ScriptVariable& TolowerLC();

        //! Concatenation
    ScriptVariable operator+(const char *o2) const;
        //! Appending
    ScriptVariable& operator+=(const char *o2);
        //! Assignment
    ScriptVariable& operator=(const char *o2);
        //! Concatenation
    ScriptVariable operator+(char c) const;
        //! Appending
    ScriptVariable& operator+=(char c);
        //! Assignment
    ScriptVariable& operator=(char c);
        //! Concatenation
    ScriptVariable operator+(const ScriptVariable &o2) const;
        //! Appending
    ScriptVariable& operator+=(const ScriptVariable &o2);
        //! Assignment
    ScriptVariable& operator=(const ScriptVariable &o2);

    bool operator==(const ScriptVariable &o2) const
        { return IsValid() ? (Strcmp(o2) == 0) : o2.IsInvalid(); }
    bool operator!=(const ScriptVariable &o2) const
        { return IsValid() ? (Strcmp(o2) != 0) : o2.IsValid(); }
    bool operator<(const ScriptVariable &o2) const
        { return Strcmp(o2) < 0; }
    bool operator>(const ScriptVariable &o2) const
        { return Strcmp(o2) > 0; }
    bool operator<=(const ScriptVariable &o2) const
        { return IsValid() && o2.IsValid() && Strcmp(o2) <= 0; }
    bool operator>=(const ScriptVariable &o2) const
        { return IsValid() && o2.IsValid() && Strcmp(o2) >= 0; }

    bool HasPrefix(const char *prefix) const;
    bool HasPrefix(const ScriptVariable& prefix) const;
    bool HasSuffix(const char *suffix) const;
    bool HasSuffix(const ScriptVariable& suffix) const;

    ScriptVariable& Trim(const char *spaces = " \t\n\r");

    class Substring {
        friend class ScriptVariable;
    protected:
        ScriptVariable *master;
        int pos;
        int len;
    public:
        Substring() : master(0), pos(0), len(-1) {}
            /*!
                 \par pos starting position; zero means the begin
                      of the string, positive value is the offset from the
                      begin, negative value is the offset from the end
                      of the string.
             */
        Substring(ScriptVariable &master, int pos=0, int len=-1);
        void Erase();
        void Replace(const char *what);
        ScriptVariable Get() const;
        char operator[](int i) const { return (*master)[i+pos]; }
        bool operator==(const Substring &o) const
            { return master == o.master && pos == o.pos && len == o.len; }

        Substring Before() const;
        Substring After() const;

        bool FetchWord(Substring &word, const char *spaces = " \t\n\r");
        bool FetchToken(Substring &token,
                        const char *delimiters = ",",
                        const char *trim_spaces = 0);

        Substring FetchWord(const char *spaces = " \t\n\r");
        Substring FetchToken(const char *delimiters = ",",
                             const char *trim_spaces = 0);

        Substring& Move(int delta);
        Substring& Resize(int delta);

        Substring& ExtendToBegin();
        Substring& ExtendToEnd();

        Substring& SetLength(int newlength);

           /*! Trims the substring and returns *this */
        Substring& Trim(const char *spaces = " \t\n\r");

        const ScriptVariable& Master() const { return *master; }
        int Index() const { return pos; }
        int Length() const { return len; }

        bool IsValid() const
            { return master && (len >= 0) && master->IsValid(); }
        bool IsInvalid() const
            { return !master || len < 0 || master->IsInvalid(); }
        void Invalidate() { master = 0; len = -1; }
    };
    friend class ScriptVariable::Substring;

    class Iterator : public Substring {
        bool just_started;
    public:
        Iterator(ScriptVariable &master) : Substring(master, 0, 0)
            { just_started = true; }
        bool NextWord(const char *spaces = " \t\n\r");
        bool NextToken(const char *delimiters = ",",
                       const char *trim_spaces = 0);
        bool PrevWord(const char *spaces = " \t\n\r");
        bool PrevToken(const char *delimiters = ",",
                       const char *trim_spaces = 0);
    };
    friend class ScriptVariable::Iterator;

    Substring Range(int pos, int len=-1)
       { return Substring(*this, pos, len); }

    Substring Whole() { return Range(0,-1); }

    Substring Strchr(int c);
    Substring Strrchr(int c);
    Substring Strstr(const char *str);
    Substring Strrstr(const char *str);

        /*! \param radix = 0 means C-style 0x, 0, etc... */
    bool GetLong(long &l, int radix = 0) const;
        /*! \param radix = 0 means C-style 0x, 0, etc... */
    bool GetLongLong(long long &l, int radix = 0) const;
    bool GetDouble(double &d) const;

    bool GetRational(long &p, long &q) const;

private:
    void Unlink();
    void Assign(ScriptVariableImplementation *q);
    void Create(int len);
    void EnsureOwnCopy();
};


class ScriptVariableInv : public ScriptVariable {
public:
    ScriptVariableInv() : ScriptVariable((char*)0) {}
    template <class T> ScriptVariable& operator=(T t)
        { return ScriptVariable::operator=(t); }
};


//! String representation of numbers
/*!
    \warning Conversions from floating point types are implemented
      using the ``printf-style'' constructor of the ScriptVariable
      class, which, in turn, sucks in a lot of library code (all the
      [vsf]printf implementation).  However, all constructors that
      accept floating-point numbers are implemented in the scrvar_x
      module so if you don't use them, you will not get the *printf
      code.  Conversions from integral types are implemented using no
      features from external libraries.
 */
class ScriptNumber : public ScriptVariable {
public:
    ScriptNumber(short int i);
    ScriptNumber(unsigned short int i);
    ScriptNumber(int i);
    ScriptNumber(unsigned int i);
    ScriptNumber(long i);
    ScriptNumber(unsigned long int i);
    ScriptNumber(long long int i);
    ScriptNumber(unsigned long long int i);
    ScriptNumber(float f);
    ScriptNumber(double f);
    ScriptNumber(long double f);
    ScriptNumber(float f, int pre);
    ScriptNumber(double f, int pre);
    ScriptNumber(long double f, int pre);
};


//! Underlying implementation
/*! \warning This is NOT intended to be used by user programs directly!
    It is only here because two .cpp files (not one) need it.
 */
struct ScriptVariableImplementation {
    int refcount;
    int maxlen;  // buf[maxlen] may still be accessed but is always 0
    int len_cached; // -1 means no info, strlen must be used
    char buf[1];
};

#endif
