server: server.o
	$(CC) -o $@ $^

install: server
	cp server ../../bin

server.o: server.c

.PHONY:
clean:
	rm server.o server

CFLAGS=-g -O0
