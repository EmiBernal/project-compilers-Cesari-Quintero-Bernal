// threeAddressCode.h

#ifndef THREE_ADDRESS_CODE_H
#define THREE_ADDRESS_CODE_H

#include "ast.h"

/* Genera codigo de 3 direcciones (TAC) a partir del AST e imprime
 * cada instruccion generada. Devuelve el "lugar" (nombre de variable,
 * constante o temporal) donde quedo almacenado el resultado del nodo,
 * para que el nodo padre lo pueda usar como operando.
 */
char* generateCode(ASTNode* node);

#endif
