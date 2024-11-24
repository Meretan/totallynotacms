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




#ifndef SCRIPTPP_CONFFILE_HPP_SENTRY
#define SCRIPTPP_CONFFILE_HPP_SENTRY

#include "scrvar.hpp"
#include "cmd.hpp"

class ReadConfigFile : public ReadStream {
    int error_line;
protected:
    int error_code;
public:
    enum {
        err_noerror,             // no error
        err_nofile,              // file not open
        err_nodelim,             // ``='' not found
        err_stray,               // continuation line with no main line
        err_max_base_error = err_stray
    };

    ReadConfigFile()
        : ReadStream(), error_line(-1), error_code(err_noerror) {}
    virtual ~ReadConfigFile() { if(IsOpen()) FClose(); }

    // to open the file/stream, use the parent's (ReadStream class') methods

    bool RunParser();
    void GetError(int &line, int &code) const
        { line = error_line; code = error_code; }

    virtual bool SpecialLineHook(const ScriptVariable &line)
        { return false; }
    virtual bool PairReady(const ScriptVariable &name,
                           const ScriptVariable &value) = 0;
};

class ReadConfigFileToVector : public ReadConfigFile {
    class ScriptVector *vec, *dest;
public:
    ReadConfigFileToVector();
    ~ReadConfigFileToVector();

    bool ReadTo(ScriptVector &target);

    virtual bool PairReady(const ScriptVariable &name,
                           const ScriptVariable &value);

    ScriptVector *Get() const { return vec; }
};

#endif
