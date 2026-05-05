CFLAGS=-Wall -Wextra -g -std=gnu11 -fsanitize=address

.PHONY: all clean

all: chatserver

chatserver: chatserver.c client.o

client.o: client.c client.h

clean:
	rm -f chatserver *.o
