#include "ui.h"
#include <ncurses.h>
#include <string.h>

#define COL_WIDTH 15  // ancho fijo de columna

void display_sheet(Sheet *sheet) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    int ch;
    int start_row = 0, start_col = 0;
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    while (1) {
        clear();

        // calcular cuántas columnas caben horizontalmente
        int max_visible_cols = max_x / COL_WIDTH;

        for (int i = start_row; i < sheet->nrows && i < start_row + max_y - 1; i++) {
            for (int j = start_col; j < sheet->ncols && j < start_col + max_visible_cols; j++) {
                char buffer[COL_WIDTH+1];
                strncpy(buffer, sheet->cells[i][j].data, COL_WIDTH);
                buffer[COL_WIDTH] = '\0';
                printw("%-15s", buffer);
            }
            printw("\n");
        }

        printw("\nUse UP/DOWN/LEFT/RIGHT to scroll, q to quit.\n");
        refresh();

        ch = getch();
        if (ch == 'q') break;
        else if (ch == KEY_DOWN && start_row < sheet->nrows - (max_y - 1)) start_row++;
        else if (ch == KEY_UP && start_row > 0) start_row--;
        else if (ch == KEY_RIGHT && start_col < sheet->ncols - max_visible_cols) start_col++;
        else if (ch == KEY_LEFT && start_col > 0) start_col--;
    }

    endwin();
}
