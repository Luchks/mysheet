#include "undo.h"
#include <string.h>

static Action undo_stack[STACK_SIZE];
static int undo_top = -1;

static Action redo_stack[STACK_SIZE];
static int redo_top = -1;

void push_undo(int row, int col, const char *old_val, const char *new_val) {
    if (undo_top < STACK_SIZE - 1) {
        undo_top++;
        undo_stack[undo_top].row = row;
        undo_stack[undo_top].col = col;
        strncpy(undo_stack[undo_top].old_val, old_val, CELL_LEN);
        strncpy(undo_stack[undo_top].new_val, new_val, CELL_LEN);
        // limpiar pila de redo cuando se agrega una acción nueva
        redo_top = -1;
    }
}

int perform_undo(Sheet *sheet) {
    if (undo_top < 0) return 0;

    Action act = undo_stack[undo_top--];

    // guardar en redo
    if (redo_top < STACK_SIZE - 1) {
        redo_top++;
        redo_stack[redo_top] = act;
    }

    // restaurar old_val en la celda
    strncpy(sheet->cells[act.row][act.col].data, act.old_val, CELL_LEN);
    sheet->cells[act.row][act.col].data[CELL_LEN-1] = '\0';
    return 1;
}

int perform_redo(Sheet *sheet) {
    if (redo_top < 0) return 0;

    Action act = redo_stack[redo_top--];

    // volver a poner en undo
    if (undo_top < STACK_SIZE - 1) {
        undo_top++;
        undo_stack[undo_top] = act;
    }

    // aplicar new_val en la celda
    strncpy(sheet->cells[act.row][act.col].data, act.new_val, CELL_LEN);
    sheet->cells[act.row][act.col].data[CELL_LEN-1] = '\0';
    return 1;
}
