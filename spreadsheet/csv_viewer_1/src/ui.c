#include "ui.h"
#include <ncurses.h>

void display_sheet(Sheet *sheet) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    int ch;
    int start_row = 0;
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);  // ahora correcto
    (void)max_x; // evita warning

    while (1) {
        clear();
        for (int i = start_row; i < sheet->nrows && i < start_row + max_y - 1; i++) {
            for (int j = 0; j < sheet->ncols; j++) {
                if (sheet->cells[i][j].data[0] != '\0')
                    printw("%-15s ", sheet->cells[i][j].data);
                else
                    printw("%-15s ", "");
            }
            printw("\n");
        }
        printw("\nUse UP/DOWN to scroll, q to quit.\n");
        refresh();

        ch = getch();
        if (ch == 'q') break;
        else if (ch == KEY_DOWN && start_row < sheet->nrows - max_y) start_row++;
        else if (ch == KEY_UP && start_row > 0) start_row--;
    }

    endwin();
}
