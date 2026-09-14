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

%%

{ws} {/* no hay accion y no hay retorno*/}
"int" {return INT;}
"bool" {return BOOL;}
"return" {return RETURN;}
"main" {return MAIN;}
"void" {return VOID;}
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
