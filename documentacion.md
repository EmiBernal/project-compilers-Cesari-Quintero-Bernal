# Compilador para el lenguaje C-TDS

**"Entrega por etapas"**

**Universidad Nacional de Río Cuarto**
Facultad de Ciencias Exactas Físico Químicas y Naturales
Departamento de Computación
Taller de Diseño de Software
Prof. Francisco Bavera

**Integrantes:**
- Emiliano Bernal
- Agustín Cesari
- Samuel Quintero

Octubre 2026

---

## 1. Decisiones de diseño: análisis léxico y sintáctico

### 1.1. Conflictos *shift/reduce* y *reduce/reduce* en `var_decl` y `method_decl`

La gramática inicial producía conflictos *shift/reduce* y *reduce/reduce* al definir `var_decl` y `method_decl`, ya que ambas producciones comparten el prefijo `type id` y el parser LALR(1) debe decidir, con un único token de anticipación (*lookahead*), qué regla aplicar.

Concretamente, la gramática inicial definía las declaraciones de variables y de métodos como dos listas recursivas separadas, una a continuación de la otra:

```
program: var_decls method_decls;

var_decls: %empty | var_decls var_decl;

method_decls: %empty | method_decls method_decl;

var_decl: type ID SEMI;

method_decl: type ID LPAREN params RPAREN block
           | VOID ID LPAREN params RPAREN block
           ;
```

Detectamos esta situación al intentar compilar `parser.y` con el siguiente ejemplo:

```c
int main(){
    int x;
    int y;
    x = 1;
    y = 1;
    x = x + 3 * 2 * y;
    y = 2 + 2;
    return;
}
```

#### 1.1.1. Resolución del conflicto

Para resolverlo, se unificó la gramática en una única lista recursiva que admite `var_decl` y `method_decl` en cualquier orden:

```
program: %empty
       | program var_decl
       | program method_decl
       ;
```

Esta unificación elimina la ambigüedad estructural: al no forzar un orden estricto entre declaraciones de variables y declaraciones de métodos, el parser deja de tener que decidir entre dos producciones que comienzan con el mismo prefijo en el mismo contexto, y los conflictos desaparecen.

#### 1.1.2. Actualización del conflicto de la gramática

La anterior unificación, si bien solucionó el conflicto mencionado, propaga la aparición de elementos que no son declaraciones a la izquierda del árbol AST, lo cual se aleja de la especificación y complica innecesariamente el árbol. Decidimos en clase la siguiente modificación sobre la gramática.

```
program: var_decls method_decls
| method_decls
;

var_decls: var_decls var_decl
| var_decl
;

method_decls: method_decls method_decl
| method_decl
;
```

De esta manera, solucionamos el conflicto manteniendo el orden de las variables declaradas y los métodos. Ahora se declaran todas las variables al principio y luego los métodos obligatoriamente.

## 2. Decisiones de diseño: análisis semántico

### 2.1. Decisiones, asunciones y aclaraciones

1. **El operador `%` solo admite operandos enteros.** La especificación solo exige que los operandos aritméticos sean `int` o `float`. Decidimos ser más estrictos con el módulo, porque su significado sobre números de punto flotante es ambiguo. Por lo tanto, `3.5 % 2` se informa como error semántico.

2. **El método `main` puede tener cualquier tipo de retorno.** La especificación exige que `main` exista y que no tenga parámetros, pero no restringe su tipo de retorno. Se chequean exactamente esas dos condiciones.

3. **Un método es visible dentro de su propio cuerpo.** El método se inserta en la tabla de símbolos *antes* de analizar su cuerpo, de modo que se admiten llamadas recursivas.

4. **Parámetros y variables locales comparten un único scope.** Siguiendo la especificación, declarar en el cuerpo de un método una variable con el mismo nombre que un parámetro es un error de declaración duplicada.

5. **Los bloques anidados abren un scope propio y admiten *shadowing*.** Una variable declarada en un bloque interno puede tener el mismo nombre que una declarada en un scope exterior; la declaración interna oculta a la externa dentro del bloque.

6. **Variables y métodos comparten el espacio de nombres global.** No pueden existir una variable global y un método con el mismo nombre. Usar el nombre de un método como si fuera una variable también es un error.

7. **Coerción entre `int` y `float`.** Se permite asignar, pasar como argumento y retornar valores numéricos de un tipo donde se espera el otro (regla 14 de la especificación). El resultado de una operación aritmética es `float` si al menos un operando es `float`; en caso contrario es `int`. Elegimos `float` para no perder información en las operaciones mixtas.

8. **Resultado de operadores relacionales y lógicos.** Los operadores relacionales, la igualdad y los operadores lógicos siempre producen un valor `boolean`, aun cuando sus operandos sean inválidos. Esto evita errores derivados en expresiones que los contienen.

9. **El análisis no se detiene en el primer error.** Se informan todos los errores semánticos independientes, pero se evita repetir los que son consecuencia de uno ya informado (ver sección [2.3](#23-propagación-de-errores-con-type_error)).

10. **Declaración antes del uso.** El análisis se realiza en una sola pasada y en orden textual. Un identificador debe estar declarado antes de su primer uso, tal como lo exige la especificación. En consecuencia, un método no puede invocar a otro definido más abajo en el archivo.

### 2.2. Compatibilidad de tipos y coerción

La relación de compatibilidad se centralizó en una función auxiliar:

```c
static bool areCompatible(DataType expected, DataType received) {
    if (expected == TYPE_ERROR || received == TYPE_ERROR) {
        return true;
    }
    return expected == received
        || (isNumeric(expected) && isNumeric(received));
}
```

Dos tipos son compatibles si son iguales o si ambos son numéricos. Esta única función se usa en asignaciones, argumentos de llamadas, expresiones de retorno e igualdad, de modo que la regla de coerción de la especificación queda definida en un solo lugar. El tipo resultante de cada operador es:

| Operador | Operandos válidos | Resultado |
|---|---|---|
| `+ - * /` | `int` o `float` | `float` si alguno es `float`, si no `int` |
| `%` | solo `int` | `int` |
| `< >` | `int` o `float` | `boolean` |
| `==` | tipos compatibles | `boolean` |
| `&& \|\|` | `boolean` | `boolean` |
| `!` | `boolean` | `boolean` |
| `-` (unario) | `int` o `float` | mismo tipo del operando |

### 2.3. Propagación de errores con `TYPE_ERROR`

**Problema detectado.** Al probar el analizador con la sentencia `x = z + 1;`, donde `z` no está declarada, se informaban varios errores por una única falta: primero que `z` no existe, luego que los operandos de `+` no son válidos y finalmente que la asignación mezcla tipos incompatibles. Los errores derivados no aportan información y ocultan la causa real. Lo detectamos con el caso de prueba `err_nodecl.ctds`, que debía producir un único error y producía dos.

**Decisión.** Se agregó el valor `TYPE_ERROR` al final de la enumeración `DataType` (para no alterar los valores existentes). Significa "en esta expresión ya se informó un error; continuar en silencio y propagarlo". Cuando `checkExpr` encuentra una falta, la informa una única vez y devuelve `TYPE_ERROR`. Los nodos superiores que reciben ese tipo no informan nada y lo devuelven hacia arriba.

Para `x = z + 1;` el flujo es:

1. `checkExpr` sobre `z`: no está declarada, se informa el error y se devuelve `TYPE_ERROR`.
2. `checkExpr` sobre `+`: un operando es `TYPE_ERROR`, no se informa nada y se devuelve `TYPE_ERROR`.
3. `checkNode` sobre la asignación: `areCompatible` devuelve `true` si algún tipo es `TYPE_ERROR`, por lo que no se informa nada.

Resultado: un único mensaje, el de la causa.

**Implementación.** Tres piezas colaboran:

```c
/* 1) areCompatible: si algun lado ya tiene error, no hay motivo nuevo */
if (expected == TYPE_ERROR || received == TYPE_ERROR) return true;

/* 2) operadores: no validan si algun operando ya fallo */
bool operandHasError = left == TYPE_ERROR || right == TYPE_ERROR;
if (!operandHasError && (left != TYPE_BOOL || right != TYPE_BOOL)) {
    /* informar error */
}

/* 3) condiciones de if/while y operadores unarios */
if (condition != TYPE_ERROR && condition != TYPE_BOOL) {
    /* informar error */
}
```

Notar que `areCompatible` devuelve `true` ante un `TYPE_ERROR` no porque los tipos sean correctos, sino porque no hay un motivo nuevo para informar. Es un `true` para continuar la propagacion y no de aprobación.

**Limitación.** El silenciamiento tiene un costo: si una misma expresión tiene dos faltas independientes en operandos distintos (por ejemplo, `z + true` con `z` sin declarar), solo se informa la primera; la segunda aparece al corregirla. Se priorizó que el compilador nunca repita errores derivados.

## 3. Decisiones de diseño: Generador de codigo intermedio

### 3.1. Interacción entre el Análisis Semántico y la Generación de Código Intermedio

Durante la fase de análisis semántico, el compilador no solo valida la correctitud del programa, sino que también deja en el campo evalType de los nodos sentado cual es el tipo evaluado teniendo en cuenta los hijos y el operador en cuestion. Un aspecto clave de este proceso es la resolución y almacenamiento de los tipos de datos en las expresiones.

#### 3.1.1. Consumo en la Generación de Código Intermedio

En la etapa de generación de código, la función `generateMidRepresentation` recorre este AST ya decorado. Dado que el análisis semántico garantizó la validez de las operaciones y resolvió los tipos, el generador opera mecánicamente sin necesidad de recalcular compatibilidades.

Cuando el generador procesa nodos de operaciones (NODE_OP o NODE_UNOP) y requiere almacenar un resultado parcial, lee directamente el atributo evalType del nodo para instanciar una nueva variable temporal con el tipo exacto. Esto se implementa mediante la instrucción:

```c
Symbol* temp = createTempSymbol(node->evalType);
```

#### 3.1.2. Ventajas del Enfoque

- **Menor redundancia:** Evitamos recalcular tipos y consistencia en el generador de codigo intermedio.
- **Precisión en el Código de Tres Direcciones (TAC):** Al propagar `node->evalType` hacia `createTempSymbol`, las instrucciones del TAC preservan la naturaleza exacta de los datos temporales (distinguiendo entre enteros, flotantes o booleanos), lo cual es indispensable para la posterior asignación de registros y generación de código ensamblador.

#### 3.1.3. Representación mediante Código de Tres Direcciones

Cada instrucción computacional se descompone en operaciones atómicas con un máximo de tres operandos (dos fuentes y un destino).

#### 3.1.4. Aplanamiento del Control de Flujo

Otra decisión de diseño fue la traducción de las estructuras de control jerárquicas (`if`, `while`) a una secuencia plana de ejecución basada exclusivamente en saltos y etiquetas.

- Este aplanamiento abstrae la sintaxis original del lenguaje fuente. La etapa final del compilador ya no requerirá conocimiento de la gramática original, reduciendo su responsabilidad a una simple mapeo de las instrucciones de salto del AST a los *jump* del código ensamblador final.

### 3.2. Métodos, parámetros y llamadas

#### 3.2.1. Delimitación de métodos: `OP_BEGIN_FUNC` y `OP_END_FUNC`

Al traducir los nodos `NODE_METHOD` agregamos dos nuevos opcodes: `OP_BEGIN_FUNC` y `OP_END_FUNC`. Marcan dónde empieza y dónde termina el código de cada método, y llevan como operando el símbolo del método:

```
begin_func suma
t0 = a + b
return t0
end_func suma
```

#### 3.2.2. Parámetros formales

Los parámetros formales (`NODE_PARAM`) no generan ninguna instrucción. El análisis semántico ya los declaró en la tabla de símbolos dentro del scope del método, así que en el cuerpo se usan directamente por su nombre, como cualquier otra variable.

#### 3.2.3. Llamadas a métodos

En cambio, en cada llamada (`NODE_CALL`) sí hay que generar código para los argumentos:

1. Se evalúa cada argumento real y, si hace falta, se lo convierte al tipo del parámetro formal correspondiente (coerción `int`/`float`).
2. Una vez evaluados todos, se emite un `param` por cada uno, en orden.
3. Se emite el `call`. Si el método no es `void`, el resultado queda en un temporal nuevo del tipo de retorno del método.

Los `param` se emiten recién después de evaluar todos los argumentos, para que no queden intercalados con los de otra llamada cuando un argumento es a su vez una llamada. Por ejemplo, para `x = suma(1, 2);` y `f = mitad(x);` (con `mitad(float v)`) se genera:

```
param 1
param 2
t3 = call suma
x = t3
t4 = (float) x
param t4
t5 = call mitad
f = t5
```

Dentro del método, en cada `return` con valor también se convierte la expresión al tipo de retorno declarado.

### 3.3. Evaluación en cortocircuito de `&&` y `||`

Los operadores `&&` y `||` se traducen con evaluación en cortocircuito: el operando derecho solo se evalúa si el izquierdo no alcanza para decidir el resultado. Si en un `&&` el lado izquierdo es `false`, ya sabemos que el resultado es falso; si en un `||` es `true`, ya sabemos que es verdadero.

El esquema generado es:

1. Se evalúa el operando izquierdo y se guarda en un temporal `t`.
2. Se salta a la etiqueta de fin si el resultado ya está decidido: `if_false t goto L` para `&&`, `if_true t goto L` para `||`.
3. Se evalúa el operando derecho y su valor se asigna a `t`.
4. Se marca la etiqueta `L`. El resultado de la expresión es `t`.

Para `b = false && marcar();` se genera:

```
t1 = false
if_false t1 goto L0
t2 = call marcar
t1 = t2
L0:
b = t1
```

#### 3.3.1. Generar vs. ejecutar

Hay que distinguir entre *generar* y *ejecutar*. El compilador siempre genera el código del operando derecho, pero como queda después del salto, en tiempo de ejecución solo corre si no se saltó. Si se ejecuta, el valor del lado derecho reemplaza al del izquierdo en el temporal: en `true && x` el resultado es `x`, y en `false || x` también.

## 4. Decisiones de diseño: Generador de código objeto

### 4.1. Uso de registros y convención de operandos

#### 4.1.1. Sintaxis y tamaño de los datos

El código objeto se genera de manera que el operando de la derecha (el segundo) es el destino de la instrucción. Las operaciones se hacen sobre enteros de 32 bits (instrucciones con sufijo `l` y registros `%eax`, `%ecx`). Los booleanos se representan como enteros de 32 bits con valores 0 (`false`) y 1 (`true`).

Por ejemplo, `addl %ecx, %eax` calcula `eax = eax + ecx`: el resultado queda en `%eax`.

#### 4.1.2. Convención para instrucciones de tres direcciones

Para la instrucción `t1 = a + t0` se genera:

```asm
movl -4(%rbp), %eax      
movl -12(%rbp), %ecx    
addl %ecx, %eax      (resultado en eax)
movl %eax, -16(%rbp)     
```

El metodo `static void writePrologue(FILE* out, Symbol* method, int frameSize)` que esta en el archivo assemblyGenerator es el encargado de armar el frame y reservar slots. Mientras que el metodo `static void writeEpilogue(FILE* out)` desarma el frame y vuelve.

Todas las instrucciones del codigo intermedio siguen el mismo patron: 

1) Cargar en registros
2) Operar
3) Guardar

OP_ASSIGN (x = t1) es solamente:
asm
    movl -8(%rbp), %eax
    movl %eax, -12(%rbp)

Hace falta pasar por %eax porque x86 no permite un mov de memoria a memoria.

#### 4.1.3. Observaciones

- **`%ecx` es un registro auxiliar** que no se conserva entre llamadas, por lo que se lo puede usar libremente sin tener que guardarlo.
- **Los valores no se mantienen en registros entre instrucciones.** Cada instrucción del código intermedio carga sus operandos desde memoria (pila o `.data`) y guarda su resultado en memoria. Es menos eficiente, pero simplifica el generador y evita implementar una asignación de registros.
- La línea `.note.GNU-stack` le avisa al linker que el programa no necesita una pila ejecutable. Sin ella, gcc/ld tira una advertencia.

### 4.2. Funciones auxiliares

#### 4.2.1. `needsStackSlot`

 Indica si el símbolo necesita un lugar en la pila: una variable local `VAR` no global, un parámetro `PARAM` o un temporal `TEMPORARY`. Las globales, constantes, etiquetas y métodos no lo necesitan.

#### 4.2.2. `assignStackSlot`

Le asigna al símbolo su lugar en el marco de la función. Suma 4 bytes (`INT_SIZE_BYTES`) a `frameSize` y guarda en `símbolo->offset` el valor negativo correspondiente.
Si el símbolo ya tiene `offset` distinto de 0 o no necesita lugar entonces no hace nada.

#### 4.2.3. `assignInstructionSlots`

Aplica `assignStackSlot` a los operandos de una instrucción (`arg1`, `arg2` y `result`), saltando los que son `NULL`.

#### 4.2.4. `assignParameterSlots`

Recorre la lista de parámetros de un método(`method->params`) y les asigna su lugar primero, antes que a las demás variables.

#### 4.2.5. `assignMethodSlots`

Calcula el tamaño del marco de un método. Asigna los lugares de los parámetros y después recorre las instrucciones desde `BEGIN_FUNC` hasta `END_FUNC`. Devuelve el tamaño total en bytes, sin alinear.

#### 4.2.6. `alignFrameSize`

Redondea el tamaño del marco hacia arriba al próximo múltiplo de 16 (`STACK_ALIGNMENT_BYTES`) por convencion al hacer un `call`. 

#### 4.2.6. `writePrologue` y `writeEpilogue`

- **`writePrologue` Describe el comienzo de un método: `.text`, `.globl nombre`, la etiqueta `nombre:`, `pushq %rbp`, `movq %rsp, %rbp` y `subq $tamañoAlineado, %rsp`, que reserva el espacio para las variables. 
- **`writeEpilogue` EDescribe el cierre de un método: `leave` (restaura `%rsp` y `%rbp`) y `ret`.


#### 4.2.7. `loadOperand` y `storeResult`

Las instrucciones aritméticas de x86 necesitan que al menos uno de sus operandos esté en un registro, y no admiten dos posiciones de memoria a la vez. Por eso cada instrucción del código intermedio se traduce cargando sus operandos en registros, operando, y guardando el resultado en memoria. Para no repetir esos pasos en cada caso de `translateInstruction`, se encapsularon en dos funciones:

- **`loadOperand(out, símbolo, registro)`** carga un operando en un registro. Escribe `movl operando, registro`, donde el operando lo escribe `writeOperand` según dónde viva: una constante (`$5`), una variable global (`nombre(%rip)`) o una variable local, parámetro o temporal (`offset(%rbp)`).
- **`storeResult(out, registro, símbolo)`** hace lo inverso: guarda un registro en el destino de la instrucción. Escribe `movl registro, destino`.

Con ellas, cada caso de `translateInstruction` queda con la misma forma. Por ejemplo, la suma:

```c
loadOperand(out, instruction->arg1, "%eax");        // movl a, %eax
loadOperand(out, instruction->arg2, "%ecx");        // movl b, %ecx
fprintf(out, "    addl %%ecx, %%eax\n");            // addl %ecx, %eax
storeResult(out, "%eax", instruction->result);      // movl %eax, resultado
```
#### 4.2.8. `writeOperand`
writeOperand escribe en el archivo de salida cómo se accede a un operando del código intermedio. Es la única función que sabe dónde vive cada valor, y la usan `loadOperand` y `storeResult`. Según el símbolo, hay tres casos:

- ** `Constantes`: Se escriben como un valor inmediato, con $ adelante. Los enteros usan value.ival, y los booleanos value.bval, que se escribe como 1 para true y 0 para false.

- ** `Variables globales`: Viven en la sección .data y se acceden por su nombre, relativo al registro %rip, que apunta a la instrucción que se está ejecutando.

- ** `Locales, parámetros y temporales`: Viven en la pila de la función, y se acceden por su distancia al registro %rbp, que marca la base del marco. El offset es un número negativo que le asigna assignMethodSlots a cada símbolo al comenzar la función (la primera variable recibe -4, la siguiente -8, etc).

### 4.3. Llamadas a métodos y pasaje de parámetros

Para que quien llama y quien es llamado se entiendan, seguimos la convención de llamadas de x86-64 en Linux (System V ABI). Así nuestros métodos son compatibles con funciones escritas en C.

#### 4.3.1. Dónde viaja cada argumento

- **Los primeros 6 argumentos** van en registros, en este orden: `%edi`, `%esi`, `%edx`, `%ecx`, `%r8d`, `%r9d`. Están en la tabla `PARAM_REGISTERS`, que usan los dos lados de la llamada.
- **Del 7º en adelante** van en la pila, porque el ABI solo reserva 6 registros para argumentos enteros. Cada uno ocupa 8 bytes, porque `push` siempre mueve 8 bytes.
- **El valor de retorno** queda en `%eax`.

#### 4.3.2. Lado de quien llama: `OP_PARAM`, `OP_CALL` y `writeCall`

El código intermedio emite los `param` de izquierda a derecha, pero los argumentos de la pila hay que empujarlos de derecha a izquierda. Además, hasta llegar al `call` no se sabe cuántos son. Por eso:

- **`OP_PARAM` no genera assembly.** Solo guarda el argumento en el arreglo `callArgs` y suma 1 a `paramCount`. Los dos son `static` dentro de `translateInstruction`, así que conservan su valor entre una instrucción y la siguiente.
- **`OP_CALL` llama a `writeCall`**, que escribe la llamada completa:
  1. **Relleno:** si los bytes de los argumentos en la pila no son múltiplo de 16, reserva 8 bytes extra (`subq $8, %rsp`) para que `%rsp` quede alineado en el `call`. El total se calcula con `alignFrameSize`.

    Por ejemplo si tuvieramos un metodo que usa 5 slots --> 5 * 4 = 20 y como de 5 a 8 slots usamos 32 bytes queda 32bytes

  2. **Argumentos en la pila**, del último al 7º, en orden inverso, para que el 7º quede más cerca: `movl arg, %eax` y `pushq %rax`.
  3. **Los primeros 6**, cargados en sus registros con `loadOperand`.
  4. **`call nombre`.**
  5. **Limpieza:** quien llama saca de la pila lo que empujó, con `addq $total, %rsp` (argumentos más relleno).
  6. **Resultado:** si el método no es `void`, guarda `%eax` en el temporal.

Después del `call`, `paramCount` vuelve a 0 para la próxima llamada.

Alcanza con un solo arreglo aunque haya llamadas anidadas como `f(g(1), 2)`, porque el código intermedio evalúa la llamada de adentro completa antes de emitir el primer `param` de la de afuera (ver 3.2.3).

Por ejemplo, `f(1, 2, 3, 4, 5, 6, 7)` genera:

```asm
subq $8, %rsp        # relleno: 1 argumento en la pila (8 bytes) -> 16
movl $7, %eax
pushq %rax           # el 7º argumento, a la pila
movl $1, %edi        # los primeros 6, en registros
movl $2, %esi
movl $3, %edx
movl $4, %ecx
movl $5, %r8d
movl $6, %r9d
call f
addq $16, %rsp       # saco el 7º y el relleno
movl %eax, -4(%rbp)  # guardo el resultado
```

#### 4.3.3. Lado de quien es llamado: `copyParametersToStack`

Dentro del método, `writeOperand` busca los parámetros en su slot de la pila (`offset(%rbp)`, negativo), igual que a las variables locales. Por eso, justo después del prólogo, `copyParametersToStack` copia cada parámetro desde donde llegó a su slot:

- **Del 1º al 6º:** desde su registro, por ejemplo `movl %edi, -4(%rbp)`.
- **Del 7º en adelante:** desde la pila de quien llamó. Como x86 no permite un `mov` de memoria a memoria, se pasa por `%eax`:
  ```asm
  movl 16(%rbp), %eax
  movl %eax, -28(%rbp)
  ```

Hay que copiarlos sí o sí: si un método llama a otro, los registros de argumentos se pisan. Por ejemplo, en `fact(n)`, para llamar a `fact(n - 1)` se carga `n - 1` en `%edi`, y sin la copia se perdería el `n` original.

El orden en `generateAssembly` importa: `assignMethodSlots` (asigna los offsets), después `writePrologue` (reserva el espacio) y recién después `copyParametersToStack`.

#### 4.3.4. Dónde quedan los argumentos de la pila

Después del prólogo de quien es llamado, la pila queda así (crece hacia abajo):

```
| 9º argumento         |  32(%rbp)  ┐
| 8º argumento         |  24(%rbp)  │ los empujó quien llama
| 7º argumento         |  16(%rbp)  ┘
| dirección de retorno |   8(%rbp)  <- la pone el call
| %rbp de quien llamó  |   0(%rbp)  <- pushq %rbp del prólogo
| slots del método     |  -4(%rbp), -8(%rbp), ...
```

El 7º siempre está en `16(%rbp)` (`FIRST_STACK_ARG_OFFSET`), porque entre él y `%rbp` hay siempre dos valores de 8 bytes: el `%rbp` guardado y la dirección de retorno. En general, el argumento con índice `i` (contando desde 0, con `i >= 6`) está en `16 + 8 × (i − 6)`.

Los offsets **positivos** son memoria del frame de quien llamó, y los **negativos** son del frame propio.

#### 4.3.5. Observaciones

- **`%rbp` no guarda los argumentos:** guarda una dirección, la base del frame. `16(%rbp)` significa "la memoria en la dirección `%rbp + 16`".
- **La alineación importa:** el ABI exige `%rsp` múltiplo de 16 en cada `call`. El prólogo la deja así, y el relleno de `writeCall` la mantiene cuando la cantidad de argumentos en la pila es impar.
- **Límite:** `MAX_CALL_ARGS` (256) argumentos por llamada. Si se pasa, se informa el error por `stderr`.
- **Solo enteros y booleanos:** los argumentos `float` usan otros registros (`%xmm0`–`%xmm7`) y todavía no están soportados.
- **Tests:** `seisParametros.ctds`, `nueveParametros.ctds` y `muchosParametrosRecursion.ctds` en `test/codigoObjeto/`.
