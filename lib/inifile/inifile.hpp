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




#ifndef INIFILE_HPP_SENTRY
#define INIFILE_HPP_SENTRY

class IniFileParser {
    struct Parameter {
        Parameter *next;
        char *name;
        char *value;
        Parameter(const char *nm, const char *vl);
        ~Parameter();
        void AddDictionaryData(const char *value, bool exclusive = false);
        void AddDictionaryLine(const char *value);
    };
    struct Section {
        Section *next;
        char *name;
        Parameter *firstparam;
        Section(const char *nm);
        ~Section();
        Parameter *ProvideParameter(const char *name);
    };
    struct Group {
        Group *next;
        char *name;
        Section *firstsection;
        Group(const char *nm);
        ~Group();
    };
    Group *firstgroup;

    int last_error_line;
    const char *last_error_description;

public:
    IniFileParser();
    ~IniFileParser();

    bool Load(const char *path);
    bool Save(const char *path);
    int GetLastErrorLine() const
        { return last_error_line; }
    const char *GetLastErrorDescription() const
        { return last_error_description; }

    void SetParam(const char *groupname, const char *sectionname,
                  const char *paramname, const char *paramval,
                  bool exclusive = true);

    int GetGroupCount() const;
    int GetSectionCount(const char *groupname) const;
    const char* GetSectionName(const char *groupname, int sectionindex) const;
    const char* GetParam(const char *groupname, const char *sectionname,
                    const char *paramname) const;

    void DeleteSection(const char *groupname, const char *sectionname);

    void AddIntegerParameter(const char *grname, const char *sectname,
                             const char *parmname, long value);
    void AddTextParameter(const char *grname, const char *sectname,
                          const char *parmname, const char *value);
    void SetTextParameter(const char *grname, const char *sectname,
                          const char *parmname, const char *value);

    const char *GetTextParameter(const char *grname, const char *sectname,
                       const char *parmname, const char *defaultvalue) const;
    const char *GetModifiedTextParameter(const char *grname,
                                         const char *sectname,
                                         const char *parmname,
                                         const char *modifier,
                                         const char *defaultvalue) const;
    long GetIntegerParameter(const char *grname, const char *sectname,
                             const char *parmname, long defaultvalue) const;
    long GetModifiedIntegerParameter(const char *grname,
                                     const char *sectname,
                                     const char *parmname,
                                     const char *modifier,
                                     long defaultvalue) const;

    static char **BreakParameterAsCSV(const char *param_text);
    static void DisposeBrokenParameter(char **bparm);
private:
    Section *ProvideSection(const char *groupname, const char *sectionname);
    Group *const*FindGroupP(const char *groupname) const;
    Section *const*FindSectionP(Group *grp, const char *sectionname) const;

    static bool ExtractHeaderData(char *buf,
                                  char **groupname,
                                  char **sectionname);
    static bool ExtractDictionaryData(char *buf,
                                      char **name,
                                      char **value);
};


#endif
