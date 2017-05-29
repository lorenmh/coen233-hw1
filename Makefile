CC=gcc
CFLAGS=-c -std=c99
ODIR=bin
CMD=$(CC) $(CFLAGS)

# on MacOS, gcc is symlinked to clang, weirdness may occur as a result

all:
	$(CC) server.c protocol.c -o bin/server.bin
	$(CC) client.c protocol.c -o bin/client.bin
