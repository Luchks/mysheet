#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include "cell.h"
#include "csv.h"
#include "formula.h"
#include "ui.h"

int main() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(1);

    // Hoja de cálculo
    Cell sheet[MAX_ROWS][MAX_COLS];
    memset(sheet, 0, sizeof(sheet));
    int cur_row = 0, cur_col = 0;
    int nrows = 10, ncols = 5;

    // Modos y buffers
    int formula_mode = 0;
    char formula_buffer[FORMULA_MAX];
    int formula_row = -1, formula_col = -1;
    int dynamic_pos = -1;

    int edit_mode = 0;
    char edit_buffer[CELL_LEN];
    int last_ch = 0;

    // Scroll offsets
    int row_offset = 0, col_offset = 0;

    // Filtros
    int filter_active = 0;
    int filter_col = -1;
    char filter_value[CELL_LEN];

    int ch;
    while(1) {
        draw_sheet_filtered(sheet, nrows, ncols, cur_row, cur_col,
                            &row_offset, &col_offset,
                            edit_mode, formula_mode, formula_buffer,
                            filter_active, filter_col, filter_value,
                            edit_buffer, last_ch);

        ch = getch();
        process_input(ch, sheet,
                      &cur_row, &cur_col,
                      &nrows, &ncols,
                      &formula_mode, formula_buffer,
                      &formula_row, &formula_col,
                      &dynamic_pos, &edit_mode,
                      edit_buffer, &row_offset, &col_offset,
                      &last_ch, &filter_active, &filter_col,
                      filter_value);
    }

    endwin();
    return 0;
}
