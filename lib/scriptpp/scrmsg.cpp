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




#include "scrmsg.hpp"

#include "headbody.h"

HeadedTextMessage::HeadedTextMessage(bool heading_line_expected)
    : parser(new headbody_parser)
{
    headbody_parser_init(parser);
    parser->userdata = this;
    parser->headline_cb = heading_line_expected ? HeadlineCb : 0;
    parser->header_cb = HeaderCb;
    parser->body_cb = BodyCb;
}

HeadedTextMessage::~HeadedTextMessage()
{
    headbody_parser_clear(parser);
    delete parser;
}

bool HeadedTextMessage::FeedChar(int c)
{
    headbody_parser_feedchar(parser, c);
    return !headbody_parser_error(parser, 0);
}

bool HeadedTextMessage::InBody() const
{
    return headbody_parser_in_body(parser);
}

ScriptVariable HeadedTextMessage::FindHeader(const ScriptVariable &name) const
{
    int max1 = headers.Length() - 1;
    int i;
    for(i = 0; i < max1; i+=2)
        if(headers[i] == name)
            return headers[i+1];
    return ScriptVariableInv();
}

void HeadedTextMessage::FindHeaders(const ScriptVariable &name,
                                    ScriptVector &res) const
{
    res.Clear();
    int max1 = headers.Length() - 1;
    int i;
    for(i = 0; i < max1; i+=2)
        if(headers[i] == name)
            res.AddItem(headers[i+1]);
}

void HeadedTextMessage::
SetHeader(const ScriptVariable &name, const ScriptVariable &val)
{
    int max1 = headers.Length() - 1;
    int i;
    for(i = 0; i < max1; i+=2)
        if(headers[i] == name) {
            headers[i+1] = val;
            return;
        }
    headers.AddItem(name);
    headers.AddItem(val);
}

     /*! returns how many headers actually removed */
int HeadedTextMessage::RemoveHeader(const ScriptVariable &name)
{
    int cnt = 0;
    int i = 0;
    while(i < headers.Length()-1) {
        if(headers[i] == name) {
            headers.Remove(i, 2);
            cnt++;
        } else {
            i += 2;
        }
    }
    return cnt;
}

static void eol_replace(ScriptVariable &res, const ScriptVariable &val)
{
    const char *p;
    for(p = val.c_str(); *p; p++) {
        res += *p;
        if(*p == '\n')
            res += '\t';
    }
    res.Trim();  // so it can't end with NL nor NL+space
}

ScriptVariable HeadedTextMessage::Serialize() const
{
    ScriptVariable res;
    if(heading_line.IsValid() && heading_line != "") {
        res = heading_line;
        res += "\n";
    }
    int max1 = headers.Length() - 1;
    int i;
    for(i = 0; i < max1; i+=2) {
        res += headers[i];
        res += ": ";
        eol_replace(res, headers[i+1]);
        res += "\n";
    }
    res += "\n";
    res += body;
    return res;
}



bool HeadedTextMessage::Error(int &code, int &line) const
{
    int ln;
    int err = headbody_parser_error(parser, &ln);
    if(!err)
        return false;
    code = err;
    line = ln;
    return true;
}

int HeadedTextMessage::HeadlineCb(const char *s, int, void *u)
{
    HeadedTextMessage *p = (HeadedTextMessage*)u;
    p->heading_line = s;
    return 0;
}

int HeadedTextMessage::
HeaderCb(const char *h, const char *v, int, int, void *u)
{
    HeadedTextMessage *p = (HeadedTextMessage*)u;
    p->headers.AddItem(h);
    p->headers.AddItem(v);
    return 0;
}

int HeadedTextMessage::BodyCb(int c, void *u)
{
    HeadedTextMessage *p = (HeadedTextMessage*)u;
    if(c != -1)
        p->body += c;
    return 0;
}
