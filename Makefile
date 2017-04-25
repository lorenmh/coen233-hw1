CC=gcc
CFLAGS=-c
ODIR=bin
CMD=$(CC) $(CFLAGS)

all: server.o types.gch client.o
	$(CC) server.o -o $(ODIR)/server.bin
	$(CC) client.o -o $(ODIR)/client.bin

server.o: server.c
	$(CC) $(CFLAGS) server.c

client.o: client.c
	$(CC) $(CFLAGS) client.c

types.gch: types.h
	$(CC) $(CFLAGS) types.h
