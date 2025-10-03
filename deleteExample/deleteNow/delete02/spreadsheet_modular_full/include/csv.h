#ifndef CSV_H
#define CSV_H
#include "utils.h"
#include "formula.h"

void load_csv(Cell sheet[MAX_ROWS][MAX_COLS], const char *filename, int *nrows, int *ncols);
void save_csv(Cell sheet[MAX_ROWS][MAX_COLS], const char *filename, int nrows, int ncols);

#endif
