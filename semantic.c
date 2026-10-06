#include <stdio.h>
#include <stdarg.h>
#include "semantic.h"
#include "symbolTable.h"

static SymbolTable table;
static Symbol* currentMethod = NULL;
static int errors = 0;

//Funciones recursivas para chequear errores
static void checkNode(ASTNode* n);
static DataType checkExpr(ASTNode* n);
static DataType checkCall(ASTNode* n);

static void semanticError(const char* message, int line,...) {
    va_list args;
    va_start(args, line);
    fprintf(stderr, "Semantic error at line %d: ", line);
    vfprintf(stderr, message, args);
    fprintf(stderr, "\n");
    va_end(args);
    errors++;          
}

// declaraciones, sentencias, bloques, listas
static void checkNode(ASTNode* n){
    if(n == NULL) return;
    
    //Por cada tipo
    switch(n->type){
        
        case NODE_SEQ:
            checkNode(n->left);
            checkNode(n->right);
            break;    

        case NODE_DECL:
            case NODE_PARAM: {
                bool newParam = insertSymbol(&table, n->info);
                
                //Si ya fue declarado previamente
                if(!newParam){
                    semanticError("'%s' ya declarado en este bloque", n->line, n->info->name);
                }   
                break;
            }
        case NODE_METHOD:
            n->info->params = n->left;   //guardo los parametros para chequear las llamadas
            bool newMethod = insertSymbol(&table, n->info);

            if(!newMethod){
                semanticError("metodo '%s' ya declarado", n->line, n->info->name);
            }
            currentMethod = n->info;
            enterScope(&table); //Scope de parametros
            checkNode(n->left); //inserta los parametros
            checkNode(n->right); //cuerpo (si es un bloque abre su propio scope)
            exitScope(&table);
            currentMethod = NULL;
            break;

        case NODE_BLOCK:
            enterScope(&table);
            checkNode(n->left); //Declaraciones locales
            checkNode(n->right); //Sentencias
            exitScope(&table);
            break;
        
        case NODE_ASSIGN: {
                //Guardo los tipos de cada expresion
                DataType left = checkExpr(n->left);
                DataType right = checkExpr(n->right);
                //Chequeo que los tipos sean iguales, sino es un error
                if(left != right) semanticError("asignacion con tipos incompatibles", n->line);
                break;
            }

        case NODE_IF:
            case NODE_WHILE:
                if(checkExpr(n->left) != TYPE_BOOL) semanticError("La condicion debe ser de tipo boolean", n->line);
                checkNode(n->middle); //then (En un while esto es NULL)
                checkNode(n->right);  //else  
                break;

        case NODE_RETURN: {
                //Tipo esperado en base al metodo que estamos ejecutando
                DataType expected = currentMethod->type;
                if(n->left == NULL && expected != TYPE_VOID) semanticError("Se esperaba un valor de retorno", n->line);
                else if(n->left != NULL && expected == TYPE_VOID) semanticError("No se esperaba un valor de retorno", n->line);
                else if(n->left != NULL && checkExpr(n->left) != expected) semanticError("Tipo de retorno incompatible", n->line);
                break;
            }
        
        case NODE_CALL:
            checkCall(n);
            break;
        default: break;
    }
} 

//expresiones
static DataType checkExpr(ASTNode* n){
    DataType t = TYPE_VOID;
    if(n == NULL) return t;
    
    switch(n->type){
        case NODE_NUM: t = TYPE_INT; break;
        case NODE_FLOAT: t = TYPE_FLOAT; break;
        case NODE_BOOL: t = TYPE_BOOL; break;

        case NODE_ID: {
            //Obtengo el simbolo de la tabla
            Symbol* s = lookupSymbol(&table, n->info->name);
            //Si no existe es un error de declaracion
            if (s == NULL){
                semanticError("identificador '%s' no declarado", n->line, n->info->name);
            } else if (s->kind == FUNCTION){
                //Un metodo no puede usarse como variable
                semanticError("'%s' es un metodo, no puede usarse como variable", n->line, n->info->name);
            } else {
                //Si existe, la expresion toma el tipo del simbolo
                t = s->type;
            }
            break;
        }

        case NODE_CALL:
            //Llamada usada como expresion: el metodo debe devolver un valor
            t = checkCall(n);
            if (t == TYPE_VOID){
                semanticError("metodo void '%s' usado en expresion", n->line, n->info->name);
            }
            break;

        case NODE_OP: {
            //Obtengo los tipos de mis hijos
            DataType left = checkExpr(n->left);
            DataType right = checkExpr(n->right);
            switch(n->info->kind){

                //Aritmeticos: operandos int o float del mismo tipo, el resultado es de ese tipo
                case OP_ADD:
                case OP_SUB:
                case OP_MUL:
                case OP_DIV:
                    if(left != right || (left != TYPE_INT && left != TYPE_FLOAT)){
                        semanticError("Operacion aritmetica con tipos incompatibles", n->line);
                    } else {
                        t = left;
                    }
                    break;

                //Resto: solo entre enteros
                case OP_MOD:
                    if(left != TYPE_INT || right != TYPE_INT){
                        semanticError("Los operandos de %% deben ser int", n->line);
                    } else {
                        t = TYPE_INT;
                    }
                    break;

                //Logicos: operandos boolean
                case OP_AND:
                case OP_OR:
                    if(left != TYPE_BOOL || right != TYPE_BOOL){
                        semanticError("Operacion logica con tipos incompatibles", n->line);
                    }
                    t = TYPE_BOOL;
                    break;

                //Igualdad: operandos del mismo tipo (cualquiera)
                case OP_EQ:
                    if(left != right){
                        semanticError("== entre tipos distintos", n->line);
                    }
                    t = TYPE_BOOL;
                    break;

                //Relacionales: operandos int o float del mismo tipo
                case OP_LT:
                case OP_GT:
                    if(left != right || (left != TYPE_INT && left != TYPE_FLOAT)){
                        semanticError("Comparacion con tipos incompatibles", n->line);
                    }
                    t = TYPE_BOOL;
                    break;

                default: break;
            }
            break;
        }

        case NODE_UNOP: {
            DataType operand = checkExpr(n->left);
            if(n->info->kind == OP_NEG){
                //Negacion logica: operando boolean
                if(operand != TYPE_BOOL){
                    semanticError("El operando de ! debe ser boolean", n->line);
                }
                t = TYPE_BOOL;
            } else {
                //Menos unario: operando int o float
                if(operand != TYPE_INT && operand != TYPE_FLOAT){
                    semanticError("El operando de - debe ser int o float", n->line);
                } else {
                    t = operand;
                }
            }
            break;
        }

        default: break;
    }
    return t;
}

//Llamadas a funciones
static DataType checkCall(ASTNode* n){
    //Pido el simbolo
    Symbol* f = lookupSymbol(&table, n->info->name);
    //Si no existe o no es un metodo, es un error
    if(f == NULL || f->kind != FUNCTION){
        semanticError("llamada a metodo no declarado '%s'", n->line, n->info->name);
        return TYPE_VOID;
    }
    //Obtengo los parametros y las expresiones para luego comparar
    ASTNode* parameters = f->params;
    ASTNode* expresions = n->left;
    int pos = 1; //posicion del argumento, para el mensaje de error
    //Mientras haya parametros o expresiones comparo sus tipos
    while(parameters != NULL && expresions != NULL){
        //Voy leyendo ambas listas a la vez comparando tipo y expresion
        if(checkExpr(expresions->left) != parameters->left->info->type){
            semanticError("tipo incompatible en el argumento %d de la llamada a '%s'", n->line, pos, f->name);
        }
        parameters = parameters->right;
        expresions = expresions->right;
        pos++;
    }
    if(parameters != NULL || expresions != NULL){
        semanticError("cantidad de argumentos incorrecta en la llamada a '%s'", n->line, f->name);
    }
    return f->type;
} 

int semanticAnalysis(ASTNode* root){
    initSymbolTable(&table);
    enterScope(&table); //Scope global
    checkNode(root->left);
    checkNode(root->right);
    Symbol* main = lookupSymbol(&table, "main");
    if(main == NULL || main->kind != FUNCTION || main->type != TYPE_VOID){
        semanticError("Debe existir un metodo 'main' de tipo void", 0);
    }else if (main->params != NULL){
        semanticError("El metodo 'main' no debe tener parametros", 0);
    }
    exitScope(&table);
    return errors;
}

