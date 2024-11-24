# +-------------------------------------------------------------------------+
# |                   StreamFilters library vers. 0.1.01                    |
# |  Copyright (c) Andrey V. Stolyarov <croco at croco dot net> 2022, 2023  |
# | ----------------------------------------------------------------------- |
# | This is free software.  Permission is granted to everyone to use, copy  |
# |        or modify this software under the terms and conditions of        |
# |                 GNU LESSER GENERAL PUBLIC LICENSE, v. 2.1               |
# |     as published by Free Software Foundation (see the file LGPL.txt)    |
# |                                                                         |
# | Please visit http://www.croco.net/software/stfilter to get a fresh copy |
# | ----------------------------------------------------------------------- |
# |   This code is provided strictly and exclusively on the "AS IS" basis.  |
# | !!! THERE IS NO WARRANTY OF ANY KIND, NEITHER EXPRESSED NOR IMPLIED !!! |
# +-------------------------------------------------------------------------+




CC = gcc
CXX = g++
CXXFLAGS = -Wall -g
CFLAGS = -Wall -g

FILES = stfilter.o stfhtml.o stfencod.o stfbs64.o

LIBNAME = stfilter
LIBFILES = lib$(LIBNAME).a
HEADERFILES = $(FILES:.o=.hpp)

all: lib$(LIBNAME).a text2text bs64test

lib$(LIBNAME).a: $(FILES)
	$(AR) -crs $@ $(FILES)

%.o:	%.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o:	%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

version.h:      Version
	echo '#define STFILTER_VERSION "'`head -1 Version`'"' > $@
	echo '#define STFILTER_VERSIONID '`tail -1 Version` >> $@

text2text: text2text.cpp lib$(LIBNAME).a version.h
	$(CXX) $(CXXFLAGS) -I. $^ -l $(LIBNAME) -L. -o $@

bs64test: stfbs64.cpp stfilter.o
	$(CXX) $(CXXFLAGS) -I. -DBS64TEST $^ -o $@

# scrtest: tests.o $(FILES) scrtest.cpp
# 	$(CXX) $(CXXFLAGS) $^ -o $@
#
# run:	scrtest
# 	./scrtest

deps.mk:	$(wildcard *.cpp) version.h
	$(CXX) -MM $^ > $@

clean:
	rm -f *.o deps.mk version.h *~ text2text lib$(LIBNAME).a

ifneq (clean, $(MAKECMDGOALS))
-include deps.mk
endif

include install.mk
