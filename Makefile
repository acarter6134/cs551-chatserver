CFLAGS=-Wall -Wextra -g -std=gnu11 -fsanitize=address

.PHONY: all clean

all: chatserver

chatserver: chatserver.c client_handler.o

client_handler.o: client_handler.c client_handler.h

clean:
	rm -f chatserver *.o
