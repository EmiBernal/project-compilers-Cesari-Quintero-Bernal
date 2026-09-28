#ifndef AST_H
#define AST_H

#include <stdbool.h>

typedef enum {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_BOOL,
    TYPE_VOID
} DataType;

typedef enum {
    NODE_NUM,
    NODE_FLOAT,
    NODE_BOOL,
    NODE_ID,
    NODE_SEQ,
    NODE_DECL,
    NODE_PARAM,//parametro;
    NODE_METHOD,//metodo
    NODE_BLOCK,
    NODE_ASSIGN,
    NODE_IF,
    NODE_WHILE,
    NODE_RETURN,
    NODE_CALL,//llamada a metodo
    NODE_OP,
    NODE_UNOP,//operador unario
    NODE_PROG
} NodeType;

typedef enum {
    VAR,
    CONSTANT,
    FUNCTION,
    PARAM,
    //operadores
    OP_ADD,
    OP_MUL,
    OP_SUB,
    OP_DIV,
    OP_MOD,
    //relacional
    OP_LT,
    OP_GT,
    OP_EQ,
    //logicos
    OP_AND,
    OP_OR,
    //unarios
    OP_UMINUS,
    OP_NEG //negacion
} SymbolType;


typedef struct Symbol{
    SymbolType kind ;               /*Tipo del simbolo */
    int value;                      /*Valor del simbolo */
    char* name;                     /*Id del simbolo */
    DataType type;
} Symbol;


//Defino la estructura de datos del AST
typedef struct ASTNode{
    NodeType type;              /*Tipo del nodo */
    Symbol* info;           /*Simbolo del nodo */
    struct ASTNode* left;        /*Nodo izquierdo */
    struct ASTNode* right;        /*Nodo derecho */
} ASTNode;

#endif
