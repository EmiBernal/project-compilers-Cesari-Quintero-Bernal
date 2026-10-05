Repositorio para proyecto de la materia de Compiladores de la Universidad Nacional de Rio Cuarto

## Taller-Compiladores-Proyecto

Integrantes
- Cesari Agustin
- Samuel Quintero
- Emiliano Bernal

## Tecnologias y lenguajes utilizados
- Flex
- Bison
- C

## Para compilar
bison -d parser.y          # genera parser.tab.c y parser.tab.h
flex lexer.l               # genera lex.yy.c
gcc -o mi_compilador parser.tab.c lex.yy.c ast.c symbolTable.c interpreter.c Assembly3Direcciones.c