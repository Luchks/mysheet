#ifndef CELL_H
#define CELL_H
#include "utils.h"

void insert_row(Cell sheet[MAX_ROWS][MAX_COLS], int pos, int *nrows);
void remove_row(Cell sheet[MAX_ROWS][MAX_COLS], int pos, int *nrows);
void insert_col(Cell sheet[MAX_ROWS][MAX_COLS], int pos, int *ncols, int nrows);
void remove_col(Cell sheet[MAX_ROWS][MAX_COLS], int pos, int *ncols, int nrows);

#endif
