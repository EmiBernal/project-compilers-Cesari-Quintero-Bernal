#!/usr/bin/env bash
# Compila un programa .ctds con ctds_parser y con gcc, ejecuta los dos y compara el resultado.
#
# Uso: fuzzing/fuzz/compare.sh programa.ctds
#
# Imprime una de estas lineas:
#   OK <valor>                    los dos devuelven lo mismo
#   RECHAZADO                     ctds_parser no acepto el programa (devolvio un codigo distinto de 0)
#   ERROR_ASM                     el .ass de ctds no ensambla
#   ERROR_LINK                    el .ass ensambla pero no linkea con el harness y runtime.c
#   ERROR_GCC                     gcc no compila el .c (problema del programa, no de ctds)
#   CRASH_CTDS                    bug: el programa de ctds crashea y el de gcc no
#   CRASH_GCC                     el programa de gcc crashea y el de ctds no
#   CRASH_AMBOS                   crashean los dos: casi seguro el programa tiene algo indefinido
#   DIFERENCIA ctds=X gcc=Y       bug: los resultados no coinciden

FUZZ_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$FUZZ_DIR/../.." && pwd)"
COMPILER="$ROOT/ctds_parser"
HARNESS="$FUZZ_DIR/manual/harness.c"
# Funciones extern (printInt, getInt): se linkean de los dos lados para poder fuzzear programas que imprimen
RUNTIME="$ROOT/runtime.c"

# Validar que el script reciba exactamente un unico argumento
if [ $# -ne 1 ]; then
    echo "Uso: $0 programa.ctds" >&2
    exit 2
fi
PROGRAM="$(realpath "$1")"

# Cada caso corre en una carpeta nueva: si ctds_parser falla no hay un .ass
# viejo que se pueda usar por error, y varios casos no se pisan entre si
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
cd "$WORK"

# 1. ctds: -target assembly para que solo genere el .ass (el ejecutable lo armamos
# nosotros con el harness). El compilador devuelve 0 si acepto el programa.
# El .ass se genera en la carpeta actual con el nombre del programa: prog.ctds -> prog.ass
if ! "$COMPILER" -target assembly "$PROGRAM" > ctds_stdout.txt 2> ctds_stderr.txt; then
    echo "RECHAZADO"
    exit 1
fi
ASM="$(basename "$PROGRAM" .ctds).ass"

# 2. Renombrar main para linkear con el harness. Se guarda como .s para que gcc lo
# reconozca como assembly. Ensamblar y linkear van por separado para saber cual fallo
sed 's/\bmain\b/ctds_main/g' "$ASM" > ctds.s
if ! gcc -c ctds.s -o ctds.o 2> asm_stderr.txt; then
    echo "ERROR_ASM"
    exit 1
fi
if ! gcc ctds.o "$HARNESS" "$RUNTIME" -o prog_ctds 2> link_stderr.txt; then
    echo "ERROR_LINK"
    exit 1
fi

# 3. Oraculo: prelude + el .ctds sin cambios
{
    printf 'typedef int boolean;\n#define true 1\n#define false 0\n#define main ctds_main\n'
    cat "$PROGRAM"
} > prog.c
# -fwrapv: el overflow de int da la vuelta (como addl/imull en ctds) en vez de ser UB
if ! gcc -Wall -std=gnu17 -fwrapv prog.c "$HARNESS" "$RUNTIME" -o prog_gcc 2> gcc_stderr.txt; then
    echo "ERROR_GCC"
    exit 1
fi

# 4. Ejecutar los dos y comparar.
# $? es el codigo de salida del ultimo comando: el harness devuelve 0, asi que
# cualquier otro valor es un crash (por ejemplo una division por cero). Si no se
# mira, dos programas que crashean imprimen "" y parece que coinciden.
CTDS_OUT="$(./prog_ctds 2> /dev/null)"
CTDS_STATUS=$?
GCC_OUT="$(./prog_gcc 2> /dev/null)"
GCC_STATUS=$?

if [ $CTDS_STATUS -ne 0 ] && [ $GCC_STATUS -ne 0 ]; then
    echo "CRASH_AMBOS ctds=$CTDS_STATUS gcc=$GCC_STATUS"
    exit 1
elif [ $CTDS_STATUS -ne 0 ]; then
    echo "CRASH_CTDS ctds=$CTDS_STATUS gcc_salida=$(echo $GCC_OUT)"
    exit 1
elif [ $GCC_STATUS -ne 0 ]; then
    echo "CRASH_GCC gcc=$GCC_STATUS ctds_salida=$(echo $CTDS_OUT)"
    exit 1
fi

# Se compara toda la salida: lo que imprimio printInt y, en la ultima linea, lo que
# devolvio main (lo imprime el harness). $(echo $X) junta las lineas en una sola
if [ "$CTDS_OUT" = "$GCC_OUT" ]; then
    echo "OK $(echo $CTDS_OUT)"
else
    echo "DIFERENCIA ctds=$(echo $CTDS_OUT) gcc=$(echo $GCC_OUT)"
    exit 1
fi
