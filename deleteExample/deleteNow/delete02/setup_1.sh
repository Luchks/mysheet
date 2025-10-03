#!/bin/bash
# Script para crear proyecto modular de hoja de cálculo en C

PROJECT="spreadsheet_modular_full"
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
# formula.c / formula.h
# ==========================
cat > $PROJECT/src/formula.c << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "formula.h"
#include "utils.h"

double eval_expr(const char **s, Cell sheet[MAX_ROWS][MAX_COLS]);

double eval_formula(const char *formula, Cell sheet[MAX_ROWS][MAX_COLS]) {
    if (!formula || formula[0]!='=') return 0;
    const char *s = formula +1;
    return eval_expr(&s, sheet);
}

double eval_expr(const char **s, Cell sheet[MAX_ROWS][MAX_COLS]) {
    double res=0,num=0;
    char op='+';
    while(**s) {
        if (isspace(**s)) { (*s)++; continue; }
        if (**s=='(') { (*s)++; num=eval_expr(s,sheet); }
        else if (isalpha(**s)) {
            char ref[16]; int j=0;
            while(isalpha(**s)||isdigit(**s)) ref[j++]=*(*s)++;
            ref[j]='\0';
            int r,c;
            if(parse_cell(ref,&r,&c)) {
                if(sheet[r][c].data[0]=='=') num=eval_formula(sheet[r][c].data,sheet);
                else num=atof(sheet[r][c].data);
            } else num=0;
        } else if (isdigit(**s)||**s=='.') num=strtod(*s,(char**)s);
        else if (**s==')') { (*s)++; break; }
        else { op=**s; (*s)++; continue; }

        switch(op) {
            case '+': res+=num; break;
            case '-': res-=num; break;
            case '*': res*=num; break;
            case '/': res/=num; break;
        }
        op=0;
    }
    return res;
}
EOF

cat > $PROJECT/include/formula.h << 'EOF'
#ifndef FORMULA_H
#define FORMULA_H
#include "utils.h"

double eval_formula(const char *formula, Cell sheet[MAX_ROWS][MAX_COLS]);

#endif
EOF

# ==========================
# cell.c / cell.h
# ==========================
cat > $PROJECT/src/cell.c << 'EOF'
#include <string.h>
#include "cell.h"
#include "utils.h"

void insert_row(Cell sheet[MAX_ROWS][MAX_COLS], int pos, int *nrows) {
    if (*nrows >= MAX_ROWS) return;
    for (int i = *nrows; i > pos; i--)
        memcpy(sheet[i], sheet[i-1], sizeof(Cell)*MAX_COLS);
    memset(sheet[pos], 0, sizeof(Cell)*MAX_COLS);
    (*nrows)++;
}

void remove_row(Cell sheet[MAX_ROWS][MAX_COLS], int pos, int *nrows) {
    if (*nrows <= 1) return;
    for (int i = pos; i < *nrows - 1; i++)
        memcpy(sheet[i], sheet[i+1], sizeof(Cell)*MAX_COLS);
    memset(sheet[*nrows - 1], 0, sizeof(Cell)*MAX_COLS);
    (*nrows)--;
}

void insert_col(Cell sheet[MAX_ROWS][MAX_COLS], int pos, int *ncols, int nrows) {
    if (*ncols >= MAX_COLS) return;
    for (int i = 0; i < nrows; i++)
        for (int j = *ncols; j > pos; j--)
            sheet[i][j] = sheet[i][j-1];
    for (int i = 0; i < nrows; i++)
        memset(&sheet[i][pos], 0, sizeof(Cell));
    (*ncols)++;
}

void remove_col(Cell sheet[MAX_ROWS][MAX_COLS], int pos, int *ncols, int nrows) {
    if (*ncols <= 1) return;
    for (int i = 0; i < nrows; i++)
        for (int j = pos; j < *ncols - 1; j++)
            sheet[i][j] = sheet[i][j+1];
    for (int i = 0; i < nrows; i++)
        memset(&sheet[i][*ncols - 1], 0, sizeof(Cell));
    (*ncols)--;
}
EOF

cat > $PROJECT/include/cell.h << 'EOF'
#ifndef CELL_H
#define CELL_H
#include "utils.h"

void insert_row(Cell sheet[MAX_ROWS][MAX_COLS], int pos, int *nrows);
void remove_row(Cell sheet[MAX_ROWS][MAX_COLS], int pos, int *nrows);
void insert_col(Cell sheet[MAX_ROWS][MAX_COLS], int pos, int *ncols, int nrows);
void remove_col(Cell sheet[MAX_ROWS][MAX_COLS], int pos, int *ncols, int nrows);

#endif
EOF

# ==========================
# csv.c / csv.h
# ==========================
cat > $PROJECT/src/csv.c << 'EOF'
#include <stdio.h>
#include <string.h>
#include "csv.h"
#include "formula.h"
#include "utils.h"

void load_csv(Cell sheet[MAX_ROWS][MAX_COLS], const char *filename, int *nrows, int *ncols) {
    FILE *f = fopen(filename,"r");
    if(!f) return;
    char line[4096];
    int row=0;
    *nrows=0; *ncols=0;
    while(fgets(line,sizeof(line),f) && row<MAX_ROWS){
        int col=0;
        char *token = strtok(line,",\n");
        while(token && col<MAX_COLS){
            strncpy(sheet[row][col].data, token, CELL_LEN-1);
            sheet[row][col].data[CELL_LEN-1]='\0';
            sanitize(sheet[row][col].data);
            col++;
            token=strtok(NULL,",\n");
        }
        if(col>*ncols) *ncols=col;
        row++;
    }
    *nrows=row;
    fclose(f);
}

void save_csv(Cell sheet[MAX_ROWS][MAX_COLS], const char *filename, int nrows, int ncols){
    FILE *f = fopen(filename,"w");
    if(!f) return;
    for(int i=0;i<nrows;i++){
        for(int j=0;j<ncols;j++){
            if(sheet[i][j].data[0]=='=')
                fprintf(f,"%.2f", eval_formula(sheet[i][j].data,sheet));
            else fprintf(f,"%s",sheet[i][j].data);
            if(j<ncols-1) fprintf(f,",");
       
        fprintf(f,"\n");
    }
    fclose(f);
}
EOF

cat > $PROJECT/include/csv.h << 'EOF'
#ifndef CSV_H
#define CSV_H
#include "utils.h"
#include "formula.h"

void load_csv(Cell sheet[MAX_ROWS][MAX_COLS], const char *filename, int *nrows, int *ncols);
void save_csv(Cell sheet[MAX_ROWS][MAX_COLS], const char *filename, int nrows, int ncols);

#endif
EOF

# ==========================
# ui.c / ui.h ya generados anteriormente
# ==========================

# ==========================
# Makefile
# ==========================
cat > $PROJECT/Makefile << 'EOF'
CC=gcc
CFLAGS=-Wall -lncurses -Iinclude
SRC=$(wildcard src/*.c)
OBJ=$(SRC:.c=.o)
EXEC=spreadsheet

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f $(OBJ) $(EXEC)
EOF

# ==========================
# Fin del script
# ==========================
echo "Proyecto modular creado en '$PROJECT'."
echo "Para compilar: cd $PROJECT && make"
echo "Para ejecutar: ./spreadsheet"
