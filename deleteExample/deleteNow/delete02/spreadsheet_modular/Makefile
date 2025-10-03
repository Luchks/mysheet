CC=gcc
CFLAGS=-Wall -Iinclude -lncurses
SRC=$(wildcard src/*.c)
OBJ=$(SRC:.c=.o)
TARGET=spreadsheet

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(OBJ) $(TARGET)
