# Nombre del ejecutable
TARGET = programa

# Directorios
SRC_DIR = src
INC_DIR = include

# Buscar todos los .c en src/
SRCS = $(wildcard $(SRC_DIR)/*.c)

# Generar lista de .o a partir de los .c
OBJS = $(SRCS:.c=.o)

# Compilador y banderas
CC = gcc
CFLAGS = -Wall -I$(INC_DIR)

# Regla principal
all: $(TARGET)

# Cómo crear el ejecutable a partir de los objetos
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

# Regla genérica para compilar cada .c a .o
$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Limpieza
clean:
	rm -f $(SRC_DIR)/*.o $(TARGET)
