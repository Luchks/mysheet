#include "csv_reader.h"
#include "ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: %s archivo.csv\n", argv[0]);
        return 1;
    }

    // Asignar Sheet en heap para evitar stack overflow
    Sheet *sheet = malloc(sizeof(Sheet));
    if (!sheet) {
        perror("No se pudo asignar memoria para Sheet");
        return 1;
    }
    memset(sheet, 0, sizeof(Sheet));

    if (load_csv(argv[1], sheet) != 0) {
        free(sheet);
        return 1;
    }

    display_sheet(sheet);

    free(sheet);
    return 0;
}
