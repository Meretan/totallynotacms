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




#include "scrvect.hpp"
#include "conffile.hpp"

static bool is_cont_char(int ch)
{
    return ch == ' ' || ch == '\t' || ch == '+';
}

bool ReadConfigFile::RunParser()
{
    if(!IsOpen()) {
        error_line = -1;
        error_code = err_nofile;
        return false;
    }

    int lnc = 0;
    ScriptVariable line, name, value;
    name.Invalidate();
    value.Invalidate();
    while(ReadLine(line)) {
        lnc++;
        if(SpecialLineHook(line))
            continue;
        bool continued_line = line.Length() > 0 && is_cont_char(line[0]);
        line.Trim(" \t\r\n");
        if(line.Length() == 0) {
            if(name.IsValid()) {
                if(!PairReady(name, value))
                    return false;
                name.Invalidate();
            }
            continue;
        }
        if(continued_line) {
            if(!name.IsValid()) {
                error_line = lnc;
                error_code = err_stray;
                return false;
            }
            value += "\n";
            value += line[0] == '+' ? line.c_str()+1 : line.c_str();
            //value += line.c_str() + (line[0] == '+' ? 1 : 0);
            continue;
        }
        // line with ``name = value'' pair; if there's already one, pass it
        if(name.IsValid()) {
            if(!PairReady(name, value))
                return false;
        }
        ScriptVariable::Substring pos = line.Strchr('=');
        if(pos.IsInvalid()) {
            error_line = lnc;
            error_code = err_nodelim;
            return false;
        }
        name = pos.Before().Trim().Get();
        value = pos.After().Trim().Get();
    }
    if(name.IsValid()) {
        if(!PairReady(name, value))
            return false;
    }
    return true;
}



ReadConfigFileToVector::ReadConfigFileToVector()
    : vec(0), dest(0)
{
}

ReadConfigFileToVector::~ReadConfigFileToVector()
{
    if(vec)
        delete vec;
}

bool ReadConfigFileToVector::ReadTo(ScriptVector &target)
{
    ScriptVector *v = dest;
    dest = &target;
    bool res = RunParser();
    dest = v;
    return res;
}

bool ReadConfigFileToVector::PairReady(const ScriptVariable &name,
                                       const ScriptVariable &value)
{
    if(!dest) {
        if(!vec)
            vec = new ScriptVector;
        dest = vec;
    }
    dest->AddItem(name);
    dest->AddItem(value);
    return true;
}
