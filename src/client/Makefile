client: client.o
	$(CC) -o $@ $^

install: client
	cp client ../../bin

client.o: client.c

.PHONY:
clean:
	rm client.o client

CFLAGS=-g -O0
