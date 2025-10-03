#include <stdio.h>
#include <string.h>
#include "csv.h"

void sanitize(char *s);

void load_csv(const char *filename, Cell sheet[MAX_ROWS][MAX_COLS], int *nrows, int *ncols) {
    FILE *f=fopen(filename,"r"); if(!f) return;
    char line[4096]; int row=0; *ncols=0;
    while(fgets(line,sizeof(line),f) && row<MAX_ROWS){
        int col=0; char *token=strtok(line,",\n");
        while(token && col<MAX_COLS){
            strncpy(sheet[row][col].data,token,CELL_LEN-1);
            sheet[row][col].data[CELL_LEN-1]='\0';
            sanitize(sheet[row][col].data);
            col++; token=strtok(NULL,",\n");
        }
        if(col>*ncols) *ncols=col;
        row++;
    }
    *nrows=row; fclose(f);
}

void save_csv(const char *filename, Cell sheet[MAX_ROWS][MAX_COLS], int nrows, int ncols) {
    FILE *f=fopen(filename,"w"); if(!f) return;
    for(int i=0;i<nrows;i++){
        for(int j=0;j<ncols;j++){
            if(sheet[i][j].data[0]=='=') fprintf(f,"%.2f",atof(sheet[i][j].data)); 
            else fprintf(f,"%s",sheet[i][j].data);
            if(j<ncols-1) fprintf(f,",");
        }
        fprintf(f,"\n");
    }
    fclose(f);
}
