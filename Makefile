CC=gcc
CFLAGS=-c
ODIR=bin
CMD=$(CC) $(CFLAGS)

all:
	$(CC) server.c protocol.c -o bin/server.bin
	$(CC) client.c protocol.c -o bin/client.bin
