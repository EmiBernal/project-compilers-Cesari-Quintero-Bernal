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
gcc lex.yy.c parser.tab.c ast.c semantic.c symbolTable.c intermediateCode.c assemblyGenerator.c cli.c -o ctds_parser
```

`flex` y `bison` solo hace falta volver a correrlos si cambiás `lexer.l` o `parser.y`. Si solo tocaste archivos `.c`, alcanza con el `gcc`.

## Como usar el compilador?
```bash
./ctds_parser [opciones] nombreArchivo.ctds
```

El archivo tiene que terminar en `.ctds` y no puede empezar con `-`. Las opciones pueden ir antes o después del archivo.

| Opción | Qué hace |
|--------|----------|
| `-debug` | Muestra el AST, el código intermedio y los mensajes de cada etapa |
| `-target <etapa>` | Compila hasta la etapa dada: `scan`, `parse`, `codinter` o `assembly` (por ahora solo funciona `assembly`) |
| `-o <salida>` | Le cambia el nombre al ejecutable (por defecto `nombreArchivo.out`) |
| `-opt all` | Activa todas las optimizaciones *(en desarrollo)* |

Si la compilación sale bien **no se imprime nada** (salvo con `-debug`) y se generan `nombreArchivo.s` y el ejecutable `nombreArchivo.out` **en la carpeta desde donde se ejecuta el compilador** (como gcc), no en la carpeta del `.ctds`. Los errores se muestran siempre.

Para saber si compiló, mirá el código de salida con `echo $?`: `0` es que salió bien y `1` que hubo algún error (de sintaxis, semántico, o en las opciones).

Ejemplos:
```bash
./ctds_parser test/codigoObjeto/globalSimple.ctds; echo $?          # no muestra nada y devuelve 0
./ctds_parser -debug test/codigoObjeto/globalSimple.ctds            # muestra el AST y el codigo intermedio
./ctds_parser -dinosaurio test/codigoObjeto/globalSimple.ctds       # error: opcion desconocida
./ctds_parser prog.c                                                # error: la extension tiene que ser .ctds
```

## Como correr los tests?
Los tests están en `test/`, separados en carpetas: `sintaxis`, `semantico`, `intermedio` y `codigoObjeto`.

Un test solo (con `-debug` para ver qué hizo):
```bash
./ctds_parser -debug test/intermedio/ir_if.ctds
```

Todos los de una carpeta, mostrando los errores de cada uno:
```bash
for f in test/semantico/*.ctds; do echo "== $f"; ./ctds_parser "$f"; done
```

Todos los tests, mostrando solo si cada uno compiló o no:
```bash
for f in test/*/*.ctds; do
    if ./ctds_parser "$f" 2> /dev/null; then echo "OK     $f"; else echo "ERROR  $f"; fi
done
```

## Codigo objeto y ejecutable
El compilador genera el assembly (`.s`) y, si no se pasa `-target`, llama a gcc para armar el ejecutable linkeándolo con `runtime.c`. Por eso **hay que correrlo desde la raíz del repo** (ahí está `runtime.c`).

```bash
./ctds_parser test/codigoObjeto/recursionFact.ctds     # genera recursionFact.s y recursionFact.out
./recursionFact.out; echo $?                           # 120
```

Con `-o` el ejecutable se llama como quieras:
```bash
./ctds_parser -o hola test/codigoObjeto/globalSimple.ctds
./hola; echo $?                                        # 8
```

Con `-target assembly` se genera solo el `.s`, sin ejecutable.

- El `./` hace falta para ejecutar un programa de la carpeta actual.
- `echo $?` muestra lo que devolvió `main`, pero **módulo 256** (por ejemplo, `-13` se ve como `243` y `350` como `94`). Cada test de `test/codigoObjeto` tiene en la primera línea un comentario con el resultado esperado.

### Imprimir valores: `printInt`
El ejecutable **no muestra nada en pantalla** por sí solo: lo que devuelve `main` queda como código de salida y solo se ve con `echo $?`. Para imprimir algo, el programa tiene que usar `printInt`, que está en `runtime.c` y hay que declarar como `extern` al principio del `.ctds`:

```c
extern void printInt(int x);

int main() {
    printInt(42);      // imprime 42 en una linea
    return 0;
}
```

`test/codigoObjeto/printFact.ctds` es un ejemplo:
```bash
./ctds_parser test/codigoObjeto/printFact.ctds
./printFact.out                                        # 720
```

`runtime.c` también tiene `getInt` (lee un entero del teclado), que se declara como `extern int getInt();`.

### Todos los tests de código objeto
```bash
for f in test/codigoObjeto/*.ctds; do
    ./ctds_parser "$f" && ./"$(basename "$f" .ctds).out" > /dev/null
    echo "$f -> $?"
done
```

`$(basename "$f" .ctds)` saca la carpeta y la extensión: de `test/codigoObjeto/x.ctds` queda `x`, y el ejecutable que generó el compilador es `x.out`.
