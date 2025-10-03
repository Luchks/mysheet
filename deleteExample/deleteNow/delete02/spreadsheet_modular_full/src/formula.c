#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "formula.h"
#include "utils.h"

double eval_expr(const char **s, Cell sheet[MAX_ROWS][MAX_COLS]);

double eval_formula(const char *formula, Cell sheet[MAX_ROWS][MAX_COLS]) {
    if (!formula || formula[0]!='=') return 0;
    const char *s = formula +1;
    return eval_expr(&s, sheet);
}

double eval_expr(const char **s, Cell sheet[MAX_ROWS][MAX_COLS]) {
    double res=0,num=0;
    char op='+';
    while(**s) {
        if (isspace(**s)) { (*s)++; continue; }
        if (**s=='(') { (*s)++; num=eval_expr(s,sheet); }
        else if (isalpha(**s)) {
            char ref[16]; int j=0;
            while(isalpha(**s)||isdigit(**s)) ref[j++]=*(*s)++;
            ref[j]='\0';
            int r,c;
            if(parse_cell(ref,&r,&c)) {
                if(sheet[r][c].data[0]=='=') num=eval_formula(sheet[r][c].data,sheet);
                else num=atof(sheet[r][c].data);
            } else num=0;
        } else if (isdigit(**s)||**s=='.') num=strtod(*s,(char**)s);
        else if (**s==')') { (*s)++; break; }
        else { op=**s; (*s)++; continue; }

        switch(op) {
            case '+': res+=num; break;
            case '-': res-=num; break;
            case '*': res*=num; break;
            case '/': res/=num; break;
        }
        op=0;
    }
    return res;
}
