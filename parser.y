%code requires {
    #include "ast.h"
}

%{

#include <stdio.h>
#include <stdlib.h>
#include "cli.h"
#include "ast.h"
#include "semantic.h"
#include "intermediateCode.h"
#include "assemblyGenerator.h"

#ifndef RUNTIME_PATH
#define RUNTIME_PATH "runtime.c"
#endif

extern FILE *yyin;
extern char *yytext;
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
%token INT BOOLEAN RETURN VOID TRUE FALSE IF WHILE FLOAT ELSE EXTERN
%token <string> ID
%token <num> NUM
%token <fnum> FNUM
%token TIMES ASSIGN SEMI LPAREN RPAREN LBRACE RBRACE PLUS MINUS SLASH MOD COMMA
%token LT GT EQ AND OR NOT

%type <dtype> type
%type <node> program var_decls method_decls var_decl id_list method_decl extern_decl params param_list block block_var_decls statements statement method_call args arg_list expr
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
        | extern_decl {$$ = $1;}
        ;

extern_decl: EXTERN type ID LPAREN params RPAREN SEMI {$$ = createExternNode($2, $3, $5);}
    | EXTERN VOID ID LPAREN params RPAREN SEMI {$$ = createExternNode(TYPE_VOID, $3, $5);}
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

static int scanTokens(FILE *out){
    int tok;
    while((tok = yylex()) != 0){
        if(tok < 0){
            return 1;
        }
        fprintf(out, "%d %s %s\n", yylineno, yysymbol_name(YYTRANSLATE(tok)), yytext);
        if(tok == ID){
            free(yylval.string);
        }
    }
    return 0;
}


int main(int argc, char **argv){
    Options opts;
    if(parseArgs(argc, argv, &opts) != 0){
        printUsage(argv[0]);
        return 1;
    }

    yyin = fopen(opts.input, "r");
    if(!yyin){
        fprintf(stderr, "No se pudo abrir el archivo %s\n", opts.input);
        return 1;
    }

    //Pasa a 0 solo si todo sale bien. Un unico return al final asegura que siempre se cierre yyin
    int resultado = 1;

    if(opts.target == TARGET_SCAN){
        char *lexName = outputName(opts.input, NULL, ".lex");
        FILE *lexFile = NULL;
        if(lexName != NULL){
            lexFile = fopen(lexName, "w");
        }
        if(lexFile == NULL){
            if(lexName != NULL){
                fprintf(stderr, "No se pudo crear %s\n", lexName);
            } else {
                fprintf(stderr, "No se pudo crear el archivo .lex\n");
            }
        } else {
            if(scanTokens(lexFile) == 0){
                resultado = 0;
            }
            fclose(lexFile);
            //Si hubo un error lexico se borra el .lex a medio escribir, como en las demas etapas
            if(resultado != 0){
                remove(lexName);
            } else if(opts.debug){
                printf("Creado %s.\n", lexName);
            }
        }
        free(lexName);
    } else if(yyparse() == 0){

        if(opts.debug){
            printAST(root, 0, stdout);
            printf("AST generado correctamente.\n");
        }

        if(opts.target == TARGET_PARSE){
            char *sintName = outputName(opts.input, NULL, ".sint");

            FILE* sintFile = NULL;
            if(sintName != NULL){
                sintFile = fopen(sintName, "w");
            }

            if(sintFile == NULL){
                if(sintName != NULL){
                    fprintf(stderr, "No se pudo crear %s\n", sintName);
                } else {
                    fprintf(stderr, "No se pudo crear el archivo .sint\n");
                }
            } else {
                printAST(root, 0, sintFile);
                fclose(sintFile);
                if(opts.debug){
                    printf("Creado %s.\n", sintName);
                }
                resultado = 0;
            }
            free(sintName);
        } else if(semanticAnalysis(root) == 0){
            if(opts.target == TARGET_SEM){
                char *semName = outputName(opts.input, NULL, ".sem");

                FILE* semFile = NULL;
                if(semName != NULL){
                    semFile = fopen(semName, "w");
                }

                if(semFile == NULL){
                    if(semName != NULL){
                        fprintf(stderr, "No se pudo crear %s\n", semName);
                    } else {
                        fprintf(stderr, "No se pudo crear el archivo .sem\n");
                    }
                } else {
                    printAST(root, 0, semFile);
                    fclose(semFile);
                    if(opts.debug){
                        printf("Creado %s.\n", semName);
                    }
                    resultado = 0;
                }
                free(semName);
            } else {
                IntermediateList irList;
                initIntermediateList(&irList);
                generateMidRepresentation(root, &irList);

                if(opts.debug){
                    printIntermediateList(&irList, stdout);
                    printf("Codigo intermedio generado correctamente.\n");
                }

                if(opts.target == TARGET_CODINTER){
                    char *ciName = outputName(opts.input, NULL, ".ci");

                    FILE* ciFile = NULL;
                    if(ciName != NULL){
                        ciFile = fopen(ciName, "w");
                    }

                    if(ciFile == NULL){
                        if(ciName != NULL){
                            fprintf(stderr, "No se pudo crear %s\n", ciName);
                        } else {
                            fprintf(stderr, "No se pudo crear el archivo .ci\n");
                        }
                    } else {
                        printIntermediateList(&irList, ciFile);
                        fclose(ciFile);
                        if(opts.debug){
                            printf("Creado %s.\n", ciName);
                        }
                        resultado = 0;
                    }
                    free(ciName);
                } else {
                    //-o solo renombra el ejecutable, por eso el .ass no lo usa
                    char *assemblyName = outputName(opts.input, NULL, ".ass");

                    //Si outputName no consiguio memoria no hay nombre, y fopen(NULL) no se puede llamar
                    FILE* assemblyFile = NULL;
                    if(assemblyName != NULL){
                        assemblyFile = fopen(assemblyName, "w");
                    }

                    if(assemblyFile == NULL){
                        if(assemblyName != NULL){
                            fprintf(stderr, "No se pudo crear %s\n", assemblyName);
                        } else {
                            fprintf(stderr, "No se pudo crear el archivo .ass\n");
                        }
                    } else {
                        generateAssembly(&irList, assemblyFile);
                        fclose(assemblyFile);
                        if(opts.debug){
                            printf("Creado %s.\n", assemblyName);
                        }

                        if(opts.target == TARGET_EXEC){
                            //Sin -target se sigue hasta el ejecutable: gcc ensambla el .ass y lo linkea con runtime.c
                            //Aca si se usa -o: si se paso, es el nombre del ejecutable; si no, nombreArchivo.out
                            char *execName = outputName(opts.input, opts.output, ".out");
                            if(execName == NULL){
                                fprintf(stderr, "No se pudo generar el nombre del ejecutable\n");
                            } else {
                                //Las comillas simples evitan que la shell separe nombres con espacios
                                //gcc no reconoce la extension .ass: -x assembler le dice que lo que sigue es assembly
                                //y -x none vuelve a decidir por la extension, para que runtime.c se compile como C
                                char command[1024];
                                snprintf(command, sizeof(command), "gcc -o '%s' -x assembler '%s' -x none '%s'", execName, assemblyName, RUNTIME_PATH);
                                if(opts.debug){
                                    printf("Ejecutando: %s\n", command);
                                }
                                //system devuelve 0 si gcc termino bien
                                if(system(command) != 0){
                                    fprintf(stderr, "No se pudo generar el ejecutable %s\n", execName);
                                } else {
                                    if(opts.debug){
                                        printf("Creado %s.\n", execName);
                                    }
                                    resultado = 0;
                                }
                                free(execName);
                            }
                        } else {
                            //Con -target assembly la compilacion termina en el .ass
                            resultado = 0;
                        }
                    }
                    free(assemblyName);
                }

                freeIntermediateList(&irList);
            }
        }

        freeAST(root);
    }

    fclose(yyin);

    return resultado;
}