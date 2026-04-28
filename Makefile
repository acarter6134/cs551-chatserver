TARGET=chatserver
CFLAGS=-Wall -Wextra -g -std=gnu11 -fsanitize=address

.PHONY: all clean

all: $(TARGET)

clean:
	rm -f $(TARGET)
