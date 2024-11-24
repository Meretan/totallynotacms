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




#ifndef CONFINFO_HPP_SENTRY
#define CONFINFO_HPP_SENTRY

#include "scrmap.hpp"
#include "scrsubs.hpp"
#include "conffile.hpp"

class ConfigInformation : public ReadConfigFile, public ScriptMap {
    class ScriptSubstitution *sub;
public:
    enum { err_duplicate = ReadConfigFile::err_max_base_error + 1 };

    ConfigInformation();
    ~ConfigInformation();

        //! Get value with substitutions done as appropriate
        /*! \note Use ScriptMap::GetItem to get the raw value */
    ScriptVariable GetValue(const ScriptVariable &val) const;

private:
    bool PairReady(const ScriptVariable &name, const ScriptVariable &value);
    void MakeSub();
};


#endif
