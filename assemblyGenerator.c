#include "assemblyGenerator.h"
#include <stdlib.h>
#define INT_SIZE_BYTES 4 //Cada valor ocupa 4 bytes
#define STACK_ALIGNMENT_BYTES 16
#define MAX_REGISTER_PARAMS 6
#define STACK_ARG_SIZE_BYTES 8 
#define MAX_CALL_ARGS 256
#define FIRST_STACK_ARG_OFFSET 16

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
                Symbol** aux = realloc(globals, capacity * sizeof(Symbol*));
                if(aux == NULL){
                    fprintf(stderr, "Error: no hay memoria para las variables globales\n");
                    free(globals);
                    return;
                }
                globals = aux;
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
    while(parameters != NULL){
        Symbol* param = parameters->left->info;
        if(index < MAX_REGISTER_PARAMS){
            //Los primeros 6 llegan en registros
            fprintf(out, "    movl %s, %d(%%rbp)\n", PARAM_REGISTERS[index], param->offset); //param->offset ya tiene valor pq assignMethodSlots corrio antes
        }else{
            //Del 7mo en adelante estan en la pila de quien llamo: 16(%rbp), 24(%rbp), 32(%rbp)...
            int stackOffset = FIRST_STACK_ARG_OFFSET + (index - MAX_REGISTER_PARAMS) * STACK_ARG_SIZE_BYTES;
            //No existe mov de memoria a memoria, paso por %eax
            fprintf(out, "    movl %d(%%rbp), %%eax\n", stackOffset);
            fprintf(out, "    movl %%eax, %d(%%rbp)\n", param->offset);
        }
        parameters = parameters->right;
        index++;
    }
}

static void writePrologue(FILE* out, Symbol* method, int frameSize){
    fprintf(out, "    .text\n");                       //Lo que sigue va en la seccion de codigo
    fprintf(out, "    .globl %s\n", method->name);     //el simbolo f es visible para el linker
    fprintf(out, "%s:\n", method->name);               //etiqueta que marca el inicio de la funcion
    fprintf(out, "    enter $%d, $0\n", alignFrameSize(frameSize)); // coloca el valor actual del %rbp, almacena el valor actual de %rsp en %rbp y reduce el %rsp para dejar espacio a x variables locales
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

//Escribe una llamada completa: argumentos en registros y en la pila, call, limpieza y retorno
static void writeCall(FILE* out, Instruction* instruction, Symbol** args, int argCount){
    //Del 7mo argumento en adelante van a la pila
    int stackArgs = 0;
    if(argCount > MAX_REGISTER_PARAMS){
        stackArgs = argCount - MAX_REGISTER_PARAMS;
    }
    //Bytes que ocupan los argumentos en la pila, y el total redondeado a 16 para que %rsp quede alineado en el call
    int stackBytes = stackArgs * STACK_ARG_SIZE_BYTES;
    int relleno = alignFrameSize(stackBytes);
    //Si sobra, es relleno: lo reservo antes de los push
    if(relleno > stackBytes){
        fprintf(out, "    subq $%d, %%rsp\n", relleno - stackBytes);
    }
    //Empujo los de la pila en orden inverso, asi el 7mo queda mas cerca (en 16(%rbp) de quien es llamado)
    for(int i = argCount - 1; i >= MAX_REGISTER_PARAMS; i--){
        loadOperand(out, args[i], "%eax");
        fprintf(out, "    pushq %%rax\n"); //push necesita el registro de 64 bits
    }
    //Los primeros 6 van en registros
    for(int i = 0; i < argCount && i < MAX_REGISTER_PARAMS; i++){
        loadOperand(out, args[i], PARAM_REGISTERS[i]);
    }
    fprintf(out, "    movl $0, %%eax\n");
    fprintf(out, "    call %s\n", instruction->arg1->name);
    //Quien llama saca de la pila todo lo que bajo (argumentos + relleno)
    if(relleno > 0){
        fprintf(out, "    addq $%d, %%rsp\n", relleno);
    }
    //Si el metodo devuelve algo, el valor queda en %eax
    if(instruction->result != NULL){
        storeResult(out, "%eax", instruction->result);
    }
}

void translateInstruction(FILE* out, Instruction* instruction){
    static Symbol* callArgs[MAX_CALL_ARGS]; //Argumentos de la llamada actual, en orden
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
            fprintf(out, "    movl $0, %%eax\n");
            fprintf(out, "    movl $1, %%ecx\n");
            fprintf(out, "    cmovl %%ecx, %%eax\n");
            storeResult(out, "%eax", instruction->result);
            break;

        case OP_GT:
            loadOperand(out, instruction->arg1, "%eax");
            loadOperand(out, instruction->arg2, "%ecx");
            fprintf(out, "    cmpl %%ecx, %%eax\n");
            fprintf(out, "    movl $0, %%eax\n");
            fprintf(out, "    movl $1, %%ecx\n");
            fprintf(out, "    cmovg %%ecx, %%eax\n");
            storeResult(out, "%eax", instruction->result);
            break;  

        case OP_EQ:
            loadOperand(out, instruction->arg1, "%eax");
            loadOperand(out, instruction->arg2, "%ecx");
            fprintf(out, "    cmpl %%ecx, %%eax\n");
            fprintf(out, "    movl $0, %%eax\n");
            fprintf(out, "    movl $1, %%ecx\n");
            fprintf(out, "    cmove %%ecx, %%eax\n");
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
            if(paramCount >= MAX_CALL_ARGS){
                fprintf(stderr, "Error: mas de %d argumentos no esta soportado\n", MAX_CALL_ARGS);
            }else{
                callArgs[paramCount] = instruction->arg1;
                paramCount++;
            }
            break;

        case OP_CALL:
            writeCall(out, instruction, callArgs, paramCount);
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




