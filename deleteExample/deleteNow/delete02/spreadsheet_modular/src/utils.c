#include <ctype.h>
#include <string.h>
#include "utils.h"

void sanitize(char *s) {
    char *p=s;
    while(*p) { if(*p=='\r') *p='\0'; p++; }
}

void cell_name(int row, int col, char *buf) {
    char colname[10];
    int c=col,len=0;
    do { colname[len++]='A'+(c%26); c=c/26-1; } while(c>=0);
    for(int i=0;i<len;i++) buf[i]=colname[len-1-i];
    sprintf(buf+len,"%d", row+1);
}

int parse_cell(const char *ref, int *row, int *col) {
    int c=0,r=0,i=0;
    while(isalpha(ref[i])) { c=c*26 + (toupper(ref[i])- 'A' +1); i++; }
    c--;
    while(isdigit(ref[i])) { r=r*10 + (ref[i]-'0'); i++; }
    r--;
    if(r<0||r>=MAX_ROWS||c<0||c>=MAX_COLS) return 0;
    *row=r; *col=c;
    return 1;
}
