#ifndef AGENDA_H
#define AGENDA_H

#include "reserva.h"

/* Interface comum de uma agenda de reservas de UM recurso.
   O conflito e definido por sobreposicao de horario: duas reservas conflitam
   quando a.inicio < b.fim && b.inicio < a.fim.

   A mesma interface e implementada por duas arvores balanceadas (AVL e
   Rubro-Negra), ordenadas pelo inicio da reserva. O tipo Agenda e opaco: o
   codigo cliente (aplicacao, testes, benchmark) usa so as funcoes abaixo e nao
   sabe qual arvore esta por tras — basta trocar o construtor para comparar as
   duas. */
typedef struct Agenda Agenda;

typedef enum {
    RESERVA_OK = 0,        /* inserida com sucesso */
    RESERVA_CONFLITO = 1,  /* conflita com uma reserva existente (nao inserida) */
    RESERVA_INVALIDA = 2   /* dados invalidos (ex.: inicio >= fim) */
} ResultadoReserva;

/* Construtores — cada arvore fornece o seu. Retornam NULL se faltar memoria. */
Agenda *agenda_criar_avl(void);
Agenda *agenda_criar_rb(void);
void    agenda_destruir(Agenda *a);

/* Insere r. Havendo conflito, NAO insere e, se conflito != NULL, copia para
   *conflito a reserva conflitante. Retorna o resultado da operacao. */
ResultadoReserva agenda_inserir(Agenda *a, Reserva r, Reserva *conflito);

/* Cancela a reserva de id dado. Retorna 1 se removeu, 0 se nao existia. */
int agenda_cancelar(Agenda *a, int id);

/* Verifica, sem inserir, se [inicio, fim) conflita com alguma reserva.
   Retorna 1 e preenche *conflito (se != NULL) quando ha conflito; 0 senao. */
int agenda_buscar_conflito(const Agenda *a, long inicio, long fim, Reserva *conflito);

/* Percorre, em ordem crescente de inicio, as reservas que intersectam
   [inicio, fim], chamando visita(&reserva, ctx) para cada uma. */
void agenda_listar_periodo(const Agenda *a, long inicio, long fim,
                           void (*visita)(const Reserva *, void *), void *ctx);

/* Primeiro instante t >= apartir em que cabe uma reserva de 'duracao' minutos
   sem conflito. Retorna -1 se nao houver (nao deve ocorrer numa agenda aberta). */
long agenda_proximo_livre(const Agenda *a, long apartir, long duracao);

/* --- Metricas para o benchmark AVL x Rubro-Negra --- */
int  agenda_tamanho(const Agenda *a);   /* numero de reservas guardadas */
int  agenda_altura(const Agenda *a);    /* altura da arvore */
long agenda_rotacoes(const Agenda *a);  /* rotacoes acumuladas desde a criacao */

#endif /* AGENDA_H */
