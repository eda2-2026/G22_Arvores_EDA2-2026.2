#ifndef SISTEMA_H
#define SISTEMA_H

#include "agenda.h"

/* Camada da aplicacao acima da Agenda.

   A Agenda (AVL ou Rubro-Negra) guarda as reservas de UM recurso. O Sistema
   mantem uma agenda por recurso (Sala1, Lab2, Quadra...) e garante o que a
   arvore sozinha nao garante:
     - ids unicos em todo o sistema;
     - cancelamento direto na agenda certa (mapa id -> recurso);
     - geracao do proximo id livre.
   Conflito so existe entre reservas do MESMO recurso. */

typedef enum { ARVORE_AVL, ARVORE_RB } TipoArvore;

typedef enum {
    SIS_OK = 0,
    SIS_CONFLITO,       /* sobrepoe outra reserva do mesmo recurso */
    SIS_INVALIDA,       /* inicio >= fim, recurso vazio... */
    SIS_ID_DUPLICADO,   /* ja existe reserva com esse id */
    SIS_SEM_MEMORIA
} ResultadoSistema;

typedef struct Sistema Sistema;

Sistema *sistema_criar(TipoArvore tipo);
void     sistema_destruir(Sistema *s);
TipoArvore sistema_tipo(const Sistema *s);
const char *sistema_nome_arvore(TipoArvore tipo);   /* "AVL" / "Rubro-Negra" */

/* Insere r na agenda do recurso r.recurso (criando-a se preciso).
   Em conflito, copia a reserva conflitante para *conflito (se != NULL). */
ResultadoSistema sistema_reservar(Sistema *s, Reserva r, Reserva *conflito);

/* Cancela pelo id. Retorna 1 se removeu, 0 se o id nao existe. */
int sistema_cancelar(Sistema *s, int id);

/* Retorna 1 se o id ja esta em uso. */
int sistema_existe_id(const Sistema *s, int id);

/* Menor id ainda nao usado e maior que todos ja vistos. */
int sistema_proximo_id(const Sistema *s);

/* Agenda de um recurso, ou NULL se ele ainda nao tem reservas registradas. */
const Agenda *sistema_agenda(const Sistema *s, const char *recurso);

/* Iteracao sobre os recursos, na ordem em que foram criados. */
int           sistema_num_recursos(const Sistema *s);
const char   *sistema_recurso_nome(const Sistema *s, int i);
const Agenda *sistema_recurso_agenda(const Sistema *s, int i);

int sistema_total_reservas(const Sistema *s);

#endif /* SISTEMA_H */