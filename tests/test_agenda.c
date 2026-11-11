/* tests/test_agenda.c
 *
 * Teste de contrato: roda a mesma bateria de verificacoes nas DUAS arvores
 * (AVL e Rubro-Negra), que implementam a mesma interface Agenda. No fim, faz
 * uma verificacao CRUZADA: com as mesmas reservas nas duas arvores, elas devem
 * dar exatamente as mesmas respostas de conflito e de listagem — se divergirem,
 * uma delas esta errada.
 */
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

#define MAXS 5000
static int sobrepoe(long i1, long f1, long i2, long f2) { return i1 < f2 && i2 < f1; }

static Reserva nova(int id, long inicio, long dur) {
    Reserva r;
    r.id = id;
    snprintf(r.recurso, TAM_RECURSO, "Sala");
    r.inicio = inicio;
    r.fim = inicio + dur;
    snprintf(r.responsavel, TAM_RESPONSAVEL, "resp%d", id);
    return r;
}

typedef struct { Reserva v[MAXS]; int n; } Coletor;
static void coletar(const Reserva *r, void *ctx) { Coletor *c = ctx; c->v[c->n++] = *r; }

typedef Agenda *(*Criar)(void);

/* Bateria de verificacoes para uma arvore qualquer (via interface). */
static void suite(const char *nome, Criar criar) {
    printf("-- %s --\n", nome);
    Reserva sombra[MAXS]; int ns = 0;
    Agenda *a = criar();
    char buf[80];

    snprintf(buf, sizeof buf, "%s: agenda vazia", nome);
    checa(buf, a != NULL && agenda_tamanho(a) == 0 && agenda_altura(a) == 0);

    snprintf(buf, sizeof buf, "%s: inserir invalida", nome);
    checa(buf, agenda_inserir(a, nova(999, 500, 0), NULL) == RESERVA_INVALIDA);

    int ok_ins = 1;
    srand(7);
    for (int it = 0; it < 3000; it++) {
        Reserva r = nova(ns + 1, rand() % 20000, 1 + rand() % 60);
        int esp = 0;
        for (int k = 0; k < ns; k++)
            if (sobrepoe(r.inicio, r.fim, sombra[k].inicio, sombra[k].fim)) { esp = 1; break; }
        ResultadoReserva res = agenda_inserir(a, r, NULL);
        if (esp) { if (res != RESERVA_CONFLITO) ok_ins = 0; }
        else     { if (res != RESERVA_OK) ok_ins = 0; else sombra[ns++] = r; }
    }
    snprintf(buf, sizeof buf, "%s: insercao concorda com a sombra", nome);
    checa(buf, ok_ins);
    snprintf(buf, sizeof buf, "%s: tamanho correto", nome);
    checa(buf, agenda_tamanho(a) == ns);

    /* altura dentro do limite da Rubro-Negra (2*log2(n+1)), que vale para as duas */
    double lim = 2.0 * (log(ns + 1) / log(2.0)) + 1.0;
    snprintf(buf, sizeof buf, "%s: altura dentro do limite (n=%d, h=%d, lim=%.1f)",
             nome, ns, agenda_altura(a), lim);
    checa(buf, agenda_altura(a) <= lim);

    /* listar_periodo correto e ordenado */
    Coletor col; col.n = 0;
    agenda_listar_periodo(a, 5000, 9000, coletar, &col);
    int esp = 0, ord = 1;
    for (int k = 0; k < ns; k++)
        if (sobrepoe(sombra[k].inicio, sombra[k].fim, 5000, 9000)) esp++;
    for (int k = 1; k < col.n; k++) if (col.v[k-1].inicio > col.v[k].inicio) ord = 0;
    snprintf(buf, sizeof buf, "%s: listar_periodo correto e em ordem", nome);
    checa(buf, col.n == esp && ord);

    snprintf(buf, sizeof buf, "%s: cancelar inexistente devolve 0", nome);
    checa(buf, agenda_cancelar(a, 123456) == 0);

    agenda_destruir(a);
}

/* Verificacao cruzada: as duas arvores com as mesmas reservas devem concordar. */
static void cruzada(void) {
    printf("-- Verificacao cruzada AVL x Rubro-Negra --\n");
    Agenda *avl = agenda_criar_avl();
    Agenda *rb  = agenda_criar_rb();
    srand(99);
    int id = 1;
    for (int it = 0; it < 4000; it++) {
        Reserva r = nova(id, rand() % 30000, 1 + rand() % 50);
        ResultadoReserva ra = agenda_inserir(avl, r, NULL);
        ResultadoReserva rr = agenda_inserir(rb, r, NULL);
        if (ra == RESERVA_OK) id++;
        if (ra != rr) { checa("as duas aceitam/rejeitam igual", 0); goto fim; }
    }
    checa("as duas aceitam/rejeitam as mesmas reservas", 1);
    checa("mesmo tamanho nas duas arvores", agenda_tamanho(avl) == agenda_tamanho(rb));

    int ok_q = 1;
    for (int q = 0; q < 5000; q++) {
        long i = rand() % 30000, f = i + 1 + rand() % 50;
        if (agenda_buscar_conflito(avl, i, f, NULL) != agenda_buscar_conflito(rb, i, f, NULL)) { ok_q = 0; break; }
    }
    checa("buscar_conflito identico nas duas", ok_q);

    Coletor ca; ca.n = 0; Coletor cr; cr.n = 0;
    agenda_listar_periodo(avl, 8000, 15000, coletar, &ca);
    agenda_listar_periodo(rb, 8000, 15000, coletar, &cr);
    int igual = (ca.n == cr.n);
    for (int k = 0; igual && k < ca.n; k++)
        if (ca.v[k].id != cr.v[k].id) igual = 0;
    checa("listar_periodo identico nas duas", igual);
fim:
    agenda_destruir(avl);
    agenda_destruir(rb);
}

int main(void) {
    printf("== Teste de contrato das duas arvores ==\n");
    suite("AVL", agenda_criar_avl);
    suite("Rubro-Negra", agenda_criar_rb);
    cruzada();
    printf("\n== Resumo: %d/%d passaram ==\n", passaram, total);
    if (passaram == total) { printf("RESULTADO: SUCESSO\n"); return 0; }
    printf("RESULTADO: FALHA\n");
    return 1;
}
