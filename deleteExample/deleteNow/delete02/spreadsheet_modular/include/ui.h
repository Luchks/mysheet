#ifndef UI_H
#define UI_H
#include "cell.h"

void draw_sheet_filtered(Cell sheet[MAX_ROWS][MAX_COLS], int nrows, int ncols,
                         int cur_row, int cur_col, int *row_offset, int *col_offset,
                         int edit_mode, int formula_mode, char formula_buffer[FORMULA_MAX],
                         int filter_active, int filter_col, char filter_value[CELL_LEN],
                         char edit_buffer[CELL_LEN], int last_ch);

void process_input(int ch, Cell sheet[MAX_ROWS][MAX_COLS],
                   int *cur_row, int *cur_col, int *nrows, int *ncols,
                   int *formula_mode, char formula_buffer[FORMULA_MAX],
                   int *formula_row, int *formula_col, int *dynamic_pos,
                   int *edit_mode, char edit_buffer[CELL_LEN],
                   int *row_offset, int *col_offset,
                   int *last_ch, int *filter_active, int *filter_col,
                   char filter_value[CELL_LEN]);
#endif
