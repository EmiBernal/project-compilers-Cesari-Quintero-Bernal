#include <stdlib.h>
#include "deadCode.h"

// Despues de un return o un goto, la ejecucion nunca pasa a la instruccion siguiente
static bool endsFlows(Instruction* instruction){
    return instruction->opCode == OP_RETURN || instruction->opCode == OP_JUMP;
}
// A un label alguien puede saltar, y end_func cierra el metodo: ahi se deja de borrar
static bool startsReachableCode(Instruction* instruction){
    return instruction->opCode == OP_LABEL || instruction->opCode == OP_END_FUNC;
}

static bool removeUnreachableCode(IntermediateList* list){
    bool changed = false; 
    InstructionNode* current = list->head;
    while(current != NULL){
        if(endsFlows(current->instruction)){
            while (current->next != NULL && !startsReachableCode(current->next->instruction)){
                InstructionNode* dead = current->next; // apunta al nodo que vamos a eliminar
                current->next = dead->next; // lo desengancha
                free(dead->instruction); // liberamos memoria para la instruccion
                free(dead); // liberamos memoria para nodo instruccion
                changed = true;
            }
            if(current->next == NULL){
                list->tail = current;
            }
        }
        current = current->next;
    }
    return changed;
}

static bool isLabelUsed(IntermediateList* list, Symbol* label){
    for (InstructionNode* node = list->head; node != NULL; node = node->next){
        SymbolType op = node->instruction->opCode;
        bool isJump = op == OP_JUMP || op == OP_JUMP_IF_FALSE || op == OP_JUMP_IF_TRUE;
        if(isJump && node->instruction->result == label){
            return true;
        }
    }
    return false;
}

static bool removeUnusedLabels(IntermediateList* list){
    bool changed = false; 
    InstructionNode* current = list->head; 
    while(current != NULL && current->next != NULL){
        Instruction* next = current->next->instruction;
        if (next->opCode == OP_LABEL && !isLabelUsed(list, next->result)){
            InstructionNode* dead = current->next;
            current->next = dead->next;
            free(dead->instruction);
            free(dead);
            changed = true;
        } else {
            current = current->next;
        }

        if(current->next == NULL){
            list->tail = current;
        }
    }
    return changed; 
}

//Devuelve true si el simbolo es una constante booleana con valor value
static bool isBoolConstant(Symbol* symbol, bool value){
    return symbol != NULL && symbol->kind == CONSTANT
        && symbol->type == TYPE_BOOL && symbol->value.bval == value;
}

static bool simplifyConstantJumps(IntermediateList* list){
    bool changed = false;
    InstructionNode* current = list->head;
    while(current != NULL && current->next != NULL){
        Instruction* next = current->next->instruction;
        bool isIfFalse = next->opCode == OP_JUMP_IF_FALSE;
        bool isIfTrue  = next->opCode == OP_JUMP_IF_TRUE;
        if((isIfFalse && isBoolConstant(next->arg1, false)) ||
           (isIfTrue  && isBoolConstant(next->arg1, true))){
            next->opCode = OP_JUMP; 
            next->arg1 = NULL;
            changed = true; 
            current = current->next;
        } else if((isIfFalse && isBoolConstant(next->arg1, true)) ||
               ( isIfTrue  && isBoolConstant(next->arg1, false))){
            InstructionNode* dead = current->next; 
            current->next = dead->next;
            free(dead->instruction);
            free(dead);
            changed = true;
        } else {
            current = current->next;
        }
    }
    if(current != NULL && current->next == NULL){
        list->tail = current;
    }
    return changed;
}

//Borra los "goto L" que estan seguidos inmediatamente por "L:"
static bool removeJumpsToNext(IntermediateList* list){
    bool changed = false;
    InstructionNode* current = list->head;
    while(current != NULL && current->next != NULL){
        InstructionNode* jump = current->next;   //candidato a borrar
        InstructionNode* after = jump->next;     //la instruccion que le sigue al candidato
        bool isGoto = jump->instruction->opCode == OP_JUMP;
        bool nextIsItsLabel = after != NULL
                              && after->instruction->opCode == OP_LABEL
                              && after->instruction->result == jump->instruction->result;
        if(isGoto && nextIsItsLabel){
            current->next = after;   //desengancho el goto
            free(jump->instruction);
            free(jump);
            changed = true;
        } else {
            current = current->next;
        }
    }
    return changed;
}

bool eliminateDeadCode(IntermediateList* list){
    bool any = false;
    bool changed = true; 
    while (changed){
        changed = simplifyConstantJumps(list);
        changed = removeUnreachableCode(list) || changed;
        changed = removeJumpsToNext(list) || changed;
        changed = removeUnusedLabels(list) || changed;
        any = any || changed;
    }
    return any;
}