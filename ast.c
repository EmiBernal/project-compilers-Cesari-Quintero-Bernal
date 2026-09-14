#include "ast.h"
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

static ASTNode* createNode(NodeType type){
    ASTNode* node = (ASTNode*) malloc(sizeof(ASTNode));
    if(node == NULL){
        return NULL;
    }
    node->type = type;
    node->info = NULL;
    node->left = NULL;
    node->right = NULL;
    return node;
}

static Symbol* createSymbol(SymbolType kind, int value, char* name, DataType type){
    Symbol* sym = (Symbol*) malloc(sizeof(Symbol));
    if(sym == NULL){
        return NULL;
    }
    sym->kind = kind;
    sym->value = value;
    sym->name = name;
    sym->type = type;
    return sym;
}

/*Implementacion de funciones auxiliares */
ASTNode* createNumNode(int val) {
    ASTNode* node = createNode(NODE_NUM);
    if(node == NULL){
        return NULL;
    }
    node->info = createSymbol(CONSTANT, val, NULL, TYPE_INT);
    return node;
}

ASTNode* createIdNode(char* name) {
    ASTNode* node = createNode(NODE_ID);
    if(node == NULL){
        return NULL;
    }
    node->info = createSymbol(VAR, 0, name, TYPE_VOID); // VER
    return node;
}

ASTNode* createOpNode(char op, ASTNode* left, ASTNode* right) {
    ASTNode* node = createNode(NODE_OP);
    if(node == NULL){
        return NULL;
    }
    SymbolType kind;
    if(op == '+'){
        kind = OP_ADD;
    } else if (op == '*'){
        kind = OP_MUL;
    } else {
        fprintf(stderr, "Operador no reconocido: %c\n", op);
        free(node);
        return NULL;
    }
    node->info = createSymbol(kind, 0, NULL, TYPE_VOID);
    node->left = left;
    node->right = right;
    return node;
}

ASTNode* createBoolNode(bool val){
    ASTNode* node = createNode(NODE_BOOL);
    if(node == NULL){
        return NULL;
    }
    node->info = createSymbol(CONSTANT, val, NULL, TYPE_BOOL); 
    return node;
}

ASTNode* createReturnNode(ASTNode* expression){
    ASTNode* node = createNode(NODE_RETURN);
    if(node == NULL){
        return NULL;
    }
    node->left = expression;
    return node;
}

ASTNode* createAssignNode(char* id_name, ASTNode* expression){
    ASTNode* node = createNode(NODE_ASSIGN);
    if(node == NULL){
        return NULL;
    }
    node->info = createSymbol(VAR, 0, id_name, TYPE_VOID);//
    node->left = expression;
    return node;
}

ASTNode* createDeclNode(DataType type_var, char* name_id){
    ASTNode* node = createNode(NODE_DECL);
    if (node == NULL) {
        return NULL;
    }

    node->info = createSymbol(VAR, 0, name_id, type_var);//

    return node;
}

ASTNode* createSeqNode(ASTNode* current, ASTNode* next){
    ASTNode* node = createNode(NODE_SEQ);
    if(node == NULL){
        return NULL;
    }
    node->left = current;
    node->right = next;
    return node;
}

ASTNode* createProgNode(DataType return_type, ASTNode* decls, ASTNode* sents){
    ASTNode* node = createNode(NODE_PROG);
    if(node == NULL){
        return NULL;
    }

    node->info = createSymbol(FUNCTION, 0, "main", return_type);

    node->left = decls;
    node->right = sents;
    return node;
}

void printAST(ASTNode* node, int level){
    if(node == NULL){
        return;
    }

    for(int i = 0; i < level; i++){
        printf(" ");
    }

    switch(node->type){

                case NODE_NUM:
                    printf("NUM: %d\n", node->info->value);
                    break;

                case NODE_BOOL:
                    printf("BOOL: %s\n",
                           node->info->value ? "true" : "false");
                    break;

                case NODE_ID:
                    printf("ID: %s\n", node->info->name);
                    break;

                case NODE_OP:
                    if (node->info->kind == OP_ADD) {
                        printf("OP: +\n");
                    } else if (node->info->kind == OP_MUL) {
                        printf("OP: *\n");
                    } else {
                        printf("OP\n");
                    }
                    break;

                case NODE_ASSIGN:
                    printf("ASSIGN: %s\n", node->info->name);
                    break;

                case NODE_DECL:
                    printf("DECL: %s\n", node->info->name);
                    break;

                case NODE_RETURN:
                    printf("RETURN\n");
                    break;

                case NODE_SEQ:
                    printf("SEQ\n");
                    break;

                case NODE_PROG:
                    printf("PROGRAM: %s\n", node->info->name);
                    break;

                default:
                    printf("UNKNOWN NODE\n");
            }
            //Recursivamente imprimir los hijos
            printAST(node->left, level + 1);
            printAST(node->right, level + 1);
}
