# +-------------------------------------------------------------------------+
# |                     I n i F i l e    vers. 0.3.24                       |
# | Copyright (c) Andrey Vikt. Stolyarov [http://www.croco.net/]  2003-2023 |
# | ----------------------------------------------------------------------- |
# | This is free software.  Permission is granted to everyone to use, copy  |
# |        or modify this software under the terms and conditions of        |
# |                 GNU LESSER GENERAL PUBLIC LICENSE, v. 2.1               |
# |     as published by Free Software Foundation (see the file LGPL.txt)    |
# |                                                                         |
# | Please visit http://www.croco.net/software/scriptpp to get a fresh copy |
# | ----------------------------------------------------------------------- |
# |   This code is provided strictly and exclusively on the "AS IS" basis.  |
# | !!! THERE IS NO WARRANTY OF ANY KIND, NEITHER EXPRESSED NOR IMPLIED !!! |
# +-------------------------------------------------------------------------+




CXX = g++
CXXFLAGS = -Wall -g
CFLAGS = -Wall -g

FILES = inifile.o

LIBNAME = inifile
LIBFILES = lib$(LIBNAME).a
BINFILES = inifile
HEADERFILES = $(FILES:.o=.hpp)
NOINCLUDEDIR = yes
NOSHAREDLIBS = yes


all: $(LIBFILES) $(BINFILES)

lib$(LIBNAME).a: $(FILES)
	$(AR) -crs $@ $(FILES)

%.o:	%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

inifile: cli.cpp $(LIBFILES)
	$(CXX) $(CXXFLAGS) -I. $^ -o $@

deps.mk:	$(wildcard *.cpp)
	$(CXX) -MM $^ > $@

clean:
	rm -rf *.o *.a *~ deps.mk $(BINFILES)

ifneq (clean, $(MAKECMDGOALS))
-include deps.mk
endif


include install.mk
