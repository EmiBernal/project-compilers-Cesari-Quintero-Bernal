#include "symbolTable.h"
#include "ast.h"
#include <string.h>
#include <stdio.h>

void initTable(SymbolTable* table){
    table->count = 0;
}

static int findVar(SymbolTable* table, char* name){
    for(int i = 0; i < table->count; i++){
        if(strcmp(table->vars[i]->name, name) == 0){
            return i;
        }
    }
    return -1;
}

Symbol* getVarValue(SymbolTable* table, char* name){
    int id = findVar(table, name);
    if(id == -1) {
        return NULL;
    }
    return table->vars[id];
}

void setVarValue(SymbolTable* table, Symbol* var){
    int id = findVar(table, var->name);
    if(id == -1) /* significa que no lo encontro */ {
        if(table->count >= MAX_VARS){
            fprintf(stderr, "Error: la tabla de simbolos llena\n");
            return;
        }
        table->vars[table->count] = var;
        table->count++;
    } else {
        table->vars[id] = var;
    }
}