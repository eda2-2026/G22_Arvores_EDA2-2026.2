"""Gera os graficos do benchmark AVL x Rubro-Negra (tools/benchmark.c).

uso: python scripts/graficos.py [resultados.csv] [pasta_saida]
     (padrao: resultados.csv na raiz e graficos em resultados/)

O benchmark grava, para cada N, o tempo total de inserir N reservas, a altura,
o total de rotacoes e o tempo de 200000 buscas de conflito. Para comparar
tamanhos diferentes no mesmo grafico, os valores sao convertidos para "por
operacao" (us por insercao, rotacoes por insercao, us por busca).
Requer matplotlib (pip install matplotlib).
"""
import csv
import math
import os
import sys

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402
from matplotlib.ticker import FuncFormatter  # noqa: E402

ENTRADA = sys.argv[1] if len(sys.argv) > 1 else "resultados.csv"
SAIDA = sys.argv[2] if len(sys.argv) > 2 else "resultados"
M_CONSULTAS = 200000  # igual a M_CONSULTAS em tools/benchmark.c

COR = {"avl": "#2a78d6", "rb": "#eb6834"}
NOME = {"avl": "AVL", "rb": "Rubro-Negra"}
TEXTO, TEXTO2, GRADE, FUNDO = "#0b0b0b", "#52514e", "#e4e3df", "#fcfcfb"

plt.rcParams.update({
    "figure.facecolor": FUNDO, "axes.facecolor": FUNDO, "savefig.facecolor": FUNDO,
    "axes.edgecolor": GRADE, "axes.labelcolor": TEXTO2, "axes.titlecolor": TEXTO,
    "xtick.color": TEXTO2, "ytick.color": TEXTO2, "font.size": 10,
    "axes.titlesize": 11, "axes.titleweight": "bold", "axes.titlelocation": "left",
    "axes.spines.top": False, "axes.spines.right": False, "axes.spines.left": False,
    "axes.grid": True, "axes.grid.axis": "y", "grid.color": GRADE, "grid.linewidth": 0.8,
    "legend.frameon": False,
})

dados = {}  # (n, arvore) -> linha
with open(ENTRADA, newline="") as f:
    for l in csv.DictReader(f):
        dados[(int(l["n"]), l["arvore"])] = l
NS = sorted({n for n, _ in dados})


def metrica(n, arv, nome):
    l = dados[(n, arv)]
    if nome == "us_insercao":
        return float(l["insercao_ms"]) * 1000 / n
    if nome == "rot_insercao":
        return int(l["rotacoes"]) / n
    if nome == "altura":
        return int(l["altura"])
    if nome == "us_consulta":
        return float(l["consulta_ms"]) * 1000 / M_CONSULTAS
    raise ValueError(nome)


def barras(ax, nome, titulo, rotulo_y, fmt):
    largura = 0.36
    for k, arv in enumerate(("avl", "rb")):
        xs = [i + (k - 0.5) * (largura + 0.02) for i in range(len(NS))]
        ys = [metrica(n, arv, nome) for n in NS]
        ax.bar(xs, ys, largura, color=COR[arv], label=NOME[arv], edgecolor=FUNDO, linewidth=2)
        for x, y in zip(xs, ys):
            ax.annotate(fmt.format(y).replace(".", ","), (x, y), xytext=(0, 3),
                        textcoords="offset points", ha="center", fontsize=8, color=TEXTO2)
    ax.yaxis.set_major_formatter(FuncFormatter(lambda v, _: f"{v:g}".replace(".", ",")))
    ax.set_xticks(range(len(NS)))
    ax.set_xticklabels([f"N = {n:,}".replace(",", ".") for n in NS])
    ax.set_title(titulo)
    ax.set_ylabel(rotulo_y)
    ax.margins(y=0.15)


os.makedirs(SAIDA, exist_ok=True)
fig, eixos = plt.subplots(2, 2, figsize=(11, 7.5))
barras(eixos[0][0], "us_insercao", "Tempo por inserção", "µs por inserção", "{:.2f}")
barras(eixos[0][1], "rot_insercao", "Rotações por inserção", "rotações / inserção", "{:.2f}")
barras(eixos[1][0], "altura", "Altura final da árvore", "altura", "{:.0f}")
barras(eixos[1][1], "us_consulta", "Tempo por busca de conflito", "µs por busca", "{:.3f}")

# referencia: altura minima possivel
ax = eixos[1][0]
for i, n in enumerate(NS):
    otimo = math.ceil(math.log2(n + 1))
    ax.plot([i - 0.42, i + 0.42], [otimo, otimo], "--", color=TEXTO, linewidth=1.2, zorder=3,
            label="altura mínima ⌈log₂(N+1)⌉" if i == 0 else None)
ax.legend(loc="upper left", fontsize=8)

eixos[0][0].legend(loc="upper left")
fig.suptitle("Benchmark AVL × Rubro-Negra — inserção em ordem aleatória", x=0.01, ha="left",
             fontsize=13, fontweight="bold", color=TEXTO)
fig.tight_layout()
destino = os.path.join(SAIDA, "benchmark.png")
fig.savefig(destino, dpi=130)
print("grafico salvo em", destino)