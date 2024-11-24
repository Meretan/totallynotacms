# This Makefile was added by Andrey "Croco" Stolyarov
# for the Thalassa CMS project.  Please note it is only
# needed for unified library handling, but certainly
# it's kinda overkill to have a Makefile and specially
# a libXX.a file for a ``library'' that consists of
# one module.
#
# If you don't like this, simply remove everything but
# the md5.c and md5.h files, and use these as just one
# of your modules.  They'll serve.
#


CC=gcc
CFLAGS=-ansi -Wall -g

libmd5.a: md5.o
	ar -rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o libmd5.a
