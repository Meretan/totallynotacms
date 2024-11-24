/*
 * The headbody reader module (header file)
 * Copyright (c) Andrey V. Stolyarov, 2020
 *
 * This module is not a part of the scriptpp library, it is maintained
 * elsewhere and is being used in other projects without the library,
 * and sometimes under different licenses.
 *
 * However, as a part of the scriptpp library this module is distributed
 * under the license of the library, as if it was one of the library's
 * modules; this is legally possible because the module and the library
 * both have the same author (who is also the copyright owner).
 */



#ifndef HEADBODY_H_SENTRY
#define HEADBODY_H_SENTRY

#ifdef __cplusplus
extern "C" {
#endif


struct headbody_parser {
    void *data;         /* internal (obscure) parser data */
    void *userdata;     /* the pointer to be passed to callbacks */
    int headlen_limit;  /* size limit for a single header line */

    int (*headline_cb)(const char *str, int len, void *userdata);
    int (*header_cb)(const char *name, const char *val,
                     int namelen, int vallen, void *userdata);
    int (*body_cb)(int chr, void *userdata);
};

/* note that when headline_cb and header_cb are called, they are expected
   to copy all strings or to otherwise use them right off; storing these
   pointers is no good as they can become invalid once the callback returns

   headline_cb left to be 0 means no headline is expected

   callbacks should return 0 in case everything's okay, or a (custom?)
   non-zero error code to abort the operation
 */

enum headbody_parser_errors {
    hbperr_ok = 0,
    hbperr_unexpected_eot,
    hbperr_incomplete_heading,
    hbperr_space_in_header_name,
    hbperr_no_colon_in_header,
    hbperr_header_line_incomplete,
    hbperr_chars_after_finish,
    hbperr_max = hbperr_chars_after_finish
};



    /* constructor */
    /* callback function ptrs and userdata ptr must be filled manually */
void headbody_parser_init(struct headbody_parser *parser);

    /* destructor */
    /* memory used by internal data is freed; userdata remains untouched */
void headbody_parser_clear(struct headbody_parser *parser);

    /* feed another char to the automata */
    /* c == -1 means EOF/EOT/whatever, it may lead to error */
void headbody_parser_feedchar(struct headbody_parser *parser, int c);

    /* 0 is no error; otherwise, the function may return either one of
       these hbperr_*, or the (non-zero) value returned by a callback
       function; if line is not NULL, the error line number is stored
       in *line.
     */
int headbody_parser_error(const struct headbody_parser *parser, int *line);

    /* 0 = we're still analysing the headers; 1 = we're in the body */
int headbody_parser_in_body(const struct headbody_parser *parser);

#ifdef __cplusplus
}
#endif

#endif
