CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude 

LOGGER_SRC := src/logger.c $(wildcard src/dependencies/*.c)
SERVER_SRC := src/server.c $(wildcard src/dependencies/*.c src/parsers/*.c)
CLIENT_SRC := src/client.c $(wildcard src/dependencies/*.c)

LOGGER = logger
SERVER = main
CLIENT = client

.PHONY: all clean run

all: $(LOGGER) $(SERVER) $(CLIENT)

$(LOGGER): $(LOGGER_SRC)
	$(CC) $(CFLAGS) -o $(LOGGER) $(LOGGER_SRC) -lm

$(SERVER): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $(SERVER) $(SERVER_SRC) -lm

$(CLIENT): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $(CLIENT) $(CLIENT_SRC) -lm

run: $(LOGGER) $(SERVER) $(CLIENT)
	./$(SERVER) & 

clean:
	rm -f $(LOGGER) $(SERVER) $(CLIENT)
	> log.txt
