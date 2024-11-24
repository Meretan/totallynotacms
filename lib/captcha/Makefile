CC=gcc
CFLAGS=-xc -ansi -Wall -g
LETTERS=A B C D E F G H I J K L M N P Q R S T U V W X Y Z 1 2 3 4 5 6 7 8 9

libcaptcha.a: _letters.o lodepng.o captcha.o
	ar -rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $<

_bin2c: _bin2c.c
	$(CC) $(CFLAGS) _bin2c.c -o _bin2c

_letters.c: _bin2c
	for i in $(LETTERS) ; do \
		./_bin2c pictures/$$i.png letter_image_$$i ; \
	done > $@

captcha.o: captcha.c captcha.h lodepng.h _letters.c

lodepng.o:	lodepng.c lodepng.h
	$(CC) $(CFLAGS) \
		-D LODEPNG_NO_COMPILE_DISK \
		-D LODEPNG_NO_COMPILE_ERROR_TEXT \
		-D LODEPNG_NO_COMPILE_CPP \
		-c $<
#	$(CC) $(CFLAGS) \
#		-D LODEPNG_NO_COMPILE_DISK \
#		-D LODEPNG_NO_COMPILE_ANCILLARY_CHUNKS \
#		-D LODEPNG_NO_COMPILE_ERROR_TEXT \
#		-D LODEPNG_NO_COMPILE_CPP \
#		-c $<

clean:
	rm -f *.o libcaptcha.a _letters.c _bin2c
