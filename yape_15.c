#include <ncurses.h>
#include <stdlib.h>
#include <string.h>

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
int dynamic_pos = -1;  // posición donde insertar refs dinámicas

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

// Dibujar hoja en ncurses
void draw_sheet() {
    clear();
    for (int i = 0; i < nrows; i++) {
        for (int j = 0; j < ncols; j++) {
            mvprintw(i, j * 12, "%-11s", sheet[i][j].data[0] ? sheet[i][j].data : ".");
        }
    }
    // Cabeceras de columnas
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

// Insertar fila en la posición actual
void insert_row(int pos) {
    if (nrows >= MAX_ROWS) return;
    for (int i = nrows; i > pos; i--)
        memcpy(sheet[i], sheet[i-1], sizeof(Cell)*MAX_COLS);
    memset(sheet[pos], 0, sizeof(Cell)*MAX_COLS);
    nrows++;
}

// Eliminar fila en la posición actual
void remove_row(int pos) {
    if (nrows <= 1) return;
    for (int i = pos; i < nrows-1; i++)
        memcpy(sheet[i], sheet[i+1], sizeof(Cell)*MAX_COLS);
    memset(sheet[nrows-1], 0, sizeof(Cell)*MAX_COLS);
    nrows--;
}

// Insertar columna en la posición actual
void insert_col(int pos) {
    if (ncols >= MAX_COLS) return;
    for (int i = 0; i < nrows; i++)
        for (int j = ncols; j > pos; j--)
            sheet[i][j] = sheet[i][j-1];
    for (int i = 0; i < nrows; i++)
        memset(&sheet[i][pos], 0, sizeof(Cell));
    ncols++;
}

// Eliminar columna en la posición actual
void remove_col(int pos) {
    if (ncols <= 1) return;
    for (int i = 0; i < nrows; i++)
        for (int j = pos; j < ncols-1; j++)
            sheet[i][j] = sheet[i][j+1];
    for (int i = 0; i < nrows; i++)
        memset(&sheet[i][ncols-1], 0, sizeof(Cell));
    ncols--;
}

// Extender fórmula a toda la columna
void fill_formula_column(int col) {
    if (col < 0 || col >= ncols) return;
    char *base = sheet[0][col].data;
    if (!base || base[0] != '=') return;

    for (int i = 1; i < nrows; i++) {
        char tmp[FORMULA_MAX];
        tmp[0] = '=';
        tmp[1] = '\0';
        for (int k = 1; base[k] != '\0'; k++) {
            if (base[k] >= 'A' && base[k] <= 'Z') { // columna
                char c[2] = { base[k], '\0' };
                strcat(tmp, c);
                char row_num = '1' + i; // reemplaza fila
                char r[2] = { row_num, '\0' };
                strcat(tmp, r);
            } else { // operador u otro
                char c[2] = { base[k], '\0' };
                strcat(tmp, c);
            }
        }
        strncpy(sheet[i][col].data, tmp, CELL_LEN-1);
        sheet[i][col].data[CELL_LEN-1] = '\0';
    }
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

        if (!formula_mode) { // Modo normal
            switch(ch) {
                case 'q': endwin(); return 0;
                case '=':
                    formula_mode = 1;
                    formula_row = cur_row;
                    formula_col = cur_col;
                    strcpy(formula_buffer, "=");
                    dynamic_pos = 1;
                    strcpy(sheet[cur_row][cur_col].data, formula_buffer);
                    break;
                case 'h': if(cur_col>0) cur_col--; break;
                case 'l': if(cur_col<ncols-1) cur_col++; break;
                case 'k': if(cur_row>0) cur_row--; break;
                case 'j': if(cur_row<nrows-1) cur_row++; break;
                case 'i': insert_row(cur_row); break;
                case 'd': remove_row(cur_row); break;
                case 'I': insert_col(cur_col); break;
                case 'D': remove_col(cur_col); break;
                case 'f': fill_formula_column(cur_col); break;
            }
        } else { // Modo fórmula
            if (ch == 27) { // Esc
                formula_mode = 0; formula_row = formula_col = -1; dynamic_pos = -1;
            } else if (ch == '\n') { // Enter
                formula_mode = 0; formula_row = formula_col = -1; dynamic_pos = -1;
            } else if (ch == 'h' && cur_col>0) { cur_col--; update_dynamic_ref(); }
            else if (ch == 'l' && cur_col<ncols-1) { cur_col++; update_dynamic_ref(); }
            else if (ch == 'k' && cur_row>0) { cur_row--; update_dynamic_ref(); }
            else if (ch == 'j' && cur_row<nrows-1) { cur_row++; update_dynamic_ref(); }
            else if (ch == '+' || ch == '-' || ch == '*' || ch == '/') {
                int len = strlen(formula_buffer);
                if (len < FORMULA_MAX-2) {
                    formula_buffer[len] = (char)ch;
                    formula_buffer[len+1] = '\0';
                    dynamic_pos = len+1;
                    strcpy(sheet[formula_row][formula_col].data, formula_buffer);
                }
            }
        }
    }

    endwin();
    return 0;
}
