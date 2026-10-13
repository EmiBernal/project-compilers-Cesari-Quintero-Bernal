#include "intermediateCode.h"
#include <stdlib.h>
#include <stdio.h>

#define MAX_BYTES 16 
static int tempCount = 0; 
static int labelCount = 0;

void initIntermediateList(IntermediateList* list){
    list->head = NULL;
    list->tail = NULL;
}

void addInstruction(IntermediateList* list, SymbolType opCode, Symbol* arg1, Symbol* arg2, Symbol* result){
    Instruction* instruction = (Instruction*) malloc(sizeof(Instruction));
    if(instruction == NULL){
        return;
    }
    instruction->opCode = opCode;
    instruction->arg1 = arg1;
    instruction->arg2 = arg2;
    instruction->result = result;



    InstructionNode* node = (InstructionNode*) malloc(sizeof(InstructionNode));
    if(node == NULL){
        free(instruction);
        return;
    }
    node->instruction = instruction;
    node->next = NULL; 

    if(list->head == NULL){
        list->head = node;
        list->tail = node;
    } else {
        list->tail->next = node;
        list->tail = node; 
    }
}

Symbol* createTempSymbol(DataType type){
    Symbol* symbol = (Symbol*) calloc(1, sizeof(Symbol)); // arrancan con valor 0, sin basura;
    if(symbol == NULL){
        return NULL;
    }
    symbol->kind = TEMPORARY; 
    symbol->type = type;

    char* buffer = (char*) malloc(MAX_BYTES);
    if(buffer == NULL){
        free(symbol);
        return NULL;
    }
    snprintf(buffer, MAX_BYTES, "t%d", tempCount);
    tempCount++;
    symbol->name = buffer;

    return symbol;
}

Symbol* createLabelSymbol(void){
    Symbol* symbol = (Symbol*) calloc(1, sizeof(Symbol));
    if(symbol == NULL){
        return NULL;
    }
    symbol->kind = LABEL;

    char* buffer = (char*) malloc(MAX_BYTES);
    if(buffer == NULL){
        free(symbol);
        return NULL;
    }
    snprintf(buffer, MAX_BYTES, "L%d", labelCount);
    labelCount++;
    symbol->name = buffer;

    return symbol;
}

const char* opCodeToString(SymbolType opCode){
    switch(opCode){
        case OP_ADD:      return "+";
        case OP_SUB:      return "-";
        case OP_MUL:      return "*";
        case OP_DIV:      return "/";
        case OP_MOD:      return "%";
        case OP_LT:       return "<";
        case OP_GT:       return ">";
        case OP_EQ:       return "==";
        case OP_AND:      return "&&";
        case OP_OR:       return "||";
        case OP_UMINUS:   return "-";
        case OP_NEG:      return "!";
        default:          return "?";
    }
}

static void printOperand(Symbol* symbol){
    if(symbol == NULL){
        return;
    }
    if(symbol->kind == CONSTANT){
        switch(symbol->type){
            case TYPE_INT:   printf("%d", symbol->value.ival); break;
            case TYPE_FLOAT: printf("%g", symbol->value.fval); break;
            case TYPE_BOOL:  printf("%s", symbol->value.bval ? "true" : "false"); break;
            default: break;
        }
    } else {
        printf("%s", symbol->name);
    }
}

static void printInstruction(Instruction* instruction){
    switch(instruction->opCode){
        case OP_ASSIGN:
            printOperand(instruction->result);
            printf(" = ");
            printOperand(instruction->arg1);
            break;
        case OP_UMINUS:
        case OP_NEG:
            printOperand(instruction->result);
            printf(" = %s", opCodeToString(instruction->opCode));
            printOperand(instruction->arg1);
            break;
        case OP_JUMP:
            printf("goto ");
            printOperand(instruction->result);
            break;
        case OP_JUMP_IF_FALSE:
            printf("if_false ");
            printOperand(instruction->arg1);
            printf(" goto ");
            printOperand(instruction->result);
            break;
        case OP_JUMP_IF_TRUE:
            printf("if_true ");
            printOperand(instruction->arg1);
            printf(" goto ");
            printOperand(instruction->result);
            break;
        case OP_LABEL:
            printOperand(instruction->result);
            printf(":");
            break;
        default:
            printOperand(instruction->result);
            printf(" = ");
            printOperand(instruction->arg1);
            printf(" %s ", opCodeToString(instruction->opCode));
            printOperand(instruction->arg2);
            break;
    }
    printf("\n");
}

void printIntermediateList(IntermediateList* list){
    InstructionNode* current = list->head;
    while(current != NULL){
        printInstruction(current->instruction);
        current = current->next;
    }
    
}

Symbol* generateMidRepresentation(ASTNode* node, IntermediateList* list) {
    if (node == NULL) return NULL;
    switch (node->type) {
        
        case NODE_NUM:
        case NODE_FLOAT:
        case NODE_BOOL:
        case NODE_ID:
            return node->info; //simbolo en info

        //operacion binarias

        case NODE_OP: {
            Symbol* arg1 = generateMidRepresentation(node->left, list);
            Symbol* arg2 = generateMidRepresentation(node->right, list);
            
            Symbol* temp = createTempSymbol(node->evalType);
            addInstruction(list, node->info->kind, arg1, arg2, temp);
            
            return temp;
        }

        //operaciones unarias
        case NODE_UNOP: {
            Symbol* arg1 = generateMidRepresentation(node->left, list);
            
            Symbol* temp = createTempSymbol(node->evalType);
            
            addInstruction(list, node->info->kind, arg1, NULL, temp);
            
            return temp;
        }

        // asignacion

        case NODE_ASSIGN: {

            Symbol* dest = node->left->info;
            
            Symbol* value = generateMidRepresentation(node->right, list);
            
            addInstruction(list, OP_ASSIGN, value, NULL, dest);
            
            return dest;
        }
            // Agrega esto en tu switch temporalmente para que el árbol se recorra
        case NODE_PROG:
        case NODE_METHOD:
        case NODE_BLOCK:
        case NODE_SEQ:
        case NODE_DECL:
            generateMidRepresentation(node->left, list);
            generateMidRepresentation(node->middle, list);
            generateMidRepresentation(node->right, list);
            return NULL;
            
    }
    return NULL;
}