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




#ifndef SCRIPTPP_SCRVECT_HPP_SENTRY
#define SCRIPTPP_SCRVECT_HPP_SENTRY

/*! \file scrvect.hpp
    \brief This file introduces the ScriptVector class
 */


//! Dynamically resized array of strings (ScriptVariable objects)
/*! This class represents an array of strings.  The array can be
    built up either by adding items to it or by tokenizing a given
    string.  See the ScriptVariable class description for explanation
    of tokens vs. words.
 */
class ScriptVector {
    class ScriptVariable *vec;
    int len;
    int maxlen;
public:
    ScriptVector();
        /*!
        This form of the constructor takes the source ScriptVariable
        and produces the vector of words (or tokens) it contains.
        \param sv the string to parse
        \param delims chars to consider delimiters. If it is
               0, the vector of one string is produced and no
               separation is done.
        \param trimspaces controls the mode of the separation.
               If delims is not 0 and trimspaces is 0, then
               the string is divided down to words, that is,
               several delimiters one after other considered
               just like one delimiter, and there can be no empty tokens.
               If trimspaces is not 0, the string is separated
               down to tokens, that is, two delimiters produce three
               tokens, even if all the tokens are empty; if the token
               begins and/or ends with characters found in the
               string pointed to by trimspaces, they get removed.
        \par Rationale here is simple. If you need words (as opposit
               to tokens), then you don't need trimspaces -- you
               can just add all the unwanted symbols to the set of
               delimiters and they'll go away.
        \par So, once you specify trimspaces separately, then you
               definitely mean to break the string to tokens not words.
        \par If you don't need to trim anything but still want tokens
               not words, then specify "" instead of 0.
        \note the default behaviour is to separate the string to
              words using whitespace as such.
        \note For example, you could find the following useful to break
              down comma-separated lists:
                         ScriptVector vect(sv, ",", " \t\n");
              And the following might be good to parse passwd-like file:
                         ScriptVector vect(sv, ":", "\n");
              (we trim off the "\n" in case it is read in by fgets())
              Finally, this might be good if you're sure there's no "\n":
                         ScriptVector vect(sv, ":", "");
        \note It might be not a bad idea to use ScriptWordVector and
              ScriptTokenVector subclasses for clarity.
         */
    ScriptVector(const ScriptVariable &sv,
                 const char *delims = " \t\r\n",
                 const char *trimspaces = 0);

        /*! This form of the constructor takes a 0-terminated array of
            char* pointers and produces the vector of their copies.
            \param v the vector pointer
         */
    ScriptVector(const char * const *v);

        /*! This form of constructor takes a part of another vector;
            By default, it just copies the other vector so it is a
            copy constructor as well
         */
    ScriptVector(const ScriptVector &other, int idx = 0, int len = -1);

        /*! Destructor */
    ~ScriptVector();

    const ScriptVector& operator=(const ScriptVector &other);


    ScriptVariable& operator[](int i);
    ScriptVariable operator[](int i) const;

    void Insert(int idx, const ScriptVariable& sv);
    void Insert(int idx, const ScriptVector& svec);
    void Remove(int idx, int amount = 1);
    void AddItem(const ScriptVariable& sv);

        /*! Remove all items */
    void Clear();

        /*! Join the elements of the vector back to a single string.
            \param separator If separator is given, it is inserted
                   between each two elements.
            \param idx is the starting index
            \param len is how many elements are to be joined;
                   -1 means "to the end of the vector"
         */
    ScriptVariable Join(const char *separator = 0,
                        int idx = 0, int len = -1) const;

        /*! How many elements does the vector contain?  */
    int Length() const { return len; }

        /*! Creates a C data structure known as 'argv' (suitable
            to be passed to, e.g., execv(3))
            \note The data is dynamically assigned using unspecified
            technique (that is, we reserve the right to change the
            technique) so the only correct way to dispose the
            structure is to use the DeleteArgv() method.
         */
    char** MakeArgv() const;

        /*! Disposes a data structure created by MakeArgv() */
    static void DeleteArgv(char** argv);

private:
    void DoInsert(int idx, const ScriptVariable *vars, int n);
    void ProvideVectorLength(int i);
};

//! ScriptVector built by splitting a string down to words
class ScriptWordVector : public ScriptVector {
public:
    ScriptWordVector(const ScriptVariable &v, const char *delims = " \t\r\n")
        : ScriptVector(v, delims) {}
};

//! ScriptVector built by splitting a string down to tokens
class ScriptTokenVector : public ScriptVector {
public:
    ScriptTokenVector(const ScriptVariable &v,
                      const char *delims, const char *trim = 0)
        : ScriptVector(v, delims, trim ? trim : "") {}
};


#endif
