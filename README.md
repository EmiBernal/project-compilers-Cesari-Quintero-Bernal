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
gcc lex.yy.c parser.tab.c ast.c semantic.c symbolTable.c intermediateCode.c assemblyGenerator.c -o ctds_parser
```

`flex` y `bison` solo hace falta volver a correrlos si cambiás `lexer.l` o `parser.y`. Si solo tocaste archivos `.c`, alcanza con el `gcc`.

## Como correr los tests?
Los tests están en `test/`, separados en carpetas: `sintaxis`, `semantico`, `intermedio` y `codigoObjeto`.

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

## Codigo objeto (assembly x86-64)
Cada vez que se corre `ctds_parser`, además de mostrar el código intermedio, genera el archivo `output.s` (en el directorio desde donde se ejecuta). Para ensamblarlo, ejecutarlo y ver lo que devuelve `main`:
```bash
./ctds_parser test/codigoObjeto/recursionFact.ctds
gcc output.s -o prog && ./prog; echo $?
```

`echo $?` muestra el valor de retorno de `main` módulo 256 (por ejemplo, `-13` se ve como `243`). Cada test de `test/codigoObjeto` tiene en la primera línea un comentario con el resultado esperado.

Todos los tests de código objeto:
```bash
for f in test/codigoObjeto/*.ctds; do
    ./ctds_parser "$f" > /dev/null && gcc output.s -o prog && ./prog
    echo "$f -> $?"
done
```
