CC=gcc
CFLAGS=-W -Wall -std=c99 -pedantic 
LDFLAGS=

.PHONY: clean

all: client server

client: client.c
	$(CC) -o client.o -c client.c $(CFLAGS)
	$(CC) $(LDFLAGS) -o client client.o

server: server.c
	$(CC) -o server.o -c server.c $(CFLAGS)
	$(CC) $(LDFLAGS) -lrt -o server server.o

clean:
	@rm -f *.o
	@rm -f client


