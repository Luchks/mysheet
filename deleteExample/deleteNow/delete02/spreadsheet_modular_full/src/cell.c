#include <string.h>
#include "cell.h"
#include "utils.h"

void insert_row(Cell sheet[MAX_ROWS][MAX_COLS], int pos, int *nrows) {
    if (*nrows >= MAX_ROWS) return;
    for (int i = *nrows; i > pos; i--)
        memcpy(sheet[i], sheet[i-1], sizeof(Cell)*MAX_COLS);
    memset(sheet[pos], 0, sizeof(Cell)*MAX_COLS);
    (*nrows)++;
}

void remove_row(Cell sheet[MAX_ROWS][MAX_COLS], int pos, int *nrows) {
    if (*nrows <= 1) return;
    for (int i = pos; i < *nrows - 1; i++)
        memcpy(sheet[i], sheet[i+1], sizeof(Cell)*MAX_COLS);
    memset(sheet[*nrows - 1], 0, sizeof(Cell)*MAX_COLS);
    (*nrows)--;
}

void insert_col(Cell sheet[MAX_ROWS][MAX_COLS], int pos, int *ncols, int nrows) {
    if (*ncols >= MAX_COLS) return;
    for (int i = 0; i < nrows; i++)
        for (int j = *ncols; j > pos; j--)
            sheet[i][j] = sheet[i][j-1];
    for (int i = 0; i < nrows; i++)
        memset(&sheet[i][pos], 0, sizeof(Cell));
    (*ncols)++;
}

void remove_col(Cell sheet[MAX_ROWS][MAX_COLS], int pos, int *ncols, int nrows) {
    if (*ncols <= 1) return;
    for (int i = 0; i < nrows; i++)
        for (int j = pos; j < *ncols - 1; j++)
            sheet[i][j] = sheet[i][j+1];
    for (int i = 0; i < nrows; i++)
        memset(&sheet[i][*ncols - 1], 0, sizeof(Cell));
    (*ncols)--;
}
