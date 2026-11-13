"""Generador de programas CTDS aleatorios a partir de la gramatica de parser.y.

Uso:
    python3 fuzzing/fuzz/gen.py --seed 42
    python3 fuzzing/fuzz/gen.py --seed 42 --depth 6
"""
import argparse
import os
import random
import subprocess
import tempfile
import xml.etree.ElementTree as ET

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
GRAMATICA = os.path.join(ROOT, "parser.y")

# Los tokens no se pueden deducir de la gramática: hay que decir
# qué texto concreto representa cada uno.
TOKENS = {
    "NUM":    lambda rng: str(rng.randint(0, 9)),
    "PLUS":   lambda rng: "+",
    "MINUS":  lambda rng: "-",
    "TIMES":  lambda rng: "*",
    "LPAREN": lambda rng: "(",
    "RPAREN": lambda rng: ")",
}

#Lista blanca de producciones, si dejan de existir filtrar() tira un error
PERMITIDAS = {
    "expr": {
        ("NUM",),
        ("expr", "PLUS", "expr"),
        ("expr", "MINUS", "expr"),
        ("expr", "TIMES", "expr"),
        ("LPAREN", "expr", "RPAREN"),
    },
}

#Bison puede exportar la gramatica. 
#Con esta funcion la gramatica se mantiene actualizada
def generar_xml(destino):
    subprocess.run(["bison", "--xml=" + destino, "-o", os.devnull, GRAMATICA],
                   check=True, capture_output=True)

#Lee el XML y arma dos estructuras: reglas y terminales
def cargar(path):
    reglas = {}                      
    raiz = ET.parse(path).getroot()
    terminales = {t.get("name") for t in raiz.iter("terminal")}
    for r in raiz.iter("rule"):
        lhs = r.find("lhs").text
        rhs = [s.text for s in r.find("rhs").iter("symbol")]
        if lhs == "$accept":
            continue
        rhs = [s for s in rhs if s != "$end"]
        reglas.setdefault(lhs, []).append(rhs)
    return reglas, terminales

#Filtra las reglas permitidas, si hay alguna regla no permitida tira error
def filtrar(reglas):
    filtradas = {}
    for nt, permitidas in PERMITIDAS.items():
        filtradas[nt] = [p for p in reglas[nt] if tuple(p) in permitidas]
        faltan = permitidas - {tuple(p) for p in filtradas[nt]}
        if faltan:
            raise ValueError(f"producciones de {nt} que no estan en parser.y: {faltan}")
    return filtradas

def alturas_minimas(reglas, terminales):
    """Para cada no terminal: mínima profundidad con la que se puede terminar."""
    h = {nt: float("inf") for nt in reglas}
    cambio = True
    while cambio:
        cambio = False
        for nt, prods in reglas.items():
            for p in prods:
                hs = [0 if s in terminales else h.get(s, float("inf")) for s in p]
                v = 1 + max(hs, default=0)
                if v < h[nt]:
                    h[nt], cambio = v, True
    return h

def generar(simbolo, presupuesto, rng, reglas, terminales, h):
    if simbolo in terminales:
        return TOKENS[simbolo](rng)
    prods = reglas[simbolo]
    def altura(p):
        return 1 + max((0 if s in terminales else h[s] for s in p), default=0)
    validas = [p for p in prods if altura(p) <= max(presupuesto, h[simbolo])]
    p = rng.choice(validas)
    return " ".join(generar(s, presupuesto - 1, rng, reglas, terminales, h) for s in p)

def programa(seed, depth):
    with tempfile.TemporaryDirectory() as tmp:
        xml = os.path.join(tmp, "parser.xml")
        generar_xml(xml) #Genera el xml en una carpeta personal
        reglas, terminales = cargar(xml) #Arma las estructuras
    reglas = filtrar(reglas) #Filtra las reglas
    h = alturas_minimas(reglas, terminales) #Calcula las alturas
    rng = random.Random(seed) #El rng depende de la semilla
    expr = generar("expr", depth, rng, reglas, terminales, h) #Generamos la expresion
    return f"// seed {seed}\nint main() {{\n    return {expr};\n}}\n" #Envolvemos la expresion 

if __name__ == "__main__":
    ap = argparse.ArgumentParser(description="Genera un programa CTDS aleatorio")
    ap.add_argument("--seed", type=int, default=0)
    ap.add_argument("--depth", type=int, default=4, help="profundidad maxima de la expresion")
    args = ap.parse_args()
    print(programa(args.seed, args.depth), end="")
