#include "csv_reader.h"
#include <stdio.h>
#include <string.h>

int load_csv(const char *filename, Sheet *sheet) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Error al abrir archivo CSV");
        return -1;
    }

    char line[1024];
    int row = 0;
    sheet->nrows = 0;
    sheet->ncols = 0;

    while (fgets(line, sizeof(line), fp)) {
        int col = 0;
        char *token = strtok(line, ",\n");
        while (token && col < MAX_COLS) {
            strncpy(sheet->cells[row][col].data, token, CELL_LEN);
            sheet->cells[row][col].data[CELL_LEN-1] = '\0';
            token = strtok(NULL, ",\n");
            col++;
        }
        if (col > sheet->ncols) sheet->ncols = col;
        row++;
        if (row >= MAX_ROWS) break;
    }

    sheet->nrows = row;
    fclose(fp);
    return 0;
}
