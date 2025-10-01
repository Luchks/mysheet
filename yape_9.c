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

    // Construir nueva fórmula en buffer temporal
    char tmp[FORMULA_MAX];
    strncpy(tmp, formula_buffer, dynamic_pos);
    tmp[dynamic_pos] = '\0';

    strncat(tmp, ref, FORMULA_MAX - strlen(tmp) - 1);

    // Copiar de vuelta a buffers globales
    strncpy(formula_buffer, tmp, FORMULA_MAX - 1);
    formula_buffer[FORMULA_MAX - 1] = '\0';

    strncpy(sheet[formula_row][formula_col].data, formula_buffer, CELL_LEN - 1);
    sheet[formula_row][formula_col].data[CELL_LEN - 1] = '\0';
}

// Mostrar hoja
void draw_sheet() {
    clear();

    // Dibujar celdas
    for (int i = 0; i < nrows; i++) {
        for (int j = 0; j < ncols; j++) {
            mvprintw(i, j * 12, "%-11s", sheet[i][j].data[0] ? sheet[i][j].data : ".");
        }
    }

    // Dibujar cabeceras
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

        if (!formula_mode) { // ----- modo normal -----
            if (ch == 'q') break;
            else if (ch == '=') {
                formula_mode = 1;
                formula_row = cur_row;
                formula_col = cur_col;
                strcpy(formula_buffer, "=");
                dynamic_pos = 1;  // después del '='
                strcpy(sheet[cur_row][cur_col].data, formula_buffer);
            }
            else if (ch == 'h' && cur_col > 0) cur_col--;
            else if (ch == 'l' && cur_col < ncols - 1) cur_col++;
            else if (ch == 'k' && cur_row > 0) cur_row--;
            else if (ch == 'j' && cur_row < nrows - 1) cur_row++;
        } else { // ----- modo fórmula -----
            if (ch == 27) { // Esc
                formula_mode = 0;
                formula_row = formula_col = -1;
                dynamic_pos = -1;
            }
            else if (ch == '\n') { // Enter
                formula_mode = 0;
                formula_row = formula_col = -1;
                dynamic_pos = -1;
            }
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
