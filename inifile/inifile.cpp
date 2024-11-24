// +-------------------------------------------------------------------------+
// |                     I n i F i l e    vers. 0.3.24                       |
// | Copyright (c) Andrey Vikt. Stolyarov [http://www.croco.net/]  2003-2023 |
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




#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "inifile.hpp"

IniFileParser::IniFileParser()
{
    firstgroup = 0;
    last_error_line = -1;
    last_error_description = 0;
}

IniFileParser::~IniFileParser()
{
    if(firstgroup) delete firstgroup;
}

static bool is_empty_line(const char *s)
{
    int idx = 0;
    do {
        if(s[idx] == ';' || s[idx] == '#') /* comment */
            return true;
        if(!isspace(s[idx]))
            return false;
        idx++;
    } while(s[idx]);
    return true;
}

bool IniFileParser::Load(const char *path)
{
    last_error_line = -1;
    int current_line = 0;
    last_error_description = 0;
    FILE *fl = fopen(path, "r");
    if(!fl) {
        last_error_description = "couldn't open the file";
        return false;
    }
    char buf[1024];
    buf[sizeof(buf)-1] = '\0';
    Section *current_section = 0;
    Parameter *current_parameter = 0;
    while(fgets(buf, sizeof(buf), fl)) {
        current_line++;
        if(buf[sizeof(buf)-1]!='\0') {
            last_error_line = current_line;
            last_error_description = "line too long (max 1024)";
            fclose(fl);
            return false;
        }
        if(!buf[0] || is_empty_line(buf)) {
            /* empty line */
            continue;
        } else if(isspace(buf[0]) || buf[0] == '+') {
            /* continued line */
            if(!current_parameter) {
                last_error_line = current_line;
                last_error_description = "data outside of a parameter";
                return false;
            }
            current_parameter->AddDictionaryLine(buf+1);
        } else if(buf[0] == '#' || buf[0] == ';') {
            /* comment -- ignore */
            continue;
        } else if(buf[0] == '[') {
            /* section header */
            char* groupname;
            char* sectionname;
            if(!ExtractHeaderData(buf, &groupname, &sectionname)) {
                last_error_line = current_line;
                last_error_description = "invalid header syntax";
                return false;
            }
            current_section = ProvideSection(groupname, sectionname);
        } else {
            /* must be a dictionary line */
            if(!current_section) {
                last_error_line = current_line;
                last_error_description = "data outside of a section";
                return false;
            }
            char * name;
            char * value;
            if(!ExtractDictionaryData(buf, &name, &value)) {
                last_error_line = current_line;
                last_error_description = "invalid dictionary line syntax";
                return false;
            }
            current_parameter = current_section->ProvideParameter(name);
            current_parameter->AddDictionaryData(value);
        }
    }
    fclose(fl);
    return true;
}

bool IniFileParser::Save(const char *path)
{
    last_error_line = -1;
    last_error_description = 0;
    FILE *fl = fopen(path, "w");
    if(!fl) {
        last_error_description = "couldn't open the file";
        return false;
    }
    char buf[1024];
    buf[sizeof(buf)-1] = '\0';
    for(Group *grp = firstgroup; grp; grp =  grp->next) {
        for(Section *sect = grp->firstsection; sect; sect=sect->next) {
              fprintf(fl, "[%s %s]\n\n", grp->name, sect->name);
              for(Parameter *parm = sect->firstparam; parm; parm=parm->next) {
                  fprintf(fl, "%s = %s\n", parm->name, parm->value);
              }
              fprintf(fl, "\n\n");
        }
    }
    fclose(fl);
    return true;
}

int IniFileParser::GetGroupCount() const
{
    int c;
    Group *tmp;
    for(tmp=firstgroup, c=0; tmp; tmp=tmp->next, c++) {}
    return c;
}

int IniFileParser::GetSectionCount(const char *groupname) const
{
    Group* pgrp = *FindGroupP(groupname);
    if(!pgrp) return 0;
    int c;
    Section *tmp;
    for(tmp=pgrp->firstsection, c=0; tmp; tmp=tmp->next, c++) {}
    return c;
}

const char* IniFileParser::GetSectionName(const char *groupname, int
           sectionindex) const
{
    Group *grp = *FindGroupP(groupname);
    if(!grp) return "";
    int i;
    Section *tmp;
    for(tmp=grp->firstsection, i=0; tmp && i<sectionindex; tmp=tmp->next, i++)
        {}
    if(tmp)
        return tmp->name;
    else
        return "";
}

void IniFileParser::SetParam(const char *groupname, const char *sectionname,
                             const char *paramname, const char *paramval,
                             bool exclusive)
{
    Section* sect = ProvideSection(groupname, sectionname);
    sect->ProvideParameter(paramname)->AddDictionaryData(paramval, exclusive);
}

const char* IniFileParser::GetParam(const char *groupname,
                                    const char *sectionname,
                                    const char *paramname) const
{
    Group* grp = *FindGroupP(groupname);
    if(!grp) return "";
    Section* sect = *FindSectionP(grp, sectionname);
    if(!sect) return "";
    Parameter *tmp;
    for(tmp=sect->firstparam;
        tmp && strcmp(tmp->name, paramname)!=0;
        tmp=tmp->next) {}
    if(tmp)
        return tmp->value;
    else
        return "";
}

void IniFileParser::DeleteSection(const char *groupname,
          const char *sectionname)
{
    Group** grp = (Group**)FindGroupP(groupname);
    if(!*grp) return;
    Section** sect = (Section**)FindSectionP(*grp, sectionname);
    if(!*sect) return;
    Section *tmp = *sect;
    *sect = (*sect)->next;
    tmp->next = 0;
    delete tmp;
    // Now remove the group if it's empty
    if(!((*grp)->firstsection)) {
        Group *tmp = *grp;
        *grp = (*grp)->next;
        tmp->next = 0;
        delete tmp;
    }
}



bool IniFileParser::ExtractHeaderData(char *buf,
            char **name,
            char **param)
{
    char *p = buf;
    while(*p&&isspace(*p)) p++;
    if(*p!='[') return false;
    p++;
    while(*p&&isspace(*p)) p++;
    *name = p;
    while(*p&&!isspace(*p) && !(*p==']')) p++;
    if(*p==']') {
        // no param found
        *param = 0;
        *p = 0;
        return true;
    }
    *p = 0;
    p++;
    while(*p&&isspace(*p)) p++;
    *param = p;
    while(*p&&!isspace(*p) && !(*p==']')) p++;
    if(*p==']') {
        *p=0;
        return true;
    }
    *p=0;
    p++;
    // now check for the closing ']'
    while(*p&&isspace(*p)) p++;
    if(*p!=']') {
        return false;
    } else {
        return true;
    }
}

bool IniFileParser::ExtractDictionaryData(char *buf,
            char** name,
            char** value)
{
    char *p = buf;
    while(*p && isspace(*p)) p++;
    *name = p;
    while(*p && !isspace(*p)) p++;
    if(isspace(*p)) {
        *p=0;
        p++;
        while(*p && isspace(*p)) p++;
    }
    if(*p!='=') return false;
    *p=0;
    p++;
    while(*p && isspace(*p)) p++;
    *value = p;
    while(*p) p++;
    p--;
    while(*p && isspace(*p)) p--;
    p++;
    *p=0;
    return true;
}

IniFileParser::Section*
IniFileParser::ProvideSection(const char* groupname, const char* sectionname)
{
    Group** ppgrp = (Group**)FindGroupP(groupname);
    if(!(*ppgrp)) {
        *ppgrp = new Group(groupname);
    }
    Section** ppsect = (Section**)FindSectionP(*ppgrp, sectionname);
    if(!(*ppsect)) {
        *ppsect = new Section(sectionname ? sectionname : "");
    }
    return *ppsect;
}

IniFileParser::Group*const*
IniFileParser::FindGroupP(const char* groupname) const
{
    Group* const * ppgrp = &firstgroup;
    while(*ppgrp && strcmp((*ppgrp)->name, groupname)!=0)
        ppgrp = &((*ppgrp)->next);
    return ppgrp;
}

IniFileParser::Section*const*
IniFileParser::FindSectionP(Group* pgrp, const char* sectionname) const
{
    Section* const* ppsect = &(pgrp->firstsection);
    if(sectionname)
        while(*ppsect && strcmp((*ppsect)->name, sectionname)!=0)
            ppsect = &((*ppsect)->next);
    return ppsect;
}

void IniFileParser::
AddIntegerParameter(const char *grname, const char *sectname,
      const char *parmname, long value)
{
    char buf[32];
    sprintf(buf, "%ld", value);
    SetParam(grname, sectname, parmname, buf, false);
}

void IniFileParser::
AddTextParameter(const char *grname, const char *sectname,
     const char *parmname, const char *value)
{
    SetParam(grname, sectname, parmname, value, false);
}

void IniFileParser::
SetTextParameter(const char *grname, const char *sectname,
     const char *parmname, const char *value)
{
    SetParam(grname, sectname, parmname, value, true);
}

long IniFileParser::
GetIntegerParameter(const char *grname, const char *sectname,
      const char *parmname, long def) const
{
    const char* res = GetParam(grname, sectname, parmname);
    if(res[0] == '\0') return def;
    char *err;
    long l = strtol(res, &err, 10);
    if(*err != '\0') return def;
    return l;
}

const char* IniFileParser::
GetTextParameter(const char *grname, const char *sectname,
     const char *parmname, const char* def) const
{
    const char* res = GetParam(grname, sectname, parmname);
    return res[0]=='\0' ? def : res;
}

const char* IniFileParser::
GetModifiedTextParameter(const char *grname, const char *sectname,
    const char *parmname, const char *modifier, const char* def) const
{
    int l1;
    char *modname = new char[(l1=strlen(parmname))+1+strlen(modifier)+1];
    strcpy(modname, parmname);
    modname[l1]=':';
    strcpy(modname+l1+1, modifier);
    const char* res = GetParam(grname, sectname, modname);
    delete[] modname;
    if(res[0]=='\0')
        res = GetParam(grname, sectname, parmname);
    return res[0]=='\0' ? def : res;
}

long IniFileParser::
GetModifiedIntegerParameter(const char *grname, const char *sectname,
    const char *parmname, const char *modifier, long def) const
{
    const char *res =
        GetModifiedTextParameter(grname, sectname, parmname, modifier, 0);
    if(!res) return def;
    if(res[0] == '\0') return def;
    char *err;
    long l = strtol(res, &err, 10);
    if(*err != '\0') return def;
    return l;
}

////////////////////////////////////////////////////////////

IniFileParser::Group::Group(const char *aname)
{
    name = new char[strlen(aname)+1];
    strcpy(name, aname);
    next = 0;
    firstsection = 0;
}

IniFileParser::Group::~Group()
{
    delete[] name;
    if(next)
        delete next;
    if(firstsection)
        delete firstsection;
}

////////////////////////////////////////////////////////////

IniFileParser::Section::Section(const char *aname)
{
    name = new char[strlen(aname)+1];
    strcpy(name, aname);
    next = 0;
    firstparam = 0;
}

IniFileParser::Section::~Section()
{
    delete[] name;
    if(next)
        delete next;
    if(firstparam)
        delete firstparam;
}

IniFileParser::Parameter*
IniFileParser::Section::ProvideParameter(const char * name)
{
    Parameter **tmp;
    for(tmp = &firstparam;
        *tmp && strcmp((*tmp)->name, name) != 0;
        tmp = &((*tmp)->next));
    if(!*tmp) {
        (*tmp) = new Parameter(name, "");
    }
    return *tmp;
}

//////////////////////////////////////////////////////

IniFileParser::Parameter::Parameter(const char *aname, const char *aval)
{
    name = new char[strlen(aname)+1];
    strcpy(name, aname);
    value = new char[strlen(aval)+1];
    strcpy(value, aval);
    next = 0;
}

IniFileParser::Parameter::~Parameter()
{
    delete[] name;
    delete[] value;
    if(next) delete next;
}

void IniFileParser::Parameter::AddDictionaryData(const char* a_value,
               bool exclusive)
{
    int ll = strlen(a_value);
    while(ll > 0 && isspace(a_value[ll-1])) ll--;
    if(exclusive || value[0]=='\0') {
        delete[] value;
        value = new char[ll+1];
        strncpy(value, a_value, ll);
        value[ll] = 0;
    } else {
        int lv = strlen(value);
        char *newval = new char[lv+1+ll+1];
        snprintf(newval, lv+1+ll+1, "%s,%s", value, a_value);
        delete[] value;
        value = newval;
    }
}

void IniFileParser::Parameter::AddDictionaryLine(const char *a_value)
{
    int lv = strlen(value);
    int ll = strlen(a_value);
    while(ll > 0 && isspace(a_value[ll-1])) ll--;
    char *newval = new char[lv+ll+2];
    strcpy(newval, value);
    newval[lv] = '\n';
    strncpy(newval+lv+1, a_value, ll);
    newval[lv+1+ll] = 0;
    delete[] value;
    value = newval;
}


char **IniFileParser::BreakParameterAsCSV(const char *param_text)
{
    int len = 0, fld = 1;
    enum { out, quoted, dq } state = out;
    for(const char *s = param_text; *s; s++) {
        switch(state) {
            case out:
                if(*s == '"')
                    state = quoted;
                else
                if(*s == ',') {
                    len++; fld++;
                }
                else
                    len++;
                break;
            case quoted:
                if(*s == '"')
                    state = dq;
                else
                    len++;
                break;
            case dq:
                if(*s == '"') {
                    len++; state = quoted;
                } else
                if(*s == ',') {
                    len++; fld++;
                    state = out;
                } else {
                    len++; state = out;
                }
        }
    }

    char **res = new char*[fld+1];
    res[fld] = 0;
    res[0] = new char[len+1];
    int curf = 0, curch = 0;

    state = out;
    for(const char *s = param_text; *s; s++) {
        switch(state) {
            case out:
                if(*s == '"')
                    state = quoted;
                else
                if(*s == ',') {
                    res[0][curch++] = 0;
                    res[++curf] = res[0] + curch;
                }
                else
                    res[0][curch++] = *s;
                break;
            case quoted:
                if(*s == '"')
                    state = dq;
                else
                    res[0][curch++] = *s;
                break;
            case dq:
                if(*s == '"') {
                    res[0][curch++] = '"';
                    state = quoted;
                } else
                if(*s == ',') {
                    res[0][curch++] = 0;
                    res[++curf] = res[0] + curch;
                    state = out;
                } else {
                    res[0][curch++] = *s;
                    state = out;
                }
        }
    }
    res[0][curch] = 0;
    return res;
}

void IniFileParser::DisposeBrokenParameter(char** bparm)
{
    delete[] bparm[0];
    delete[] bparm;
}
