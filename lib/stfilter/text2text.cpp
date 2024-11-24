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




#include <stdio.h>
#include <string.h>
#include "stfilter.hpp"
#include "stfencod.hpp"
#include "stfhtml.hpp"
#include "version.h"


class StreamFilterOutput : public StreamFilter {
    FILE *f;
public:
    StreamFilterOutput() : StreamFilter(0), f(stdout) {}
    ~StreamFilterOutput() {}

    void SetStream(FILE *s) { f = s; }
private:
    virtual void FeedChar(int c) { fputc(c, f); }
};

#if 0
class StreamFilterE : public StreamFilter {
public:
    StreamFilterE(StreamFilter *next) : StreamFilter(next) {}
    ~StreamFilterE() {}
    virtual void FeedChar(int c) { PutChar(c); }
};
#endif


/* differs from the parent in that is accepts a string which
   contains space-separated list of tags, builds the array
   for the parent and owns both the array and the string
 */
class StreamFilterHtmlTagsHolder : public StreamFilterHtmlTags {
    char *str;
    char **tags;
public:
    StreamFilterHtmlTagsHolder(const char *ts, StreamFilter *rest);
    ~StreamFilterHtmlTagsHolder();
};

static const char * const the_space_preserving_tags[] = {
    "pre", "ul", "ol", 0
};

static char *strdup_n(const char *s)
{
    int len = strlen(s);
    char *res = new char[len+1];
    strcpy(res, s);
    return res;
}

// modifies the str placing zeroes as appropriate
// the array returned consists of pointers into the string
static char **make_argv(char *str, int n)
{
    char **res;
    while(*str && (*str == ' ' || *str == '\t'))
        str++;
    if(!*str) {
         res = new char* [n+1];
         res[n] = 0;
         return res;
    }
    char *pos = str;
    while(*str && *str != ' ' && *str != '\t')
        str++;
    if(*str) {
        *str = 0;
        str++;
    }
    res = make_argv(str, n+1);
    res[n] = pos;
    return res;
}

StreamFilterHtmlTagsHolder::
StreamFilterHtmlTagsHolder(const char *ts, StreamFilter *rest)
    : StreamFilterHtmlTags((tags = make_argv((str = strdup_n(ts)), 0)), rest)
{
    // nothing to do
}

StreamFilterHtmlTagsHolder::~StreamFilterHtmlTagsHolder()
{
    delete[] tags;
    delete[] str;
}


static void help()
{
    fprintf(stderr,
        "text2text vers. " STFILTER_VERSION " (compiled " __DATE__ ")\n"
        "Copyright (c) Andrey Vikt. Stolyarov, 2023\n"
        "\n"
        "Usage: text2text <filters> [inputfile] [-o outputfile]\n"
        "where <filters> are one or more of:\n"
        "    -htags[+|-] 'tags'  filter off HTML tags except the listed\n"
        "           (tags is a space-separated list, be sure to use quotes)\n"
        "           The ``+'' enables a slave (controlled) NL converter.\n"
        "           The ``-'' also enables NL converter, but for empty\n"
        "           lines only (that is, single NLs are left untouched)\n"
        "    -hprot              replace '<', '>' and '&' with HTML entities\n"
        "    -hbrk               replace newlines with HTML paragraph breaks\n"
        "    -hbrk+              the same, but not inside certain tags\n"
        "    -hpar[+]            same as hbrk[+], but only empty lines are\n"
        "           replaced, and single NLs are left untouched\n"
        "    -from_utf[_ht] enc  convert utf8 to the 'enc' encoding\n"
        "           (_ht means to represent unknown chars with html entities\n"
        "    -to_utf enc         convert the 'enc'-encoded text to utf8\n"
        "\n"
        "Run ``text2text -L'' for the list of available encodings;\n"
        "all other params are ignored if -L is given.\n"
    );
}

struct cmdl_params {
    const char *input;
    const char *output;

    StreamFilterOutput *dest;

    cmdl_params() : input(0), output(0) {}
    ~cmdl_params() {}  /* both strings aren't owned */
};

#define CMDLSKIP ((StreamFilter*)0)
#define CMDLERR ((StreamFilter*)-1)

static const char *find_str(const char *str, const char *const *arr)
{
    int i;
    for(i = 0; arr[i]; i++)
        if(0 == strcmp(str, arr[i]))
            return arr[i];
    return 0;
}

static StreamFilter *
make_filter_fromutf(const char *enc, bool html, StreamFilter *next)
{
    const int * const *tbl = StreamFilterUtf8ToExtAscii::FindTable(enc);
    if(!tbl) {
        fprintf(stderr, "Encoding [%s] not found\n", enc);
        return CMDLERR;
    }
    if(html)
        return new StreamFilterUtf8ToHtml(tbl, next);
    else
        return new StreamFilterUtf8ToExtAscii(tbl, next);
}

static StreamFilter *make_filter(const char *cmd, const char *arg,
                                 StreamFilter *next)
{
    if(0 == strcmp(cmd, "hprot"))
        return new StreamFilterHtmlProtect(next);
    if(0 == strcmp(cmd, "hbrk"))
        return new StreamFilterHtmlReplaceNL(false, next);
    if(0 == strcmp(cmd, "hbrk+")) {
        StreamFilterHtmlTags *p = new StreamFilterHtmlTags(0, next);
        p->AddControlledNLReplacer(the_space_preserving_tags, false);
        return p;
    }
    if(0 == strcmp(cmd, "hpar"))
        return new StreamFilterHtmlReplaceNL(true, next);
    if(0 == strcmp(cmd, "hpar+")) {
        StreamFilterHtmlTags *p = new StreamFilterHtmlTags(0, next);
        p->AddControlledNLReplacer(the_space_preserving_tags, true);
        return p;
    }
    if(0 == strcmp(cmd, "htags"))
        return new StreamFilterHtmlTagsHolder(arg, next);
    if(0 == strcmp(cmd, "htags+")) {
        StreamFilterHtmlTagsHolder *p =
            new StreamFilterHtmlTagsHolder(arg, next);
        p->AddControlledNLReplacer(the_space_preserving_tags, false);
        return p;
    }
    if(0 == strcmp(cmd, "htags-")) {
        StreamFilterHtmlTagsHolder *p =
            new StreamFilterHtmlTagsHolder(arg, next);
        p->AddControlledNLReplacer(the_space_preserving_tags, true);
        return p;
    }
    if(0 == strcmp(cmd, "to_utf")) {
        const int *tbl = StreamFilterExtAsciiToUtf8::FindTable(arg);
        if(!tbl) {
            fprintf(stderr, "Encoding [%s] not found\n", arg);
            return CMDLERR;
        }
        return new StreamFilterExtAsciiToUtf8(tbl, next);
    }
    if(0 == strcmp(cmd, "from_utf"))
        return make_filter_fromutf(arg, false, next);
    if(0 == strcmp(cmd, "from_utf_ht"))
        return make_filter_fromutf(arg, true, next);
    fprintf(stderr, "Command %s not yet implemented, sorry\n", cmd);
    return CMDLERR;  /* shouldn't happen actually */
}

static void print_encoding_name_list()
{
    int i, n;
    n = streamfilter_encoding_name_count();
    for(i = 0; i < n; i++)
        printf("%s\n", streamfilter_encoding_name_by_index(i));
}

static StreamFilter *parse_cmdline(char **arg_rest, cmdl_params *cp)
{
    static const char * const keys0[] =  /* keys with no parameter */
        { "hprot", "hbrk", "hbrk+", "hpar", "hpar+", 0 };
    static const char * const keys1[] =  /* keys with 1 parameter */
        { "htags", "htags+", "htags-",
          "from_utf", "from_utf_ht", "to_utf", "o", 0 };

    if(!*arg_rest) {
        cp->dest = new StreamFilterOutput();
        return cp->dest;
    }

    if(*arg_rest[0] != '-') {    /* may be it's the input filename? */
        if(!cp->input) {
            cp->input = *arg_rest;
            return parse_cmdline(arg_rest + 1, cp);
        } else {
            fprintf(stderr, "too many input files, only one is allowed\n");
            return CMDLERR;
        }
    }

    if(arg_rest[0][1] == 'L' && !arg_rest[0][2]) {   /* just list them? */
        print_encoding_name_list();
        return CMDLSKIP;
    }

    const char *cmd;
    const char *cmdarg;
    char **arg_tail;

    cmd = find_str((*arg_rest) + 1, keys0);
    if(cmd) {
        arg_tail = arg_rest + 1;
        cmdarg = 0;
    } else {
        cmd = find_str((*arg_rest) + 1, keys1);
        if(cmd) {
            arg_tail = arg_rest + 2;
            cmdarg = arg_rest[1];
            if(!cmdarg || *cmdarg == '-') {
                fprintf(stderr, "-%s requires an argument\n", cmd);
                return CMDLERR;
            }
        }
    }

    if(!cmd) {
        fprintf(stderr, "Key unknown [%s], run with no args for help\n",
                        *arg_rest);
        return CMDLERR;
    }

    if(0 == strcmp(cmd, "o")) {
        cp->output = cmdarg;
        return parse_cmdline(arg_tail, cp);
    }

    StreamFilter *tail = parse_cmdline(arg_tail, cp);
    if(tail == CMDLERR)
        return CMDLERR;

    StreamFilter *ret = make_filter(cmd, cmdarg, tail);
    if(ret == CMDLERR)
        tail->DeleteChain();
    return ret;
}

int main(int argc, char **argv)
{
    if(argc < 2) {
        help();
        return 1;
    }
    cmdl_params cp;
    StreamFilter *chain = parse_cmdline(argv + 1, &cp);
    if(chain == CMDLSKIP) /* everything already done */
        return 0;
    if(chain == CMDLERR)  /* diags already printed */
        return 1;

    FILE *in_f = stdin;
    FILE *out_f = stdout;

    if(cp.input) {
        in_f = fopen(cp.input, "r");
        if(!in_f) {
            perror(cp.input);
            return 2;
        }
    }

    if(cp.output) {
        out_f = fopen(cp.output, "w");
        if(!out_f) {
            perror(cp.output);
            return 3;
        }
        cp.dest->SetStream(out_f);
    }

    int c;
    while((c = fgetc(in_f)) != EOF)
        chain->FeedChar(c);
    chain->FeedEnd();

    if(cp.input)
        fclose(in_f);
    if(cp.output)
        fclose(out_f);

    return 0;
}
