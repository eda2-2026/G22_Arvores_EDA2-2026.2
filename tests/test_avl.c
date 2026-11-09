#include "agenda.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static int total = 0, passaram = 0;
static void checa(const char *nome, int cond) {
    total++;
    if (cond) { passaram++; printf("  [ok]     %s\n", nome); }
    else      {             printf("  [FALHOU] %s\n", nome); }
}

/* ---- modelo-sombra: as reservas que sabemos estar na agenda ---- */
#define MAXS 5000
static Reserva sombra[MAXS];
static int nsombra = 0;

static int sobrepoe(long i1, long f1, long i2, long f2) {
    return i1 < f2 && i2 < f1;
}
static int conflita_sombra(long i, long f) {
    for (int k = 0; k < nsombra; k++)
        if (sobrepoe(i, f, sombra[k].inicio, sombra[k].fim)) return 1;
    return 0;
}

static Reserva nova(int id, long inicio, long dur) {
    Reserva r;
    r.id = id;
    snprintf(r.recurso, TAM_RECURSO, "Sala");
    r.inicio = inicio;
    r.fim = inicio + dur;
    snprintf(r.responsavel, TAM_RESPONSAVEL, "resp%d", id);
    return r;
}

/* coletor para o listar_periodo */
typedef struct { Reserva v[MAXS]; int n; } Coletor;
static void coletar(const Reserva *r, void *ctx) {
    Coletor *c = ctx;
    c->v[c->n++] = *r;
}

int main(void) {
    printf("== Testes da AVL (via interface Agenda) ==\n");
    srand(7);

    Agenda *a = agenda_criar_avl();
    checa("criar agenda nao retorna NULL", a != NULL);

    /* vazio */
    checa("tamanho inicial 0", agenda_tamanho(a) == 0);
    checa("altura inicial 0", agenda_altura(a) == 0);
    Reserva c;
    checa("buscar_conflito em agenda vazia", agenda_buscar_conflito(a, 0, 100, &c) == 0);

    /* invalida */
    checa("inserir invalida (inicio>=fim)",
          agenda_inserir(a, nova(999, 500, 0), NULL) == RESERVA_INVALIDA);

    /* insercao aleatoria conferida contra a sombra */
    int id = 1, ok_ins = 1, ok_conf = 1;
    for (int it = 0; it < 3000; it++) {
        long inicio = rand() % 20000;
        long dur    = 1 + rand() % 60;
        Reserva r = nova(id, inicio, dur);
        int esperado_conflito = conflita_sombra(r.inicio, r.fim);
        Reserva cf;
        ResultadoReserva res = agenda_inserir(a, r, &cf);
        if (esperado_conflito) {
            if (res != RESERVA_CONFLITO) ok_ins = 0;
            /* a reserva devolvida deve de fato sobrepor */
            if (!sobrepoe(r.inicio, r.fim, cf.inicio, cf.fim)) ok_conf = 0;
        } else {
            if (res != RESERVA_OK) ok_ins = 0;
            else { sombra[nsombra++] = r; id++; }
        }
    }
    checa("insercao concorda com a sombra (conflito/ok)", ok_ins);
    checa("reserva conflitante devolvida realmente sobrepoe", ok_conf);
    checa("tamanho bate com a sombra", agenda_tamanho(a) == nsombra);

    /* limite de altura da AVL: h <= 1.4405*log2(n+2) */
    int n = agenda_tamanho(a);
    double limite = 1.4405 * (log(n + 2) / log(2.0)) + 1.0;
    printf("     (n=%d, altura=%d, limite AVL=%.1f)\n", n, agenda_altura(a), limite);
    checa("altura dentro do limite da AVL", agenda_altura(a) <= limite);

    /* buscar_conflito concorda com a sombra em consultas aleatorias */
    int ok_busca = 1;
    for (int it = 0; it < 3000; it++) {
        long i = rand() % 20000, f = i + 1 + rand() % 60;
        int esp = conflita_sombra(i, f);
        int got = agenda_buscar_conflito(a, i, f, NULL);
        if ((esp != 0) != (got != 0)) ok_busca = 0;
    }
    checa("buscar_conflito concorda com a sombra", ok_busca);

    /* listar_periodo: mesmo conjunto que a sombra, e em ordem crescente */
    {
        long A = 5000, B = 9000;
        Coletor col; col.n = 0;
        agenda_listar_periodo(a, A, B, coletar, &col);
        int esp = 0;
        for (int k = 0; k < nsombra; k++)
            if (sobrepoe(sombra[k].inicio, sombra[k].fim, A, B)) esp++;
        int ordenado = 1;
        for (int k = 1; k < col.n; k++)
            if (col.v[k-1].inicio > col.v[k].inicio) ordenado = 0;
        checa("listar_periodo: quantidade bate com a sombra", col.n == esp);
        checa("listar_periodo: resultado em ordem crescente", ordenado);
    }

    /* proximo_livre: o horario devolvido realmente cabe e e >= apartir */
    {
        long apartir = 3000, dur = 30;
        long t = agenda_proximo_livre(a, apartir, dur);
        checa("proximo_livre >= apartir", t >= apartir);
        checa("proximo_livre realmente livre", !conflita_sombra(t, t + dur));
    }

    /* cancelamento: cancela metade e confere */
    {
        int cancelados = 0, ok_cancel = 1;
        int alvo = nsombra / 2;
        for (int k = 0; k < alvo; k++) {
            Reserva r = sombra[k];
            if (agenda_cancelar(a, r.id) != 1) ok_cancel = 0;
            /* apos remover, o slot exato nao deve mais conflitar */
            if (agenda_buscar_conflito(a, r.inicio, r.fim, NULL) != 0) ok_cancel = 0;
            cancelados++;
        }
        checa("cancelar devolve 1 e libera o slot", ok_cancel);
        checa("cancelar id inexistente devolve 0", agenda_cancelar(a, 123456) == 0);
        checa("tamanho apos cancelamentos", agenda_tamanho(a) == nsombra - cancelados);
        /* arvore continua dentro do limite de altura apos remocoes */
        int m = agenda_tamanho(a);
        double lim2 = 1.4405 * (log(m + 2) / log(2.0)) + 1.0;
        checa("altura dentro do limite apos remocoes", agenda_altura(a) <= lim2);
    }

    printf("     (rotacoes acumuladas: %ld)\n", agenda_rotacoes(a));
    checa("houve rotacoes (arvore balanceou)", agenda_rotacoes(a) > 0);

    agenda_destruir(a);

    printf("\n== Resumo: %d/%d passaram ==\n", passaram, total);
    if (passaram == total) { printf("RESULTADO: SUCESSO\n"); return 0; }
    printf("RESULTADO: FALHA\n");
    return 1;
}
