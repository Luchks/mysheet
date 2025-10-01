// minisheet.c
// Compilar: gcc -O2 -o minisheet minisheet.c -lncurses

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <ncurses.h>

#define MAX_ROWS 200
#define MAX_COLS 52
#define CELL_RAW_LEN 256

typedef struct {
    char raw[CELL_RAW_LEN]; // lo que el usuario escribe (ej. "hola", "=A1+B2-3")
    double value;           // valor numérico evaluado si aplica
    int evaluated;          // 0 = no, 1 = sí (para caches de evaluación)
    int visiting;           // para detección de ciclos
} Cell;

static Cell sheet[MAX_ROWS][MAX_COLS];
static int nrows = 20;
static int ncols = 8;
static int cur_r = 0, cur_c = 0;
static int win_rows, win_cols, grid_row_offset = 0, grid_col_offset = 0;

//////////////////////
// Utilidades
//////////////////////

// Convertir número de columna (0-based) a etiqueta (A, B, ..., Z, AA, AB...)
void col_label(int c, char *out, int outlen) {
    if (c < 26) {
        snprintf(out, outlen, "%c", 'A' + c);
    } else {
        int hi = c / 26 - 1;
        int lo = c % 26;
        snprintf(out, outlen, "%c%c", 'A' + hi, 'A' + lo);
    }
}

// Convertir etiqueta como "A" o "AB" a índice (0-based). Devuelve -1 si inválido.
int col_index(const char *s) {
    int i = 0;
    int idx = 0;
    while (s[i] && isalpha((unsigned char)s[i])) {
        idx = idx * 26 + (toupper((unsigned char)s[i]) - 'A' + 1);
        i++;
        if (i > 3) return -1;
    }
    return idx - 1;
}

// Parsear referencia tipo A1, B12 -> fila y columna. Devuelve 0 si bien.
int parse_ref(const char *token, int *out_r, int *out_c) {
    // separar parte letra y numero
    int i = 0;
    while (token[i] && isalpha((unsigned char)token[i])) i++;
    if (i == 0) return -1;
    char colpart[8]; memset(colpart,0,sizeof(colpart));
    strncpy(colpart, token, i);
    int col = col_index(colpart);
    if (col < 0) return -1;
    int row = atoi(token + i);
    if (row <= 0) return -1;
    *out_r = row - 1;
    *out_c = col;
    if (*out_r < 0 || *out_r >= MAX_ROWS || *out_c < 0 || *out_c >= MAX_COLS) return -1;
    return 0;
}

//////////////////////
// Evaluador simple (+ -) con referencias y detección de ciclos
//////////////////////

// El evaluador asume expresiones sin paréntesis, tokens separados por + o -
// Ejemplos válidos: "A1+3-B2+4.5", "10-5", "A1"
double eval_cell_recursive(int r, int c, int *err);

double parse_number_or_ref(const char *tok, int *err) {
    // si empieza con letra -> referencia
    if (isalpha((unsigned char)tok[0])) {
        int rr, cc;
        if (parse_ref(tok, &rr, &cc) == 0) {
            return eval_cell_recursive(rr, cc, err);
        } else {
            *err = 1;
            return 0.0;
        }
    } else {
        // número literal
        char *endp;
        double v = strtod(tok, &endp);
        if (endp == tok) { *err = 1; return 0.0; }
        return v;
    }
}

double eval_cell_recursive(int r, int c, int *err) {
    if (r < 0 || r >= nrows || c < 0 || c >= ncols) { *err = 1; return 0.0; }
    Cell *cell = &sheet[r][c];
    if (cell->visiting) { *err = 1; return 0.0; } // ciclo
    if (cell->evaluated) return cell->value;

    // si no es fórmula, intentar convertir a número; si no, devolver 0 y err=1 si requerido
    if (cell->raw[0] != '=') {
        char *endp;
        double v = strtod(cell->raw, &endp);
        if (endp != cell->raw) {
            cell->value = v;
            cell->evaluated = 1;
            return v;
        } else {
            cell->value = 0.0;
            cell->evaluated = 1;
            return 0.0; // texto tratado como 0
        }
    }

    // fórmula: evaluar
    cell->visiting = 1;
    const char *expr = cell->raw + 1; // saltar '='
    // tokenizar por + y -, pero manteniendo operadores
    // recorrer manualmente
    int i = 0, L = strlen(expr);
    char token[128];
    int pos = 0;
    double total = 0.0;
    int expect_number = 1;
    char op = '+'; // operador actual
    while (i <= L) {
        char ch = expr[i];
        if (ch == '+' || ch == '-' || ch == '\0') {
            if (pos == 0) { // caso: unary +/-
                if (ch == '-') {
                    // handle unary minus by starting token with '-'
                    token[pos++] = '-';
                    i++; continue;
                } else { i++; continue; }
            }
            token[pos] = '\0';
            int local_err = 0;
            double val = parse_number_or_ref(token, &local_err);
            if (local_err) { *err = 1; cell->visiting = 0; return 0.0; }
            if (op == '+') total += val; else total -= val;
            // reset token
            pos = 0;
            op = ch;
            i++;
        } else if (!isspace((unsigned char)ch)) {
            if (pos < (int)sizeof(token)-1) token[pos++] = ch;
            i++;
        } else {
            i++;
        }
    }

    cell->value = total;
    cell->evaluated = 1;
    cell->visiting = 0;
    return total;
}

double eval_cell(int r, int c, int *err) {
    // limpiar flags de evaluacion globalmente antes
    for (int i=0;i<nrows;i++) for (int j=0;j<ncols;j++) {
        sheet[i][j].evaluated = 0;
        sheet[i][j].visiting = 0;
    }
    *err = 0;
    return eval_cell_recursive(r, c, err);
}

//////////////////////
// I/O CSV
//////////////////////

void load_csv(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) return;
    char line[4096];
    int r = 0;
    while (fgets(line, sizeof(line), f) && r < MAX_ROWS) {
        // simple CSV parse: split by comma, no quoting handling advanced
        int c = 0;
        char *p = line;
        char *celltok;
        // strtok_r with comma and newline
        char *saveptr;
        celltok = strtok_r(p, ",\n\r", &saveptr);
        while (celltok && c < MAX_COLS) {
            strncpy(sheet[r][c].raw, celltok, CELL_RAW_LEN-1);
            sheet[r][c].raw[CELL_RAW_LEN-1] = '\0';
            sheet[r][c].evaluated = 0;
            sheet[r][c].visiting = 0;
            c++;
            celltok = strtok_r(NULL, ",\n\r", &saveptr);
        }
        if (c > ncols) ncols = c;
        r++;
    }
    nrows = (r>0)?r:nrows;
    fclose(f);
}

void save_csv(const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f) return;
    for (int i=0;i<nrows;i++) {
        for (int j=0;j<ncols;j++) {
            fprintf(f, "%s", sheet[i][j].raw);
            if (j < ncols-1) fprintf(f, ",");
        }
        fprintf(f, "\n");
    }
    fclose(f);
}

//////////////////////
// Manipulación filas/columnas
//////////////////////

void insert_row(int at) {
    if (nrows >= MAX_ROWS) return;
    if (at < 0 || at > nrows) at = nrows;
    // mover hacia abajo
    for (int r = nrows; r > at; r--) {
        for (int c=0;c<ncols;c++)
            sheet[r][c] = sheet[r-1][c];
    }
    // limpiar fila nueva
    for (int c=0;c<ncols;c++) {
        sheet[at][c].raw[0] = '\0';
        sheet[at][c].evaluated = 0;
        sheet[at][c].visiting = 0;
    }
    nrows++;
}

void delete_row(int at) {
    if (nrows <= 1) return;
    if (at < 0 || at >= nrows) return;
    for (int r = at; r < nrows-1; r++) {
        for (int c=0;c<ncols;c++)
            sheet[r][c] = sheet[r+1][c];
    }
    // limpiar ultima
    for (int c=0;c<ncols;c++) {
        sheet[nrows-1][c].raw[0] = '\0';
        sheet[nrows-1][c].evaluated = 0;
        sheet[nrows-1][c].visiting = 0;
    }
    nrows--;
}

void insert_col(int at) {
    if (ncols >= MAX_COLS) return;
    if (at < 0 || at > ncols) at = ncols;
    for (int r=0;r<nrows;r++) {
        for (int c=ncols;c>at;c--)
            sheet[r][c] = sheet[r][c-1];
        sheet[r][at].raw[0] = '\0';
        sheet[r][at].evaluated = 0;
        sheet[r][at].visiting = 0;
    }
    ncols++;
}

void delete_col(int at) {
    if (ncols <= 1) return;
    if (at < 0 || at >= ncols) return;
    for (int r=0;r<nrows;r++) {
        for (int c=at;c<ncols-1;c++)
            sheet[r][c] = sheet[r][c+1];
        sheet[r][ncols-1].raw[0] = '\0';
        sheet[r][ncols-1].evaluated = 0;
        sheet[r][ncols-1].visiting = 0;
    }
    ncols--;
}

//////////////////////
// UI con ncurses
//////////////////////

void draw() {
    clear();
    getmaxyx(stdscr, win_rows, win_cols);

    // encabezado de columnas
    mvaddstr(0, 0, "     ");
    int colx = 5;
    for (int c = grid_col_offset; c < ncols && colx + 10 < win_cols; c++) {
        char label[8]; col_label(c, label, sizeof(label));
        mvprintw(0, colx, "%-10s", label);
        colx += 10;
    }

    // filas
    int rowy = 1;
    for (int r = grid_row_offset; r < nrows && rowy < win_rows-2; r++) {
        mvprintw(rowy, 0, "%4d ", r+1);
        colx = 5;
        for (int c = grid_col_offset; c < ncols && colx + 10 < win_cols; c++) {
            // mostrar valor: si fórmula -> evaluar; si texto -> raw (corto)
            int err=0;
            double val = eval_cell(r,c,&err);
            char celldisp[64];
            if (sheet[r][c].raw[0] == '=') {
                if (err) snprintf(celldisp, sizeof(celldisp), "ERR");
                else snprintf(celldisp, sizeof(celldisp), "%.4g", val);
            } else {
                if (strlen(sheet[r][c].raw) == 0) strcpy(celldisp,"");
                else {
                    // mostrar raw si cabe, si es número pref muestra como número
                    char *endp;
                    strtod(sheet[r][c].raw, &endp);
                    if (endp != sheet[r][c].raw) snprintf(celldisp, sizeof(celldisp), "%.4g", val);
                    else {
                        strncpy(celldisp, sheet[r][c].raw, sizeof(celldisp)-1);
                        celldisp[sizeof(celldisp)-1] = '\0';
                    }
                }
            }
            // resaltar celda actual
            if (r == cur_r && c == cur_c) {
                attron(A_REVERSE);
                mvprintw(rowy, colx, "%-10s", celldisp);
                attroff(A_REVERSE);
            } else {
                mvprintw(rowy, colx, "%-10s", celldisp);
            }
            colx += 10;
        }
        rowy++;
    }

    // status bar
    mvhline(win_rows-2, 0, '-', win_cols);
    char status[256];
    char collabel[8]; col_label(cur_c, collabel, sizeof(collabel));
    int err;
    double curval = eval_cell(cur_r, cur_c, &err);
    if (sheet[cur_r][cur_c].raw[0] == '=') {
        snprintf(status, sizeof(status), "Mode: NORMAL | Pos: %s%d | Raw: %s | Val: %s",
                 collabel, cur_r+1,
                 sheet[cur_r][cur_c].raw,
                 err ? "ERR" : (strlen(sheet[cur_r][cur_c].raw)? (char[32]){} : ""))
                 ;
        // compose val separately due to snprintf complexity
        move(win_rows-1,0);
    } else {
        snprintf(status, sizeof(status), "Mode: NORMAL | Pos: %s%d | Raw: %s",
                 collabel, cur_r+1,
                 sheet[cur_r][cur_c].raw);
    }
    // print status left and hints right
    mvprintw(win_rows-1, 0, "%s", status);
    mvprintw(win_rows-1, win_cols-40, "h/j/k/l mover | i editar | :w guardar | :q salir");

    refresh();
}

// entrada en modo edición (simple): leer línea en la parte inferior
void edit_cell() {
    echo();
    curs_set(1);
    char input[CELL_RAW_LEN];
    // prompt
    mvprintw(win_rows-3, 0, "Edit (%s%d): ", "", 0);
    // limpiar linea
    move(win_rows-3,0);
    clrtoeol();
    char collabel[8]; col_label(cur_c, collabel, sizeof(collabel));
    mvprintw(win_rows-3, 0, "Edit %s%d: ", collabel, cur_r+1);
    mvgetnstr(win_rows-3, 10 + strlen(collabel), input, CELL_RAW_LEN-1);
    // set
    strncpy(sheet[cur_r][cur_c].raw, input, CELL_RAW_LEN-1);
    sheet[cur_r][cur_c].raw[CELL_RAW_LEN-1] = '\0';
    sheet[cur_r][cur_c].evaluated = 0;
    sheet[cur_r][cur_c].visiting = 0;
    noecho();
    curs_set(0);
}

//////////////////////
// Main loop y comandos tipo ':' (guardar, salir, insertar, borrar)
//////////////////////

void colon_command() {
    echo();
    curs_set(1);
    char cmd[256];
    mvprintw(win_rows-3, 0, ":");
    clrtoeol();
    mvgetnstr(win_rows-3, 1, cmd, 255);
    // parse
    if (strcmp(cmd, "q") == 0) {
        endwin();
        exit(0);
    } else if (strcmp(cmd, "w") == 0) {
        // pedir nombre de archivo
        char fname[256];
        mvprintw(win_rows-3, 0, "Save as: ");
        clrtoeol();
        mvgetnstr(win_rows-3, 9, fname, 255);
        save_csv(fname);
    } else if (strcmp(cmd, "ir") == 0) {
        insert_row(cur_r+1);
    } else if (strcmp(cmd, "ic") == 0) {
        insert_col(cur_c+1);
    } else if (strcmp(cmd, "dr") == 0) {
        delete_row(cur_r);
        if (cur_r >= nrows) cur_r = nrows-1;
    } else if (strcmp(cmd, "dc") == 0) {
        delete_col(cur_c);
        if (cur_c >= ncols) cur_c = ncols-1;
    } else if (strncmp(cmd, "open ", 5) == 0) {
        load_csv(cmd + 5);
    } else {
        mvprintw(win_rows-3, 0, "Comando desconocido: %s", cmd);
        getch();
    }
    noecho();
    curs_set(0);
}

int main(int argc, char **argv) {
    // inicializar sheet vacía
    for (int i=0;i<MAX_ROWS;i++) for (int j=0;j<MAX_COLS;j++) {
        sheet[i][j].raw[0] = '\0';
        sheet[i][j].value = 0.0;
        sheet[i][j].evaluated = 0;
        sheet[i][j].visiting = 0;
    }
    if (argc > 1) load_csv(argv[1]);

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    getmaxyx(stdscr, win_rows, win_cols);

    draw();
    int ch;
    while (1) {
        draw();
        ch = getch();
        if (ch == 'h') {
            if (cur_c > 0) cur_c--;
            if (cur_c < grid_col_offset) grid_col_offset = cur_c;
        } else if (ch == 'l') {
            if (cur_c < ncols-1) cur_c++;
            if (cur_c >= grid_col_offset + (win_cols-6)/10) grid_col_offset++;
        } else if (ch == 'j') {
            if (cur_r < nrows-1) cur_r++;
            if (cur_r >= grid_row_offset + (win_rows-4)) grid_row_offset++;
        } else if (ch == 'k') {
            if (cur_r > 0) cur_r--;
            if (cur_r < grid_row_offset) grid_row_offset = cur_r;
        } else if (ch == 'i') {
            edit_cell();
        } else if (ch == ':') {
            colon_command();
        } else if (ch == 'I') {
            insert_row(cur_r);
        } else if (ch == 'D') {
            delete_row(cur_r);
        } else if (ch == 'C') {
            insert_col(cur_c);
        } else if (ch == 'X') {
            delete_col(cur_c);
        } else if (ch == 'w') {
            // quick save to same file if provided
            if (argc > 1) save_csv(argv[1]);
            else {
                mvprintw(win_rows-3,0,"No filename provided. Use :w and specify.");
                getch();
            }
        } else if (ch == 'q') {
            break;
        }
    }

    endwin();
    return 0;
}
