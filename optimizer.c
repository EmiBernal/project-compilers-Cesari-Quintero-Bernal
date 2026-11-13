#include "optimizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>

typedef struct KnownConstant {
    Symbol* var;
    Symbol* value;
    struct KnownConstant* next;
} KnownConstant;

static Symbol* lookupConstant(KnownConstant* table, Symbol* var){
    KnownConstant* current = table;
    while (current != NULL){
        //Comparo por puntero, no por nombre
        if(current->var == var){
            return current->value;
        }
        current = current->next;
    }
    return NULL;
}         

static KnownConstant* setConstant(KnownConstant* table, Symbol* var, Symbol* value){
    KnownConstant* current = table;
    while(current != NULL){
        if(current->var == var){
            current->value = value;
            return table;
        }
        current = current->next;
    }
    //No estaba: lo agrego adelante, el nuevo pasa a ser la cabeza
    KnownConstant* newConstant = malloc(sizeof(KnownConstant));
    if(newConstant == NULL){
        return table;
    }
    newConstant->value = value;
    newConstant->var = var;
    newConstant->next = table;
    return newConstant;
}

//Saca a var de la tabla (si estaba). Devuelve la cabeza, que cambia si se borra el primero
static KnownConstant* forgetConstant(KnownConstant* table, Symbol* var){
    KnownConstant* current = table;
    KnownConstant* prev = NULL;
    while(current != NULL){
        if(current->var == var){
            if(prev == NULL){
                //Es el primero: la cabeza pasa a ser el siguiente
                table = current->next;
            } else {
                //Esta en el medio o al final: el anterior se saltea a current
                prev->next = current->next;
            }
            free(current);
            return table;
        }
        prev = current;
        current = current->next;
    }
    return table;
}

//Libera todos los nodos (no los Symbol, que siguen usandose en las instrucciones)
static KnownConstant* clearConstants(KnownConstant* table){
    KnownConstant* current = table;
    while(current != NULL){
        KnownConstant* aux = current->next;
        free(current);
        current = aux;
    }
    return NULL;
}

//Devuelve true si el simbolo es un literal (CONSTANT). Si es NULL devuelve false
static bool isConstant(Symbol* s){
    return s != NULL && s->kind == CONSTANT;
}

static bool isIntConstant(Symbol* s){
    return isConstant(s) && s->type == TYPE_INT;
}

static bool isBoolConstant(Symbol* s){
    return isConstant(s) && s->type == TYPE_BOOL;
}

static Symbol* makeIntConstant(int value){
    Symbol* symbol = (Symbol*) calloc(1, sizeof(Symbol)); // arrancan con valor 0, sin basura;
    if(symbol == NULL){
        return NULL;
    }
    symbol->kind = CONSTANT;
    symbol->type = TYPE_INT;
    symbol->value.ival = value;
    return symbol;
}

static Symbol* makeBoolConstant(bool value){
    Symbol* symbol = (Symbol*) calloc(1, sizeof(Symbol)); // arrancan con valor 0, sin basura;
    if(symbol == NULL){
        return NULL;
    }
    symbol->kind = CONSTANT;
    symbol->type = TYPE_BOOL;
    symbol->value.bval = value;
    return symbol;
}

static bool foldIntBinary(SymbolType op, int a, int b, int* out){
    switch(op){
        case OP_ADD: *out = (int)((unsigned)a + (unsigned)b); return true;
        case OP_SUB: *out = (int)((unsigned)a - (unsigned)b); return true;
        case OP_MUL: *out = (int)((unsigned)a * (unsigned)b); return true;
        case OP_DIV: {
            if(b == 0 || (a == INT_MIN && b == -1)){
                return false;
            }
            *out = a / b; return true;
        }
        case OP_MOD: {
            if(b == 0 || (a == INT_MIN && b == -1)){
                return false;
            }
            *out = a % b; return true;
        }
        case OP_LT: *out = a < b; return true;
        case OP_GT: *out = a > b; return true;
        case OP_EQ: *out = a == b; return true;
        default: return false;
    }
}

//Pliega "-x" y "!x" 
static Symbol* foldUnary(Instruction* instruction){
    Symbol* arg = instruction->arg1;
    switch(instruction->opCode){
        case OP_UMINUS:
            //En unsigned como en el resto: -INT_MIN da INT_MIN, igual que negl
            if(isIntConstant(arg)){
                return makeIntConstant((int)(0u - (unsigned)arg->value.ival));
            }
            return NULL;
        case OP_NEG:
            if(isBoolConstant(arg)){
                return makeBoolConstant(!arg->value.bval);
            }
            return NULL;
        default:
            return NULL;
    }
}

static void replaceWithConstant(Instruction* instruction, Symbol* constant){
    instruction->opCode = OP_ASSIGN;
    instruction->arg1 = constant;
    instruction->arg2 = NULL;
}

void optimizeConstants(IntermediateList* list){
    KnownConstant* table = NULL;
    //Recorro la lista
    InstructionNode* node = list->head;
    while(node != NULL){
        Instruction* instruction = node->instruction;
        Symbol* known = lookupConstant(table, instruction->arg1);
        if(known != NULL){ instruction->arg1 = known; }
        Symbol* known2 = lookupConstant(table, instruction->arg2);
        if(known2 != NULL){ instruction->arg2 = known2; }
        if(isIntConstant(instruction->arg1) && isIntConstant(instruction->arg2)){
            int a = instruction->arg1->value.ival;
            int b = instruction->arg2->value.ival;
            int value;
            //Si la operacion se puede plegar, la reemplazo por su resultado
            if(foldIntBinary(instruction->opCode, a, b, &value)){
                Symbol* result;
                if(instruction->result->type == TYPE_BOOL){
                    result = makeBoolConstant(value);
                } else {
                    result = makeIntConstant(value);
                }
                if(result != NULL){
                    replaceWithConstant(instruction, result);
                }
            }
        } else if(instruction->arg2 == NULL){
            //Unarias: solo tienen arg1
            Symbol* result = foldUnary(instruction);
            if(result != NULL){
                replaceWithConstant(instruction, result);
            }
        }
        //Actualizo la tabla
        if(instruction->opCode == OP_LABEL || instruction->opCode == OP_BEGIN_FUNC){
            
            table = clearConstants(table);        
        
        } else if(instruction->opCode == OP_ASSIGN && (isIntConstant(instruction->arg1) || isBoolConstant(instruction->arg1))){
            
            table = setConstant(table, instruction->result, instruction->arg1);
        
        } else if(instruction->result != NULL && (instruction->result->kind == VAR || instruction->result->kind == TEMPORARY)){
            
            table = forgetConstant(table, instruction->result);
        
        }
        
        node = node->next;
    }
    table = clearConstants(table);
}
