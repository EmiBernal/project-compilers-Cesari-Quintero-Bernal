%{
#include <stdio.h>
#include <stdlib.h>

extern FILE *yyin;

int yylex(void);
void yyerror(const char *s);

%}

%union {
    int num;       /* Para almacenar el valor de los números */
    float fnum;		/* Para almacenar el valor de los números flotantes*/
    char* string;   /* Para almacenar los nombres de los identificadores */

}

%token INT BOOLEAN RETURN VOID TRUE FALSE IF WHILE FLOAT ELSE
%token <string> ID
%token <num> NUM
%token <fnum> FNUM
%token TIMES ASSIGN SEMI LPAREN RPAREN LBRACE RBRACE PLUS MINUS SLASH MOD COMMA
%token LT GT EQ AND OR NOT

%start program

%left OR
%left AND
%nonassoc EQ
%nonassoc LT GT
%left PLUS MINUS
%left TIMES SLASH MOD
%precedence NOT
%precedence UMINUS

%%

program:

%%
void yyerror(const char *s){
    fprintf(stderr, "Error de sintaxis: %s\n", s);
}

int main(int argc, char **argv){
    if(argc > 1){
        yyin = fopen(argv[1],"r");
        if(!yyin){
            fprintf(stderr, "No se pudo abrir el archivo %s\n", argv[1]);
            return 1;
        }
    }

    if(yyparse() == 0){
        printf("Programa aceptado correctamente.\n");
        
    }

    return 0;
}