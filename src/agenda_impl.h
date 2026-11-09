#ifndef AGENDA_IMPL_H
#define AGENDA_IMPL_H

#include "agenda.h"

/* Cabecalho interno: so as implementacoes das arvores (agenda_avl.c,
   agenda_rb.c) e o despachante (agenda.c) usam isto. O codigo cliente inclui
   apenas agenda.h.

   Cada arvore preenche uma AgendaOps com as suas funcoes. O primeiro parametro
   (self) e o ponteiro para a estrutura concreta da arvore (ArvoreAVL/ArvoreRB),
   guardado em Agenda.impl. */
typedef struct {
    ResultadoReserva (*inserir)(void *self, Reserva r, Reserva *conflito);
    int  (*cancelar)(void *self, int id);
    int  (*buscar_conflito)(const void *self, long inicio, long fim, Reserva *conflito);
    void (*listar_periodo)(const void *self, long inicio, long fim,
                           void (*visita)(const Reserva *, void *), void *ctx);
    long (*proximo_livre)(const void *self, long apartir, long duracao);
    int  (*tamanho)(const void *self);
    int  (*altura)(const void *self);
    long (*rotacoes)(const void *self);
    void (*destruir)(void *self);
} AgendaOps;

struct Agenda {
    const AgendaOps *ops;  /* tabela de operacoes da arvore concreta */
    void *impl;            /* ponteiro para a arvore concreta */
};

/* Monta o envelope Agenda para uma implementacao concreta. Cada construtor
   (agenda_criar_avl / agenda_criar_rb) cria a sua arvore e chama isto.
   Retorna NULL se faltar memoria. */
Agenda *agenda_envolver(const AgendaOps *ops, void *impl);

#endif /* AGENDA_IMPL_H */
