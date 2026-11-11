/* gerador — grava um CSV de reservas aleatorias sem sobreposicao.

   uso: gerador -n N [-r RECURSOS] [-s SEMENTE] [--ordenado] [-o ARQUIVO]

   Exemplos:
     gerador -n 20 -r 2                     # 20 reservas em Sala1/Sala2 (stdout)
     gerador -n 100000 -o data/grande.csv   # carga grande para testar a CLI */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "csv.h"
#include "gerador.h"

static void uso(void) {
    fprintf(stderr,
        "uso: gerador -n N [-r RECURSOS] [-s SEMENTE] [--ordenado] [-o ARQUIVO]\n"
        "  -n N         quantidade de reservas (obrigatorio, > 0)\n"
        "  -r RECURSOS  numero de recursos Sala1..SalaR (padrao 1)\n"
        "  -s SEMENTE   semente do sorteio (padrao 42) - mesma semente, mesmo arquivo\n"
        "  --ordenado   grava em ordem cronologica (padrao: embaralhado)\n"
        "  -o ARQUIVO   arquivo de saida (padrao: saida padrao)\n");
}

static int ler_num(const char *txt, long minimo, long *v) {
    char *fim;
    long x = strtol(txt, &fim, 10);
    if (fim == txt || *fim != '\0' || x < minimo) return 0;
    *v = x;
    return 1;
}

int main(int argc, char **argv) {
    long n = -1, recursos = 1, semente = 42;
    int ordenado = 0;
    const char *saida = NULL;

    for (int i = 1; i < argc; i++) {
        int tem_valor = i + 1 < argc;
        if (strcmp(argv[i], "-n") == 0 && tem_valor) {
            if (!ler_num(argv[++i], 1, &n)) { uso(); return 1; }
        } else if (strcmp(argv[i], "-r") == 0 && tem_valor) {
            if (!ler_num(argv[++i], 1, &recursos)) { uso(); return 1; }
        } else if (strcmp(argv[i], "-s") == 0 && tem_valor) {
            if (!ler_num(argv[++i], 0, &semente)) { uso(); return 1; }
        } else if (strcmp(argv[i], "--ordenado") == 0) {
            ordenado = 1;
        } else if (strcmp(argv[i], "-o") == 0 && tem_valor) {
            saida = argv[++i];
        } else {
            uso();
            return 1;
        }
    }
    if (n < 1 || n > 50000000L) { uso(); return 1; }

    Reserva *v = malloc((size_t)n * sizeof(Reserva));
    if (v == NULL) { fprintf(stderr, "sem memoria para %ld reservas\n", n); return 1; }
    gerador_reservas(v, (int)n, (int)recursos,
                     ordenado ? ORDEM_CRONOLOGICA : ORDEM_ALEATORIA, (uint32_t)semente);

    FILE *f = saida ? fopen(saida, "w") : stdout;
    if (f == NULL) { fprintf(stderr, "nao foi possivel criar '%s'\n", saida); free(v); return 1; }
    csv_escrever_cabecalho(f);
    for (long i = 0; i < n; i++) csv_escrever_reserva(f, &v[i]);
    if (saida) {
        fclose(f);
        fprintf(stderr, "%ld reservas gravadas em %s\n", n, saida);
    }
    free(v);
    return 0;
}