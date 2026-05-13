CFLAGS=-Wall -Wextra -g -std=gnu11 -fsanitize=address

.PHONY: all clean

all: chatserver

chatserver: chatserver.c client_handler.o messages.o cJSON.o

client_handler.o: client_handler.c common.h

messages.o: messages.c messages.h

cJSON.o: CFLAGS=
cJSON.o: cJSON.c cJSON.h

clean:
	rm -f chatserver *.o
