#include "optimizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>

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

//Pliega "-x" y "!x" cuando x es constante. Devuelve la constante nueva, o NULL si no se puede
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
    //Recorro la lista
    InstructionNode* node = list->head;
    while(node != NULL){
        Instruction* instruction = node->instruction;
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
        node = node->next;
    }
}
