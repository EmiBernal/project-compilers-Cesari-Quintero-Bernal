%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.tab.h"
%}

/* definiciones regulares*/

delim [ \t\n]
ws  {delim}+
letra [a-zA-Z]
digito [0-9]
id {letra}({letra}|{digito})*
numero {digito}+

%x COMMENTS

%%

"/*" {BEGIN COMMENTS;}
<COMMENTS>"*/" {BEGIN INITIAL;}
<COMMENTS>. { /* Ignorar caracteres*/}
<COMMENTS>\n { /* Ignorar saltos de linea */}
<COMMENTS><<EOF>> {
    printf("Error: comentario sin cerrar\n");
    return -1; // o lo que uses para error léxico
}

{ws} {/* no hay accion y no hay retorno*/}

"true" {return TRUE;}
"false" {return FALSE;}

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

"+" {return PLUS;}
"*" {return TIMES;}
"=" {return ASSIGN;}
";" {return SEMI;}
"(" {return LPAREN;}
")" {return RPAREN;}
"{" {return LBRACE;}
"}" {return RBRACE;}

%%
int yywrap(){
    return 1;
}
