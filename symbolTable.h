#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "ast.h"
typedef struct SymbolTableEntry{
    Symbol* symbol; 
    struct SymbolTableEntry* next;
} SymbolTableEntry; /* un puntero a un simbolo con enlace al siguiente */

typedef struct Scope{
    SymbolTableEntry* symbols; 
    struct Scope* previous; 
} Scope; /* un puntero a una tabla de simbolos con enlace al scope anterior */

typedef struct SymbolTable{
    Scope* top; 
} SymbolTable; /* un puntero al scope actual de la tabla de simbolos */

void initSymbolTable(SymbolTable* table);
void enterScope(SymbolTable* table);
void exitScope(SymbolTable* table);
bool insertSymbol(SymbolTable* table, Symbol* symbol);
Symbol* lookupSymbol(SymbolTable* table, const char* name);

#endif 