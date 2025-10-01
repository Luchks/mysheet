#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAX_ROWS 20
#define MAX_COLS 20
#define CELL_LEN 64
#define FORMULA_MAX 256

typedef struct {
    char data[CELL_LEN];
    int value;
} Cell;

Cell sheet[MAX_ROWS][MAX_COLS];
int cur_row = 0, cur_col = 0;
int nrows = 10, ncols = 5;

int formula_mode = 0;
char formula_buffer[FORMULA_MAX];
int formula_row = -1, formula_col = -1;
int dynamic_pos = -1;

// Convierte índices a nombre estilo Excel
void cell_name(int row, int col, char *buf) {
    char colname[10];
    int c = col;
    int len = 0;
    do {
        colname[len++] = 'A' + (c % 26);
        c = c / 26 - 1;
    } while (c >= 0);
    for (int i = 0; i < len; i++)
        buf[i] = colname[len - 1 - i];
    sprintf(buf + len, "%d", row + 1);
}

// Convierte referencia estilo Excel a índice
int parse_cell_ref(const char *ref, int *row, int *col) {
    if (!ref || !*ref) return 0;
    int c = 0, i = 0;
    while (ref[i] >= 'A' && ref[i] <= 'Z') {
        c = c * 26 + (ref[i] - 'A' + 1);
        i++;
    }
    if (c == 0) return 0;
    *col = c - 1;
    int r = atoi(ref + i);
    if (r <= 0) return 0;
    *row = r - 1;
    return 1;
}

// Evalúa fórmula simple con + - * /
int evaluate_formula(const char *expr) {
    char buffer[FORMULA_MAX];
    strncpy(buffer, expr, FORMULA_MAX - 1);
    buffer[FORMULA_MAX - 1] = '\0';

    char *token = strtok(buffer, "+-*/");
    char ops[64];
    int op_count = 0;
    int values[64];
    int val_count = 0;

    while (token) {
        int r, c, v;
        if (parse_cell_ref(token, &r, &c)) {
            v = sheet[r][c].value;
        } else {
            v = atoi(token);
        }
        values[val_count++] = v;

        token += strlen(token);
        if (*token == '+' || *token == '-' || *token == '*' || *token == '/') {
            ops[op_count++] = *token;
            token++;
        }
        token = strtok(NULL, "+-*/");
    }

    if (val_count == 0) return 0;
    int res = values[0];
    for (int i = 0; i < op_count; i++) {
        if (ops[i] == '+') res += values[i+1];
        else if (ops[i] == '-') res -= values[i+1];
        else if (ops[i] == '*') res *= values[i+1];
        else if (ops[i] == '/' && values[i+1]!=0) res /= values[i+1];
    }
    return res;
}

// Actualiza referencia dinámica de la fórmula
void update_dynamic_ref() {
    if (formula_row < 0 || formula_col < 0 || dynamic_pos < 0) return;
    char ref[16];
    cell_name(cur_row, cur_col, ref);

    char tmp[FORMULA_MAX];
    strncpy(tmp, formula_buffer, dynamic_pos);
    tmp[dynamic_pos] = '\0';
    strncat(tmp, ref, FORMULA_MAX - strlen(tmp) - 1);

    strncpy(formula_buffer, tmp, FORMULA_MAX - 1);
    formula_buffer[FORMULA_MAX - 1] = '\0';

    strncpy(sheet[formula_row][formula_col].data, formula_buffer, CELL_LEN - 1);
    sheet[formula_row][formula_col].data[CELL_LEN - 1] = '\0';
    sheet[formula_row][formula_col].value = evaluate_formula(formula_buffer + 1);
}

// Inserta columna en posición pos
void insert_col(int pos) {
    if (ncols >= MAX_COLS) return;
    for (int i = 0; i < nrows; i++) {
        for (int j = ncols; j > pos; j--)
            sheet[i][j] = sheet[i][j-1];
        memset(&sheet[i][pos], 0, sizeof(Cell));
    }
    ncols++;
}

// Elimina columna en posición pos
void delete_col(int pos) {
    if (ncols <= 0) return;
    for (int i = 0; i < nrows; i++) {
        for (int j = pos; j < ncols-1; j++)
            sheet[i][j] = sheet[i][j+1];
        memset(&sheet[i][ncols-1], 0, sizeof(Cell));
    }
    ncols--;
}

// Inserta fila en posición pos
void insert_row(int pos) {
    if (nrows >= MAX_ROWS) return;
    for (int i = nrows; i > pos; i--)
        for (int j = 0; j < ncols; j++)
            sheet[i][j] = sheet[i-1][j];
    for (int j = 0; j < ncols; j++)
        memset(&sheet[pos][j], 0, sizeof(Cell));
    nrows++;
}

// Elimina fila en posición pos
void delete_row(int pos) {
    if (nrows <= 0) return;
    for (int i = pos; i < nrows-1; i++)
        for (int j = 0; j < ncols; j++)
            sheet[i][j] = sheet[i+1][j];
    for (int j = 0; j < ncols; j++)
        memset(&sheet[nrows-1][j], 0, sizeof(Cell));
    nrows--;
}

// Mostrar hoja
void draw_sheet() {
    clear();
    for (int i = 0; i < nrows; i++)
        for (int j = 0; j < ncols; j++)
            mvprintw(i, j*12, "%-11d", sheet[i][j].data[0]?sheet[i][j].value:0);

    for (int j = 0; j < ncols; j++) {
        char colname[10];
        cell_name(0, j, colname);
        mvprintw(nrows+1, j*12, "%-11s", colname);
    }

    mvprintw(nrows+3, 0, "Modo: %s", formula_mode?"FORMULA":"NORMAL");
    if (formula_mode) mvprintw(nrows+4, 0, "Formula: %s", formula_buffer);

    move(cur_row, cur_col*12);
    refresh();
}

int main() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    memset(sheet, 0, sizeof(sheet));

    int ch;
    while (1) {
        draw_sheet();
        ch = getch();

        if (!formula_mode) { // modo normal
            if (ch == 'q') break;
            else if (ch == '=') {
                formula_mode = 1;
                formula_row = cur_row;
                formula_col = cur_col;
                strcpy(formula_buffer, "=");
                dynamic_pos = 1;
                strcpy(sheet[cur_row][cur_col].data, formula_buffer);
            }
            else if (ch == 'h' && cur_col>0) cur_col--;
            else if (ch == 'l' && cur_col<ncols-1) cur_col++;
            else if (ch == 'k' && cur_row>0) cur_row--;
            else if (ch == 'j' && cur_row<nrows-1) cur_row++;
            else if (ch == 'i') insert_col(cur_col);       // insertar columna
            else if (ch == 'd') delete_col(cur_col);       // eliminar columna
            else if (ch == 'L') insert_col(ncols);        // añadir columna final
            else if (ch == 'I') insert_row(cur_row);      // insertar fila
            else if (ch == 'D') delete_row(cur_row);      // eliminar fila
        } else { // modo fórmula
            if (ch == 27) {
                formula_mode = 0;
                formula_row = formula_col = -1;
                dynamic_pos = -1;
            }
            else if (ch == '\n') {
                formula_mode = 0;
                formula_row = formula_col = -1;
                dynamic_pos = -1;
            }
            else if (ch == 'h' && cur_col>0) { cur_col--; update_dynamic_ref(); }
            else if (ch == 'l' && cur_col<ncols-1) { cur_col++; update_dynamic_ref(); }
            else if (ch == 'k' && cur_row>0) { cur_row--; update_dynamic_ref(); }
            else if (ch == 'j' && cur_row<nrows-1) { cur_row++; update_dynamic_ref(); }
            else if (ch=='+'||ch=='-'||ch=='*'||ch=='/') {
                int len = strlen(formula_buffer);
                if (len < FORMULA_MAX-2) {
                    formula_buffer[len]=(char)ch;
                    formula_buffer[len+1]='\0';
                    dynamic_pos = len+1;
                    strncpy(sheet[formula_row][formula_col].data, formula_buffer, CELL_LEN-1);
                    sheet[formula_row][formula_col].data[CELL_LEN-1]='\0';
                    sheet[formula_row][formula_col].value = evaluate_formula(formula_buffer+1);
                }
            }
        }
    }

    endwin();
    return 0;
}
