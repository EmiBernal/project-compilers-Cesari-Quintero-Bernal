%{
#include <stdio.h>
#include <stdlib.h>
#include "ast.h"
#include "interpreter.h"
#include "Assembly3Direcciones.h"

extern FILE *yyin;

int yylex(void);
void yyerror(const char *s);

/* Raíz del AST */
ASTNode* root = NULL;
%}

%code requires {
    #include "ast.h"
}

%union {
    int num;       /* Para almacenar el valor de los números */
    char* string;   /* Para almacenar los nombres de los identificadores */
    ASTNode* node;  /* Para almacenar los nodos del AST */
    DataType type;  /* Para almacenar los tipos de datos */
}

%token INT BOOL RETURN MAIN VOID TRUE FALSE PLUS
%token <string> ID
%token <num> NUM
%token TIMES ASSIGN SEMI LPAREN RPAREN LBRACE RBRACE

%start program

/* Asigno tipos a los nodos del AST */
%type <type> var_type return_type
%type <node> e sent sents decl decls program

/* Declaramos la precedencia y asociatividad */
%left PLUS
%left TIMES

%%

return_type:
    INT { $$ = TYPE_INT; }
    | BOOL { $$ = TYPE_BOOL; }
    | VOID { $$ = TYPE_VOID; }

var_type:
    INT { $$ = TYPE_INT; }
    | BOOL { $$ = TYPE_BOOL; }

program:

    return_type MAIN LPAREN RPAREN LBRACE decls sents RBRACE {
        $$ = createProgNode($1, $6, $7);
        root = $$;
    };


decls:

    decl decls {
        $$ = createSeqNode($1, $2);
    }

    | decl {
        $$ = $1;
    };

decl:

    var_type ID SEMI {
        $$ = createDeclNode($1, $2);
    };

sents:

    sent sents {
        $$ = createSeqNode($1, $2);
    }

    | sent {
        $$ = $1;
    }

sent:

    ID ASSIGN e SEMI {
        $$ = createAssignNode($1, $3);
    }

    | RETURN SEMI {
        $$ = createReturnNode(NULL);
    }

    | RETURN e SEMI {
        $$ = createReturnNode($2);
    }

    | LBRACE sents RBRACE {
        $$ = $2;
    }

e:
    e PLUS e {
        $$ = createOpNode('+', $1, $3);
    }

    | e TIMES e {
        $$ = createOpNode('*', $1, $3);
    }

    | LPAREN e RPAREN {
        $$ = $2;
    }

    | NUM {
        $$ = createNumNode($1);
    }

    | ID {
        $$ = createIdNode($1);
    }

    | TRUE {
        $$ = createBoolNode(true);
    }

    | FALSE {
        $$ = createBoolNode(false);
    };

%%
void yyerror(const char *s){
    fprintf(stderr, "Error de sintaxis: %s\n", s);
}

int main(int argc, char **argv){
    int opcion;
    if(argc > 1){
        yyin = fopen(argv[1],"r");
        if(!yyin){
            fprintf(stderr, "No se pudo abrir el archivo %s\n", argv[1]);
            return 1;
        }
    }

    if(yyparse() == 0){
        printf("Programa aceptado correctamente.\n");
        printf("Ingrese 1 para ver el arbolAST, ingrese 2 para ver el codigo en assembly 3D o ingrese 3 para acceder al interprete:\n");
        scanf("%d", &opcion);

        if(opcion == 1){
            printAST(root, 0);
        }else if(opcion == 2){
            generateCode(root);
        }else if(opcion == 3){
            int resultado = interpreter(root);
            printf("Resultado de la ejecucion: %d\n", resultado);
        }
    }

    return 0;
}
