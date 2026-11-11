/* tools/benchmark.c
 *
 * Compara as duas arvores (AVL e Rubro-Negra) sobre a MESMA carga, em dois
 * cenarios:
 *   1) Insercao: constroi a agenda com N reservas (sem conflito, inseridas em
 *      ordem aleatoria) e mede tempo, altura final e numero de rotacoes.
 *   2) Consulta: com a agenda pronta, mede o tempo de M buscas de conflito.
 *
 * As reservas sao geradas na memoria: a reserva i ocupa [i*PASSO, i*PASSO+dur)
 * com dur < PASSO, o que garante que nunca se sobrepoem; a ordem de insercao e
 * embaralhada para nao inserir em ordem crescente.
 *
 * Compilar (da raiz do projeto):
 *   gcc -Wall -Wextra -std=c11 -O2 -Isrc -o benchmark tools/benchmark.c \
 *       src/agenda.c src/agenda_avl.c src/agenda_rb.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "agenda.h"

#define LIMIAR_CLOCKS (CLOCKS_PER_SEC / 20)   /* ~50 ms por medicao, para precisao */
#define PASSO 100                              /* espacamento entre reservas */

static const int TAMANHOS[] = { 1000, 10000, 100000 };
#define NUM_TAMANHOS (int)(sizeof TAMANHOS / sizeof TAMANHOS[0])
#define M_CONSULTAS 200000

typedef Agenda *(*Criar)(void);

static Reserva nova(int id, long inicio, long dur) {
    Reserva r;
    r.id = id;
    r.recurso[0] = '\0';
    r.inicio = inicio;
    r.fim = inicio + dur;
    r.responsavel[0] = '\0';
    return r;
}

/* Gera n reservas sem conflito e embaralha a ordem (Fisher-Yates). */
static void gerar(Reserva *v, int n, unsigned semente) {
    srand(semente);
    for (int i = 0; i < n; i++)
        v[i] = nova(i + 1, (long)i * PASSO, 1 + rand() % (PASSO - 1));
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        Reserva tmp = v[i]; v[i] = v[j]; v[j] = tmp;
    }
}

typedef struct { double ms; int altura; long rotacoes; } ResIns;

/* Mede a construcao da agenda com as n reservas (repete ate ter precisao). */
static ResIns medir_insercao(Criar criar, const Reserva *res, int n) {
    clock_t acc = 0;
    int exec = 0, altura = 0;
    long rot = 0;
    while (acc < LIMIAR_CLOCKS) {
        Agenda *a = criar();
        clock_t t0 = clock();
        for (int i = 0; i < n; i++) agenda_inserir(a, res[i], NULL);
        clock_t t1 = clock();
        acc += (t1 - t0);
        exec++;
        altura = agenda_altura(a);
        rot = agenda_rotacoes(a);
        agenda_destruir(a);
    }
    ResIns r = { (double)acc * 1000.0 / CLOCKS_PER_SEC / exec, altura, rot };
    return r;
}

/* Mede M buscas de conflito sobre uma agenda ja construida. */
static double medir_consulta(const Agenda *a, const long *qi, const long *qf, int m) {
    clock_t acc = 0;
    int exec = 0;
    volatile int soma = 0;
    while (acc < LIMIAR_CLOCKS) {
        clock_t t0 = clock();
        for (int i = 0; i < m; i++)
            soma += agenda_buscar_conflito(a, qi[i], qf[i], NULL);
        clock_t t1 = clock();
        acc += (t1 - t0);
        exec++;
    }
    (void)soma;
    return (double)acc * 1000.0 / CLOCKS_PER_SEC / exec;
}

int main(int argc, char *argv[]) {
    unsigned semente = 42;
    if (argc >= 2) semente = (unsigned)strtoul(argv[1], NULL, 10);

    printf("Benchmark AVL x Rubro-Negra (agenda de reservas)\n");
    printf("Semente: %u | consultas por medicao: %d\n\n", semente, M_CONSULTAS);
    fflush(stdout);

    double ins_avl[NUM_TAMANHOS], ins_rb[NUM_TAMANHOS];
    int    alt_avl[NUM_TAMANHOS], alt_rb[NUM_TAMANHOS];
    long   rot_avl[NUM_TAMANHOS], rot_rb[NUM_TAMANHOS];
    double con_avl[NUM_TAMANHOS], con_rb[NUM_TAMANHOS];

    for (int t = 0; t < NUM_TAMANHOS; t++) {
        int n = TAMANHOS[t];
        Reserva *res = malloc((size_t)n * sizeof(Reserva));
        if (res == NULL) { fprintf(stderr, "sem memoria\n"); return 1; }
        gerar(res, n, semente);

        /* consultas aleatorias dentro do intervalo coberto [0, n*PASSO) */
        long *qi = malloc((size_t)M_CONSULTAS * sizeof(long));
        long *qf = malloc((size_t)M_CONSULTAS * sizeof(long));
        srand(semente + 1);
        for (int i = 0; i < M_CONSULTAS; i++) {
            long ini = rand() % ((long)n * PASSO);
            qi[i] = ini;
            qf[i] = ini + 1 + rand() % 50;
        }

        ResIns ia = medir_insercao(agenda_criar_avl, res, n);
        ResIns ir = medir_insercao(agenda_criar_rb, res, n);
        ins_avl[t] = ia.ms; alt_avl[t] = ia.altura; rot_avl[t] = ia.rotacoes;
        ins_rb[t]  = ir.ms; alt_rb[t]  = ir.altura; rot_rb[t]  = ir.rotacoes;

        Agenda *avl = agenda_criar_avl();
        Agenda *rb  = agenda_criar_rb();
        for (int i = 0; i < n; i++) { agenda_inserir(avl, res[i], NULL); agenda_inserir(rb, res[i], NULL); }
        con_avl[t] = medir_consulta(avl, qi, qf, M_CONSULTAS);
        con_rb[t]  = medir_consulta(rb,  qi, qf, M_CONSULTAS);
        agenda_destruir(avl); agenda_destruir(rb);

        printf("  n=%d medido\n", n);
        fflush(stdout);
        free(res); free(qi); free(qf);
    }

    /* Tabela 1: insercao (tempo, altura, rotacoes) */
    printf("\n### Insercao de N reservas (media de varias execucoes)\n\n");
    printf("| N | tempo AVL (ms) | tempo RB (ms) | altura AVL | altura RB | rotacoes AVL | rotacoes RB |\n");
    printf("| --- | --- | --- | --- | --- | --- | --- |\n");
    for (int t = 0; t < NUM_TAMANHOS; t++)
        printf("| %d | %.3f | %.3f | %d | %d | %ld | %ld |\n",
               TAMANHOS[t], ins_avl[t], ins_rb[t], alt_avl[t], alt_rb[t], rot_avl[t], rot_rb[t]);

    /* Tabela 2: consulta de conflito */
    printf("\n### %d buscas de conflito (media, em ms)\n\n", M_CONSULTAS);
    printf("| N | consulta AVL (ms) | consulta RB (ms) |\n");
    printf("| --- | --- | --- |\n");
    for (int t = 0; t < NUM_TAMANHOS; t++)
        printf("| %d | %.3f | %.3f |\n", TAMANHOS[t], con_avl[t], con_rb[t]);

    /* CSV para os graficos */
    FILE *csv = fopen("resultados.csv", "w");
    if (csv != NULL) {
        fprintf(csv, "n,arvore,insercao_ms,altura,rotacoes,consulta_ms\n");
        for (int t = 0; t < NUM_TAMANHOS; t++) {
            fprintf(csv, "%d,avl,%.3f,%d,%ld,%.3f\n", TAMANHOS[t], ins_avl[t], alt_avl[t], rot_avl[t], con_avl[t]);
            fprintf(csv, "%d,rb,%.3f,%d,%ld,%.3f\n",  TAMANHOS[t], ins_rb[t],  alt_rb[t],  rot_rb[t],  con_rb[t]);
        }
        fclose(csv);
        printf("\nResultados salvos em resultados.csv\n");
    }
    return 0;
}
