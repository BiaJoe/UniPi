CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude

all: main

main: src/server/main.c src/parsers/parse_rescuers2.c
	$(CC) $(CFLAGS) -o main src/server/main.c src/parsers/parse_rescuers2.c

run: main
	./main

clean:
	rm -f main