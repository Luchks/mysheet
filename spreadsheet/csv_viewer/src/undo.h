#ifndef UNDO_H
#define UNDO_H

#include "csv_reader.h"

#define STACK_SIZE 1000

typedef struct {
    int row;
    int col;
    char old_val[CELL_LEN];
    char new_val[CELL_LEN];
} Action;

void push_undo(int row, int col, const char *old_val, const char *new_val);
int perform_undo(Sheet *sheet);
int perform_redo(Sheet *sheet);

#endif
