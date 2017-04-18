CC=gcc
CFLAGS=-c
CMD=$(CC) $(CFLAGS)

all: main.o
	$(CC) main.o -o bin


main.o: main.c
	$(CC) $(CFLAGS) main.c
