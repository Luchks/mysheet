#!/bin/bash
# script.sh - Genera proyecto modular C tipo hoja de cálculo

echo "Creando carpetas..."
mkdir -p src include obj

echo "Creando archivos..."

# --- include/cell.h ---
cat <<'EOF' > include/cell.h
#ifndef CELL_H
#define CELL_H

#define MAX_ROWS 1000
#define MAX_COLS 1000
#define CELL_LEN 64

typedef struct {
    char data[CELL_LEN];
} Cell;

#endif
EOF

# --- include/formula.h ---
cat <<'EOF' > include/formula.h
#ifndef FORMULA_H
#define FORMULA_H

#include "cell.h"

double eval_formula(const char *formula, Cell sheet[MAX_ROWS][MAX_COLS]);

#endif
EOF

# --- include/csv.h ---
cat <<'EOF' > include/csv.h
#ifndef CSV_H
#define CSV_H

#include "cell.h"

void load_csv(const char *filename, Cell sheet[MAX_ROWS][MAX_COLS], int *nrows, int *ncols);
void save_csv(const char *filename, Cell sheet[MAX_ROWS][MAX_COLS], int nrows, int ncols);

#endif
EOF

# --- include/ui.h ---
cat <<'EOF' > include/ui.h
#ifndef UI_H
#define UI_H

#include "cell.h"
#include "formula.h"
#include "csv.h"
#include "utils.h"

void draw_sheet_filtered(Cell sheet[MAX_ROWS][MAX_COLS],
                         int nrows, int ncols,
                         int cur_row, int cur_col,
                         int *row_offset, int *col_offset,
                         int edit_mode, int formula_mode,
                         char *formula_buffer,
                         int filter_active, int filter_col,
                         char *filter_value,
                         char *edit_buffer,
                         int last_ch);

void process_input(int ch, Cell sheet[MAX_ROWS][MAX_COLS],
                   int *cur_row, int *cur_col,
                   int *nrows, int *ncols,
                   int *formula_mode, char *formula_buffer,
                   int *formula_row, int *formula_col,
                   int *dynamic_pos,
                   int *edit_mode, char *edit_buffer,
                   int *row_offset, int *col_offset,
                   int *last_ch,
                   int *filter_active, int *filter_col,
                   char *filter_value);

#endif
EOF

# --- include/utils.h ---
cat <<'EOF' > include/utils.h
#ifndef UTILS_H
#define UTILS_H

#include "cell.h"

void sanitize(char *s);
void cell_name(int row, int col, char *buf);
int parse_cell(const char *ref, int *row, int *col);

#endif
EOF

# --- src/cell.c ---
cat <<'EOF' > src/cell.c
#include "cell.h"
// Aquí puedes agregar funciones de manipulación de celdas si quieres
EOF

# --- src/formula.c ---
cat <<'EOF' > src/formula.c
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "formula.h"
#include "cell.h"

double eval_expr(const char **s, Cell sheet[MAX_ROWS][MAX_COLS]);

double eval_formula(const char *formula, Cell sheet[MAX_ROWS][MAX_COLS]) {
    if (!formula || formula[0] != '=') return 0;
    const char *s = formula + 1;
    return eval_expr(&s, sheet);
}

double eval_expr(const char **s, Cell sheet[MAX_ROWS][MAX_COLS]) {
    double res = 0, num = 0;
    char op = '+';
    while (**s) {
        if (isspace(**s)) { (*s)++; continue; }
        if (**s == '(') { (*s)++; num = eval_expr(s, sheet); }
        else if (isalpha(**s)) {
            char ref[16]; int j=0;
            while (isalpha(**s) || isdigit(**s)) ref[j++] = *(*s)++;
            ref[j]='\0';
            int r,c;
            if (parse_cell(ref,&r,&c)) {
                if(sheet[r][c].data[0]=='=') num = eval_formula(sheet[r][c].data,sheet);
                else num = atof(sheet[r][c].data);
            } else num = 0;
        } else if (isdigit(**s) || **s=='.') num = strtod(*s,(char **)s);
        else if (**s==')') { (*s)++; break; }
        else { op=**s; (*s)++; continue; }

        switch(op) { case '+': res+=num; break; case '-': res-=num; break; case '*': res*=num; break; case '/': res/=num; break; }
        op=0;
    }
    return res;
}
EOF

# --- src/csv.c ---
cat <<'EOF' > src/csv.c
#include <stdio.h>
#include <string.h>
#include "csv.h"

void sanitize(char *s);

void load_csv(const char *filename, Cell sheet[MAX_ROWS][MAX_COLS], int *nrows, int *ncols) {
    FILE *f=fopen(filename,"r"); if(!f) return;
    char line[4096]; int row=0; *ncols=0;
    while(fgets(line,sizeof(line),f) && row<MAX_ROWS){
        int col=0; char *token=strtok(line,",\n");
        while(token && col<MAX_COLS){
            strncpy(sheet[row][col].data,token,CELL_LEN-1);
            sheet[row][col].data[CELL_LEN-1]='\0';
            sanitize(sheet[row][col].data);
            col++; token=strtok(NULL,",\n");
        }
        if(col>*ncols) *ncols=col;
        row++;
    }
    *nrows=row; fclose(f);
}

void save_csv(const char *filename, Cell sheet[MAX_ROWS][MAX_COLS], int nrows, int ncols) {
    FILE *f=fopen(filename,"w"); if(!f) return;
    for(int i=0;i<nrows;i++){
        for(int j=0;j<ncols;j++){
            if(sheet[i][j].data[0]=='=') fprintf(f,"%.2f",atof(sheet[i][j].data)); 
            else fprintf(f,"%s",sheet[i][j].data);
            if(j<ncols-1) fprintf(f,",");
        }
        fprintf(f,"\n");
    }
    fclose(f);
}
EOF

# --- src/utils.c ---
cat <<'EOF' > src/utils.c
#include <ctype.h>
#include <string.h>
#include "utils.h"

void sanitize(char *s) {
    char *p=s; while(*p){ if(*p=='\r') *p='\0'; p++; }
}

void cell_name(int row, int col, char *buf) {
    char colname[10]; int c=col,len=0;
    do { colname[len++]='A'+(c%26); c=c/26-1; } while(c>=0);
    for(int i=0;i<len;i++) buf[i]=colname[len-1-i];
    sprintf(buf+len,"%d",row+1);
}

int parse_cell(const char *ref, int *row, int *col) {
    int c=0,r=0,i=0;
    while(isalpha(ref[i])) { c=c*26+(toupper(ref[i])- 'A'+1); i++; }
    c--;
    while(isdigit(ref[i])) { r=r*10+(ref[i]-'0'); i++; }
    r--;
    if(r<0 || r>=MAX_ROWS || c<0 || c>=MAX_COLS) return 0;
    *row=r; *col=c; return 1;
}
EOF

# --- src/ui.c ---
cat <<'EOF' > src/ui.c
#include <ncurses.h>
#include <string.h>
#include "ui.h"

// Implementa aquí draw_sheet_filtered y process_input según tu código modular
EOF

# --- src/main.c ---
cat <<'EOF' > src/main.c
#include <ncurses.h>
#include <string.h>
#include "ui.h"
#include "cell.h"
#include "csv.h"
#include "formula.h"
#include "utils.h"

int main() {
    Cell sheet[MAX_ROWS][MAX_COLS];
    int cur_row=0, cur_col=0;
    int nrows=10, ncols=5;
    int formula_mode=0, formula_row=-1, formula_col=-1, dynamic_pos=-1;
    int edit_mode=0;
    char formula_buffer[256];
    char edit_buffer[CELL_LEN];
    int row_offset=0, col_offset=0;
    int last_ch=0;
    int filter_active=0, filter_col=-1;
    char filter_value[CELL_LEN];

    memset(sheet,0,sizeof(sheet));
    initscr();
    cbreak();
    noecho();
    keypad(stdscr,TRUE);

    int ch;
    while(1){
        draw_sheet_filtered(sheet,nrows,ncols,cur_row,cur_col,
                            &row_offset,&col_offset,
                            edit_mode,formula_mode,formula_buffer,
                            filter_active,filter_col,filter_value,
                            edit_buffer,last_ch);
        ch=getch();
        process_input(ch,sheet,&cur_row,&cur_col,&nrows,&ncols,
                      &formula_mode,formula_buffer,&formula_row,&formula_col,
                      &dynamic_pos,&edit_mode,edit_buffer,
                      &row_offset,&col_offset,&last_ch,
                      &filter_active,&filter_col,filter_value);
    }

    endwin();
    return 0;
}
EOF

# --- Makefile ---
cat <<'EOF' > Makefile
CC=gcc
CFLAGS=-Wall -Iinclude
LDFLAGS=-lncurses
OBJDIR=obj
SRCDIR=src
OBJS=$(OBJDIR)/cell.o $(OBJDIR)/csv.o $(OBJDIR)/formula.o $(OBJDIR)/utils.o $(OBJDIR)/ui.o $(OBJDIR)/main.o
TARGET=mysheet

all: $(TARGET)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

clean:
	rm -f $(OBJDIR)/*.o $(TARGET)
EOF

echo "Archivos creados correctamente."
