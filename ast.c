#include <stdio.h>
#include <string.h>
#include "ast.h"

/*
 * Convencion de hijos por tipo de nodo:
 *   NODE_OP      left = operando izq, right = operando der (operador en info->kind)
 *   NODE_UNOP    left = operando (operador en info->kind)
 *   NODE_ASSIGN  left = ID, right = expr
 *   NODE_IF      left = condicion, middle = then, right = else (o NULL)
 *   NODE_WHILE   left = condicion, right = cuerpo
 *   NODE_RETURN  left = expr (o NULL)
 *   NODE_CALL    left = argumentos (SEQ)
 *   NODE_METHOD  left = parametros (SEQ), right = cuerpo (tipo y nombre en info)
 *   NODE_BLOCK   left = declaraciones (SEQ), right = sentencias (SEQ)
 *   NODE_SEQ     left = elemento actual, right = resto de la lista
 *   NODE_PROG    left = globales, right = metodos
 */

//Funciones auxiliares privadas

//Reserva memoria para un nodo e inicializa sus campos
static ASTNode* newNode(NodeType type, Symbol* info, ASTNode* left, ASTNode* middle, ASTNode* right) {
    // TODO: implementar
    return NULL;
}

//Reserva memoria para un simbolo e inicializa sus campos
static Symbol* newSymbol(SymbolType kind, DataType type, char* name) {
    // TODO: implementar
    return NULL;
}


//Hojas (literales e identificadores)

ASTNode* createNumNode(int val) {
    // TODO: implementar
    return NULL;
}

ASTNode* createBoolNode(bool val) {
    // TODO: implementar
    return NULL;
}

ASTNode* createFloatNode(float val) {
    // TODO: implementar
    return NULL;
}

ASTNode* createIdNode(char* name) {
    // TODO: implementar
    return NULL;
}


//Expresiones

ASTNode* createOpNode(SymbolType op, ASTNode* left, ASTNode* right) {
    // TODO: implementar
    return NULL;
}

ASTNode* createUnopNode(SymbolType op, ASTNode* expr) {
    // TODO: implementar
    return NULL;
}

ASTNode* createCallNode(char* function_name, ASTNode* args) {
    // TODO: implementar
    return NULL;
}


//Sentencias

ASTNode* createAssignNode(char* id_name, ASTNode* expression) {
    // TODO: implementar
    return NULL;
}

ASTNode* createReturnNode(ASTNode* expression) {
    // TODO: implementar
    return NULL;
}

ASTNode* createIfNode(ASTNode* condition, ASTNode* then_branch, ASTNode* else_branch) {
    // TODO: implementar
    return NULL;
}

ASTNode* createWhileNode(ASTNode* cond, ASTNode* body) {
    // TODO: implementar
    return NULL;
}

ASTNode* createBlockNode(ASTNode* decls, ASTNode* stmts) {
    // TODO: implementar
    return NULL;
}


//Declaraciones

ASTNode* createDeclNode(DataType var_type, char* id_name) {
    // TODO: implementar
    return NULL;
}

ASTNode* createParamNode(DataType type, char* name) {
    // TODO: implementar
    return NULL;
}

ASTNode* createMethodNode(DataType return_type, char* name, ASTNode* params, ASTNode* body) {
    // TODO: implementar
    return NULL;
}


//Listas y programa

ASTNode* createSeqNode(ASTNode* current, ASTNode* next) {
    // TODO: implementar
    return NULL;
}

ASTNode* createProgNode(ASTNode* globals, ASTNode* methods) {
    // TODO: implementar
    return NULL;
}


//Impresion del arbol (level = profundidad, para indentar)
void printAST(ASTNode* node, int level) {
    // TODO: implementar
}

//Liberacion de memoria (recorrido post-orden)
void freeAST(ASTNode* node) {
    // TODO: implementar
}
