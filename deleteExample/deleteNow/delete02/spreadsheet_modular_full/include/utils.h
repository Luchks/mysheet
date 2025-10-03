#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>

#define MAX_ROWS 1000
#define MAX_COLS 1000
#define CELL_LEN 64
#define FORMULA_MAX 256

typedef struct {
    char data[CELL_LEN];
} Cell;

void sanitize(char *s);
void cell_name(int row, int col, char *buf);
int parse_cell(const char *ref, int *row, int *col);

#endif
