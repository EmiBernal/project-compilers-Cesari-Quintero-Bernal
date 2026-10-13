#ifndef INTERMEDIATE_CODE_H
#define INTERMEDIATE_CODE_H

#include "ast.h"

typedef struct Instruction {
    SymbolType opCode;
    Symbol* arg1;
    Symbol* arg2;
    Symbol* result;
} Instruction;

typedef struct InstructionNode {
    Instruction* instruction;
    struct InstructionNode* next;
} InstructionNode;

typedef struct IntermediateList {
    InstructionNode* head;
    InstructionNode* tail;
} IntermediateList;


void initIntermediateList(IntermediateList* list);
void addInstruction(IntermediateList* list, SymbolType opCode, Symbol* arg1, Symbol* arg2, Symbol* result);
Symbol* createTempSymbol(DataType type);
Symbol* createLabelSymbol(void);
const char* opCodeToString(SymbolType opCode);
void printIntermediateList(IntermediateList* list);
Symbol* generateMidRepresentation(ASTNode* node, IntermediateList* list);

#endif // INTERMEDIATE_CODE_H