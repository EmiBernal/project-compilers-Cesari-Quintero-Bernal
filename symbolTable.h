#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H
#include "ast.h"
#define MAX_VARS 100 

typedef struct {
    Symbol* vars[MAX_VARS];
    int count;
} SymbolTable;

void initTable(SymbolTable* table);
Symbol* getVarValue(SymbolTable* table, char* name);
void setVarValue(SymbolTable* table, Symbol* var);

#endif