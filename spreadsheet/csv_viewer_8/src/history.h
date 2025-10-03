#ifndef HISTORY_H
#define HISTORY_H

#include "csv_reader.h"

#define MAX_HISTORY 1000

typedef struct {
    int row;
    int col;
    char old_value[CELL_LEN];
    char new_value[CELL_LEN];
} Change;

void push_undo(Change c);
Change pop_undo();
void push_redo(Change c);
Change pop_redo();
void clear_redo();

void undo(Sheet *sheet);
void redo(Sheet *sheet);

#endif
