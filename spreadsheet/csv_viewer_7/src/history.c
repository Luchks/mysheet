#include "history.h"
#include <string.h>

static Change undo_stack[MAX_HISTORY];
static Change redo_stack[MAX_HISTORY];
static int undo_top = -1;
static int redo_top = -1;

void push_undo(Change c) {
    if (undo_top < MAX_HISTORY - 1) {
        undo_stack[++undo_top] = c;
    }
}

Change pop_undo() {
    return undo_stack[undo_top--];
}

void push_redo(Change c) {
    if (redo_top < MAX_HISTORY - 1) {
        redo_stack[++redo_top] = c;
    }
}

Change pop_redo() {
    return redo_stack[redo_top--];
}

void clear_redo() {
    redo_top = -1;
}

void undo(Sheet *sheet) {
    if (undo_top >= 0) {
        Change c = pop_undo();
        Change redo_c = c;
        strncpy(redo_c.new_value, sheet->cells[c.row][c.col].data, CELL_LEN);
        push_redo(redo_c);

        strncpy(sheet->cells[c.row][c.col].data, c.old_value, CELL_LEN);
        sheet->cells[c.row][c.col].data[CELL_LEN-1] = '\0';
    }
}

void redo(Sheet *sheet) {
    if (redo_top >= 0) {
        Change c = pop_redo();
        Change undo_c = c;
        strncpy(undo_c.old_value, sheet->cells[c.row][c.col].data, CELL_LEN);
        push_undo(undo_c);

        strncpy(sheet->cells[c.row][c.col].data, c.new_value, CELL_LEN);
        sheet->cells[c.row][c.col].data[CELL_LEN-1] = '\0';
    }
}
