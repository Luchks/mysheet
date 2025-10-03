#ifndef CSV_READER_H
#define CSV_READER_H

#define MAX_ROWS 1000
#define MAX_COLS 100
#define CELL_LEN 128

typedef struct {
    char data[CELL_LEN];
} Cell;

typedef struct {
    Cell cells[MAX_ROWS][MAX_COLS];
    int nrows;
    int ncols;
} Sheet;

int load_csv(const char *filename, Sheet *sheet);

#endif
