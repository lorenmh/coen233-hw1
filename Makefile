CC=gcc
CFLAGS=-std=c99
ODIR=bin
CMD=$(CC) $(CFLAGS)

# on MacOS, gcc is symlinked to clang, weirdness may occur as a result

all:
	$(CMD) server.c protocol.c -o bin/server.bin
	$(CMD) client.c protocol.c -o bin/client.bin
