#include "symbolTable.h"
#include <string.h>
#include <stdlib.h>

void initSymbolTable(SymbolTable* table){
    table->top = NULL;
}

void enterScope(SymbolTable* table){
    Scope* newScope = malloc(sizeof(Scope));
    newScope->symbols = NULL; /* no tiene simbolos todavia */
    newScope->previous = table->top;
    table->top = newScope; 
}

void exitScope(SymbolTable* table){
    SymbolTableEntry* current = table->top->symbols;
    while(current != NULL){
        SymbolTableEntry* aux = current->next; /* apunta al siguiente */
        free(current); 
        current = aux;  
    }

    Scope* scopeToRemove = table->top; 
    table->top = table->top->previous; 
    free(scopeToRemove);
}

bool insertSymbol(SymbolTable* table, Symbol* symbol){
    SymbolTableEntry* current = table->top->symbols; 
    while(current != NULL){
        if(strcmp(current->symbol->name, symbol->name) == 0){
            return false;
        }
        current = current->next;
    }
    SymbolTableEntry* newSymbol = malloc(sizeof(SymbolTableEntry));
    newSymbol->symbol = symbol; 
    newSymbol->next = table->top->symbols; /* apunta al primer elemento */
    table->top->symbols = newSymbol; /* el nuevo es el primero */
    return true;
}

Symbol* lookupSymbol(SymbolTable* table, const char* name){
    Scope* currentScope = table->top;
    while(currentScope != NULL){
        SymbolTableEntry* currentSymbol = currentScope->symbols;
        while(currentSymbol != NULL){
            if(strcmp(currentSymbol->symbol->name, name) == 0){
                return currentSymbol->symbol;
            }
            currentSymbol = currentSymbol->next;
        }
        currentScope = currentScope->previous;
    }
    return NULL;
}
