#ifndef INTERPRETER_H 
#define INTERPRETER_H

#include "ast.h"
#include "symbolTable.h"

Symbol evalExpression(ASTNode* node, SymbolTable* table);
int interpreter(ASTNode* root);
void execDecls(ASTNode* node, SymbolTable* table);
int execSents(ASTNode* node, SymbolTable* table);

#endif