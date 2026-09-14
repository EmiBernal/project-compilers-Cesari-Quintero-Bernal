#include <stdio.h>
#include <stdlib.h>
#include "Assembly3Direcciones.h"

static int tempCounter = 0;

/* Generador de nombres de temporales: t0, t1, t2... */
static char* newTemp(void) {
    char* name = (char*) malloc(16);
    sprintf(name, "t%d", tempCounter++);
    return name;
}

/* Convierte un valor entero constante a string, para poder
 * devolverlo igual que una variable o un temporal. */
static char* intToStr(int value) {
    char* str = (char*) malloc(16);
    sprintf(str, "%d", value);
    return str;
}

char* generateCode(ASTNode* node) {
    if (node == NULL) return NULL;

    switch (node->type) {

        case NODE_NUM:
            return intToStr(node->info->value);

        case NODE_BOOL:
            return intToStr(node->info->value);

        case NODE_ID:

            return node->info->name;

        case NODE_OP: {
            char* left  = generateCode(node->left);
            char* right = generateCode(node->right);
            char* result = newTemp();

            const char* opStr = (node->info->kind == OP_ADD) ? "+" : "*";
            printf("%s = %s %s %s\n", result, left, opStr, right);

            return result;
        }

        case NODE_ASSIGN: {
            char* value = generateCode(node->left);
            printf("%s = %s\n", node->info->name, value);
            return node->info->name;
        }

        case NODE_DECL:
            /* En TAC no hace falta emitir nada: la variable ya se referencia
             * por nombre directamente en las instrucciones */
            return NULL;

        case NODE_RETURN:
            if (node->left != NULL) {
                char* value = generateCode(node->left);
                printf("return %s\n", value);
            } else {
                printf("return\n");
            }
            return NULL;

        case NODE_SEQ:
            generateCode(node->left);
            generateCode(node->right);
            return NULL;

        case NODE_PROG:
            printf("FUNC main\n");
            generateCode(node->left);   /* declaraciones */
            generateCode(node->right);  /* sentencias */
            printf("END\n");
            return NULL;

        default:
            return NULL;
    }
}
