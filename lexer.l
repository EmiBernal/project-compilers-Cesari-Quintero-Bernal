%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.tab.h"
%}

/* definiciones regulares*/

delim [ \t\n]
ws  {delim}+
alpha           [a-zA-Z]
digit           [0-9]+
arith_op        "+"|"-"|"*"|"/"|"%"
relOp           "<"|">"|"=="
cond_op         "&&"|"||"
int_literal     {digit}{digit}*
bool_literal    "true"|"false"
float_literal   {digit}{digit}*"."{digit}{digit}*
bin_op          {arith_op}|{relOp}|{cond_op}
literal         {int_literal}|{bool_literal}|{float_literal}
id              {alpha}{alpha_num}*
alpha_num       {alpha}|{digit}|_




%x COMMENTS

%%
[ \t]+      { /* nada */ }
\n          { /* nada */ }

"/*" {BEGIN COMMENTS;}
<COMMENTS>"*/" {BEGIN INITIAL;}
<COMMENTS>. { /* Ignorar caracteres*/}
<COMMENTS>\n { /* Ignorar saltos de linea */}
<COMMENTS><<EOF>> {
    printf("Error: comentario sin cerrar\n");
    return -1; // o lo que uses para error léxico
}

{ws} {/* no hay accion y no hay retorno*/}

/* Palabras reservadas */
"boolean"   {return BOOLEAN;}
"else"      {return ELSE;}
"false"     {return FALSE;}
"if"        {return IF;}
"int"       {return INT;}
"return"    {return RETURN;}
"true"      {return TRUE;}
"void"      {return VOID;}
"while"     {return WHILE;}
"float"     {return FLOAT;}





{digit}+ {
    //Convierto el valor a numero
    long val = strtol(yytext, NULL, 10);
    if(val < -2147483648 || val > 2147483647) {
        printf("Error: numero fuera de rango\n");
        return -1;
    }
    yylval.num = (int)val; //casteo a int
    return NUM;
}


{id} {
    /* Guardamos una copia del lexema real en yylval para que Bison sepa el nombre*/
    yylval.string = strdup(yytext);
    return ID;
    }

{numero} {
    /* Convertimos el lexema a numero entero y lo pasamos por yylval al parser*/
    yylval.num = atoi(yytext);
    return NUM;
    }

%%
int yywrap(){
    return 1;
}
