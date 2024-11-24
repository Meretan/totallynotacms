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




#include "confinfo.hpp"

ConfigInformation::ConfigInformation()
    : sub(0)
{}

ConfigInformation::~ConfigInformation()
{
    if(sub)
        delete sub;
}

ScriptVariable ConfigInformation::GetValue(const ScriptVariable &name) const
{
    ScriptVariable s = GetItem(name);
    if(s.IsInvalid())
        return s;
    if(!sub)
        const_cast<ConfigInformation*>(this)->MakeSub();
    return (*sub)(s);
}

bool ConfigInformation::PairReady(const ScriptVariable &name,
                                  const ScriptVariable &value)
{
    bool res = AddItem(name, value);
    if(!res) {
        error_code = err_duplicate;
        return false;
    }
    return true;
}



class ConfigInformationSubVar : public ScriptSubstitution::Var {
    const class ScriptMap *the_map;
public:
    ConfigInformationSubVar (const ScriptMap &map)
        : the_map(&map) {}
    ScriptVariable Sub(const char *name, int len) const {
        ScriptVariable nms(name, len);
        return the_map->GetItem(nms);  // invalid remains invalid
    }
};

void ConfigInformation::MakeSub()
{
    sub = new ScriptSubstitution();
    sub->AddVar(new ConfigInformationSubVar(*this));
}
