TARGET = programa
SRC_DIR = src
INC_DIR = include

# Encuentra todos los .c recursivamente
SRCS = $(shell find $(SRC_DIR) -name '*.c')
OBJS = $(SRCS:.c=.o)

CC = gcc
CFLAGS = -Wall -I$(INC_DIR)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

# Regla genérica para compilar cualquier .c a .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
