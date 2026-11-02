#!/usr/bin/env bash
# Compila un programa .ctds con ctds_parser y con gcc, ejecuta los dos y compara el resultado.
#
# Uso: fuzzing/fuzz/compare.sh programa.ctds
#
# Imprime una de estas lineas:
#   OK <valor>                    los dos devuelven lo mismo
#   RECHAZADO                     ctds_parser no acepto el programa
#   ERROR_ASM                     el output.s de ctds no ensambla
#   ERROR_GCC                     gcc no compila el .c (problema del programa, no de ctds)
#   CRASH_CTDS                    bug: el programa de ctds crashea y el de gcc no
#   CRASH_GCC                     el programa de gcc crashea y el de ctds no
#   CRASH_AMBOS                   crashean los dos: casi seguro el programa tiene algo indefinido
#   DIFERENCIA ctds=X gcc=Y       bug: los resultados no coinciden

FUZZ_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$FUZZ_DIR/../.." && pwd)"
COMPILER="$ROOT/ctds_parser"
HARNESS="$FUZZ_DIR/manual/harness.c"

# Validar que el script reciba exactamente un unico argumento
if [ $# -ne 1 ]; then
    echo "Uso: $0 programa.ctds" >&2
    exit 2
fi
PROGRAM="$(realpath "$1")"

# Cada caso corre en una carpeta nueva: si ctds_parser falla no hay output.s
# viejo que se pueda usar por error, y varios casos no se pisan entre si
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
cd "$WORK"

# 1. ctds: el codigo de salida siempre es 0, asi que se mira el mensaje de stdout.
# Se guarda en un archivo en vez de usar un pipe: grep -q cierra el pipe apenas
# encuentra el mensaje y ctds_parser podria morir (SIGPIPE) antes de escribir output.s
"$COMPILER" "$PROGRAM" > ctds_stdout.txt 2> ctds_stderr.txt
if ! grep -q "Programa aceptado correctamente." ctds_stdout.txt; then
    echo "RECHAZADO"
    exit 1
fi

# 2. Renombrar main para linkear con el harness
sed 's/\bmain\b/ctds_main/g' output.s > ctds.s
if ! gcc ctds.s "$HARNESS" -o prog_ctds 2> asm_stderr.txt; then
    echo "ERROR_ASM"
    exit 1
fi

# 3. Oraculo: prelude + el .ctds sin cambios
{
    printf 'typedef int boolean;\n#define true 1\n#define false 0\n#define main ctds_main\n'
    cat "$PROGRAM"
} > prog.c
# -fwrapv: el overflow de int da la vuelta (como addl/imull en ctds) en vez de ser UB
if ! gcc -Wall -std=gnu17 -fwrapv prog.c "$HARNESS" -o prog_gcc 2> gcc_stderr.txt; then
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
    echo "CRASH_CTDS ctds=$CTDS_STATUS gcc_salida=$GCC_OUT"
    exit 1
elif [ $GCC_STATUS -ne 0 ]; then
    echo "CRASH_GCC gcc=$GCC_STATUS ctds_salida=$CTDS_OUT"
    exit 1
fi

if [ "$CTDS_OUT" = "$GCC_OUT" ]; then
    echo "OK $CTDS_OUT"
else
    echo "DIFERENCIA ctds=$CTDS_OUT gcc=$GCC_OUT"
    exit 1
fi
