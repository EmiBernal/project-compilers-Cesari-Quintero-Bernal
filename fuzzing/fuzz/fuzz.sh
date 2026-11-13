#!/usr/bin/env bash
# Corre el fuzzer: genera programas con gen.py y los compara con compare.sh.
#
# Uso: fuzzing/fuzz/fuzz.sh [cantidad] [semilla_inicial] [profundidad]

FUZZ_DIR="$(cd "$(dirname "$0")" && pwd)"

#{1:-100} significa 'el primer argumento o 100 si no pasaron ninguno'
N="${1:-100}"
START="${2:-1}"
DEPTH="${3:-4}"

#fuzz.sh son 100 casos, semilla 1 y profundidad 4

# Calcular la ultima semilla 

END=$((START + N - 1))

PROG=/tmp/prog.ctds

#Carpeta donde se guardan los programas que fallan 
BUGS="$FUZZ_DIR/bugs"
mkdir -p "$BUGS"

#Contadores para el resumen
OK_COUNT=0
FAIL_COUNT=0

for s in $(seq "$START" "$END"); do

    #Generar el programa de esta semilla
    python3 "$FUZZ_DIR/gen.py" --seed "$s" --depth "$DEPTH" > "$PROG"

    #Compararlo con gcc y guardar lo que imprime compare.sh
    RESULT="$("$FUZZ_DIR/compare.sh" "$PROG")"

    #Guardo el tipo del resultado
    KIND="${RESULT%% *}"

    if [ "$KIND" = "OK" ]; then
        OK_COUNT=$((OK_COUNT + 1))
    else
        FAIL_COUNT=$((FAIL_COUNT + 1))
        #El nombre del archivo dice el tipo de fallo y la semilla
        cp "$PROG" "$BUGS/$KIND-seed$s.ctds"
        #Mostrar el resultado y el programa que fallo
        echo "semilla $s: $RESULT"
        cat "$PROG"
        echo
    fi

done

echo "Casos: $N   OK: $OK_COUNT   Fallos: $FAIL_COUNT   (guardados en $BUGS)"