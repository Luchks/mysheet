#!/bin/bash

# Nombre del proyecto
PROJECT="hola_mundo"

# Crear estructura de carpetas
mkdir -p $PROJECT/src
mkdir -p $PROJECT/include

# Crear main.c
cat > $PROJECT/src/main.c << 'EOF'
#include "saludar.h"

int main(void) {
    saludar();
    return 0;
}
EOF

# Crear saludar.c
cat > $PROJECT/src/saludar.c << 'EOF'
#include <stdio.h>
#include "saludar.h"

void saludar(void) {
    printf("Hola, mundo\n");
}
EOF

# Crear saludar.h
cat > $PROJECT/include/saludar.h << 'EOF'
#ifndef SALUDAR_H
#define SALUDAR_H

void saludar(void);

#endif
EOF

# Crear Makefile (automático)
cat > $PROJECT/Makefile << 'EOF'
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
EOF

echo "Proyecto creado en la carpeta $PROJECT"
echo "Para compilar: cd $PROJECT && make"
echo "Para ejecutar: ./programa"
