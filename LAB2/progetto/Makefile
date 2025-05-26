CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude 

SERVER_SRC := src/server.c $(wildcard src/dependencies/*.c src/parsers/*.c)
CLIENT_SRC := src/client.c $(wildcard src/dependencies/*.c src/parsers/*.c)

SERVER = main
CLIENT = client

all: $(SERVER) $(CLIENT)

$(SERVER): $(SRC)
	$(CC) $(CFLAGS) -o $(SERVER) $(SERVER_SRC) -lm

$(CLIENT): $(SRC)
	$(CC) $(CFLAGS) -o $(CLIENT) $(CLIENT_SRC)

run: $(SERVER)
	./$(SERVER)

clean:
	rm -f $(SERVER)
	rm -f $(CLIENT)
	> log.txt