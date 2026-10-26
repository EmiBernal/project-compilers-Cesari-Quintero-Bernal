#ifndef ASSEMBLY_GENERATOR_H
#define ASSEMBLY_GENERATOR_H
#include <stdio.h>
#include "ast.h"
#include "intermediateCode.h"

void generateAssembly(IntermediateList* list, FILE* out);
void writeOperand(FILE* out, Symbol* symbol);
void translateInstruction(FILE* out, Instruction* instruction);


#endif