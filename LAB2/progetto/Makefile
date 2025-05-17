CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude

# Get all .c files in src/server/ and src/parsers/
SRC := $(wildcard src/server/*.c src/parsers/*.c)

# Output binary
TARGET = main

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)