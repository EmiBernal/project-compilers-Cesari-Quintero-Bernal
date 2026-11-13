# Fuzzer - comandos

Todos los comandos se corren desde la raiz del repo.

## Compilar el compilador

```bash
flex lexer.l
bison -d parser.y
gcc -Wall lex.yy.c parser.tab.c ast.c semantic.c symbolTable.c intermediateCode.c assemblyGenerator.c cli.c optimizer.c deadCode.c -o ctds_parser
```

## Harness

`echo $?` solo muestra el resultado modulo 256 (350 se ve como 94), porque el codigo de
salida de un proceso es de 8 bits. Para comparar los 32 bits, el `main` del programa se
renombra a `ctds_main` y se linkea con `harness.c`, que tiene su propio `main`, llama a
`ctds_main()` e imprime lo que devuelve (el valor llega en `%eax`, por la System V ABI).


El mismo harness se usa para los dos lados, asi cualquier diferencia viene del compilador.

## Compilar y ejecutar con ctds

```bash
./ctds_parser -target assembly fuzzing/fuzz/manual/bueno.ctds
sed 's/\bmain\b/ctds_main/g' bueno.ass > ctds.s
gcc ctds.s fuzzing/fuzz/manual/harness.c runtime.c -o prog_ctds
./prog_ctds
```

- `-target assembly` hace que el compilador genere solo `bueno.ass` (en la carpeta desde donde se ejecuta) y no arme su propio ejecutable: el ejecutable lo armamos nosotros con el harness.
- El compilador no imprime nada si sale bien: para saber si acepto el programa se mira el codigo de salida (`0` = aceptado).
- La copia se guarda como `ctds.s` porque gcc reconoce el assembly por la extension `.s` (con `.ass` habria que pasarle `-x assembler`).
- `runtime.c` tiene `printInt` y `getInt`: se linkea siempre, asi tambien se pueden probar programas que imprimen.
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
gcc -Wall -std=gnu17 -fwrapv fuzzing/fuzz/manual/bueno.c fuzzing/fuzz/manual/harness.c runtime.c -o prog_gcc
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
| `OK <salida>` | ctds y gcc dan la misma salida: lo que imprimio `printInt` (si lo usa) y, al final, lo que devolvio `main` |
| `RECHAZADO` | `ctds_parser` no acepto el programa (devolvio un codigo de salida distinto de 0) |
| `ERROR_ASM` | el `.ass` de ctds no ensambla: bug del backend |
| `ERROR_LINK` | el `.ass` ensambla pero no linkea con el harness y `runtime.c` (por ejemplo, llama a un `extern` que no existe) |
| `ERROR_GCC` | gcc no compila el `.c`: el programa no es C valido (con el generador, bug del generador) |
| `CRASH_CTDS` | el programa de ctds crashea y el de gcc no: bug de ctds |
| `CRASH_GCC` | el programa de gcc crashea y el de ctds no |
| `CRASH_AMBOS` | crashean los dos: casi seguro el programa generado tiene algo indefinido (por ejemplo dividir por cero) |
| `DIFERENCIA ctds=X gcc=Y` | las salidas no coinciden: bug en el codigo generado |

Los `CRASH_*` muestran el codigo de salida del programa. Si es mayor a 128, lo mato una
senal: el numero de senal es el codigo menos 128 (136 = 128 + 8 = SIGFPE, error aritmetico;
139 = 128 + 11 = SIGSEGV, acceso invalido a memoria).

Devuelve 0 si dio `OK` y 1 en cualquier otro caso.

### Como funciona
A partir del mismo .ctds que le pasa arma el codigo .c ya que escribe el preludio, copia el .ctds tal cual como esta y guarda todo como prog.c dentro de una carpeta temporal

Luego compila ese prog.c con gcc, el harness y `runtime.c`. Del lado de ctds, ensambla y linkea por separado (`gcc -c` y despues el linkeo), asi se sabe si fallo el assembly (`ERROR_ASM`) o el linkeo (`ERROR_LINK`). Cuando el script termina, la carpeta temporal se borra y el .c desaparece con ella.

## Flags de gcc que fuimos agregando

| Flag | Para que |
|------|----------|
| `-std=gnu17` | Fija el estandar de C para que el oraculo no cambie al actualizar gcc (gcc 15+ usa C23 por defecto, donde por ejemplo `int f()` significa "sin parametros") |
| `-Wall` | Muestra warnings, que pueden indicar que el programa generado tiene algo raro |
| `-fwrapv` | El overflow de `int` da la vuelta (como `addl`/`imull` en el assembly de ctds) en lugar de ser comportamiento indefinido |


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
gcc -Wall lex.yy.c parser.tab.c ast.c semantic.c symbolTable.c intermediateCode.c assemblyGenerator.c cli.c optimizer.c deadCode.c -o ctds_parser
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

```bash
fuzzing/fuzz/fuzz.sh 1000 1 4      # 1000 casos, desde la semilla 1, profundidad 4
```

Corrida de 1000 casos (semillas 1 a 1000, profundidad 4): **1000 OK, 0 fallos**, en unos 4 minutos.

Los programas que fallan se guardan en `fuzzing/fuzz/bugs/` con el tipo de fallo y la semilla en el nombre (por ejemplo `DIFERENCIA-seed42.ctds`). La carpeta esta en `.gitignore`: es el resultado de cada corrida, no se sube.

## Probar las optimizaciones: `CTDS_OPTS`

Por defecto `compare.sh` llama a `ctds_parser` sin opciones. Para probar una optimizacion se le pasan las opciones en la variable de entorno `CTDS_OPTS`. Solo las usa ctds: gcc (el oraculo) compila siempre igual, asi que si la optimizacion cambia el resultado aparece como `DIFERENCIA`.

```bash
CTDS_OPTS="-opt constantes" fuzzing/fuzz/compare.sh test/optimizaciones/constantes/constProp1.ctds
CTDS_OPTS="-opt all" fuzzing/fuzz/fuzz.sh 300 1 6      # fuzz.sh se la pasa a compare.sh sin cambios
```

Para correr todos los tests contra gcc con todas las optimizaciones:

```bash
for f in test/optimizaciones/*/*.ctds test/codigoObjeto/*.ctds; do
    printf '%-55s ' "$f"; CTDS_OPTS="-opt all" fuzzing/fuzz/compare.sh "$f"
done
```

`constFold5` y `divisionPorCeroEjecucion` dan `CRASH_AMBOS`: dividen por cero a proposito.

El generador solo arma expresiones con numeros, `+`, `-`, `*` y parentesis, asi que con optimizaciones prueba el plegado de esas operaciones. Las reglas de la propagacion (variables, labels, llamadas y globales) las cubren los tests de `test/optimizaciones/constantes/`.

## Compartimientos indefinidos
El testing diferencial supone que **gcc da la respuesta correcta**. Pero el estandar de C dice que algunas operaciones tienen *comportamiento indefinido*

Por lo tanto, el generador solo puede producir programas sin comportamientos indefinidos

### Division por cero
Cuando en gcc trabajamos con divisiones por cero, devuelve 136. Ese numero es el codigo de salida que devuelve el compilador cuando el programa se cierra de forma abruta. 

136 = 128 + 8 (señal Floating Point Exception)
