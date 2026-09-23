-La gramatica incial produce conflictos shift/reduce y reduce/recude a la hora de definir var_decl y method_decl ya que comparten el prefijo type id y el parser decide con solo 1 token de anticipación. Detectamos esta situación al intentar compilar nuestro parser.y con el siguiente ejemplo.

int main(){
    int x;
    int y;
    x = 1;
    y = 1;
    x = x + 3 * 2 * y;
    y = 2 + 2;
    return;
}

Para resolverlo, se unificó la gramática en una única lista recursiva que admite var_decl y method_decl en cualquier orden (program: program var_decl | program method_decl) eliminando la ambigüedad.