#include "assemblyGenerator.h"
#include <stdlib.h>
#define INT_SIZE_BYTES 4 //Cada valor ocupa 4 bytes
#define STACK_ALIGNMENT_BYTES 16
#define MAX_REGISTER_PARAMS 6

static const char* PARAM_REGISTERS[MAX_REGISTER_PARAMS] = {
    "%edi", "%esi", "%edx", "%ecx", "%r8d", "%r9d"
};

static bool needsStackSlot(Symbol* symbol);
static void assignStackSlot(Symbol* symbol, int* frameSize);
static void assignInstructionSlots(Instruction* instruction, int* frameSize);
static int assignMethodSlots(InstructionNode* beginNode);
static void assignParameterSlots(Symbol* method, int* frameSize);
static int alignFrameSize(int frameSize);
static void writePrologue(FILE* out, Symbol* method, int frameSize);
static void writeEpilogue(FILE* out);

static bool isGlobalVar(Symbol* symbol){
    return symbol != NULL && symbol->kind == VAR && symbol->global;
}

static bool containsSymbol(Symbol** symbols, int count, Symbol* symbol){
    for(int i = 0; i < count; i++){
        if(symbols[i] == symbol){
            return true;
        }
    }
    return false;
}

static void writeGlobal(FILE* out, Symbol* symbol){
    int value = (symbol->type == TYPE_BOOL)
                ? (symbol->value.bval ? 1 : 0)
                : symbol->value.ival;

    fprintf(out, "    .globl %s\n", symbol->name);
    fprintf(out, "    .align 4\n");
    fprintf(out, "%s:\n", symbol->name);
    fprintf(out, "    .long %d\n", value);
}

static void writeDataSection(FILE* out, IntermediateList* list){
    Symbol** globals = NULL;
    int count = 0;
    int capacity = 0;

    for(InstructionNode* node = list->head; node != NULL; node = node->next){
        Symbol* operands[3] = { node->instruction->arg1, node->instruction->arg2, node->instruction->result };
        //Bucle por cada op
        for(int i = 0; i < 3; i++){
            //Me guardo el simbolo
            Symbol* symbol = operands[i];
            //Si no es global o ya la tengo, la salteo
            if(!isGlobalVar(symbol) || containsSymbol(globals, count, symbol)){
                continue; //Paso al siguiente operando
            }
            //Si no hay lugar, duplico la capacidad
            if(count == capacity){
                //Primero verifico que la capacidad sea mayor a 0
                if(capacity == 0){
                    //Inicializo
                    capacity = 8;
                }else{
                    //Duplico capacidad
                    capacity = capacity * 2;
                }
                globals = realloc(globals, capacity * sizeof(Symbol*));
            }
            globals[count] = symbol;
            count++;
        }
    }
    
    //Solo escribo .data si hay alguna global
    if(count > 0){
        fprintf(out, "    .data\n");
        for(int i = 0; i < count; i++){
            writeGlobal(out, globals[i]);
        }
    }
    free(globals);
}

//Este metodo filtra qué necesita lugar en el stack
static bool needsStackSlot(Symbol* symbol){
    //Si es una variable local o parametro o temporario
    if((symbol->kind == VAR && symbol->global == false) || symbol->kind == PARAM || symbol->kind == TEMPORARY){
       return true;
    }
    return false;
}

//Este metodo asigna el lugar en el stack
static void assignStackSlot(Symbol* symbol, int* frameSize){
    if(symbol->offset != 0 || !needsStackSlot(symbol)){
        return; 
    }
    *frameSize += INT_SIZE_BYTES; //Agranda el frame en 4 Bytes
    symbol->offset = -*frameSize; //Devuelve el offset negativo pq la pila crece hacia abajo 
}

//Recibe instrucciones y utiliza el metodo que asigna el lugar en el stack
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

//Recorre un metodo par 
static int assignMethodSlots(InstructionNode* beginNode){
    int frameSize = 0;
    InstructionNode* aux = beginNode; 
    Symbol* method = beginNode->instruction->result;

    //Primero van los parametros
    assignParameterSlots(method, &frameSize); //Asignar lugar a los parametros
    
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

//Metodo estatico con el cual copio los parametros al stack
static void copyParametersToStack(FILE* out, Symbol* method){
    ASTNode* parameters = method->params;
    int index = 0;
    while(parameters != NULL && index < MAX_REGISTER_PARAMS){
        Symbol* param = parameters->left->info;
        fprintf(out, "    movl %s, %d(%%rbp)\n", PARAM_REGISTERS[index], param->offset); //param->offset ya tiene valor pq assignMethodSlots corrio antes
        parameters = parameters->right;
        index++;
    }
}

static void writePrologue(FILE* out, Symbol* method, int frameSize){
    fprintf(out, "    .text\n");                       //Lo que sigue va en la seccion de codigo
    fprintf(out, "    .globl %s\n", method->name);     //el simbolo f es visible para el linker
    fprintf(out, "%s:\n", method->name);               //etiqueta que marca el inicio de la funcion
    fprintf(out, "    pushq %%rbp\n");                 //guardo el frame pointer de quien me llamo
    fprintf(out, "    movq %%rsp, %%rbp\n");           //mi frame empieza aca
    fprintf(out, "    subq $%d, %%rsp\n", alignFrameSize(frameSize)); 
}

static void writeEpilogue(FILE* out){
    fprintf(out, "    leave\n");
    fprintf(out, "    ret\n");
}

void writeOperand(FILE* out, Symbol* symbol){
    if (symbol->kind == CONSTANT) {

        if (symbol->type == TYPE_BOOL){
            fprintf(out, "$%d", symbol->value.bval ? 1 : 0);       //true = 1, false = 0     
        }else{
            fprintf(out, "$%d", symbol->value.ival); //Es inmediato
        }
        
    } else if (symbol->kind == VAR && symbol->global) { 
        fprintf(out, "%s(%%rip)", symbol->name); //Direcion de la etiqueta 
    } else {
        fprintf(out, "%d(%%rbp)", symbol->offset); //el slot que asignamos
    }
}

static void loadOperand(FILE* out, Symbol* symbol, const char* reg){
    fprintf(out, "    movl ");
    writeOperand(out, symbol);
    fprintf(out, ", %s\n", reg);
}

static void storeResult(FILE* out,const char* reg, Symbol* symbol){
    fprintf(out, "    movl %s, ", reg);
    writeOperand(out, symbol);
    fprintf(out, "\n");
}

void translateInstruction(FILE* out, Instruction* instruction){
    static int paramCount = 0; 
    switch(instruction->opCode){

        case OP_ASSIGN:
            loadOperand(out, instruction->arg1, "%eax");
            storeResult(out, "%eax", instruction->result);
            break;

        case OP_ADD:
            loadOperand(out, instruction->arg1, "%eax");
            loadOperand(out, instruction->arg2, "%ecx");
            fprintf(out, "    addl %%ecx, %%eax\n");
            storeResult(out, "%eax", instruction->result);
            break;

        case OP_SUB:
            loadOperand(out, instruction->arg1, "%eax");
            loadOperand(out, instruction->arg2, "%ecx");
            fprintf(out, "    subl %%ecx, %%eax\n");
            storeResult(out, "%eax", instruction->result);
            break;

        case OP_MUL:
            loadOperand(out, instruction->arg1, "%eax");
            loadOperand(out, instruction->arg2, "%ecx");
            fprintf(out, "    imull %%ecx, %%eax\n");
            storeResult(out, "%eax", instruction->result);
            break;

        case OP_DIV:
            loadOperand(out, instruction->arg1, "%eax");
            loadOperand(out, instruction->arg2, "%ecx");            
            fprintf(out, "    cltd\n");
            fprintf(out, "    idivl %%ecx\n");
            storeResult(out, "%eax", instruction->result);
            break;

        case OP_MOD:
            loadOperand(out, instruction->arg1, "%eax");
            loadOperand(out, instruction->arg2, "%ecx");  
            fprintf(out, "    cltd\n");
            fprintf(out, "    idivl %%ecx\n");
            storeResult(out, "%edx", instruction->result);
            break;
        
        case OP_LT:
            loadOperand(out, instruction->arg1, "%eax");
            loadOperand(out, instruction->arg2, "%ecx");
            fprintf(out, "    cmpl %%ecx, %%eax\n");
            fprintf(out, "    setl %%al\n");
            fprintf(out, "    movzbl %%al, %%eax\n");
            storeResult(out, "%eax", instruction->result);
            break;

        case OP_GT:
            loadOperand(out, instruction->arg1, "%eax");
            loadOperand(out, instruction->arg2, "%ecx");
            fprintf(out, "    cmpl %%ecx, %%eax\n");
            fprintf(out, "    setg %%al\n");
            fprintf(out, "    movzbl %%al, %%eax\n");
            storeResult(out, "%eax", instruction->result);
            break;  

        case OP_EQ:
            loadOperand(out, instruction->arg1, "%eax");
            loadOperand(out, instruction->arg2, "%ecx");
            fprintf(out, "    cmpl %%ecx, %%eax\n");
            fprintf(out, "    sete %%al\n");
            fprintf(out, "    movzbl %%al, %%eax\n");
            storeResult(out, "%eax", instruction->result);
            break;

        case OP_UMINUS:
            loadOperand(out, instruction->arg1, "%eax");
            fprintf(out, "    negl %%eax\n");
            storeResult(out, "%eax", instruction->result);
            break;

        case OP_NEG:
            loadOperand(out, instruction->arg1, "%eax");
            fprintf(out, "    xorl $1, %%eax\n");
            storeResult(out, "%eax", instruction->result);
            break;
            
        case OP_LABEL:
            fprintf(out, ".%s:\n", instruction->result->name);
            break;

        case OP_JUMP:
            fprintf(out, "    jmp .%s\n", instruction->result->name);
            break;

        case OP_JUMP_IF_FALSE:
            loadOperand(out, instruction->arg1, "%eax");
            fprintf(out, "    cmpl $0, %%eax\n");
            fprintf(out, "    je .%s\n", instruction->result->name);
            break;

        case OP_JUMP_IF_TRUE:
            loadOperand(out, instruction->arg1, "%eax");
            fprintf(out, "    cmpl $0, %%eax\n");
            fprintf(out, "    jne .%s\n", instruction->result->name);
            break;

        case OP_PARAM:
            //Si el contador de parametros llego al tope
            if(paramCount >= MAX_REGISTER_PARAMS){
                fprintf(stderr, "Error: mas de %d argumentos no esta soportado\n", MAX_REGISTER_PARAMS);
            }else{
                loadOperand(out, instruction->arg1, PARAM_REGISTERS[paramCount]);
            }
            paramCount++;
            break;

        case OP_CALL:
            fprintf(out, "    call %s\n", instruction->arg1->name);
            if(instruction->result != NULL){
                //Si el metodo devuelve algo
                storeResult(out, "%eax", instruction->result);
            }
            paramCount = 0;
            break;

        case OP_RETURN:
            if(instruction->arg1 != NULL){
                loadOperand(out, instruction->arg1, "%eax");
            }

            writeEpilogue(out);
            break;

        default:
            break;
    }
}

void generateAssembly(IntermediateList* list, FILE* out){
    writeDataSection(out,list); //Para las variables globales
    InstructionNode* node = list->head;
    while(node != NULL){
        if(node->instruction->opCode == OP_BEGIN_FUNC){
            int frameSize = assignMethodSlots(node); //Calcular slots
            writePrologue(out, node->instruction->result, frameSize); //Escribo el prologo
            copyParametersToStack(out, node->instruction->result);
        } else if(node->instruction->opCode == OP_END_FUNC) {
            fprintf(out, "    xorl %%eax, %%eax\n"); //Devuelvo 0 por defecto
            writeEpilogue(out);
        }else {
            translateInstruction(out, node->instruction); //Traducir instrucciones
        }
        node = node->next;
    }

    fprintf(out, "    .section .note.GNU-stack,\"\",@progbits\n");
}




