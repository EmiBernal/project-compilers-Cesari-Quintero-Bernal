#include <stdio.h>
#include "ast.h"
#include "symbolTable.h"
#include "interpreter.h"
#include <stdbool.h>

Symbol evalExpression(ASTNode* node, SymbolTable* table){
    Symbol result;
    switch(node->type) {
        case (NODE_NUM):
            result.value = node->info->value;
            result.type = TYPE_INT; 
            return result;
        case (NODE_BOOL): 
            result.value = node->info->value;
            result.type = TYPE_BOOL; 
            return result;
        case (NODE_ID): {
            Symbol* sym = getVarValue(table, node->info->name);
            if(sym == NULL){
                fprintf(stderr, "Error: variable '%s' no declarada\n", node->info->name);
                result.value = 0;
                result.type = TYPE_VOID; 
                return result;
            } 
            result.value = sym->value;
            result.type = sym->type; 
            return result;
        }
        case (NODE_OP): {
            Symbol leftVal = evalExpression(node->left, table);
            Symbol rightVal = evalExpression(node->right, table);

            result.type = TYPE_INT;
            if(node->info->kind == OP_ADD){
                result.value = leftVal.value + rightVal.value;
            } else if(node->info->kind == OP_MUL) {
                result.value = leftVal.value * rightVal.value;
            } else {
                fprintf(stderr, "Error: operador desconocido\n");
                result.value = 0;
            } 
            return result;
        } 
        
        default : 
            fprintf(stderr, "Error: nodo no evaluable");
            result.value = 0;
            result.type = TYPE_VOID;
            return result;
    }
}

int interpreter(ASTNode* root){
    SymbolTable table;
    initTable(&table);

    execDecls(root->left, &table);

    return execSents(root->right, &table);
}

void execDecls(ASTNode* node, SymbolTable* table){
    if(node == NULL) return;
    if(node->type == NODE_SEQ){
        execDecls(node->left, table);
        execDecls(node->right, table);
    } else if(node->type == NODE_DECL){
        setVarValue(table, node->info);
    }
}

int execSents(ASTNode* node, SymbolTable* table){
    if(node == NULL) return 0; 

    if(node->type == NODE_SEQ){
        int leftResult = execSents(node->left, table);
        int rightResult = execSents(node->right, table);
        return rightResult;
    }
    if(node->type == NODE_ASSIGN){
        Symbol expr = evalExpression(node->left, table);
        Symbol* sym = getVarValue(table, node->info->name);
        if(sym == NULL){
            fprintf(stderr, "Error: variable '%s' no declarada\n", node->info->name);
            return 0;
        }
        sym->value = expr.value;
        setVarValue(table, sym);

        return 0;
    }
    if(node->type == NODE_RETURN){
        if(node->left == NULL) return 0;
        Symbol result = evalExpression(node->left, table);
        return result.value;
    }

    return 0;
}