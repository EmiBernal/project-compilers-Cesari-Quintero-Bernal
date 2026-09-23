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

## Como correr un programa?
- flex lexer.l
- bison -d parser.y
- gcc lex.yy.c parser.tab.c -o ctds_parser -lfl
- ./ctds_parser test/nombreDelPrograma.ctds (por ahora es la única opción)


