#include "cli.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//Convierte el nombre de la etapa al enum. Devuelve 0 si la etapa existe, -1 si no
static int parseTarget(const char *nombre, Target *target){
    if (strcmp(nombre, "scan") == 0) *target = TARGET_SCAN;
    else if (strcmp(nombre, "parse") == 0) *target = TARGET_PARSE;
    else if (strcmp(nombre, "sem") == 0) *target = TARGET_SEM;
    else if (strcmp(nombre, "codinter") == 0) *target = TARGET_CODINTER;
    else if (strcmp(nombre, "assembly") == 0) *target = TARGET_ASSEMBLY;
    else return -1;
    return 0;
}

//Devuelve 1 si el nombre termina en .ctds y tiene algo antes de la extension
static int tieneExtensionCtds(const char *nombre){
    size_t largo = strlen(nombre);
    return largo > 5 && strcmp(nombre + largo - 5, ".ctds") == 0;
}

char *outputName(const char *input, const char *output, const char *ext){
    if (output != NULL){
        return strdup(output);
    }
    //Como gcc, la salida va en la carpeta actual: se saca la ruta del input
    //("test/codigoObjeto/x.ctds" -> "x.ctds"). strrchr busca la ultima '/'
    const char *nombre = input;
    const char *barra = strrchr(input, '/');
    if (barra != NULL){
        nombre = barra + 1; //el nombre empieza despues de la barra
    }

    size_t largoBase = strlen(nombre) - 5; //sin ".ctds", parseArgs ya valido la extension
    char *nombreSalida = malloc(largoBase + strlen(ext) + 1); //+1 para el '\0'
    if (nombreSalida == NULL){
        return NULL;
    }
    strncpy(nombreSalida, nombre, largoBase);
    nombreSalida[largoBase] = '\0'; //strncpy no lo agrega si corta antes del final
    strcat(nombreSalida, ext);
    return nombreSalida;
}

int parseArgs(int argc, char **argv, Options *opts)
{
    //Valores por defecto
    opts->input = NULL;
    opts->output = NULL;
    opts->target = TARGET_EXEC;
    opts->debug = 0;
    opts->opts = 0;

    //Recorro los argumentos
    for (int i = 1; i < argc; i++){
        //Comparo cada argumento con las opciones conocidas
        if (strcmp(argv[i], "-o") == 0){
            //Compruebo el siguiente arg
            if (i + 1 >= argc){
                fprintf(stderr, "Error: falta el nombre de salida despues de -o\n");
                return -1;
            }
            opts->output = argv[++i];
        } else if (strcmp(argv[i], "-debug") == 0){
            //Cambio el modo de debug a 1
            opts->debug = 1;
        } else if (strcmp(argv[i], "-opt") == 0){
            if (i + 1 >= argc){
                fprintf(stderr, "Error: falta la optimizacion despues de -opt\n");
                return -1;
            }
            i++;
            if (strcmp(argv[i], "all") == 0){
                opts->opts |= OPT_ALL;
            }else if (strcmp(argv[i], "shifteos") == 0){
                opts->opts |= OPT_SHIFT;
            } else {
                fprintf(stderr, "Error: optimizacion desconocida '%s'\n", argv[i]);
                return -1;
            }
        } else if (strcmp(argv[i], "-target") == 0){
            if (i + 1 >= argc){
                fprintf(stderr, "Error: falta la etapa despues de -target\n");
                return -1;
            }
            i++;
            if (parseTarget(argv[i], &opts->target) != 0){
                //Si devuelve -1, es porque la etapa no existe
                fprintf(stderr, "Error: etapa desconocida '%s'\n", argv[i]);
                return -1;
            }
        } else if (argv[i][0] == '-'){
            //Empieza con '-' pero no es ninguna de las opciones anteriores
            fprintf(stderr, "Error: opcion desconocida '%s'\n", argv[i]);
            return -1;
        } else {
            //No empieza con '-': es el archivo de entrada
            if (opts->input != NULL){
                fprintf(stderr, "Error: solo se acepta un archivo de entrada\n");
                return -1;
            }
            opts->input = argv[i];
        }
    }

    //Validaciones que solo se pueden hacer despues de leer todo
    if (opts->input == NULL){
        fprintf(stderr, "Error: falta el archivo de entrada\n");
        return -1;
    }
    if (!tieneExtensionCtds(opts->input)){
        fprintf(stderr, "Error: el archivo '%s' debe tener extension .ctds\n", opts->input);
        return -1;
    }
    return 0;
}

void printUsage(const char *progName){
    fprintf(stderr, "Uso: %s [opciones] nombreArchivo.ctds\n", progName);
    fprintf(stderr, "Opciones:\n");
    fprintf(stderr, "  -o <salida>         Renombra el archivo ejecutable a <salida>\n");
    fprintf(stderr, "  -target <etapa>     Compila hasta la etapa dada: scan, parse, sem, codinter o assembly\n");
    fprintf(stderr, "  -opt <optimizacion> Realiza la optimizacion dada. all realiza todas\n");
    fprintf(stderr, "  -debug              Imprime informacion de debugging\n");
}
