#ifndef ASSEMBLY_GENERATOR_H
#define ASSEMBLY_GENERATOR_H
#include <stdio.h>
#include "ast.h"
#include "intermediateCode.h"

void generateAssembly(IntermediateList* list, FILE* out);
void writeOperand(FILE* out, Symbol* symbol);
void translateInstruction(FILE* out, Instruction* instruction);
static bool needsStackSlot(Symbol* symbol);
static void assignStackSlot(Symbol* symbol, int* frameSize);
static void assignInstructionSlots(Instruction* instruction, int* frameSize);
static int assignMethodSlots(InstructionNode* beginNode);
static void assignParameterSlots(Symbol* method, int* frameSize);
static void writePrologue(FILE* out, Symbol* method, int frameSize);
static int alignFrameSize(int frameSize);
static void writeEpilogue(FILE* out);

#endif