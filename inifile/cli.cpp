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
#include <string.h>

#include <inifile.hpp>

int main(int argc, char **argv)
{
    if(argc < 3) {
        fprintf(stderr,
                "Usage: %s <filename> <command> \\\n"
                "          <group> [<section> <parameter> \\\n"
                "          [:<modifier>] [<value>]]\n", argv[0]);
        fprintf(stderr, "where <command> is ``get'', ``set'' or ``list''\n");
        return 1;
    }

    enum { mode_get, mode_list, mode_set } mode;

    if(strcmp(argv[2], "get") == 0) {
        mode = mode_get;
    } else
    if(strcmp(argv[2], "list") == 0) {
        mode = mode_list;
    } else
    if(strcmp(argv[2], "set") == 0) {
        mode = mode_set;
    } else {
        fprintf(stderr, "Operation mode %s is unknown\n", argv[2]);
        return 1;
    }

    IniFileParser par;
    bool load_res = par.Load(argv[1]);

    if(!load_res && mode != mode_set) {
        perror(argv[1]);
        fprintf(stderr, "Couldn't load ini file: %s\n",
                        par.GetLastErrorDescription());
        return 2;
    }
    switch(mode) {
        case mode_get: {
            if(argc < 6) {
                fprintf(stderr, "Not enough parameters\n");
                return 3;
            }
            if(argc >= 7 && argv[6][0] == ':') { // modifier
                printf("%s\n",
                       par.GetModifiedTextParameter(
                           argv[3], argv[4], argv[5], argv[6]+1,
                           argc >= 8 ? argv[7] : "")
                );
            } else { // no modifier
                printf("%s\n",
                       par.GetTextParameter(
                           argv[3], argv[4], argv[5],
                           argc >= 7 ? argv[6] : "")
                );
            }
            break;
        }
        case mode_list: {
            if(argc != 4) {
                fprintf(stderr,
                    "Only the group name must be specified for ``list''\n"
                );
                return 3;
            }
            int i;
            int n = par.GetSectionCount(argv[3]);
            for(i = 0; i < n; i++)
                printf("%s\n", par.GetSectionName(argv[3], i));
            break;
        }
        case mode_set: {
            if(argc < 6) {
                fprintf(stderr, "Not enough parameters\n");
                return 3;
            }
            if(argc >= 7 && argv[6][0] == ':') { // modifier?
                fprintf(stderr, "Modifiers are mot allowed for ``set''\n");
                fprintf(stderr, "Try \"%s%s\" as param name\n",
                                argv[5], argv[6]);
                return 3;
            }
            par.SetTextParameter(argv[3], argv[4], argv[5], argv[6]);
            if(!par.Save(argv[1])) {
                perror(argv[1]);
                fprintf(stderr, "WARNING: couldn't save %s: %s\n",
                                argv[1], par.GetLastErrorDescription());
                return 2;
            }
            break;
        }
    }


    return 0;
}
