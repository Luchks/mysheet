#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <ncurses.h>

typedef struct {
    char ***cells;
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

/* --- Fórmulas --- */

// Convierte "A"->0, "B"->1, ..., "Z"->25, "AA"->26, etc.
int col_from_label(const char *label) {
    int col = 0;
    for (int i = 0; label[i]; i++) {
        col = col * 26 + (toupper(label[i]) - 'A' + 1);
    }
    return col - 1;
}

// Convierte "C2" -> fila=1, col=2
int parse_ref(const char *ref, int *row, int *col) {
    char colpart[16], rowpart[16];
    int ci = 0, ri = 0;

    for (int i = 0; ref[i]; i++) {
        if (isalpha(ref[i]))
            colpart[ci++] = ref[i];
        else if (isdigit(ref[i]))
            rowpart[ri++] = ref[i];
        else
            return 0;
    }
    colpart[ci] = 0;
    rowpart[ri] = 0;
    if (ci == 0 || ri == 0) return 0;

    *col = col_from_label(colpart);
    *row = atoi(rowpart) - 1; // Excel es 1-based
    return 1;
}

// Evalúa "=A1+B2-C3"
double eval_formula(Sheet *s, const char *expr) {
    double result = 0;
    int sign = 1;
    char token[32];
    int ti = 0;

    for (int i = 1; ; i++) { // empezar después de '='
        char c = expr[i];
        if (c == '+' || c == '-' || c == 0) {
            token[ti] = 0;
            int row, col;
            if (parse_ref(token, &row, &col)) {
                const char *val = sheet_get(s, row, col);
                double num = atof(val);
                result += sign * num;
            }
            ti = 0;
            if (c == '+') sign = 1;
            if (c == '-') sign = -1;
            if (c == 0) break;
        } else if (!isspace(c)) {
            token[ti++] = c;
        }
    }
    return result;
}

// Obtiene valor mostrado: evalúa si es fórmula
const char* sheet_display(Sheet *s, int row, int col, char *buffer, int buflen) {
    const char *raw = sheet_get(s, row, col);
    if (raw[0] == '=') {
        double val = eval_formula(s, raw);
        snprintf(buffer, buflen, "%.2f", val);
        return buffer;
    }
    return raw;
}

/* --- Operaciones de filas/columnas (ya implementadas antes) --- */
// (por brevedad no repito todas, se mantienen igual que en la versión previa)

/* --- Renderizado con ncurses --- */

void sheet_print_ncurses(Sheet *s, int crow, int ccol) {
    clear();
    char buffer[64];
    for (int i = 0; i < s->nrows; i++) {
        for (int j = 0; j < s->ncols; j++) {
            const char *disp = sheet_display(s, i, j, buffer, sizeof(buffer));
            if (i == crow && j == ccol) attron(A_REVERSE);
            mvprintw(i, j * 10, "%-8s", disp);
            if (i == crow && j == ccol) attroff(A_REVERSE);
        }
    }
    mvprintw(s->nrows + 1, 0, "hjkl mover | e editar | = formula | q salir");
    refresh();
}

int main() {
    Sheet *s = sheet_create(5, 5);
    sheet_set(s, 0, 0, "10");
    sheet_set(s, 0, 1, "20");
    sheet_set(s, 1, 0, "30");
    sheet_set(s, 2, 0, "=A1+B2");

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
            case 'e': {
                echo();
                char buffer[128];
                mvprintw(s->nrows + 2, 0, "Nuevo valor: ");
                clrtoeol();
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
