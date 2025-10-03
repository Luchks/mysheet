#!/bin/bash
# Script para crear proyecto modular de hoja de cálculo en C tipo Excel

PROJECT="spreadsheet_modular"
mkdir -p $PROJECT/src $PROJECT/include

# ==========================
# main.c
# ==========================
cat > $PROJECT/src/main.c << 'EOF'
#include <ncurses.h>
#include <string.h>
#include "ui.h"
#include "cell.h"
#include "formula.h"
#include "csv.h"
#include "utils.h"

int main() {
    Cell sheet[MAX_ROWS][MAX_COLS];
    int cur_row = 0, cur_col = 0;
    int nrows = 10, ncols = 5;

    int formula_mode = 0;
    char formula_buffer[FORMULA_MAX];
    int formula_row = -1, formula_col = -1;
    int dynamic_pos = -1;

    int edit_mode = 0;
    char edit_buffer[CELL_LEN];

    int row_offset = 0, col_offset = 0;
    int last_ch = 0;

    int filter_active = 0;
    int filter_col = -1;
    char filter_value[CELL_LEN];

    memset(sheet, 0, sizeof(sheet));

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    int ch;
    while (1) {
        draw_sheet_filtered(sheet, nrows, ncols, cur_row, cur_col,
                            &row_offset, &col_offset, edit_mode, formula_mode,
                            formula_buffer, filter_active, filter_col, filter_value,
                            edit_buffer, last_ch);

        ch = getch();

        process_input(ch, sheet, &cur_row, &cur_col, &nrows, &ncols,
                      &formula_mode, formula_buffer, &formula_row, &formula_col,
                      &dynamic_pos, &edit_mode, edit_buffer,
                      &row_offset, &col_offset,
                      &last_ch, &filter_active, &filter_col, filter_value);
    }

    endwin();
    return 0;
}
EOF

# ==========================
# ui.c / ui.h
# ==========================
cat > $PROJECT/src/ui.c << 'EOF'
#include <ncurses.h>
#include <string.h>
#include "ui.h"
#include "formula.h"
#include "cell.h"
#include "utils.h"

void draw_sheet_filtered(Cell sheet[MAX_ROWS][MAX_COLS], int nrows, int ncols,
                         int cur_row, int cur_col, int *row_offset, int *col_offset,
                         int edit_mode, int formula_mode, char formula_buffer[FORMULA_MAX],
                         int filter_active, int filter_col, char filter_value[CELL_LEN],
                         char edit_buffer[CELL_LEN], int last_ch) {
    clear();
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    int visible_rows = max_y - 5;
    int visible_cols = max_x / 12;

    if (cur_row < *row_offset) *row_offset = cur_row;
    else if (cur_row >= *row_offset + visible_rows) *row_offset = cur_row - visible_rows + 1;

    if (cur_col < *col_offset) *col_offset = cur_col;
    else if (cur_col >= *col_offset + visible_cols) *col_offset = cur_col - visible_cols + 1;

    // Encabezados
    for (int j = 0; j < visible_cols && j + *col_offset < ncols; j++) {
        char colname[10];
        cell_name(0, j + *col_offset, colname);
        mvprintw(0, (j+1)*12, "%-11s", colname);
    }

    int line = 1;
    for (int i = *row_offset; i < nrows && line <= visible_rows; i++) {
        if (filter_active && filter_col >= 0 && strstr(sheet[i][filter_col].data, filter_value) == NULL)
            continue;

        mvprintw(line, 0, "%-3d", i+1);
        for (int j = 0; j < visible_cols && j + *col_offset < ncols; j++) {
            int c = j + *col_offset;
            if (edit_mode && i == cur_row && c == cur_col)
                mvprintw(line, (j+1)*12, "%-11s", edit_buffer);
            else if (sheet[i][c].data[0] == '=')
                mvprintw(line, (j+1)*12, "%-11.2f", eval_formula(sheet[i][c].data, sheet));
            else
                mvprintw(line, (j+1)*12, "%-11s", sheet[i][c].data[0] ? sheet[i][c].data : ".");
        }
        line++;
    }

    mvprintw(visible_rows + 2, 0, "Modo: %s", formula_mode ? "FORMULA" : edit_mode ? "EDIT" : "NORMAL");
    if (formula_mode) mvprintw(visible_rows + 3, 0, "Formula: %s", formula_buffer);
    if (filter_active) mvprintw(visible_rows + 4, 0, "Filtro activo: Columna %d contiene '%s'", filter_col+1, filter_value);

    move(cur_row - *row_offset + 1, (cur_col - *col_offset + 1) * 12);
    refresh();
}
EOF

cat > $PROJECT/include/ui.h << 'EOF'
#ifndef UI_H
#define UI_H
#include "cell.h"

void draw_sheet_filtered(Cell sheet[MAX_ROWS][MAX_COLS], int nrows, int ncols,
                         int cur_row, int cur_col, int *row_offset, int *col_offset,
                         int edit_mode, int formula_mode, char formula_buffer[FORMULA_MAX],
                         int filter_active, int filter_col, char filter_value[CELL_LEN],
                         char edit_buffer[CELL_LEN], int last_ch);

void process_input(int ch, Cell sheet[MAX_ROWS][MAX_COLS],
                   int *cur_row, int *cur_col, int *nrows, int *ncols,
                   int *formula_mode, char formula_buffer[FORMULA_MAX],
                   int *formula_row, int *formula_col, int *dynamic_pos,
                   int *edit_mode, char edit_buffer[CELL_LEN],
                   int *row_offset, int *col_offset,
                   int *last_ch, int *filter_active, int *filter_col,
                   char filter_value[CELL_LEN]);
#endif
EOF

# ==========================
# utils.c / utils.h
# ==========================
cat > $PROJECT/src/utils.c << 'EOF'
#include <ctype.h>
#include <string.h>
#include "utils.h"

void sanitize(char *s) {
    char *p=s;
    while(*p) { if(*p=='\r') *p='\0'; p++; }
}

void cell_name(int row, int col, char *buf) {
    char colname[10];
    int c=col,len=0;
    do { colname[len++]='A'+(c%26); c=c/26-1; } while(c>=0);
    for(int i=0;i<len;i++) buf[i]=colname[len-1-i];
    sprintf(buf+len,"%d", row+1);
}

int parse_cell(const char *ref, int *row, int *col) {
    int c=0,r=0,i=0;
    while(isalpha(ref[i])) { c=c*26 + (toupper(ref[i])- 'A' +1); i++; }
    c--;
    while(isdigit(ref[i])) { r=r*10 + (ref[i]-'0'); i++; }
    r--;
    if(r<0||r>=MAX_ROWS||c<0||c>=MAX_COLS) return 0;
    *row=r; *col=c;
    return 1;
}
EOF

cat > $PROJECT/include/utils.h << 'EOF'
#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include "cell.h"

#define MAX_ROWS 1000
#define MAX_COLS 1000
#define CELL_LEN 64
#define FORMULA_MAX 256

typedef struct {
    char data[CELL_LEN];
} Cell;

void sanitize(char *s);
void cell_name(int row, int col, char *buf);
int parse_cell(const char *ref, int *row, int *col);

#endif
EOF

# ==========================
# Makefile
# ==========================
cat > $PROJECT/Makefile << 'EOF'
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
EOF

echo "Proyecto modular completo generado en $PROJECT/"
echo "Usa 'cd $PROJECT && make' para compilar y './spreadsheet' para ejecutar"
