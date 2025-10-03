#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "cell.h"
#include "csv.h"
#include "formula.h"
#include "utils.h"

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

// Dibujar hoja con scroll y filtros
void draw_sheet_filtered(Cell sheet[MAX_ROWS][MAX_COLS],
                         int nrows, int ncols,
                         int cur_row, int cur_col,
                         int *row_offset, int *col_offset,
                         int edit_mode, int formula_mode,
                         char *formula_buffer,
                         int filter_active, int filter_col,
                         char *filter_value,
                         char *edit_buffer, int last_ch) {
    clear();
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    int visible_rows = max_y - 5;
    int visible_cols = max_x / 12;

    if (cur_row < *row_offset) *row_offset = cur_row;
    else if (cur_row >= *row_offset + visible_rows) *row_offset = cur_row - visible_rows + 1;

    if (cur_col < *col_offset) *col_offset = cur_col;
    else if (cur_col >= *col_offset + visible_cols) *col_offset = cur_col - visible_cols + 1;

    // Encabezados columnas
    for (int j = 0; j < visible_cols && j + *col_offset < ncols; j++) {
        char colname[10];
        cell_name(0, j + *col_offset, colname);
        mvprintw(0, (j+1) * 12, "%-11s", colname);
    }

    int line = 1;
    for (int i = *row_offset; i < nrows && line <= visible_rows; i++) {
        // Filtrado
        if (filter_active && filter_col >= 0 && strstr(sheet[i][filter_col].data, filter_value) == NULL)
            continue;

        mvprintw(line, 0, "%-3d", i+1);
        for (int j = 0; j < visible_cols && j + *col_offset < ncols; j++) {
            int c = j + *col_offset;
            if (edit_mode && i == cur_row && c == cur_col)
                mvprintw(line, (j+1)*12, "%-11s", edit_buffer);
            else if (sheet[i][c].data[0] == '=')
                mvprintw(line, (j+1)*12, "%-11.2f", eval_formula(sheet[i][c].data, sheet));
            else
                mvprintw(line, (j+1)*12, "%-11s", sheet[i][c].data[0] ? sheet[i][c].data : ".");
        }
        line++;
    }

    mvprintw(visible_rows + 2, 0, "Modo: %s", formula_mode ? "FORMULA" : edit_mode ? "EDIT" : "NORMAL");
    if (formula_mode) mvprintw(visible_rows + 3, 0, "Formula: %s", formula_buffer);
    if (filter_active) mvprintw(visible_rows + 4, 0, "Filtro activo: Columna %d contiene '%s'", filter_col+1, filter_value);

    move(cur_row - *row_offset + 1, (cur_col - *col_offset + 1) * 12);
    refresh();
}

// Procesar input modularizado
void process_input(int ch, Cell sheet[MAX_ROWS][MAX_COLS],
                   int *cur_row, int *cur_col,
                   int *nrows, int *ncols,
                   int *formula_mode, char *formula_buffer,
                   int *formula_row, int *formula_col,
                   int *dynamic_pos, int *edit_mode,
                   char *edit_buffer,
                   int *row_offset, int *col_offset,
                   int *last_ch, int *filter_active, int *filter_col,
                   char *filter_value) {

    if (!(*formula_mode) && !(*edit_mode)) {
        // Navegación tipo Vim gg / G
        if (*last_ch == 'g' && ch == 'g') { *cur_row = 0; *last_ch = 0; return; }
        if (ch == 'G') { *cur_row = *nrows - 1; return; }
        *last_ch = (ch == 'g') ? 'g' : 0;

        switch(ch) {
            case 'q': endwin(); exit(0);
            case '=': *formula_mode=1; *formula_row=*cur_row; *formula_col=*cur_col;
                      strcpy(formula_buffer,"="); *dynamic_pos=1;
                      strcpy(sheet[*cur_row][*cur_col].data, formula_buffer); break;
            case 'e': *edit_mode=1; strcpy(edit_buffer, sheet[*cur_row][*cur_col].data); break;
            case 'c': { echo(); char filename[256];
                        mvprintw(*nrows +5,0,"Archivo CSV a cargar: ");
                        getnstr(filename,255); noecho();
                        load_csv(filename, sheet, nrows, ncols);
                        *cur_row = *cur_col = 0; break; }
            case 's': { echo(); char filename[256];
                        mvprintw(*nrows +5,0,"Archivo CSV a guardar: ");
                        getnstr(filename,255); noecho();
                        save_csv(filename, sheet, *nrows, *ncols); break; }
            case 'h': if(*cur_col>0) (*cur_col)--; break;
            case 'l': if(*cur_col<*ncols-1) (*cur_col)++; break;
            case 'k': if(*cur_row>0) (*cur_row)--; break;
            case 'j': if(*cur_row<*nrows-1) (*cur_row)++; break;
            case 'i': insert_row(*cur_row, sheet, nrows, *ncols); break;
            case 'd': remove_row(*cur_row, sheet, nrows, *ncols); break;
            case 'I': insert_col(*cur_col, sheet, *nrows, ncols); break;
            case 'D': remove_col(*cur_col, sheet, *nrows, ncols); break;
            case 'f': fill_formula_column(*cur_col, *cur_row, sheet, *nrows); break;
            case 'R': duplicate_row(*cur_row, sheet, *ncols, *nrows); break;
            case 'C': duplicate_col(*cur_col, sheet, *nrows, *ncols); break;
            case '0': *cur_col=0; break;
            case '$': *cur_col=*ncols-1; break;
            case 'H': *cur_col=*col_offset; break;
            case 'L': *cur_col=*col_offset + (COLS/12) -1; break;
            case 'F': activate_filter(filter_active, filter_col, filter_value); break;
            case 'U': deactivate_filter(filter_active, filter_col); break;
        }
    }
    // edit_mode y formula_mode se pueden implementar igual que antes
}
