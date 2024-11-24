# +-------------------------------------------------------------------------+
# |                     Script Plus Plus vers. 0.3.70                       |
# | Copyright (c) Andrey V. Stolyarov  <croco at croco dot net>  2003--2023 |
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




CC = gcc
CXX = g++
CXXFLAGS = -Wall -g
CFLAGS = -Wall -g

FILES = scrvar.o scrvect.o scrmap.o scrsubs.o cmd.o conffile.o confinfo.o\
	headbody.o scrmsg.o scrmacro.o scrvar_x.o

LIBNAME = scriptpp
LIBFILES = lib$(LIBNAME).a
HEADERFILES = $(FILES:.o=.hpp)


lib$(LIBNAME).a: $(FILES)
	$(AR) -crs $@ $(FILES)

%.o:	%.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o:	%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

scrtest: tests.o $(FILES) scrtest.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

run:	scrtest
	./scrtest

clean:
	rm -f *.o *~ scrtest buf lib$(LIBNAME).a

include install.mk
