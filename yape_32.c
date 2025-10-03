#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_ROWS 1000
#define MAX_COLS 1000
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

int edit_mode = 0;
char edit_buffer[CELL_LEN];

// Scroll offsets
int row_offset = 0, col_offset = 0;

// Para navegación tipo Vim
int last_ch = 0;

// --- LIMPIAR \r ---
void sanitize(char *s) {
    char *p = s;
    while (*p) {
        if (*p == '\r') *p = '\0';
        p++;
    }
}

// Excel-style nombre de celda
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

// Parsear referencia
int parse_cell(const char *ref, int *row, int *col) {
    int c = 0, r = 0, i = 0;
    while (isalpha(ref[i])) {
        c = c * 26 + (toupper(ref[i]) - 'A' + 1);
        i++;
    }
    c--; 
    while (isdigit(ref[i])) {
        r = r * 10 + (ref[i] - '0');
        i++;
    }
    r--; 
    if (r < 0 || r >= MAX_ROWS || c < 0 || c >= MAX_COLS) return 0;
    *row = r; *col = c;
    return 1;
}

// Evaluar fórmula simple
double eval_formula(const char *formula);
double eval_expr(const char **s) {
    double res = 0;
    double num = 0;
    char op = '+';

    while (**s) {
        if (isspace(**s)) { (*s)++; continue; }

        if (**s == '(') {
            (*s)++;
            num = eval_expr(s);
        } else if (isalpha(**s)) {
            char ref[16]; int j = 0;
            while (isalpha(**s) || isdigit(**s)) ref[j++] = *(*s)++;
            ref[j] = '\0';
            int r, c;
            if (parse_cell(ref, &r, &c)) {
                if (sheet[r][c].data[0] == '=') num = eval_formula(sheet[r][c].data);
                else num = atof(sheet[r][c].data);
            } else num = 0;
        } else if (isdigit(**s) || **s == '.') {
            num = strtod(*s, (char **)s);
        } else if (**s == ')') {
            (*s)++;
            break;
        } else {
            op = **s;
            (*s)++;
            continue;
        }

        switch (op) {
            case '+': res += num; break;
            case '-': res -= num; break;
            case '*': res *= num; break;
            case '/': res /= num; break;
        }
        op = 0;
    }
    return res;
}

double eval_formula(const char *formula) {
    if (!formula || formula[0] != '=') return 0;
    const char *s = formula + 1;
    return eval_expr(&s);
}

// Actualiza referencia dinámica
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

// Dibujar hoja con numeración estilo Excel
void draw_sheet() {
    clear();
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    int visible_rows = max_y - 5;
    int visible_cols = max_x / 12;

    if (cur_row < row_offset) row_offset = cur_row;
    else if (cur_row >= row_offset + visible_rows) row_offset = cur_row - visible_rows + 1;

    if (cur_col < col_offset) col_offset = cur_col;
    else if (cur_col >= col_offset + visible_cols) col_offset = cur_col - visible_cols + 1;

    // Encabezados de columnas (A, B, C…)
    for (int j = 0; j < visible_cols && j + col_offset < ncols; j++) {
        char colname[10];
        cell_name(0, j + col_offset, colname);
        mvprintw(0, (j+1) * 12, "%-11s", colname);
    }

    // Dibujar filas con numeración
    for (int i = 0; i < visible_rows && i + row_offset < nrows; i++) {
        int r = i + row_offset;
        mvprintw(i+1, 0, "%-3d", r+1); // Numeración de fila
        for (int j = 0; j < visible_cols && j + col_offset < ncols; j++) {
            int c = j + col_offset;
            if (edit_mode && r == cur_row && c == cur_col)
                mvprintw(i+1, (j+1) * 12, "%-11s", edit_buffer);
            else if (sheet[r][c].data[0] == '=')
                mvprintw(i+1, (j+1) * 12, "%-11.2f", eval_formula(sheet[r][c].data));
            else
                mvprintw(i+1, (j+1) * 12, "%-11s", sheet[r][c].data[0] ? sheet[r][c].data : ".");
        }
    }

    mvprintw(visible_rows + 2, 0, "Modo: %s", formula_mode ? "FORMULA" : edit_mode ? "EDIT" : "NORMAL");
    if (formula_mode) mvprintw(visible_rows + 3, 0, "Formula: %s", formula_buffer);
    move(cur_row - row_offset + 1, (cur_col - col_offset + 1) * 12);
    refresh();
}

// Insertar/eliminar fila/col
void insert_row(int pos) {
    if (nrows >= MAX_ROWS) return;
    for (int i = nrows; i > pos; i--)
        memcpy(sheet[i], sheet[i-1], sizeof(Cell)*MAX_COLS);
    memset(sheet[pos], 0, sizeof(Cell)*MAX_COLS);
    nrows++;
}
void remove_row(int pos) {
    if (nrows <= 1) return;
    for (int i = pos; i < nrows-1; i++)
        memcpy(sheet[i], sheet[i+1], sizeof(Cell)*MAX_COLS);
    memset(sheet[nrows-1], 0, sizeof(Cell)*MAX_COLS);
    nrows--;
}
void insert_col(int pos) {
    if (ncols >= MAX_COLS) return;
    for (int i = 0; i < nrows; i++)
        for (int j = ncols; j > pos; j--)
            sheet[i][j] = sheet[i][j-1];
    for (int i = 0; i < nrows; i++)
        memset(&sheet[i][pos], 0, sizeof(Cell));
    ncols++;
}
void remove_col(int pos) {
    if (ncols <= 1) return;
    for (int i = 0; i < nrows; i++)
        for (int j = pos; j < ncols-1; j++)
            sheet[i][j] = sheet[i][j+1];
    for (int i = 0; i < nrows; i++)
        memset(&sheet[i][ncols-1], 0, sizeof(Cell));
}

// Rellenar columna fórmulas
void fill_formula_column(int col) {
    if (col < 0 || col >= ncols) return;
    int base_row = cur_row;
    char *base = sheet[base_row][col].data;
    if (!base || base[0] != '=') return;
    for (int i = 0; i < nrows; i++) {
        if (i == base_row) continue;
        char tmp[FORMULA_MAX];
        int pos = 0;
        tmp[pos++] = '=';
        tmp[pos] = '\0';
        for (int k = 1; base[k] != '\0' && pos < FORMULA_MAX - 1; ) {
            if (base[k] >= 'A' && base[k] <= 'Z') {
                char col_letter = base[k++];
                tmp[pos++] = col_letter;
                tmp[pos] = '\0';
                int row_num = 0;
                while (base[k] >= '0' && base[k] <= '9')
                    row_num = row_num * 10 + (base[k++] - '0');
                int new_row = row_num + (i - base_row);
                char row_str[16];
                int len = snprintf(row_str, sizeof(row_str), "%d", new_row);
                if (pos + len < FORMULA_MAX - 1) {
                    strcpy(tmp + pos, row_str);
                    pos += len;
                }
            } else {
                tmp[pos++] = base[k++];
                tmp[pos] = '\0';
            }
        }
        strncpy(sheet[i][col].data, tmp, CELL_LEN - 1);
        sheet[i][col].data[CELL_LEN - 1] = '\0';
    }
}

// CSV load/save
void load_csv(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) return;
    char line[4096];
    int row = 0;
    nrows = 0; ncols = 0;
    while (fgets(line, sizeof(line), f) && row < MAX_ROWS) {
        int col = 0;
        char *token = strtok(line, ",\n");
        while (token && col < MAX_COLS) {
            strncpy(sheet[row][col].data, token, CELL_LEN-1);
            sheet[row][col].data[CELL_LEN-1] = '\0';
            sanitize(sheet[row][col].data);
            col++;
            token = strtok(NULL, ",\n");
        }
        if (col > ncols) ncols = col;
        row++;
    }
    nrows = row;
    fclose(f);
}

void save_csv(const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f) return;
    for (int i = 0; i < nrows; i++) {
        for (int j = 0; j < ncols; j++) {
            if (sheet[i][j].data[0] == '=')
                fprintf(f, "%.2f", eval_formula(sheet[i][j].data));
            else
                fprintf(f, "%s", sheet[i][j].data);
            if (j < ncols - 1) fprintf(f, ",");
        }
        fprintf(f, "\n");
    }
    fclose(f);
}

// DUPLICAR con sanitize
void duplicate_row(int pos) {
    if (nrows >= MAX_ROWS) return;
    insert_row(pos + 1);
    memcpy(sheet[pos + 1], sheet[pos], sizeof(Cell)*MAX_COLS);
    for (int j = 0; j < ncols; j++) sanitize(sheet[pos+1][j].data);
}
void duplicate_col(int pos) {
    if (ncols >= MAX_COLS) return;
    insert_col(pos + 1);
    for (int i = 0; i < nrows; i++) {
        sheet[i][pos + 1] = sheet[i][pos];
        sanitize(sheet[i][pos+1].data);
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

        // --- Navegación tipo Vim ---
        if (!formula_mode && !edit_mode) {
            if (last_ch == 'g' && ch == 'g') { cur_row = 0; last_ch = 0; continue; } // gg
            if (ch == 'G') { cur_row = nrows-1; continue; } // G
            last_ch = (ch == 'g') ? 'g' : 0;

            switch(ch) {
                case 'q': endwin(); return 0;
                case '=': formula_mode = 1; formula_row = cur_row; formula_col = cur_col;
                          strcpy(formula_buffer, "="); dynamic_pos = 1;
                          strcpy(sheet[cur_row][cur_col].data, formula_buffer); break;
                case 'e': edit_mode = 1; strcpy(edit_buffer, sheet[cur_row][cur_col].data); break;
                case 'c': { echo(); char filename[256];
                            mvprintw(nrows + 5, 0, "Archivo CSV a cargar: ");
                            getnstr(filename, 255); noecho(); load_csv(filename);
                            cur_row = cur_col = 0; break; }
                case 's': { echo(); char filename[256];
                            mvprintw(nrows + 5, 0, "Archivo CSV a guardar: ");
                            getnstr(filename, 255); noecho(); save_csv(filename); break; }
                case 'h': if(cur_col>0) cur_col--; break;
                case 'l': if(cur_col<ncols-1) cur_col++; break;
                case 'k': if(cur_row>0) cur_row--; break;
                case 'j': if(cur_row<nrows-1) cur_row++; break;
                case 'i': insert_row(cur_row); break;
                case 'd': remove_row(cur_row); break;
                case 'I': insert_col(cur_col); break;
                case 'D': remove_col(cur_col); break;
                case 'f': fill_formula_column(cur_col); break;
                case 'R': duplicate_row(cur_row); break;
                case 'C': duplicate_col(cur_col); break;
                case '0': cur_col = 0; break;           // inicio de fila
                case '$': cur_col = ncols-1; break;     // fin de fila
                case 'H': cur_col = col_offset; break;  // inicio visible
                case 'L': cur_col = col_offset + (COLS/12) -1; break; // fin visible
            }
        } else if (edit_mode) {
            if (ch == 27) edit_mode = 0;
            else if (ch == '\n') {
                strncpy(sheet[cur_row][cur_col].data, edit_buffer, CELL_LEN-1);
                sheet[cur_row][cur_col].data[CELL_LEN-1] = '\0';
                sanitize(sheet[cur_row][cur_col].data);
                edit_mode = 0;
                if (cur_row < nrows-1) cur_row++;
            } else if (ch == KEY_BACKSPACE || ch == 127) {
                int len = strlen(edit_buffer);
                if (len > 0) edit_buffer[len-1] = '\0';
            } else if (ch >= 32 && ch <= 126) {
                int len = strlen(edit_buffer);
                if (len < CELL_LEN-1) {
                    edit_buffer[len] = (char)ch;
                    edit_buffer[len+1] = '\0';
                }
            }
        } else {
            if (ch == 27) formula_mode = 0, formula_row = formula_col = -1, dynamic_pos = -1;
            else if (ch == '\n') formula_mode = 0, formula_row = formula_col = -1, dynamic_pos = -1;
            else if (ch == 'h' && cur_col>0) { cur_col--; update_dynamic_ref(); }
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
