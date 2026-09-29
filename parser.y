%code requires {
    #include "ast.h"
}

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

block: LBRACE block_var_decls statements RBRACE {$$ = createBlockNode($2, $3);}
    ;
block_var_decls: %empty {$$ = NULL;} | block_var_decls var_decl {$$ = enlistSeqNode($1, $2);}
    ;
statements: %empty {$$ = NULL;}
    | statements statement {$$ = enlistSeqNode($1, $2);}
    ;

statement
        : ID ASSIGN expr SEMI {$$ = createAssignNode($1, $3);}
        | method_call SEMI {$$ = $1;}
        | IF LPAREN expr RPAREN block {$$ = createIfNode($3, $5, NULL);}
        | IF LPAREN expr RPAREN block ELSE block {$$ = createIfNode($3, $5, $7);}
        | WHILE LPAREN expr RPAREN block {$$ = createWhileNode($3, $5);}
        | RETURN expr SEMI {$$ = createReturnNode($2);}
        | RETURN SEMI {$$ = createReturnNode(NULL);}
        | SEMI {$$ = NULL;}
        | block {$$ = $1;}
        ;

method_call: ID LPAREN args RPAREN {$$ = createCallNode($1, $3);}
    ;

args: %empty {$$ = NULL;} 
    | arg_list {$$ = $1;}
    ;
arg_list: expr {$$ = createSeqNode($1, NULL);}
    | arg_list COMMA expr {$$ = enlistSeqNode($1, $3);}
    ;

expr
    : ID {$$ = createIdNode($1);}
    | method_call {$$ = $1;}
    | NUM {$$ = createNumNode($1);}
    | FNUM {$$ = createFloatNode($1);}
    | TRUE {$$ = createBoolNode(true);}
    | FALSE {$$ = createBoolNode(false);}
    | expr PLUS expr {$$ = createOpNode(OP_ADD, $1, $3);}
    | expr MINUS expr {$$ = createOpNode(OP_SUB, $1, $3);}
    | expr TIMES expr {$$ = createOpNode(OP_MUL, $1, $3);}
    | expr SLASH expr {$$ = createOpNode(OP_DIV, $1, $3);}
    | expr MOD expr {$$ = createOpNode(OP_MOD, $1, $3);}
    | expr LT expr {$$ = createOpNode(OP_LT, $1, $3);}
    | expr GT expr {$$ = createOpNode(OP_GT, $1, $3);}
    | expr EQ expr {$$ = createOpNode(OP_EQ, $1, $3);}
    | expr AND expr {$$ = createOpNode(OP_AND, $1, $3);}
    | expr OR expr {$$ = createOpNode(OP_OR, $1, $3);}
    | NOT expr {$$ = createUnopNode(OP_NEG, $2);}
    | LPAREN expr RPAREN {$$ = $2;}
    | MINUS expr %prec UMINUS {$$ = createUnopNode(OP_UMINUS, $2);}
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
        printAST(root, 0);
        printf("Programa aceptado correctamente.\n");
        
    }

    return 0;
}