#ifndef CLI_H
#define CLI_H

typedef enum { TARGET_SCAN, TARGET_PARSE, TARGET_SEM, TARGET_CODINTER, TARGET_ASSEMBLY, TARGET_EXEC } Target;

//Optimizaciones: cada una es un bit de Options.opts. OPT_ALL prende todos
#define OPT_ALL (~0)
#define OPT_SHIFT (1 << 0)
#define OPT_CONST (1 << 1)
#define OPT_DEADCODE (1 << 2)
typedef struct {
    char  *input;     // "prog.ctds"
    char  *output;    // lo que venga con -o, o NULL
    Target target;    // por defecto TARGET_EXEC
    int    debug;     // 0 o 1
    int    opts;      // una bandera por optimización
} Options;

//Metodos
int parseArgs(int argc, char **argv, Options *opts);
void printUsage(const char *progName);
char *outputName(const char *input, const char *output, const char *ext);

#endif // CLI_H