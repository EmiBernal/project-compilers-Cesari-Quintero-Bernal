#ifndef AST_H
#define AST_H

#include <stdbool.h>

typedef enum {
    TYPE_INT,
    TYPE_BOOL,
    TYPE_VOID
} DataType;

//tipos del nodo
typedef enum {
    NODE_NUM,
    NODE_BOOL,
    NODE_ID,
    NODE_SEQ,
    NODE_DECL,
    NODE_RETURN,
    NODE_OP,
    NODE_ASSIGN,
    NODE_PROG
} NodeType;

//tipos del simbolo

typedef enum {
    VAR,
    CONSTANT,
    FUNCTION,
    OP_ADD,
    OP_MUL,
    OP_AND,
    OP_OR
} SymbolType;

typedef struct Symbol{
    SymbolType kind ;     /*Tipo del simbolo */
    int value;                  /*Valor del simbolo */
    char* name;                   /*Id del simbolo */
    DataType type;
} Symbol;


//Defino la estructura de datos del AST
typedef struct ASTNode{
    NodeType type;              /*Tipo del nodo */
    Symbol* info;           /*Simbolo del nodo */
    struct ASTNode* left;        /*Nodo izquierdo */
    struct ASTNode* right;        /*Nodo derecho */
} ASTNode;


//Perfiles de funciones auxiliares
ASTNode* createNumNode(int val); //
ASTNode* createBoolNode(bool val);
ASTNode* createIdNode(char* name); //
ASTNode* createOpNode(char op, ASTNode* left, ASTNode* right); //
ASTNode* createAssignNode(char* id_name, ASTNode* expression);
ASTNode* createDeclNode(DataType var_type, char* id_name);
ASTNode* createReturnNode(ASTNode* expression); /* Puede ser NULL para "return;" */
ASTNode* createSeqNode(ASTNode* current, ASTNode* next);
ASTNode* createProgNode(DataType return_type, ASTNode* decls, ASTNode* stmts);

//Funcion para ver el arbol AST
void printAST(ASTNode* node, int level);

#endif
