CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lncurses

SRC = src/main.c src/csv_reader.c src/ui.c src/undo.c
OBJ = $(SRC:.c=.o)
TARGET = bin/csv_viewer

all: $(TARGET)

$(TARGET): $(OBJ)
	mkdir -p bin
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) $(LDFLAGS)

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
