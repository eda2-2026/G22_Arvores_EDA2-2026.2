#include <stdlib.h>
#include "agenda_impl.h"

/* Despachante: cada funcao publica repassa a chamada para a arvore concreta
   atraves da tabela de operacoes (a->ops). Nao ha logica de arvore aqui. */

Agenda *agenda_envolver(const AgendaOps *ops, void *impl) {
    if (impl == NULL) return NULL;
    Agenda *a = malloc(sizeof(Agenda));
    if (a == NULL) return NULL;
    a->ops = ops;
    a->impl = impl;
    return a;
}

void agenda_destruir(Agenda *a) {
    if (a == NULL) return;
    a->ops->destruir(a->impl);
    free(a);
}

ResultadoReserva agenda_inserir(Agenda *a, Reserva r, Reserva *conflito) {
    return a->ops->inserir(a->impl, r, conflito);
}

int agenda_cancelar(Agenda *a, int id) {
    return a->ops->cancelar(a->impl, id);
}

int agenda_buscar_conflito(const Agenda *a, long inicio, long fim, Reserva *conflito) {
    return a->ops->buscar_conflito(a->impl, inicio, fim, conflito);
}

void agenda_listar_periodo(const Agenda *a, long inicio, long fim,
                           void (*visita)(const Reserva *, void *), void *ctx) {
    a->ops->listar_periodo(a->impl, inicio, fim, visita, ctx);
}

long agenda_proximo_livre(const Agenda *a, long apartir, long duracao) {
    return a->ops->proximo_livre(a->impl, apartir, duracao);
}

int agenda_tamanho(const Agenda *a) { return a->ops->tamanho(a->impl); }
int agenda_altura(const Agenda *a)  { return a->ops->altura(a->impl); }
long agenda_rotacoes(const Agenda *a) { return a->ops->rotacoes(a->impl); }
