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

## Como compilar?
```bash
flex lexer.l
bison -d parser.y
gcc lex.yy.c parser.tab.c ast.c semantic.c symbolTable.c intermediateCode.c -o ctds_parser
```

`flex` y `bison` solo hace falta volver a correrlos si cambiás `lexer.l` o `parser.y`. Si solo tocaste archivos `.c`, alcanza con el `gcc`.

## Como correr los tests?
Los tests están en `test/`, separados en carpetas: `sintaxis`, `semantico` e `intermedio`.

Un test solo:
```bash
./ctds_parser test/intermedio/ir_if.ctds
```

Todos los de una carpeta:
```bash
for f in test/semantico/*.ctds; do echo "== $f"; ./ctds_parser "$f"; done
```

Todos los tests, mostrando solo el resultado de cada uno:
```bash
for f in test/*/*.ctds; do echo "== $f"; ./ctds_parser "$f" 2>&1 | grep -E "aceptado|error|Error"; done
```
