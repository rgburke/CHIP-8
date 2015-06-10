CC=cc
CFLAGS=-std=c99 -Wall -Wextra -pedantic -g -O2
LDFLAGS=-lSDL2 -lm

SOURCES=$(wildcard *.c)
OBJECTS=$(SOURCES:.c=.o)

BINARY=chip8

.PHONY: all
all: $(BINARY)

$(BINARY): $(OBJECTS)
	$(CC) $^ -o $@ $(LDFLAGS)

.c.o:
	$(CC) -c $(CFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -f *.o $(BINARY)
