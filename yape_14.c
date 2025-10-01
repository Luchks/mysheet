#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_ROWS 20
#define MAX_COLS 20
#define CELL_LEN 64
#define FORMULA_MAX 256

typedef struct {
    char data[CELL_LEN];
} Cell;

Cell sheet[MAX_ROWS][MAX_COLS];
int cur_row = 0, cur_col = 0;
int nrows = 10, ncols = 5;

int formula_mode = 0;
char formula_buffer[FORMULA_MAX];
int formula_row = -1, formula_col = -1;
int dynamic_pos = -1;

// Convierte índices a nombre estilo Excel (A1, B2, etc.)
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

// Convierte A1, B2 a índices [fila, columna]
int cell_index(const char *ref, int *row, int *col) {
    int i = 0, c = 0;
    while (isalpha(ref[i])) {
        c = c * 26 + (toupper(ref[i]) - 'A' + 1);
        i++;
    }
    int r = atoi(ref + i);
    if (row) *row = r - 1;
    if (col) *col = c - 1;
    return 1;
}

// Actualiza la referencia dinámica en la celda de fórmula
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
}

// Dibuja hoja
void draw_sheet() {
    clear();
    for (int i = 0; i < nrows; i++) {
        for (int j = 0; j < ncols; j++) {
            mvprintw(i, j * 12, "%-11s", sheet[i][j].data[0] ? sheet[i][j].data : ".");
        }
    }

    for (int j = 0; j < ncols; j++) {
        char colname[10];
        cell_name(0, j, colname);
        mvprintw(nrows + 1, j * 12, "%-11s", colname);
    }

    mvprintw(nrows + 3, 0, "Modo: %s", formula_mode ? "FORMULA" : "NORMAL");
    if (formula_mode)
        mvprintw(nrows + 4, 0, "Formula: %s", formula_buffer);

    move(cur_row, cur_col * 12);
    refresh();
}

// Inserta fila antes de row
void insert_row(int row) {
    if (nrows >= MAX_ROWS) return;
    for (int i = nrows; i > row; i--) {
        for (int j = 0; j < ncols; j++)
            strcpy(sheet[i][j].data, sheet[i-1][j].data);
    }
    for (int j = 0; j < ncols; j++) sheet[row][j].data[0] = '\0';
    nrows++;
}

// Inserta columna antes de col
void insert_col(int col) {
    if (ncols >= MAX_COLS) return;
    for (int j = ncols; j > col; j--) {
        for (int i = 0; i < nrows; i++)
            strcpy(sheet[i][j].data, sheet[i][j-1].data);
    }
    for (int i = 0; i < nrows; i++) sheet[i][col].data[0] = '\0';
    ncols++;
}

// Elimina fila row
void delete_row(int row) {
    if (nrows <= 1) return;
    for (int i = row; i < nrows - 1; i++) {
        for (int j = 0; j < ncols; j++)
            strcpy(sheet[i][j].data, sheet[i+1][j].data);
    }
    for (int j = 0; j < ncols; j++)
        sheet[nrows-1][j].data[0] = '\0';
    nrows--;
}

// Elimina columna col
void delete_col(int col) {
    if (ncols <= 1) return;
    for (int j = col; j < ncols - 1; j++) {
        for (int i = 0; i < nrows; i++)
            strcpy(sheet[i][j].data, sheet[i][j+1].data);
    }
    for (int i = 0; i < nrows; i++)
        sheet[i][ncols-1].data[0] = '\0';
    ncols--;
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

        if (!formula_mode) {
            if (ch == 'q') break;
            else if (ch == '=') {
                formula_mode = 1;
                formula_row = cur_row;
                formula_col = cur_col;
                strcpy(formula_buffer, "=");
                dynamic_pos = 1;
                strcpy(sheet[cur_row][cur_col].data, formula_buffer);
            }
            else if (ch == 'h' && cur_col > 0) cur_col--;
            else if (ch == 'l' && cur_col < ncols - 1) cur_col++;
            else if (ch == 'k' && cur_row > 0) cur_row--;
            else if (ch == 'j' && cur_row < nrows - 1) cur_row++;
            else if (ch == 'i') insert_row(cur_row);   // insertar fila
            else if (ch == 'c') insert_col(cur_col);   // insertar columna
            else if (ch == 'd') delete_row(cur_row);   // eliminar fila
            else if (ch == 'x') delete_col(cur_col);   // eliminar columna
        } else {
            if (ch == 27) { formula_mode = 0; formula_row = formula_col = -1; dynamic_pos = -1; }
            else if (ch == '\n') { formula_mode = 0; formula_row = formula_col = -1; dynamic_pos = -1; }
            else if (ch == 'h' && cur_col > 0) { cur_col--; update_dynamic_ref(); }
            else if (ch == 'l' && cur_col < ncols - 1) { cur_col++; update_dynamic_ref(); }
            else if (ch == 'k' && cur_row > 0) { cur_row--; update_dynamic_ref(); }
            else if (ch == 'j' && cur_row < nrows - 1) { cur_row++; update_dynamic_ref(); }
            else if (ch == '+' || ch == '-' || ch == '*' || ch == '/') {
                int len = strlen(formula_buffer);
                if (len < FORMULA_MAX - 2) {
                    formula_buffer[len] = (char)ch;
                    formula_buffer[len+1] = '\0';
                    dynamic_pos = len + 1;
                    strcpy(sheet[formula_row][formula_col].data, formula_buffer);
                }
            }
        }
    }

    endwin();
    return 0;
}
