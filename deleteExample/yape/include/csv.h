#ifndef CSV_H
#define CSV_H

#include "cell.h"

void load_csv(const char *filename, Cell sheet[MAX_ROWS][MAX_COLS], int *nrows, int *ncols);
void save_csv(const char *filename, Cell sheet[MAX_ROWS][MAX_COLS], int nrows, int ncols);

#endif
