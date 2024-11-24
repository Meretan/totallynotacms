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




#ifndef SCRIPTPP_SCRMSG_HPP_SENTRY
#define SCRIPTPP_SCRMSG_HPP_SENTRY

#include "scrvar.hpp"
#include "scrvect.hpp"

class HeadedTextMessage {
    ScriptVariable heading_line;
    ScriptVector headers;
    ScriptVariable body;

    struct headbody_parser *parser;
        // the struct is not exposed to the rest of the modules
public:
    HeadedTextMessage(bool heading_line_expected = false);
    ~HeadedTextMessage();

    bool FeedChar(int c);

    bool InBody() const; // true if the header is over and we're in the body

    enum errors {
        err_unexpected_eot = 1,
        err_incomplete_heading,
        err_space_in_header_name,
        err_no_colon_in_header,
        err_header_line_incomplete,
        err_chars_after_finish
    };
    bool Error(int &code, int &line) const;

    ScriptVariable &GetHeadline() { return heading_line; }
    ScriptVector &GetHeaders() { return headers; }
    ScriptVariable &GetBody() { return body; }

    const ScriptVariable &GetHeadline() const { return heading_line; }
    const ScriptVector &GetHeaders() const { return headers; }
    const ScriptVariable &GetBody() const { return body; }

    ScriptVariable FindHeader(const ScriptVariable &name) const;
    void FindHeaders(const ScriptVariable &name, ScriptVector &res) const;

    void SetHeader(const ScriptVariable &name, const ScriptVariable &val);
         /*! returns how many headers actually removed */
    int RemoveHeader(const ScriptVariable &name);

    void SetHeadingLine(const ScriptVariable &val) { heading_line = val; }
    void SetBody(const ScriptVariable &val) { body = val; }

    ScriptVariable Serialize() const;

private:
        /* callbacks for underlying headbody_parser */
    static int HeadlineCb(const char *s, int, void *u);
    static int HeaderCb(const char *h, const char *v, int, int, void *u);
    static int BodyCb(int c, void *u);
};


#endif
