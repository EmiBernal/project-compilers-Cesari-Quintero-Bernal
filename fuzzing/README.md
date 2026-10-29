# Fuzzer - comandos

Todos los comandos se corren desde la raiz del repo.

## Compilar el compilador

```bash
flex lexer.l
bison -d parser.y
gcc lex.yy.c parser.tab.c ast.c semantic.c symbolTable.c intermediateCode.c assemblyGenerator.c -o ctds_parser
```

## Harness

`echo $?` solo muestra el resultado modulo 256 (350 se ve como 94), porque el codigo de
salida de un proceso es de 8 bits. Para comparar los 32 bits, el `main` del programa se
renombra a `ctds_main` y se linkea con `harness.c`, que tiene su propio `main`, llama a
`ctds_main()` e imprime lo que devuelve (el valor llega en `%eax`, por la System V ABI).


El mismo harness se usa para los dos lados, asi cualquier diferencia viene del compilador.

## Compilar y ejecutar con ctds

```bash
./ctds_parser fuzzing/fuzz/manual/bueno.ctds > /dev/null
sed 's/\bmain\b/ctds_main/g' output.s > ctds.s
gcc ctds.s fuzzing/fuzz/manual/harness.c -o prog_ctds
./prog_ctds
```

- `ctds_parser` escribe `output.s` en el directorio desde donde se ejecuta, no al lado del `.ctds`.
- `> /dev/null` oculta el AST y el codigo intermedio que imprime.
- El `sed` renombra `main` en una copia (`ctds.s`): aparece en `.globl main` (simbolo visible
  para el linker) y en `main:` (etiqueta donde empieza la funcion). `\b` es limite de palabra,
  asi un nombre como `mainCount` no se toca.

## Armar el `.c` a partir del `.ctds`

El `.c` es el prelude seguido del `.ctds` sin ningun cambio, asi no hace falta traducir nada:

```bash
{ printf 'typedef int boolean;\n#define true 1\n#define false 0\n#define main ctds_main\n'; cat fuzzing/fuzz/manual/bueno.ctds; } > fuzzing/fuzz/manual/bueno.c
```

Prelude:

```c
typedef int boolean;      // opcion B: boolean es un int de 4 bytes, igual que en assemblyGenerator.c
#define true 1
#define false 0
#define main ctds_main    // el preprocesador renombra main para poder linkear con harness.c
```

Verificar que el cuerpo sea identico al `.ctds` (no tiene que mostrar ninguna diferencia).
El prelude tiene 4 lineas, por eso se empieza desde la 5:

```bash
diff <(tail -n +5 fuzzing/fuzz/manual/bueno.c) fuzzing/fuzz/manual/bueno.ctds
```

## Compilar y ejecutar con gcc (oraculo)

```bash
gcc -Wall -std=gnu17 fuzzing/fuzz/manual/bueno.c fuzzing/fuzz/manual/harness.c -o prog_gcc
./prog_gcc
```

## Comparar

Las dos salidas tienen que ser iguales:

```bash
./prog_ctds
./prog_gcc
```

## Script `compare.sh`

Es un testeo diferencial: compila el mismo programa `.ctds` con tu compilador y con `gcc`, corre ambos y compara. `gcc` es el oraculo.

Si difieren, el bug esta en nuestro compilador

`fuzzing/fuzz/compare.sh` hace todos los pasos anteriores para un `.ctds`:

```bash
fuzzing/fuzz/compare.sh fuzzing/fuzz/manual/bueno.ctds
```

Correrlo con todos los tests de codigo objeto:

```bash
for f in test/codigoObjeto/*.ctds; do printf '%-50s ' "$f"; fuzzing/fuzz/compare.sh "$f"; done
```

Imprime una de estas lineas:

| Salida | Significado |
|--------|-------------|
| `OK <valor>` | ctds y gcc devuelven lo mismo |
| `RECHAZADO` | `ctds_parser` no acepto el programa (no imprimio `Programa aceptado correctamente.`) |
| `ERROR_ASM` | el `output.s` de ctds no ensambla: bug del backend |
| `ERROR_GCC` | gcc no compila el `.c`: el programa no es C valido (con el generador, bug del generador) |
| `CRASH_CTDS` | el programa de ctds crashea y el de gcc no: bug de ctds |
| `CRASH_GCC` | el programa de gcc crashea y el de ctds no |
| `CRASH_AMBOS` | crashean los dos: casi seguro el programa generado tiene algo indefinido (por ejemplo dividir por cero) |
| `DIFERENCIA ctds=X gcc=Y` | los resultados no coinciden: bug en el codigo generado |

Los `CRASH_*` muestran el codigo de salida del programa. Si es mayor a 128, lo mato una
senal: el numero de senal es el codigo menos 128 (136 = 128 + 8 = SIGFPE, error aritmetico;
139 = 128 + 11 = SIGSEGV, acceso invalido a memoria).

Devuelve 0 si dio `OK` y 1 en cualquier otro caso.

### Como funciona
A partir del mismo .ctds que le pasa arma el codigo .c ya que escribe el preludio, copia el .ctds tal cual como esta y guarda todo como prog.c dentro de una carpeta temporal

Luego compila ese prog.c con gcc y el harness. Cuando el script termina, la carpeta temporal se borra y el .c desaparece con ella.

## Flags de gcc que fuimos agregando

| Flag | Para que |
|------|----------|
| `-std=gnu17` | Fija el estandar de C para que el oraculo no cambie al actualizar gcc (gcc 15+ usa C23 por defecto, donde por ejemplo `int f()` significa "sin parametros") |
| `-Wall` | Muestra warnings, que pueden indicar que el programa generado tiene algo raro |


## Gen.py
Es un generador que le pide a bison la gramática de parser.y en XML, se queda solo con las producciones permitidas, calcula la altura mínima de cada no-terminal y, usando una semilla para todas las decisiones aleatorias, genera una expresión respetando la profundidad máxima y la envuelve en un main.

### Semillas
Para generar los numeros random es necesario utilizar una formula matematica que, a partir de un numero inicial (la semilla), produce una secuencia que parece aleatoria pero es **siempre la misma para la misma semilla**

### Las alturas
Cada expresion se llama a si misma. Si se eligen al azar, cada expresion generar dos expresiones nuevas con bastante probabilidad. Por lo tanto, el arbol puede crecer sin control y el programa no terminar nunca

La idea: *la altura mínima*

La altura mínima de un no-terminal es la menor profundidad de árbol con la que se puede terminar de expandirlo, es decir, llegar a solo terminales.

- Un terminal tiene altura 0: ya está terminado.
- Una producción tiene altura 1 + (la altura máxima de sus símbolos), porque el árbol baja un nivel y hay que esperar al hijo más alto.
- Un no-terminal tiene altura igual a la de su producción más baja

Por lo tanto, --depth es la profundidad maxima del arbol, y nos aseguramos que la recursion siempre termina

### Como usar gen.py

Primero se compila el compilador (por si se cambia algo previamente)
```
flex lexer.l
bison -d parser.y
gcc lex.yy.c parser.tab.c ast.c semantic.c symbolTable.c intermediateCode.c assemblyGenerator.c -o ctds_parser
```

Para generar un programa (este es un ejemplo)

```
python3 fuzzing/fuzz/gen.py --seed 42 --depth 8
```

Para generar y comparar contra gcc:

```
python3 fuzzing/fuzz/gen.py --seed 42 > /tmp/prog.ctds
fuzzing/fuzz/compare.sh /tmp/prog.ctds
```

Para ver el caso suelto 

```
python3 fuzzing/fuzz/gen.py --seed 42 | tee /tmp/prog.ctds
fuzzing/fuzz/compare.sh /tmp/prog.ctds
```

## fuzz.sh

Runner para poder juntar la cantidad de casos a probar, las semillas y la profundidad que debe tener el arbol.

Cuando el arbol tiene expresiones que guarda valores chicos, podemos notar que los tests suelen funcionar y devolver ok en su mayoria