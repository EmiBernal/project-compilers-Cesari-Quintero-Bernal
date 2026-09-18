%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h> //Para los limites de enteros
#include "parser.tab.h"
%}

/* definiciones regulares*/
alpha           [a-zA-Z]
digit           [0-9]
alpha_num       {alpha}|{digit}|_
arith_op        "+"|"-"|"*"|"/"|"%"
relOp           "<"|">"|"=="
cond_op         "&&"|"||"
int_literal     {digit}{digit}*
bool_literal    "true"|"false"
float_literal   {digit}{digit}*"."{digit}{digit}*
bin_op          {arith_op}|{relOp}|{cond_op}
literal         {int_literal}|{bool_literal}|{float_literal}
id              {alpha}{alpha_num}*

%option yylineno
%x COMMENTS

%%
[ \t]+      { /* nada */ }
\n          { /* nada */ }
"//".*      { /* nada */ }

"/*" {BEGIN COMMENTS;}
<COMMENTS>"*/" {BEGIN INITIAL;}
<COMMENTS>. { /* Ignorar caracteres*/}
<COMMENTS>\n { /* Ignorar saltos de linea */}
<COMMENTS><<EOF>> {
    printf("Error en linea %d: comentario sin cerrar\n", yylineno);
    return -1; // o lo que uses para error léxico
}

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

"+"         {return PLUS;}
"*"         {return TIMES;}
"="         {return ASSIGN;}
";"         {return SEMI;}
"("         {return LPAREN;}
")"         {return RPAREN;}
"{"         {return LBRACE;}
"}"         {return RBRACE;}
"-"         {return MINUS;}
"/"         {return SLASH;}
"%"         {return MOD;}
"<"         {return LT;}
">"         {return GT;}
"=="        {return EQ;}
"&&"        {return AND;}
"||"        {return OR;}
"!"         {return NOT;}
","         {return COMMA;}

{float_literal} {
    yylval.fnum = atof(yytext);
    return FNUM;
}

{digit}+ {
    //Convierto el valor a numero
    long val = strtol(yytext, NULL, 10);
    if(val < INT_MIN || val > INT_MAX) {
        printf("Error en linea %d: numero fuera de rango\n", yylineno);
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

. {
    printf("Caracter no reconocido en linea %d: '%s'\n", yylineno, yytext);
    return -1;
}

%%
int yywrap(){
    return 1;
}
