#!/bin/bash

# Nombre del proyecto
PROJECT="hola_mundo_modular"

# Crear carpetas
mkdir -p $PROJECT/src/core
mkdir -p $PROJECT/src/io
mkdir -p $PROJECT/include

# Crear main.c
cat > $PROJECT/src/main.c << 'EOF'
#include "io.h"
#include "logic.h"

int main(void) {
    saludar();
    despedida();
    return 0;
}
EOF

# Crear io.c
cat > $PROJECT/src/io/io.c << 'EOF'
#include <stdio.h>
#include "io.h"

void imprimir(const char* mensaje) {
    printf("%s\n", mensaje);
}
EOF

# Crear io.h
cat > $PROJECT/include/io.h << 'EOF'
#ifndef IO_H
#define IO_H

void imprimir(const char* mensaje);

#endif
EOF

# Crear logic.c
cat > $PROJECT/src/core/logic.c << 'EOF'
#include "logic.h"
#include "io.h"

void saludar(void) {
    imprimir("Hola, mundo");
}

void despedida(void) {
    imprimir("Adiós, mundo");
}
EOF

# Crear logic.h
cat > $PROJECT/include/logic.h << 'EOF'
#ifndef LOGIC_H
#define LOGIC_H

void saludar(void);
void despedida(void);

#endif
EOF

# Crear Makefile que compila todos los .c recursivamente
cat > $PROJECT/Makefile << 'EOF'
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
EOF

echo "Proyecto modular escalable creado en $PROJECT"
echo "Para compilar: cd $PROJECT && make"
echo "Para ejecutar: ./programa"
