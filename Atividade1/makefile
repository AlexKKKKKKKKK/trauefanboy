CC=gcc
CFLAGS=-Wall -O2

all: server client

server: server.c function.c
	$(CC) $(CFLAGS) -o server server.c function.c

client: client.c
	$(CC) $(CFLAGS) -o client client.c

clean:
	rm -f server client

run-server:
	./server 5001

run-client:
	./client 0.0.0.0 5001
