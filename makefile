#GNU gcc makefile
#Call "make" to build executeable
#Call "make lib" to build static library

CC=gcc
CFLAGS=

AR=ar -rcs


.PHONY: all
all:
	$(CC) -o example.exe examples/example.c src/riff.c

.PHONY: lib
lib: src/riff.o
	$(AR) libriff.a $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
