#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>

typedef struct {
    char ***cells;   // matriz de strings [nrows][ncols]
    int nrows;
    int ncols;
} Sheet;

Sheet* sheet_create(int rows, int cols) {
    Sheet *s = malloc(sizeof(Sheet));
    s->nrows = rows;
    s->ncols = cols;

    s->cells = malloc(rows * sizeof(char**));
    for (int i = 0; i < rows; i++) {
        s->cells[i] = calloc(cols, sizeof(char*));
    }
    return s;
}

void sheet_free(Sheet *s) {
    for (int i = 0; i < s->nrows; i++) {
        for (int j = 0; j < s->ncols; j++) {
            free(s->cells[i][j]);
        }
        free(s->cells[i]);
    }
    free(s->cells);
    free(s);
}

const char* sheet_get(Sheet *s, int row, int col) {
    if (row >= s->nrows || col >= s->ncols) return "";
    return s->cells[row][col] ? s->cells[row][col] : "";
}

void sheet_set(Sheet *s, int row, int col, const char *value) {
    if (row >= s->nrows || col >= s->ncols) return;
    free(s->cells[row][col]);
    s->cells[row][col] = strdup(value);
}

void sheet_add_col(Sheet *s) {
    for (int i = 0; i < s->nrows; i++) {
        s->cells[i] = realloc(s->cells[i], (s->ncols + 1) * sizeof(char*));
        s->cells[i][s->ncols] = NULL;
    }
    s->ncols++;
}

// Insertar columna en posición arbitraria
void sheet_insert_col(Sheet *s, int pos) {
    if (pos < 0 || pos > s->ncols) return;
    for (int i = 0; i < s->nrows; i++) {
        s->cells[i] = realloc(s->cells[i], (s->ncols + 1) * sizeof(char*));
        for (int j = s->ncols; j > pos; j--) {
            s->cells[i][j] = s->cells[i][j - 1];
        }
        s->cells[i][pos] = NULL;
    }
    s->ncols++;
}

// Insertar fila en posición arbitraria
void sheet_insert_row(Sheet *s, int pos) {
    if (pos < 0 || pos > s->nrows) return;
    s->cells = realloc(s->cells, (s->nrows + 1) * sizeof(char**));
    // mover filas hacia abajo
    for (int i = s->nrows; i > pos; i--) {
        s->cells[i] = s->cells[i - 1];
    }
    // nueva fila vacía
    s->cells[pos] = calloc(s->ncols, sizeof(char*));
    s->nrows++;
}

void sheet_print_ncurses(Sheet *s, int crow, int ccol) {
    clear();
    for (int i = 0; i < s->nrows; i++) {
        for (int j = 0; j < s->ncols; j++) {
            if (i == crow && j == ccol) {
                attron(A_REVERSE); // resalta celda activa
            }
            mvprintw(i, j * 10, "%-8s", sheet_get(s, i, j));
            if (i == crow && j == ccol) {
                attroff(A_REVERSE);
            }
        }
    }
    mvprintw(s->nrows + 1, 0, "hjkl: mover | a: col al final | i: insertar col | I: insertar fila | e: editar | q: salir");
    refresh();
}

int main() {
    Sheet *s = sheet_create(5, 3);
    sheet_set(s, 0, 0, "10");
    sheet_set(s, 0, 1, "20");
    sheet_set(s, 1, 0, "30");

    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);

    int crow = 0, ccol = 0;
    int ch;
    sheet_print_ncurses(s, crow, ccol);

    while ((ch = getch()) != 'q') {
        switch (ch) {
            case 'h': if (ccol > 0) ccol--; break;
            case 'l': if (ccol < s->ncols - 1) ccol++; break;
            case 'k': if (crow > 0) crow--; break;
            case 'j': if (crow < s->nrows - 1) crow++; break;
            case 'a': sheet_add_col(s); break; // añadir columna al final
            case 'i': sheet_insert_col(s, ccol); break; // insertar columna aquí
            case 'I': sheet_insert_row(s, crow); break; // insertar fila aquí
            case 'e': { // editar celda actual
                echo();
                char buffer[128];
                mvprintw(s->nrows + 2, 0, "Nuevo valor: ");
                getnstr(buffer, 127);
                sheet_set(s, crow, ccol, buffer);
                noecho();
            } break;
        }
        sheet_print_ncurses(s, crow, ccol);
    }

    endwin();
    sheet_free(s);
    return 0;
}
