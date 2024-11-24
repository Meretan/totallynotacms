/*
 * The headbody reader module (implementation file)
 * Copyright (c) Andrey V. Stolyarov, 2020
 *
 * Please see the headbody.h file (heading comment) for some legal detail.
 */

#include <stdlib.h>

#include "headbody.h"

enum { initial_buffer_size = 64 };

enum states {
    st_start,     /* skipping whitespace expecting the heading line */
    st_headline,  /* reading the heading line, if any */
    st_hname,     /* reading header name, looking for ':' */
    st_hbody,     /* reading header body until '\n' */
    st_hskip0,    /* the first char after header: WS for continued header */
    st_hskip,     /* skipping space before the header continues */
    st_msgbody,   /* reading the message body (after the empty line) */
    st_finish,    /* EOT signaled when everything was okay */
    st_error      /* something erroneous happened */
};

struct hbp_data {
    int state;
    char *buffer;
    int bufsize, bufused;
    int colonpos;
    int current_line, error_code, error_line;
};

static void buf_addchar(struct hbp_data *p, int c)
{
    if(p->bufused >= p->bufsize) {
        p->bufsize *= 2;
        p->buffer = realloc(p->buffer, p->bufsize);
    }
    p->buffer[p->bufused] = c;
    p->bufused++;
}

static void buf_drop(struct hbp_data *p)
{
    p->bufused = 0;
}




   /* please note that ``this'' parameter must be named "parser"
      in order to let this macro work */
#define HBP ((struct hbp_data*)(parser->data))

void headbody_parser_init(struct headbody_parser *parser)
{
    parser->userdata = NULL;
    parser->headlen_limit = -1;
    parser->headline_cb = NULL;
    parser->header_cb = NULL;
    parser->body_cb = NULL;

    parser->data = malloc(sizeof(struct hbp_data));
    HBP->state = st_start;
    HBP->buffer = malloc(initial_buffer_size);
    HBP->bufsize = initial_buffer_size;
    HBP->bufused = 0;
    HBP->current_line = 1;
    HBP->error_code = hbperr_ok;
    HBP->error_line = -1;
}

void headbody_parser_clear(struct headbody_parser *parser)
{
    free(HBP->buffer);
    free(parser->data);
}

int headbody_parser_error(const struct headbody_parser *parser, int *line)
{
    if(HBP->state != st_error)
        return hbperr_ok;
    if(line)
        *line = HBP->error_line;
    return HBP->error_code;
}

int headbody_parser_in_body(const struct headbody_parser *parser)
{
    int s = HBP->state;
    return s == st_msgbody || s == st_finish || s == st_error;
}

/* ---------------------------------------------------------- */
/* automaton implementation                                   */

static void set_error(struct headbody_parser *parser, int err)
{
    HBP->error_code = err;
    HBP->error_line = HBP->current_line;
    HBP->state = st_error;
}

static void handle_headline(struct headbody_parser *parser, int c);
static void handle_hname(struct headbody_parser *parser, int c);

static void handle_start(struct headbody_parser *parser, int c)
{
    if(c == -1) {   /* empty message; why no? */
        HBP->state = st_finish;
        return;
    }
    if(c == '\r' || c == '\n' || c == '\t' || c == ' ')
        return;
    /* non-space char; handle it as the heading line char */
    if(parser->headline_cb) {
        HBP->state = st_headline;
        handle_headline(parser, c);
    } else {
        HBP->state = st_hname;
        handle_hname(parser, c);
    }
}

static void handle_headline(struct headbody_parser *parser, int c)
{
    if(c == -1) {
        set_error(parser, hbperr_incomplete_heading);
        return;
    }
    if(c == '\r')
        return;
    if(c == '\n') {
        int res;
        buf_addchar(HBP, 0);
        res = (*parser->headline_cb)(HBP->buffer,
                                     HBP->bufused-1,
                                     parser->userdata);
        buf_drop(HBP);
        if(res != 0) {
            set_error(parser, res);
            return;
        }
        HBP->state = st_hname;
        return;
    }
    buf_addchar(HBP, c);
}

static void handle_hname(struct headbody_parser *parser, int c)
{
    if(c == -1) {
        if(HBP->bufused == 0)
            HBP->state = st_finish;
        else
            set_error(parser, hbperr_unexpected_eot);
        return;
    }
    if(c == '\r' || c == ' ' || c == '\t') {
        set_error(parser, hbperr_space_in_header_name);
        return;
    }
    if(c == '\n') {
        if(HBP->bufused > 0)
            set_error(parser, hbperr_no_colon_in_header);
        else
            HBP->state = st_msgbody;
        return;
    }
    if(c == ':') {
        HBP->colonpos = HBP->bufused;
        buf_addchar(HBP, c);
        HBP->state = st_hbody;
        return;
    }
    buf_addchar(HBP, c);
}

static void handle_hbody(struct headbody_parser *parser, int c)
{
    if(c == -1) {
        set_error(parser, hbperr_header_line_incomplete);
        return;
    }
    if(c == '\r')
        return;
    if(c == '\n') {
        HBP->state = st_hskip0;
        return;
    }
    buf_addchar(HBP, c);
}

static void finalize_header(struct headbody_parser *parser)
{
    int i, res;
    HBP->buffer[HBP->colonpos] = 0;
    i = HBP->colonpos + 1;
    while(HBP->buffer[i] == ' ' || HBP->buffer[i] == '\t')
        i++;
    buf_addchar(HBP, 0);
    if(parser->header_cb) {
        res = (*parser->header_cb)(HBP->buffer, HBP->buffer + i,
                                   HBP->colonpos, HBP->bufused-i,
                                   parser->userdata);
        buf_drop(HBP);
        if(res != 0)
            set_error(parser, res);
    }
}

static void handle_hskip0(struct headbody_parser *parser, int c)
{
    if(c == -1) {  /* message with empty body; good. */
        HBP->state = st_finish;
        return;
    }
    if(c == ' ' || c == '\t' || c == '\r') {
        HBP->state = st_hskip;
        return;
    }
    finalize_header(parser);  /* this will reset the buffer */
    if(c == '\n') { /* this means empty line */
        HBP->state = st_msgbody;
    } else {
        /* non-space char was first; it's simply the next header */
        HBP->state = st_hname;
        handle_hname(parser, c);
    }
}

static void handle_hskip(struct headbody_parser *parser, int c)
{
    if(c == -1) {  /* empty body, with some wrong WS... accept */
        HBP->state = st_finish;
        return;
    }
    if(c == ' ' || c == '\t' || c == '\r')
        return;
    if(c == '\n') { /* actually, this means empty line (with some WS in it)
                       we accept it but it IS possible we shouldn't */
        finalize_header(parser);
        HBP->state = st_msgbody;
        return;
    }
    /* non-space; go on with the header */
    buf_addchar(HBP, '\n');  /* XXX KEEP_WRAPPED_HEADERS should be an option */
                             /* add ' ' instead of '\n' in case it is false */
    buf_addchar(HBP, c);
    HBP->state = st_hbody;
}

static void handle_msgbody(struct headbody_parser *parser, int c)
{
    int res;
    if(c == -1) {
        HBP->state = st_finish;
        return;
    }
    res = (*parser->body_cb)(c, parser->userdata);
    if(res != 0)
        set_error(parser, res);
}

static void handle_finish(struct headbody_parser *parser, int c)
{
    if(c != -1)
        set_error(parser, hbperr_chars_after_finish);
}

void headbody_parser_feedchar(struct headbody_parser *parser, int c)
{
    switch(HBP->state) {
        case st_start:    handle_start(parser, c); break;
        case st_headline: handle_headline(parser, c); break;
        case st_hname:    handle_hname(parser, c); break;
        case st_hbody:    handle_hbody(parser, c); break;
        case st_hskip0:   handle_hskip0(parser, c); break;
        case st_hskip:    handle_hskip(parser, c); break;
        case st_msgbody:  handle_msgbody(parser, c); break;
        case st_finish:   handle_finish(parser, c); break;
        case st_error:
        default:
           /* nothing to do */
           ;
    }
    if(c == '\n')
        HBP->current_line++;
}

