BIN = chip8
CC = cc
CFLAGS = -g -Wall -Wextra -O3 -pedantic -std=c11 $(shell pkg-config --cflags sdl2)
LDFLAGS = $(shell pkg-config --libs sdl2)

SOURCES = $(shell find . -name '*.c')
OBJECTS = $(SOURCES:.c=.o)

.PHONY: clean

default: $(BIN)

$(BIN): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(BIN) $(OBJECTS) $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(BIN) *.o
