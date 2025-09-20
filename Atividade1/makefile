CC=gcc
CFLAGS=-Wall -O2

all: server client

server: server.c function.c
	$(CC) $(CFLAGS) -o server server.c function.c

client: client.c
	$(CC) $(CFLAGS) -o client client.c

clean:
	rm -f chat_server chat_client

run-server:
	./chat_server 5001

run-client:
	./chat_client 127.0.0.1 5001
