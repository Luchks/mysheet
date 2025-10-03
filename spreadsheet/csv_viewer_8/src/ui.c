#include "ui.h"
#include "undo.h"
#include <ncurses.h>
#include <string.h>
#include <stdio.h>

#define COL_WIDTH 15

void col_label(int col, char *label) {
    int c = col;
    int i = 0;
    char tmp[10];
    do {
        tmp[i++] = 'A' + (c % 26);
        c = c / 26 - 1;
    } while (c >= 0);
    for (int j = 0; j < i; j++) label[j] = tmp[i - j -1];
    label[i] = '\0';
}

void edit_cell(Sheet *sheet, int row, int col) {
    echo();
    curs_set(1);
    char input[COL_WIDTH+1];
    move(row+1, COL_WIDTH + col * COL_WIDTH);
    clrtoeol(); 
    getnstr(input, COL_WIDTH);

    // --- guardar para undo ---
    char old_val[CELL_LEN];
    strcpy(old_val, sheet->cells[row][col].data);

    // cambiar valor
    strncpy(sheet->cells[row][col].data, input, COL_WIDTH);
    sheet->cells[row][col].data[COL_WIDTH] = '\0';

    // push a la pila undo
    push_undo(row, col, old_val, sheet->cells[row][col].data);

    noecho();
    curs_set(0);
}

void save_csv(Sheet *sheet, const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        mvprintw(0,0,"Error al guardar CSV");
        return;
    }

    for (int i = 0; i < sheet->nrows; i++) {
        for (int j = 0; j < sheet->ncols; j++) {
            fprintf(fp, "%s", sheet->cells[i][j].data);
            if (j < sheet->ncols - 1) fprintf(fp, ",");
        }
        fprintf(fp, "\n");
    }
    fclose(fp);
}

void display_sheet(Sheet *sheet, const char *filename) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    int ch;
    int start_row = 0, start_col = 0;
    int active_row = 0, active_col = 0;
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    int max_visible_cols = (max_x - COL_WIDTH) / COL_WIDTH;
    int max_visible_rows = max_y - 1;

    while (1) {
        clear();

        // encabezados de columna
        mvprintw(0, 0, "%-*s", COL_WIDTH, "");
        for (int j = start_col; j < sheet->ncols && j < start_col + max_visible_cols; j++) {
            char label[10];
            col_label(j, label);
            mvprintw(0, COL_WIDTH + (j - start_col) * COL_WIDTH, "%-*s", COL_WIDTH, label);
        }

        // filas visibles
        for (int i = start_row; i < sheet->nrows && i < start_row + max_visible_rows; i++) {
            mvprintw(i - start_row + 1, 0, "%-*d", COL_WIDTH, i+1);
            for (int j = start_col; j < sheet->ncols && j < start_col + max_visible_cols; j++) {
                char buffer[COL_WIDTH+1];
                strncpy(buffer, sheet->cells[i][j].data, COL_WIDTH);
                buffer[COL_WIDTH] = '\0';
                if (i == active_row && j == active_col) {
                    attron(A_REVERSE);
                    mvprintw(i - start_row + 1, COL_WIDTH + (j - start_col) * COL_WIDTH, "%-*s", COL_WIDTH, buffer);
                    attroff(A_REVERSE);
                } else {
                    mvprintw(i - start_row + 1, COL_WIDTH + (j - start_col) * COL_WIDTH, "%-*s", COL_WIDTH, buffer);
                }
            }
        }

        mvprintw(max_y-1, 0, "jklh: mover | i: editar | s: guardar | u: undo | Ctrl+R: redo | q: salir");
        refresh();
        ch = getch();

        if (ch == 'q') break;
        else if (ch == 'j' && active_row < sheet->nrows - 1) active_row++;
        else if (ch == 'k' && active_row > 0) active_row--;
        else if (ch == 'l' && active_col < sheet->ncols - 1) active_col++;
        else if (ch == 'h' && active_col > 0) active_col--;
        else if (ch == 'i') {
            edit_cell(sheet, active_row, active_col);
        }
        else if (ch == 's') {
            save_csv(sheet, filename);
            mvprintw(max_y-2, 0, "CSV guardado correctamente!");
            refresh();
        }
        else if (ch == 'u') {
            if (perform_undo(sheet)) {
                mvprintw(max_y-2, 0, "Undo realizado!");
            } else {
                mvprintw(max_y-2, 0, "Nada para deshacer");
            }
            refresh();
        }
        else if (ch == 18) { // Ctrl+R
            if (perform_redo(sheet)) {
                mvprintw(max_y-2, 0, "Redo realizado!");
            } else {
                mvprintw(max_y-2, 0, "Nada para rehacer");
            }
            refresh();
        }

        // scroll vertical
        if (active_row < start_row) start_row = active_row;
        else if (active_row >= start_row + max_visible_rows) start_row = active_row - max_visible_rows + 1;

        // scroll horizontal
        if (active_col < start_col) start_col = active_col;
        else if (active_col >= start_col + max_visible_cols) start_col = active_col - max_visible_cols + 1;
    }

    endwin();
}
