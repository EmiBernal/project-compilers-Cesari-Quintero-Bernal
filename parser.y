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

program: %empty | program var_decl | program method_decl;
type: INT | BOOLEAN | FLOAT;
var_decl: type id_list SEMI;
id_list: ID | id_list COMMA ID;

method_decl
        : type ID LPAREN params RPAREN block
        | VOID ID LPAREN params RPAREN block;

params: %empty | param_list;
param_list: type ID | param_list COMMA type ID;

block: LBRACE block_var_decls statements RBRACE;
block_var_decls: %empty | block_var_decls var_decl;
statements: %empty | statement statements;

statement
        : ID ASSIGN expr SEMI 
        | method_call SEMI
        | IF LPAREN expr RPAREN block
        | IF LPAREN expr RPAREN block ELSE block
        | WHILE LPAREN expr RPAREN block
        | RETURN expr SEMI
        | RETURN SEMI
        | SEMI
        | block
        ;

method_call: ID LPAREN args RPAREN;
args: %empty | arg_list;
arg_list: expr | arg_list COMMA expr;
expr
    : ID
    | method_call
    | NUM
    | FNUM
    | TRUE
    | FALSE
    | expr PLUS expr
    | expr MINUS expr
    | expr TIMES expr
    | expr SLASH expr
    | expr MOD expr
    | expr LT expr
    | expr GT expr
    | expr EQ expr
    | expr AND expr
    | expr OR expr
    | NOT expr
    | LPAREN expr RPAREN
    | MINUS expr %prec UMINUS
    ;

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