%{
#include <stdio.h>
#include <stdlib.h>
#include "ast.h"

extern FILE *yyin;
extern int yylineno;

int yylex(void);
void yyerror(const char *s);

ASTNode* root = NULL;

static void applyDeclType(ASTNode* idList, DataType type) {
    ASTNode* cursor = idList;
    while (cursor != NULL) {
        cursor->left->type = NODE_DECL;
        cursor->left->info->type = type;
        cursor = cursor->right;
    }
}

%}

%union {
    int num;       /*números Enteros*/
    float fnum;		/* Números flotantes*/
    char* string;   /* Identificadores */
    ASTNode* node;  /* Nodo del AST */
    DataType dtype;  /* Tipo de dato */
}

%define parse.error verbose
%token INT BOOLEAN RETURN VOID TRUE FALSE IF WHILE FLOAT ELSE
%token <string> ID
%token <num> NUM
%token <fnum> FNUM
%token TIMES ASSIGN SEMI LPAREN RPAREN LBRACE RBRACE PLUS MINUS SLASH MOD COMMA
%token LT GT EQ AND OR NOT

%type <dtype> type
%type <node> program var_decls method_decls var_decl id_list method_decl params param_list block block_var_decls statements statement method_call args arg_list expr
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

program:  var_decls method_decls {root = createProgNode($1, $2); $$ = root;}
    | method_decls {root = createProgNode(NULL, $1); $$ = root;}
    ; 
type: INT {$$ = TYPE_INT;} 
    | BOOLEAN {$$ = TYPE_BOOL;}
    | FLOAT {$$ = TYPE_FLOAT;}
    ;

var_decls: var_decls var_decl {$$ = enlistSeqNode($1, $2);} 
        | var_decl {$$ = createSeqNode($1, NULL);}
        ;

method_decls: method_decls method_decl {$$ = enlistSeqNode($1, $2);} 
            | method_decl {$$ = createSeqNode($1, NULL);}
            ;
var_decl: type id_list SEMI {applyDeclType($2, $1); $$ = $2;}
;
id_list: ID {$$ = createSeqNode(createIdNode($1), NULL);} 
    | id_list COMMA ID {$$ = enlistSeqNode($1, createIdNode($3));}
    ;

method_decl
        : type ID LPAREN params RPAREN block {$$ = createMethodNode($1, $2, $4, $6);}
        | VOID ID LPAREN params RPAREN block {$$ = createMethodNode(TYPE_VOID, $2, $4, $6);}
        ;

params: %empty {$$ = NULL;} 
    | param_list {$$ = $1;}
    ;
param_list: type ID {$$ = createSeqNode(createParamNode($1, $2), NULL); }
    | param_list COMMA type ID {$$ = enlistSeqNode($1, createParamNode($3, $4)); }
    ;

block: LBRACE block_var_decls statements RBRACE;
block_var_decls: %empty | block_var_decls var_decl;
statements: %empty | statements statement;

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
    fprintf(stderr, "Error de sintaxis en la linea %d: %s\n", yylineno, s);
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