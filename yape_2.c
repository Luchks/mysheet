#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char ***cells;   // matriz de strings [nrows][ncols]
    int nrows;
    int ncols;
} Sheet;

// Crear hoja vacía
Sheet* sheet_create(int rows, int cols) {
    Sheet *s = malloc(sizeof(Sheet));
    s->nrows = rows;
    s->ncols = cols;

    s->cells = malloc(rows * sizeof(char**));
    for (int i = 0; i < rows; i++) {
        s->cells[i] = calloc(cols, sizeof(char*));
    }
    return s;
}

// Liberar memoria
void sheet_free(Sheet *s) {
    for (int i = 0; i < s->nrows; i++) {
        for (int j = 0; j < s->ncols; j++) {
            free(s->cells[i][j]);
        }
        free(s->cells[i]);
    }
    free(s->cells);
    free(s);
}

// Asignar valor a una celda
void sheet_set(Sheet *s, int row, int col, const char *value) {
    if (row >= s->nrows || col >= s->ncols) return;
    free(s->cells[row][col]);
    s->cells[row][col] = strdup(value);
}

// Obtener valor
const char* sheet_get(Sheet *s, int row, int col) {
    if (row >= s->nrows || col >= s->ncols) return "";
    return s->cells[row][col] ? s->cells[row][col] : "";
}

// Añadir fila al final
void sheet_add_row(Sheet *s) {
    s->cells = realloc(s->cells, (s->nrows + 1) * sizeof(char**));
    s->cells[s->nrows] = calloc(s->ncols, sizeof(char*));
    s->nrows++;
}

// Eliminar última fila
void sheet_remove_row(Sheet *s) {
    if (s->nrows == 0) return;
    int last = s->nrows - 1;
    for (int j = 0; j < s->ncols; j++) {
        free(s->cells[last][j]);
    }
    free(s->cells[last]);
    s->nrows--;
    s->cells = realloc(s->cells, s->nrows * sizeof(char**));
}

// Añadir columna al final
void sheet_add_col(Sheet *s) {
    for (int i = 0; i < s->nrows; i++) {
        s->cells[i] = realloc(s->cells[i], (s->ncols + 1) * sizeof(char*));
        s->cells[i][s->ncols] = NULL;
    }
    s->ncols++;
}

// Eliminar última columna
void sheet_remove_col(Sheet *s) {
    if (s->ncols == 0) return;
    int last = s->ncols - 1;
    for (int i = 0; i < s->nrows; i++) {
        free(s->cells[i][last]);
        s->cells[i] = realloc(s->cells[i], (s->ncols - 1) * sizeof(char*));
    }
    s->ncols--;
}

// Mostrar hoja en consola
void sheet_print(Sheet *s) {
    for (int i = 0; i < s->nrows; i++) {
        for (int j = 0; j < s->ncols; j++) {
            printf("%s\t", sheet_get(s, i, j));
        }
        printf("\n");
    }
}

// Cargar CSV en Sheet
Sheet* sheet_load_csv(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        perror("No se pudo abrir el archivo");
        return NULL;
    }

    char line[1024];
    int rows = 0, cols = 0;

    // contar filas y columnas
    while (fgets(line, sizeof(line), f)) {
        rows++;
        int tmp_cols = 1;
        for (char *p = line; *p; p++) {
            if (*p == ',') tmp_cols++;
        }
        if (tmp_cols > cols) cols = tmp_cols;
    }
    rewind(f);

    // crear hoja
    Sheet *s = sheet_create(rows, cols);

    // cargar contenido
    int r = 0;
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = 0; // quitar salto de línea
        char *token = strtok(line, ",");
        int c = 0;
        while (token && c < cols) {
            sheet_set(s, r, c, token);
            token = strtok(NULL, ",");
            c++;
        }
        r++;
    }
    fclose(f);
    return s;
}

// Guardar CSV desde Sheet
void sheet_save_csv(Sheet *s, const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f) {
        perror("No se pudo guardar el archivo");
        return;
    }
    for (int i = 0; i < s->nrows; i++) {
        for (int j = 0; j < s->ncols; j++) {
            fprintf(f, "%s", sheet_get(s, i, j));
            if (j < s->ncols - 1) fprintf(f, ",");
        }
        fprintf(f, "\n");
    }
    fclose(f);
}

// Demo principal
int main() {
    Sheet *s = sheet_load_csv("datos.csv");
    if (!s) return 1;

    printf("CSV cargado:\n");
    sheet_print(s);

    printf("\nAñadir fila y columna:\n");
    sheet_add_row(s);
    sheet_add_col(s);
    sheet_set(s, s->nrows - 1, s->ncols - 1, "NUEVO");
    sheet_print(s);

    printf("\nGuardar en nuevo archivo:\n");
    sheet_save_csv(s, "salida.csv");

    sheet_free(s);
    return 0;
}
