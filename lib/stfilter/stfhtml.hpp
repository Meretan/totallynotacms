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




#ifndef STFHTML_HPP_SENTRY
#define STFHTML_HPP_SENTRY

#include "stfilter.hpp"
#include "stfencod.hpp"

//! Filter off HTML tags that aren't explicitly allowed
class StreamFilterHtmlTags : public StreamFilter {
    const char * const * names;
    const char * const * pre_tags;
    enum {
        start, just_started_tag, unnamed_tag, tag_name,
        allowed_tag, prohibited_tag,
        comment_in_excl, /* seen ``<!'' */
        comment_in_dash, /* seen ``<!-'' */
        comment,         /* inside the comment */
        comment_out_dash, /* seen ``-'' */
        comment_out_dash2 /* seen ``--'' */
    };
    enum { unquoted, single_quoted, double_quoted };
    char astate, qstate;    // astate <- start... ; qstate <- unquoted...
    bool negtag;
        /* qstate can only become anything other than unquoted within the
           tags AND when the tag name is known, that is, in allowed_tag
           and progibited_tag states;
           in case qstate!=unquoted, the astate is simply ignored until
           the matching quote is found and qstate is unquoted again
         */
    char *namebuf;
    int namebufsize, namebufidx;

    int pre_level;
public:
        //! The constructor
        /*! \param allowed_names must be a NULL-terminated array of string
            ptrs; neither the array nor the strings are copied nor owned;
            make sure they exist all the time the object is in use,
            and that they don't change after the filtering is started
            (but it's ok to change them between the construction of the
            object and the first time the FeedChar method is called).
            This is because the object calculates the length of the
            longest name and only stores that many chars of the analysed
            name; if the name is longer, the tag is assumed prohibited.
            NULL means all tags are allowed (makes sense if we're going
            to call AddControlledNLReplacer so nothing will be filtered off
            but the object will still tell the NLReplacer where not to
            replace NLs); to prohibit all tags, pass a pointer to a NULL
            pointer (that is, an array of zero length, as opposite to NULL).
          */
    StreamFilterHtmlTags(const char * const * allowed_names,
                         StreamFilter *next);
    ~StreamFilterHtmlTags();

    void FeedChar(int c);
    void FeedEnd();
    void Reset();

    void AddControlledNLReplacer(const char * const *pre_tags, bool texstyle);

private:
    void HandleStart(int c);
    void HandleJustStartedTag(int c);
    void HandleUnnamedTag(int c);
    void HandleTagName(int c);
    void HandleAllowedTag(int c);
    void HandleProhibitedTag(int c);
    void HandleCommentInExcl(int c);
    void HandleCommentInDash(int c);
    void HandleComment(int c);
    void HandleCommentOutDash(int c);
    void HandleCommentOutDash2(int c);

    void ProcessNameChar(int c);
    void ProcessUnexpectedNameChar(int c);
    void ProcessCharInsideTag(int c);

    void AllocateNameBuf();
    void CheckTagName();
    void ChangeNLModeIfNecessary();
};


//! Turn NLs into paragraph breaks.
/*! This filter can work in two modes, the default and the TeX-style.
    By default, it replaces a single NL with ``<br />'', and an empty
    line (that is, two NLs separated with nothing but spaces) with
    a new paragraph; actually, ``<p>'' is being placed in front of
    the first non-space char, the ``</p>\n'' is placed in the end of
    the stream, and for NLs, if there's the second one, ``</p>\n<p>''
    is transmitted, otherwise ``<br />'' is out.  All subsequent
    NLs are ignored, as well as all whitespace beetween them.
    In TeX-style, only empty lines are taken into account, which
    effectively means no ``<br />''s are output, and for <p>/</p>
    the behaviour is the same.
    \note In case a '\r' (CR) char is encountered at least once,
    all the CRs are ignored, but a CR is transmitted in front of
    each NL.
 */
class StreamFilterHtmlReplaceNL : public StreamFilter {
    bool texstyle;
    enum {
        start, plain, after_single_nl, after_two_nls, disabled
    };
    int state;
    bool out_cr;
public:
    StreamFilterHtmlReplaceNL(bool texs, StreamFilter *next)
        : StreamFilter(next), texstyle(texs), state(start), out_cr(false) {}
    ~StreamFilterHtmlReplaceNL() {}

    void FeedChar(int c);
    void FeedEnd();
    void Reset();

    void Enable();
    void Disable();
private:
    void PutNL();   //!< output [<CR>]<LF>
};


//! Replace <, > and & with HTML entites
class StreamFilterHtmlProtect : public StreamFilter {
public:
    StreamFilterHtmlProtect(StreamFilter *next)
        : StreamFilter(next) {}
    ~StreamFilterHtmlProtect() {}

    void FeedChar(int c);
};

//! Decode utf8 replacing unknown chars with HTML entities
class StreamFilterUtf8ToHtml : public StreamFilterUtf8ToExtAscii {
public:
    StreamFilterUtf8ToHtml(const int * const *table, StreamFilter *next)
        : StreamFilterUtf8ToExtAscii(table, next) {}

    void UnknownCode(int code);
};

#endif
