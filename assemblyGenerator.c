#include "assemblyGenerator.h"
#define INT_SIZE_BYTES 4
#define STACK_ALIGNMENT_BYTES 16

static bool needsStackSlot(Symbol* symbol);
static void assignStackSlot(Symbol* symbol, int* frameSize);
static void assignInstructionSlots(Instruction* instruction, int* frameSize);
static int assignMethodSlots(InstructionNode* beginNode);
static void assignParameterSlots(Symbol* method, int* frameSize);
static void writePrologue(FILE* out, Symbol* method, int frameSize);
static int alignFrameSize(int frameSize);
static void writeEpilogue(FILE* out);

static bool needsStackSlot(Symbol* symbol){
    if((symbol->kind == VAR && symbol->global == false) || symbol->kind == PARAM || symbol->kind == TEMPORARY){
       return true;
    }
    return false;
}

static void assignStackSlot(Symbol* symbol, int* frameSize){
    if(symbol->offset != 0 || !needsStackSlot(symbol)){
        return; 
    }
    *frameSize += INT_SIZE_BYTES; 
    symbol->offset = -*frameSize; 
}

static void assignInstructionSlots(Instruction* instruction, int* frameSize){
    if(instruction->arg1 != NULL){
        assignStackSlot(instruction->arg1, frameSize);
    }
    if(instruction->arg2 != NULL) {
        assignStackSlot(instruction->arg2, frameSize);
    }
    if(instruction->result != NULL){
        assignStackSlot(instruction->result, frameSize);
    }
}

static int assignMethodSlots(InstructionNode* beginNode){
    int frameSize = 0;
    InstructionNode* aux = beginNode;
    Symbol* method = beginNode->instruction->result;
    assignParameterSlots(method, &frameSize);
    
    while(aux != NULL && aux->instruction->opCode != OP_END_FUNC){
        assignInstructionSlots(aux->instruction, &frameSize);
        aux = aux->next;
    }

    return frameSize;
}

static void assignParameterSlots(Symbol* method, int* frameSize){
    ASTNode* parameters = method->params;
    while(parameters != NULL){
        Symbol* param = parameters->left->info;   // el símbolo del parámetro
        assignStackSlot(param, frameSize);
        parameters = parameters->right;           // el siguiente
    }
}

static int alignFrameSize(int frameSize){
   return ((frameSize + STACK_ALIGNMENT_BYTES - 1) / STACK_ALIGNMENT_BYTES) * STACK_ALIGNMENT_BYTES;
}

static void writePrologue(FILE* out, Symbol* method, int frameSize){
    fprintf(out, "    .text\n");
    fprintf(out, "    .globl %s\n", method->name);
    fprintf(out, "%s:\n", method->name);
    fprintf(out, "    pushq %%rbp\n");
    fprintf(out, "    movq %%rsp, %%rbp\n");
    fprintf(out, "    subq $%d, %%rsp\n", alignFrameSize(frameSize));
}

static void writeEpilogue(FILE* out){
    fprintf(out, "    leave\n");
    fprintf(out, "    ret\n");
}

void writeOperand(FILE* out, Symbol* symbol){
    if (symbol->kind == CONSTANT) {

        if (symbol->type == TYPE_BOOL){
            fprintf(out, "$%d", symbol->value.bval ? 1 : 0);            
        }else{
            fprintf(out, "$%d", symbol->value.ival);
        }
        
    } else if (symbol->kind == VAR && symbol->global) {
        fprintf(out, "%s(%%rip)", symbol->name);
    } else {
        fprintf(out, "%d(%%rbp)", symbol->offset);
    }
}

static void loadOperand(FILE* out, Symbol* symbol, const char* reg){
    fprintf(out, "   movl ");
    writeOperand(out, symbol);
    fprintf(out, ", %s\n", reg);
}

static void storeResult(FILE* out, Symbol* symbol, const char* reg){
    fprintf(out, "   movl %s, ", reg);
    writeOperand(out, symbol);
    fprintf(out, "\n");
}

void generateAssembly(IntermediateList* list, FILE* out){
    InstructionNode* node = list->head;
    while(node != NULL){
        if(node->instruction->opCode == OP_BEGIN_FUNC){
            int frameSize = assignMethodSlots(node);
            writePrologue(out, node->instruction->result, frameSize);
        } else if(node->instruction->opCode == OP_END_FUNC) {
            writeEpilogue(out);
        }
        node = node->next;
    }
}




