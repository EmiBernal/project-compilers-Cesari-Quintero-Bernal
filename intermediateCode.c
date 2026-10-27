#include "intermediateCode.h"
#include <stdlib.h>
#include <stdio.h>

#define MAX_BYTES 16 
static int tempCount = 0; 
static int labelCount = 0;
static Symbol* currentMethod = NULL;

static Symbol** createdSymbols = NULL; //Arreglo de simbolos
static int createdCount = 0;           //Elementos guardados en el arreglo
static int createdCapacity = 0;        

//Registro de los temporales y labels creados
static void registerSymbol(Symbol* symbol){
    
    //Si llego a su limite, tengo que aumentar la capacidad o inicializar
    if(createdCount == createdCapacity){
        int newCapacity;
        //No hay capacidad pq recien empieza
        if(createdCapacity == 0){
            newCapacity = 16;
        }else{
            newCapacity = createdCapacity * 2; //Duplico la capacidad
        }
        Symbol** newArray = (Symbol**) realloc(createdSymbols, newCapacity * sizeof(Symbol*)); //Le doy mas lugar, no lo borro
        if(newArray == NULL){
            return;
        }
        createdSymbols = newArray;
        createdCapacity = newCapacity;
    }
    createdSymbols[createdCount] = symbol;
    createdCount++;
}


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
    if(currentMethod != NULL){
        currentMethod->frameSize += 4;
        symbol->offset = -currentMethod->frameSize;
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
    registerSymbol(symbol);

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
    registerSymbol(symbol);

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
        case OP_ITOF:
            printOperand(instruction->result);
            printf(" = (float) ");
            printOperand(instruction->arg1);
            break;
        case OP_FTOI:
            printOperand(instruction->result);
            printf(" = (int) ");
            printOperand(instruction->arg1);
            break;
        case OP_BEGIN_FUNC:
            printf("begin_func ");
            printOperand(instruction->result);
            break;
        case OP_END_FUNC:
            printf("end_func ");
            printOperand(instruction->result);
            break;
        case OP_PARAM:
            printf("param ");
            printOperand(instruction->arg1); //arg1 pq no es resultado
            break;
        case OP_CALL:
            //Caso metodo no void: t = call f
            if(instruction->result != NULL){
                printOperand(instruction->result);
                printf(" = ");
            }
            //En ambos casos: call f
            printf("call ");
            printOperand(instruction->arg1);
            break;
        case OP_RETURN:
            printf("return");
            //En caso de devolver algo
            if(instruction->arg1 != NULL){
                printf(" ");
                printOperand(instruction->arg1);
            }
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

//Funcion para la conversion 
static Symbol* coerce(Symbol* value, DataType target, IntermediateList* list){
    //Si el valor es nulo o es del mismo valor que el target devuelvo ese mismo valor sin hacer coerce
    if (value == NULL || value->type == target){
        return value;
    }

    SymbolType conversion;
    if(value->type == TYPE_INT && target == TYPE_FLOAT){
        //IntToFloat
        conversion = OP_ITOF;
    }else if(value->type == TYPE_FLOAT && target == TYPE_INT){
        //FloatToInt
        conversion = OP_FTOI;
    } else {
        return value; //No hago nada
    }

    Symbol* temporal = createTempSymbol(target);
    addInstruction(list, conversion, value, NULL, temporal);
    return temporal;
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
            //&& y || con evaluacion de corto circuito: el segundo operando solo se evalua si hace falta
            if(node->info->kind == OP_AND || node->info->kind == OP_OR){
                Symbol* temp = createTempSymbol(TYPE_BOOL);
                Symbol* labelEnd = createLabelSymbol();
                Symbol* left = generateMidRepresentation(node->left, list);
                addInstruction(list, OP_ASSIGN, left, NULL, temp);
                SymbolType jump;
                if(node->info->kind == OP_AND){
                    jump = OP_JUMP_IF_FALSE; //False para &&
                }else{
                    jump = OP_JUMP_IF_TRUE; //True para ||
                } 
                addInstruction(list, jump, temp, NULL, labelEnd);
                //Si se cumple la condicion anterior, saltea el lado derecho

                //Si no cumple, hacemos el lado derecho
                Symbol* right = generateMidRepresentation(node->right, list);
                addInstruction(list, OP_ASSIGN, right, NULL, temp);
                addInstruction(list, OP_LABEL, NULL, NULL, labelEnd);
                return temp;
            }

            Symbol* arg1 = generateMidRepresentation(node->left, list);
            Symbol* arg2 = generateMidRepresentation(node->right, list);

            //Si se mezclan int y float, los dos operandos pasan a float
            if(arg1 != NULL && arg2 != NULL && (arg1->type == TYPE_FLOAT || arg2->type == TYPE_FLOAT)){
                arg1 = coerce(arg1, TYPE_FLOAT, list);
                arg2 = coerce(arg2, TYPE_FLOAT, list);
            }
            
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

            value = coerce(value, dest->type, list);
            
            addInstruction(list, OP_ASSIGN, value, NULL, dest);
            
            return dest;
        }
        // nodos estructurales: no generan instrucciones, solo recorren a sus hijos en orden
        case NODE_PROG:
        case NODE_BLOCK:
        case NODE_SEQ:
        case NODE_DECL:
            generateMidRepresentation(node->left, list);
            generateMidRepresentation(node->middle, list);
            generateMidRepresentation(node->right, list);
            return NULL;

        case NODE_IF: {
            //Genero la condicion ya que devuelve el simbolo donde quedo el resultado
            Symbol* condition = generateMidRepresentation(node->left, list);

            if(node->right == NULL){
                //if sin else
                Symbol* labelEnd = createLabelSymbol();
                addInstruction(list, OP_JUMP_IF_FALSE, condition, NULL, labelEnd);
                generateMidRepresentation(node->middle, list);
                addInstruction(list, OP_LABEL, NULL, NULL, labelEnd);
            } else {
                //if con else
                Symbol* labelElse = createLabelSymbol();
                Symbol* labelEnd = createLabelSymbol();
                addInstruction(list, OP_JUMP_IF_FALSE, condition, NULL, labelElse);
                generateMidRepresentation(node->middle, list);
                //Agrego esta instruccion pq ya vi el then
                addInstruction(list, OP_JUMP, NULL, NULL, labelEnd);
                //Marco la label del else
                addInstruction(list, OP_LABEL, NULL, NULL, labelElse);
                generateMidRepresentation(node->right, list);
                addInstruction(list, OP_LABEL, NULL, NULL, labelEnd);
            }
            return NULL;
        }
            
        case NODE_WHILE: {
            Symbol* labelStart = createLabelSymbol();
            Symbol* labelEnd = createLabelSymbol();
            //Inicio del while
            addInstruction(list, OP_LABEL, NULL, NULL, labelStart);
            Symbol* condition = generateMidRepresentation(node->left, list);
            addInstruction(list, OP_JUMP_IF_FALSE, condition, NULL, labelEnd);
            //Recursivamente va generando 
            generateMidRepresentation(node->right, list);
            addInstruction(list, OP_JUMP, NULL, NULL, labelStart);
            addInstruction(list, OP_LABEL, NULL, NULL, labelEnd);
            return NULL;
        }
            
        // metodos y llamadas
        case NODE_METHOD:
            currentMethod = node->info;
            addInstruction(list, OP_BEGIN_FUNC, NULL, NULL, node->info);
            generateMidRepresentation(node->right, list);
            addInstruction(list, OP_END_FUNC, NULL, NULL, node->info);
            currentMethod = NULL;
            return NULL;

        case NODE_EXTERN: 
            return NULL; // no genera codigo, esta definido en otro archivo
            
        // los parametros no generan instrucciones: se declaran al marcar el inicio del metodo
        case NODE_PARAM:
            return NULL;

        case NODE_CALL: {
            Symbol* method = node->info;
            //Cuento los argumentos para guardar espacio
            int count = 0;
            for(ASTNode* arg = node->left; arg != NULL; arg = arg->right){
                count++;
            }
            Symbol** values = NULL; //Arreglo de punteros
            if(count > 0){
                //Guardo lugar por cada uno
                values = (Symbol**) malloc(count * sizeof(Symbol*));
                if(values == NULL){
                    return NULL;
                }
            }
            ASTNode* arg = node->left; //Parametros reales
            ASTNode* param = method->params; //Parametros formales           
            
            for(int i = 0; i < count; i++){
                Symbol* value = generateMidRepresentation(arg->left, list);
                values[i] = coerce(value, param->left->info->type, list);
                //paso al siguiente
                arg = arg->right;
                param = param->right;
            }

            //Muestro los parametros antes del call
             for(int i = 0; i < count; i++){
                addInstruction(list, OP_PARAM, values[i], NULL, NULL);
            }
            free(values);

            //Si el metodo devuelve algo
            Symbol* result = NULL;
            if(method->type != TYPE_VOID){
                result = createTempSymbol(method->type);
            }
            addInstruction(list, OP_CALL, method, NULL, result);
            return result;
        }
            
        case NODE_RETURN: {
            Symbol* value = NULL;
            if(node->left != NULL){
                value = generateMidRepresentation(node->left, list);
                //Convierto al tipo de retorno del metodo
                value = coerce(value, currentMethod->type, list);
            }
            addInstruction(list, OP_RETURN, value, NULL, NULL);
            return NULL;
        }
    }
    return NULL;
}

//Libera lista de instrucciones, labels, temporales
void freeIntermediateList(IntermediateList* list){
    InstructionNode* node = list->head; //Nodo nuevo que apunta a la cabeza
    
    //Mientras no llegue al final de la lista, libera 
    while(node != NULL){
        InstructionNode* next = node->next;
        free(node->instruction);
        free(node);
        node = next;
    }

    //Libera las cabezas
    list->head = NULL;
    list->tail = NULL;

    //Libero la lista de temporales y labels
    for(int i = 0; i < createdCount; i++){
        free(createdSymbols[i]->name);
        free(createdSymbols[i]);
    }
    free(createdSymbols);
    createdSymbols = NULL;
    createdCapacity = 0;
    createdCount = 0;
}