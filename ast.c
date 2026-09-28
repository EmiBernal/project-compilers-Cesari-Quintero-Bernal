#include <stdio.h>
#include <stdlib.h>
#include "ast.h"
extern int yylineno;

static ASTNode* newNode(NodeType type, Symbol* info, ASTNode* left, ASTNode* middle, ASTNode* right) {
    ASTNode* node = (ASTNode*) malloc(sizeof(ASTNode));
    if(node == NULL){
        return NULL;
    }
    node->type = type;
    node->info = info;
    node->left = left;
    node->middle = middle;
    node->right = right;
    return node;
}

//Reserva memoria para un simbolo e inicializa sus campos
static Symbol* newSymbol(SymbolType kind, DataType type, char* name) {
    Symbol* sym = (Symbol*) malloc(sizeof(Symbol));
    if(sym == NULL){
        return NULL;
    }
    sym->kind = kind;
    sym->value.ival = 0;
    sym->type = type;
    sym->name = name;

    return sym;
}


//Hojas (literales e identificadores)

ASTNode* createNumNode(int val) {
    //Creo el simbolo
    Symbol* sym = newSymbol(CONSTANT, TYPE_INT, NULL);
    if(sym == NULL){
        return NULL;
    }
    //obtengo el valor del simbolo
    sym->value.ival = val;

    ASTNode* node = newNode(NODE_NUM, sym, NULL, NULL, NULL);
    if(node == NULL){
        free(sym);
    return NULL;
    }
    return node;
}

ASTNode* createBoolNode(bool val) {
    //Creo el simbolo
    Symbol* sym = newSymbol(CONSTANT, TYPE_BOOL, NULL);
    if(sym == NULL){
        return NULL;
    }
    //obtengo el valor del simbolo
    sym->value.bval = val;
    ASTNode* node = newNode(NODE_BOOL, sym, NULL, NULL, NULL);
    if(node == NULL){
        free(sym);
    return NULL;
    }
    return node;
}

ASTNode* createFloatNode(float val) {
    //Creo el simbolo
    Symbol* sym = newSymbol(CONSTANT, TYPE_FLOAT, NULL);
    if(sym == NULL){
        return NULL;
    }
    //obtengo el valor del simbolo
    sym->value.fval = val;

    ASTNode* node = newNode(NODE_FLOAT, sym, NULL, NULL, NULL);
    if(node == NULL){
        free(sym);
    return NULL;
    }
    return node;
}


ASTNode* createIdNode(char* name) {
    //Creo el simbolo
    Symbol* sym = newSymbol(VAR, TYPE_VOID, name);
    if(sym == NULL){
        return NULL;
    }
    ASTNode* node = newNode(NODE_ID, sym, NULL, NULL, NULL);
    if(node == NULL){
        free(sym);
        return NULL;
    }
    return node;

}

//Expresiones

ASTNode* createOpNode(SymbolType op, ASTNode* left, ASTNode* right) {
    //Creo el simbolo
    Symbol* sym = newSymbol(op, TYPE_VOID, NULL);
    if(sym == NULL){
        return NULL;
    }
    //Creo un nodo
    ASTNode* node = newNode(NODE_OP, sym, left, NULL, right);
    if(node == NULL){
        free(sym);
    return NULL;
    }
    return node;
}

ASTNode* createUnopNode(SymbolType op, ASTNode* expr) {
    //Creo el simbolo
    Symbol* sym = newSymbol(op, TYPE_VOID, NULL);
    if(sym == NULL){
        return NULL;
    }
    //Creo un nodo
    ASTNode* node = newNode(NODE_UNOP, sym, NULL, NULL, NULL);
    if(node == NULL){
        free(sym);
    return NULL;
    }
    return node;
}

ASTNode* createCallNode(char* function_name, ASTNode* args) {
    //Creo el simbolo
    Symbol* sym = newSymbol(FUNCTION, TYPE_VOID, function_name);
    if(sym == NULL){
    return NULL;
}
    //Creo un nodo
    ASTNode* node = newNode(NODE_CALL, sym, args, NULL, NULL);
    if(node == NULL){
        free(sym);
        return NULL;
    }
    return node;
}

//Sentencias

ASTNode* createAssignNode(char* id_name, ASTNode* expression) {
    ASTNode* id_node = createIdNode(id_name);
    if (id_node == NULL) {
        return NULL;
    }
    return newNode(NODE_ASSIGN, NULL, id_node, NULL, expression);
}

ASTNode* createReturnNode(ASTNode* expression) {
    return newNode(NODE_RETURN, NULL, expression, NULL, NULL);    
}

ASTNode* createIfNode(ASTNode* condition, ASTNode* then_branch, ASTNode* else_branch) {
    return newNode(NODE_IF, NULL, condition, then_branch, else_branch);    
}

ASTNode* createWhileNode(ASTNode* condition, ASTNode* body) {
    return newNode(NODE_WHILE, NULL, condition, NULL, body);
}

ASTNode* createBlockNode(ASTNode* decls, ASTNode* stmts) {
    return newNode(NODE_BLOCK, NULL, decls, NULL, stmts);
}

//Declaraciones

ASTNode* createDeclNode(DataType var_type, char* id_name) {
    Symbol* symbol = newSymbol(VAR, var_type, id_name);
    if (symbol == NULL) {
        return NULL;
    }
    return newNode(NODE_DECL, symbol, NULL, NULL, NULL);
}

ASTNode* createParamNode(DataType type, char* name) {
    Symbol* symbol = newSymbol(PARAM, type, name);
    if (symbol == NULL) {
        return NULL;
    }
    return newNode(NODE_PARAM, symbol, NULL, NULL, NULL);
}

ASTNode* createMethodNode(DataType return_type, char* name, ASTNode* params, ASTNode* body) {
    Symbol* symbol = newSymbol(FUNCTION, return_type, name);
    if (symbol == NULL) {
        return NULL;
    }
    return newNode(NODE_METHOD, symbol, params, NULL, body);
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

    if(node == NULL) return;

    //Por cada nivel imprimo un espacio para indentar
    for(int i = 0; i < level; i++) {
        printf("  ");
    }

    printf("");
}

//Liberacion de memoria (recorrido post-orden)
void freeAST(ASTNode* node) {

    //Caso base: si el nodo es NULL, no hay nada que liberar
    if (node == NULL) return;

    //Liberacion recursiva de los hijos
    freeAST(node->left);
    freeAST(node->middle);
    freeAST(node->right);

    //Liberacion del simbolo
    if(node->info != NULL){
        free(node->info->name);
        free(node->info);
    }

    //Liberacion del nodo actual
    free(node);
}
