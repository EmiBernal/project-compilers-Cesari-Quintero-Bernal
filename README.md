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
gcc -Wall lex.yy.c parser.tab.c ast.c semantic.c symbolTable.c intermediateCode.c assemblyGenerator.c cli.c optimizer.c deadCode.c -o ctds_parser
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
| `-target <etapa>` | Compila hasta la etapa dada y genera solo su archivo (ver tabla abajo) |
| `-o <salida>` | Le cambia el nombre al ejecutable (por defecto `nombreArchivo.out`) |
| `-opt all` | Activa todas las optimizaciones (shifteos, constantes y deadcode) |
| `-opt shifteos` | Activa la optimizacion de shifteos para mult o div por potencias de 2 |
| `-opt constantes` | Activa el plegado de constantes: calcula en compilacion las operaciones con operandos constantes (ej. `3 * 4` → `12`) |
| `-opt deadcode` | Elimina codigo muerto: instrucciones inalcanzables, saltos con condicion constante, `goto` al label siguiente y labels sin uso |

`-opt` se puede repetir para combinar optimizaciones, por ejemplo `-opt constantes -opt deadcode`.

Si la compilación sale bien **no se imprime nada** (salvo con `-debug`) y se generan `nombreArchivo.ass` y el ejecutable `nombreArchivo.out` **en la carpeta desde donde se ejecuta el compilador** (como gcc), no en la carpeta del `.ctds`. Los errores se muestran siempre.

### Etapas (`-target`)
Cada etapa deja su resultado en un archivo con su extensión, en la carpeta actual:

| Etapa | Genera | Contenido |
|-------|--------|-----------|
| `scan` | `nombreArchivo.lex` | Los tokens, uno por línea: `linea NOMBRE texto` (por ejemplo `2 ID g`) |
| `parse` | `nombreArchivo.sint` | El AST, solo con el análisis sintáctico |
| `sem` | `nombreArchivo.sem` | El AST, después del análisis semántico |
| `codinter` | `nombreArchivo.ci` | El código intermedio |
| `assembly` | `nombreArchivo.ass` | El assembly x86-64 |
| *(sin `-target`)* | `nombreArchivo.ass` + `nombreArchivo.out` | El assembly y el ejecutable |

```bash
./ctds_parser -target scan test/codigoObjeto/globalSimple.ctds       # genera globalSimple.lex
./ctds_parser -target parse test/codigoObjeto/globalSimple.ctds      # genera globalSimple.sint
./ctds_parser -target sem test/codigoObjeto/globalSimple.ctds        # genera globalSimple.sem
./ctds_parser -target codinter test/codigoObjeto/globalSimple.ctds   # genera globalSimple.ci
```

- `scan` solo corre el lexer (no parsea): si encuentra un carácter inválido muestra el error y devuelve 1.
- `parse` no corre el análisis semántico: un programa con errores de tipos igual genera su `.sint`.
- `sem` sí lo corre: si el programa tiene errores de tipos, los muestra, no genera el `.sem` y devuelve 1.
- `sem` no está en la tabla de targets de la consigna (sí la extensión `.sem`): es una extensión nuestra para separar el análisis sintáctico del semántico.
- `-o` solo cambia el nombre del ejecutable; los archivos de las etapas siempre se llaman `nombreArchivo.<extension>`.

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
El compilador genera el assembly (`.ass`) y, si no se pasa `-target`, llama a gcc para armar el ejecutable linkeándolo con `runtime.c`. Por eso **hay que correrlo desde la raíz del repo** (ahí está `runtime.c`).

```bash
./ctds_parser test/codigoObjeto/recursionFact.ctds     # genera recursionFact.ass y recursionFact.out
./recursionFact.out; echo $?                           # 120
```

Con `-o` el ejecutable se llama como quieras:
```bash
./ctds_parser -o hola test/codigoObjeto/globalSimple.ctds
./hola; echo $?                                        # 8
```

Con `-target assembly` se genera solo el `.ass`, sin ejecutable.

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


## .sem
No esta en la tabla de targets de la consigna, por lo que es una extension nuestra. Nos parece importante tenerla en cuenta ya que nos permite ir viendo cada una de las etapas que se estan implementando por separado.

El analisis semantico fue una etapa importante del proyecto