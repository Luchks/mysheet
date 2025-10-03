#include "ui.h"
#include <ncurses.h>
#include <string.h>

#define COL_WIDTH 15  // ancho fijo de columna

// Función para convertir índice de columna a letra A,B,C...
void col_label(int col, char *label) {
    int c = col;
    int i = 0;
    char tmp[10];
    do {
        tmp[i++] = 'A' + (c % 26);
        c = c / 26 - 1;
    } while (c >= 0);
    // invertir
    for (int j = 0; j < i; j++) label[j] = tmp[i - j -1];
    label[i] = '\0';
}

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

        int max_visible_cols = (max_x - COL_WIDTH) / COL_WIDTH; // restamos columna de fila

        // --- Imprimir encabezados de columnas ---
        mvprintw(0, 0, "%-*s", COL_WIDTH, ""); // esquina superior izquierda vacía
        for (int j = start_col; j < sheet->ncols && j < start_col + max_visible_cols; j++) {
            char label[10];
            col_label(j, label);
            mvprintw(0, COL_WIDTH + (j - start_col) * COL_WIDTH, "%-*s", COL_WIDTH, label);
        }

        // --- Imprimir filas ---
        for (int i = start_row; i < sheet->nrows && i < start_row + max_y - 1; i++) {
            // número de fila
            mvprintw(i - start_row + 1, 0, "%-*d", COL_WIDTH, i+1);

            for (int j = start_col; j < sheet->ncols && j < start_col + max_visible_cols; j++) {
                char buffer[COL_WIDTH+1];
                strncpy(buffer, sheet->cells[i][j].data, COL_WIDTH);
                buffer[COL_WIDTH] = '\0';
                mvprintw(i - start_row + 1, COL_WIDTH + (j - start_col) * COL_WIDTH, "%-*s", COL_WIDTH, buffer);
            }
        }

        mvprintw(max_y-1, 0, "Use UP/DOWN/LEFT/RIGHT para scroll, q para salir.");

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
