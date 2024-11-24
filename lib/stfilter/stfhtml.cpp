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




#include <string.h>    // for strlen, str[i]cmp

#include "stfhtml.hpp"


StreamFilterHtmlTags::
StreamFilterHtmlTags(const char * const * anames, StreamFilter *next)
    : StreamFilter(next), names(anames), pre_tags(0),
    astate(start), qstate(unquoted), namebuf(0), namebufsize(0)
{
}

StreamFilterHtmlTags::~StreamFilterHtmlTags()
{
    if(namebuf)
        delete[] namebuf;
}

// please note we don't use isalpha(3) function family
// as well as str[n]casecmp(3)
// so that we don't suck in the damn locale infrastructure

static int isspc(int c)
{
    return c==' ' || c=='\t' || c=='\r' || c=='\n' || c =='\v' || c=='\f';
}

static int is_tagnamechar(int c)
{
    return
        (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') ||
        c == '_' || c == ':' || c == '.';
}

static int lowcase(int c)
{
    return (c >= 'A' && c <= 'Z') ? c - 'A' + 'a' : c;

}

void StreamFilterHtmlTags::FeedChar(int c)
{
#if 0
    if((qstate != unquoted || astate != start) && isspc(c))
        c = ' ';
#endif
    if(qstate != unquoted) {
        if(astate == allowed_tag)
            PutChar(c);
        if((qstate == single_quoted && c == '\'') ||
            (qstate == double_quoted && c == '\"'))
        {
            qstate = unquoted;
        }
        return;
    }
    // once we're here, this means we're in unquoted state
    switch(astate) {
    case start:              HandleStart(c); break;
    case just_started_tag:   HandleJustStartedTag(c); break;
    case unnamed_tag:        HandleUnnamedTag(c); break;
    case tag_name:           HandleTagName(c); break;
    case allowed_tag:        HandleAllowedTag(c); break;
    case prohibited_tag:     HandleProhibitedTag(c); break;
    case comment_in_excl:    HandleCommentInExcl(c); break;
    case comment_in_dash:    HandleCommentInDash(c); break;
    case comment:            HandleComment(c); break;
    case comment_out_dash:   HandleCommentOutDash(c); break;
    case comment_out_dash2:  HandleCommentOutDash2(c); break;
    }
}

void StreamFilterHtmlTags::FeedEnd()
{
    if(astate == allowed_tag) {
        switch(qstate) {
        case single_quoted:
            PutChar('\'');
            break;
        case double_quoted:
            PutChar('\"');
            break;
        case unquoted:
            ;
        }
        PutChar('>');
    }
    PutEnd();
}

void StreamFilterHtmlTags::Reset()
{
    astate = start;
    qstate = unquoted;
}

void StreamFilterHtmlTags::
AddControlledNLReplacer(const char * const *pt, bool texstyle)
{
    pre_tags = pt;
    StreamFilterHtmlReplaceNL *nlr =
        new StreamFilterHtmlReplaceNL(texstyle, 0);
    InsertAfter(nlr);
    pre_level = 0;
}

void StreamFilterHtmlTags::HandleStart(int c)
{
    if(c == '<') {
        astate = just_started_tag;
        negtag = false;
        namebufidx = 0;
            /* We don't make the buffer a correct string (z-term.) here
               because the buffer may still be unallocated.  It will
               become a valid string when the first char is processed.
             */
    } else
        PutChar(c);
}

void StreamFilterHtmlTags::HandleJustStartedTag(int c)
{
    if(c == '!') {
        astate = comment_in_excl;
    } else
    if(c == '/' || is_tagnamechar(c)) {
        astate = unnamed_tag;
        HandleUnnamedTag(c);
    } else {
        PutStr("&lt;");
        PutChar(c);
        astate = start;
    }
}

static bool tagname_eq(const char *a, const char *b)
{
    while(*a && *b) {
        if(lowcase(*a) != lowcase(*b))
            return 0;
        a++;
        b++;
    }
    return !*a && !*b;
}

static bool tagname_is_there(const char *name, const char * const *array)
{
    int i;
    for(i = 0; array[i]; i++)
        if(tagname_eq(array[i], name))
             return true;
    return false;
}

void StreamFilterHtmlTags::HandleUnnamedTag(int c)
{
    switch(c) {
    case '/':
        if(negtag)  // more than one '/' before the name is too much
            astate = prohibited_tag;
        else
            negtag = true;
        break;
    case '>':
        astate = start;  // just ignore the malformed tag
        break;
    default:
        if(isspc(c))
            break;
        if(is_tagnamechar(c)) {
            astate = tag_name;
            ProcessNameChar(c);
        } else {
            ProcessUnexpectedNameChar(c);
        }
    }
}

void StreamFilterHtmlTags::HandleTagName(int c)
{
    // correct delimiter after the name may be either space, or '>', or "/>"
    if(isspc(c) || c == '>' || c == '/') {
        CheckTagName(); // this will change the state as appropriate
                        // AND print the tag beginning if needed
        if(astate == allowed_tag)
            PutChar(c);
        if(c == '>') {
            if(pre_tags && negtag)
                ChangeNLModeIfNecessary();
            astate = start;
        }
    } else
    if(is_tagnamechar(c))
        ProcessNameChar(c);
    else
        ProcessUnexpectedNameChar(c);
}

void StreamFilterHtmlTags::ProcessNameChar(int c)
{
    if(!namebuf)
        AllocateNameBuf();

    if(namebufidx+2 > namebufsize) {
        if(names) {
            // well, there are no such lenghty tags among the allowed
            // or the char is not allowed
            ProcessUnexpectedNameChar(c);
            // the state is changed by the P.Unexpected.NC
        } else {
            // all tags are allowed, including such lenghty
            // but we can't add more chars
            PutChar('<');
            if(negtag)
                PutChar('/');
            PutStr(namebuf);
            PutChar(c);
            astate = allowed_tag;
        }
        return;
    }
    namebuf[namebufidx] = c;
    namebuf[namebufidx+1] = 0;
    namebufidx++;
}

void StreamFilterHtmlTags::ProcessUnexpectedNameChar(int c)
{
    // the tag is to be skipped
    astate = prohibited_tag;
    // may be a quotation or even the end of the tag
    ProcessCharInsideTag(c);
}

void StreamFilterHtmlTags::HandleAllowedTag(int c)
{
    PutChar(c);
    ProcessCharInsideTag(c);
}

void StreamFilterHtmlTags::HandleProhibitedTag(int c)
{
    if(qstate == unquoted && c == '\n') {
        PutChar('\n');
        astate = start;
        return;
    }
    ProcessCharInsideTag(c);
}

void StreamFilterHtmlTags::HandleCommentInExcl(int c)
{
    if(c == '-') {
        astate = comment_in_dash;
    } else {
        // this is incorrect comment start! let's handle this... somehow
        PutStr("&lt;!");
        PutChar(c);
        astate = start;
    }
}

void StreamFilterHtmlTags::HandleCommentInDash(int c)
{
    if(c == '-') {
        astate = comment;
    } else {
        // this is incorrect comment start! let's handle this... somehow
        PutStr("&lt;!-");
        PutChar(c);
    }
}

void StreamFilterHtmlTags::HandleComment(int c)
{
    if(c == '-') {
        astate = comment_out_dash;
    }
}

void StreamFilterHtmlTags::HandleCommentOutDash(int c)
{
    if(c == '-') {
        astate = comment_out_dash2;
    } else {
        astate = comment;
    }
}

void StreamFilterHtmlTags::HandleCommentOutDash2(int c)
{
    switch(c) {
    case '>':
        astate = start;
        break;
    case '-':   // well, let's just remain waiting for ``-->''
                // actually, this may be smth. like ``--------->''
        break;
    default:    // failed to quit the comment :-)
        astate = comment;
    }
}


void StreamFilterHtmlTags::ProcessCharInsideTag(int c)
{
    if(c == '\'')
        qstate = single_quoted;
    else
    if(c == '\"')
        qstate = double_quoted;
    else
    if(c == '>') {
        if(pre_tags && negtag)
            ChangeNLModeIfNecessary();
        astate = start;
    }
}

void StreamFilterHtmlTags::AllocateNameBuf()
{
    int maxlen = 0;
    if(names) {
        int i, n;
        for(i = 0; names[i]; i++) {
            n = strlen(names[i]);
            if(n > maxlen)
                maxlen = n;
        }
    }
    if(pre_tags) {
        int i, n;
        for(i = 0; pre_tags[i]; i++) {
            n = strlen(pre_tags[i]);
            if(n > maxlen)
                maxlen = n;
        }
    }
    namebufsize = maxlen+1;
    namebuf = new char[namebufsize];
    namebufidx = 0;
}



void StreamFilterHtmlTags::CheckTagName()
{
    if(!names || tagname_is_there(namebuf, names)) {
        if(pre_tags && !negtag)
            ChangeNLModeIfNecessary();
        astate = allowed_tag;
        PutChar('<');
        if(negtag)
            PutChar('/');
        PutStr(namebuf);
    } else {
        astate = prohibited_tag;
    }
}

void StreamFilterHtmlTags::ChangeNLModeIfNecessary()
{
    if(tagname_is_there(namebuf, pre_tags)) {
        if(negtag) {
            if(pre_level > 0)
                pre_level--;
            if(pre_level == 0) {
                static_cast<StreamFilterHtmlReplaceNL*>(GetNext())->Enable();
            }
        } else {
            if(pre_level == 0) {
                static_cast<StreamFilterHtmlReplaceNL*>(GetNext())->Disable();
            }
            pre_level++;
        }
    }
}

void StreamFilterHtmlReplaceNL::FeedChar(int c)
{
    if(c == '\r') {
        out_cr = true;   // yes, we will transmit CRs for all NLs, ...
        return;          // but we ignore them in the text :-)
    }
    switch(state) {
    case start:
        if(isspc(c))  // we simply skip all the leading whitespace
            break;
        // non-whitespace found!
        PutStr("<p>");
        PutChar(c);
        state = plain;
        break;
    case plain:
        if(c == '\n') {
            state = after_single_nl;
            break;
        }
        PutChar(c);
        break;
    case after_single_nl:
        if(c == '\n')
            state = after_two_nls;
        else
        if(!isspc(c)) {
            if(!texstyle)
                PutStr("<br />");
            PutNL();
            PutChar(c);
            state = plain;
        }
        break;
    case after_two_nls:
        if(!isspc(c)) {
            PutStr("</p>");
            PutNL();
            PutStr("<p>");
            PutChar(c);
            state = plain;
        }
        break;
    case disabled:
        PutChar(c);
    };
}

void StreamFilterHtmlReplaceNL::FeedEnd()
{
    if(state != start)   // at least one char was output!
        PutStr("</p>\n");

    /* the only ``unfinished'' situation is after 1 NL, but, well,
       we don't want any trailing spaces anyway
     */

    PutEnd();
}

void StreamFilterHtmlReplaceNL::Reset()
{
    state = start;
}

void StreamFilterHtmlReplaceNL::PutNL()
{
    if(out_cr)
        PutChar('\r');
    PutChar('\n');
}


void StreamFilterHtmlProtect::FeedChar(int c)
{
    switch(c) {
    case '>': PutStr("&gt;"); break;
    case '<': PutStr("&lt;"); break;
    case '&': PutStr("&amp;"); break;
    default:  PutChar(c);
    }
}

void StreamFilterHtmlReplaceNL::Enable()
{
    state = start;
}

void StreamFilterHtmlReplaceNL::Disable()
{
    if(state != start)   // at least one char was output!
        PutStr("</p>\n");
    state = disabled;
}


///////////////////////////////////////////////////////////////////////
// boring tables follow

/* the following tables cover up the whole set of HTTP4 (!) entities,
   as listed at https://www.w3schools.com/charsets/ref_html_entities_4.asp

   However, three small sections are excluded intentionally to reduce the
   search time, so &fnof;, &loz;, &spades;, &clubs;, &hearts; and &diams;
   are left aside; if you need them, remove the appropriate "#if 0"s
   below.

   Please note that HTML5 entities are not in the list; there are too
   many of them, and HTML5 itself is just another comitee-born bastard.
 */

static const char *html_entities_a0[96] = {
    "nbsp", "iexcl", "cent", "pound", "curren", "yen", "brvbar", "sect",
    "uml", "copy", "ordf", "laquo", "not", "shy", "reg", "macr",
    "deg", "plusmn", "sup2", "sup3", "acute", "micro", "para", "middot",
    "cedil", "sup1", "ordm", "raquo", "frac14", "frac12", "frac34", "iquest",
    "Agrave", "Aacute", "Acirc", "Atilde", "Auml", "Aring", "AElig", "Ccedil",
    "Egrave", "Eacute", "Ecirc", "Euml", "Igrave", "Iacute", "Icirc", "Iuml",
    "ETH", "Ntilde", "Ograve", "Oacute", "Ocirc", "Otilde", "Ouml", "times",
    "Oslash", "Ugrave", "Uacute", "Ucirc", "Uuml", "Yacute", "THORN", "szlig",
    "agrave", "aacute", "acirc", "atilde", "auml", "aring", "aelig", "ccedil",
    "egrave", "eacute", "ecirc", "euml", "igrave", "iacute", "icirc", "iuml",
    "eth", "ntilde", "ograve", "oacute", "ocirc", "otilde", "ouml", "divide",
    "oslash", "ugrave", "uacute", "ucirc", "uuml", "yacute", "thorn", "yuml"
};
#if 0
static const char *html_entities_192[1] = {
    "fnof"
};
#endif
static const char *html_entities_391[70] = {
    "Alpha", "Beta", "Gamma", "Delta", "Epsilon", "Zeta", "Eta", "Theta",
    "Iota", "Kappa", "Lambda", "Mu", "Nu", "Xi", "Omicron", "Pi",
    "Rho", 0, "Sigma", "Tau", "Upsilon", "Phi", "Chi", "Psi",
    "Omega", 0, 0, 0, 0, 0, 0, 0,
    "alpha", "beta", "gamma", "delta", "epsilon", "zeta", "eta", "theta",
    "iota", "kappa", "lambda", "mu", "nu", "xi", "omicron", "pi",
    "rho", "sigmaf", "sigma", "tau", "upsilon", "phi", "chi", "psi",
    "omega", 0, 0, 0, 0, 0, 0, 0,
    "thetasym", "upsih", 0, 0, 0, "piv"
};
static const char *html_entities_2022[35] = {
    "bull", 0, 0, 0, "hellip", 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    "prime", "Prime", 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, "oline", 0, 0, 0,
    0, 0, "frasl"
};
static const char *html_entities_2111[255] = {
    "image", 0, 0, 0, 0, 0, 0, "weierp",
    0, 0, 0, "real", 0, 0, 0, 0,
    0, "trade", 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, "alefsym", 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, "larr",
    "uarr", "rarr", "darr", "harr", 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, "crarr", 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, "lArr",
    "uArr", "rArr", "dArr", "hArr", 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, "forall",
    0, "part", "exist", 0, "empty", 0, "nabla", "isin",
    "notin", 0, "ni", 0, 0, 0, "prod"
};
static const char *html_entities_2211[282] = {
    "sum", "minus", 0, 0, 0, 0, "lowast", 0,
    0, "radic", 0, 0, "prop", "infin", 0, "ang",
    0, 0, 0, 0, 0, 0, "and", "or",
    "cap", "cup", "int", 0, 0, 0, 0, 0,
    0, 0, 0, "there4", 0, 0, 0, 0,
    0, 0, 0, "sim", 0, 0, 0, 0,
    0, 0, 0, 0, "cong", 0, 0, "asymp",
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, "ne",
    "equiv", 0, 0, "le", "ge", 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, "sub", "sup", "nsub", 0, "sube", "supe", 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, "oplus", 0, "otimes", 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, "perp", 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, "sdot", 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, "lceil",
    "rceil", "lfloor", "rfloor", 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    "lang", "rang"
};
#if 0
static const char *html_entities_25ca[1] = {
    "loz"
};
static const char *html_entities_2660[7] = {
    "spades", 0, 0, "clubs", 0, "hearts", "diams"
};
#endif
struct html_entity_section {
    int base, len;
    const char **names;
};
static struct html_entity_section html_entity_table[] = {
    { 0x000a0,  96, html_entities_a0 },
#if 0
    { 0x00192,   1, html_entities_192 },
#endif
    { 0x00391,  70, html_entities_391 },
    { 0x02022,  35, html_entities_2022 },
    { 0x02111, 255, html_entities_2111 },
    { 0x02211, 282, html_entities_2211 },
#if 0
    { 0x025ca,   1, html_entities_25ca },
    { 0x02660,   7, html_entities_2660 },
#endif
    { 0, 0, 0 }
};

// end of boring tables
///////////////////////////////////////////////////////////////////////

static const char *get_html_entity(int code)
{
    struct html_entity_section *p;
    for(p = html_entity_table; p->names; p++)
        if(code >= p->base && code < p->base + p->len)
            return p->names[code - p->base];
    return 0;
}

void StreamFilterUtf8ToHtml::UnknownCode(int code)
{
    const char *e = get_html_entity(code);
    if(e) {
        PutChar('&');
        PutStr(e);
        PutChar(';');
    } else {
        PutStr("&#x");
        PutHex(code, 0, true);
        PutChar(';');
    }
}
