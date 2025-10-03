#include <ncurses.h>
#include <string.h>
#include "ui.h"
#include "formula.h"
#include "cell.h"
#include "utils.h"

void draw_sheet_filtered(Cell sheet[MAX_ROWS][MAX_COLS], int nrows, int ncols,
                         int cur_row, int cur_col, int *row_offset, int *col_offset,
                         int edit_mode, int formula_mode, char formula_buffer[FORMULA_MAX],
                         int filter_active, int filter_col, char filter_value[CELL_LEN],
                         char edit_buffer[CELL_LEN], int last_ch) {
    clear();
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    int visible_rows = max_y - 5;
    int visible_cols = max_x / 12;

    if (cur_row < *row_offset) *row_offset = cur_row;
    else if (cur_row >= *row_offset + visible_rows) *row_offset = cur_row - visible_rows + 1;

    if (cur_col < *col_offset) *col_offset = cur_col;
    else if (cur_col >= *col_offset + visible_cols) *col_offset = cur_col - visible_cols + 1;

    // Encabezados
    for (int j = 0; j < visible_cols && j + *col_offset < ncols; j++) {
        char colname[10];
        cell_name(0, j + *col_offset, colname);
        mvprintw(0, (j+1)*12, "%-11s", colname);
    }

    int line = 1;
    for (int i = *row_offset; i < nrows && line <= visible_rows; i++) {
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
