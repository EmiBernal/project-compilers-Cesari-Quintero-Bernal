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
    SymbolType kind ;                                       /*Tipo del simbolo */
    union { int ival;
            float fval;
            bool bval;
    } value;                                                /*Valor del simbolo */
    char* name;                                             /*Id del simbolo */
    DataType type;
} Symbol;


//Defino la estructura de datos del AST
typedef struct ASTNode{
    NodeType type;                  /*Tipo del nodo */
    Symbol* info;                   /*Simbolo del nodo */
    int line;                       /*Linea del programa donde se creo el nodo */
    struct ASTNode* left;          /*Nodo izquierdo */
    struct ASTNode* middle;         /*Nodo medio */
    struct ASTNode* right;          /*Nodo derecho */
} ASTNode;


//Perfiles de funciones auxiliares
ASTNode* createNumNode(int val);
ASTNode* createBoolNode(bool val);
ASTNode* createIdNode(char* name);
ASTNode* createOpNode(SymbolType op, ASTNode* left, ASTNode* right);
ASTNode* createUnopNode(SymbolType op, ASTNode* expr);                                  //op es OP_UMINUS u OP_NEG
ASTNode* createAssignNode(char* id_name, ASTNode* expression);
ASTNode* createDeclNode(DataType var_type, char* id_name);
ASTNode* createReturnNode(ASTNode* expression);
ASTNode* createFloatNode(float val);
ASTNode* createIfNode(ASTNode* condition, ASTNode* then_branch, ASTNode* else_branch);  //else_branch puede llegar a ser null
ASTNode* createWhileNode(ASTNode* condition, ASTNode* body);
ASTNode* createCallNode(char* function_name, ASTNode* args);
ASTNode* createParamNode(DataType type, char* name);
ASTNode* createMethodNode(DataType return_type, char* name, ASTNode* params, ASTNode* body);
ASTNode* createBlockNode(ASTNode* decls, ASTNode* stmts);
ASTNode* createSeqNode(ASTNode* current, ASTNode* next);
ASTNode* enlistSeqNode(ASTNode* list, ASTNode* item);
ASTNode* createProgNode(ASTNode* globals, ASTNode* methods);

//Funcion para ver el arbol AST
void printAST(ASTNode* node, int level);

//Funcion para liberar la memoria del arbol
void freeAST(ASTNode* node);

#endif
