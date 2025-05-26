CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude 

# Get all .c files in src/server/ and src/parsers/
SRC := $(wildcard src/server/*.c src/parsers/*.c)

# Output binary
SERVER = main
CLIENT = client

all: $(SERVER) $(CLIENT)

$(SERVER): $(SRC)
	$(CC) $(CFLAGS) -o $(SERVER) $(SRC) -lm

$(CLIENT): $(CLI)
	$(CC) $(CFLAGS) -o $(CLIENT) $(SRC)

run: $(SERVER)
	./$(SERVER)

clean:
	rm -f $(SERVER)
	rm -f $(CLIENT)
	> log.txt