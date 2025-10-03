#ifndef UTILS_H
#define UTILS_H

#include "cell.h"

void sanitize(char *s);
void cell_name(int row, int col, char *buf);
int parse_cell(const char *ref, int *row, int *col);

#endif
