#ifndef RESERVA_H
#define RESERVA_H

#define TAM_RECURSO     40
#define TAM_RESPONSAVEL 60

/* Uma reserva de um recurso em um intervalo de tempo [inicio, fim).
   O tempo e representado em minutos (unidade comparavel); exige inicio < fim.
   O campo recurso serve para exibicao/CSV; o conflito e por sobreposicao de
   horario (ver agenda.h). */
typedef struct {
    int  id;
    char recurso[TAM_RECURSO];
    long inicio;
    long fim;
    char responsavel[TAM_RESPONSAVEL];
} Reserva;

/* ---------- Data e hora <-> minutos ----------
   O tempo de uma reserva e contado em minutos desde 1970-01-01 00:00 (sem fuso
   horario: e so uma escala para comparar e subtrair). O formato textual usado
   no CSV e na CLI e "AAAA-MM-DD HH:MM". */

#define TAM_DATAHORA 17   /* "AAAA-MM-DD HH:MM" + '\0' */

/* Converte uma data (AAAA-MM-DD) e uma hora (HH:MM) em minutos.
   Retorna 1 se ambas forem validas (mes 1-12, dia existente no mes, inclusive
   29/02 em ano bissexto, hora 0-23, minuto 0-59) e 0 caso contrario. */
int reserva_minutos(const char *data, const char *hora, long *minutos);

/* Converte "AAAA-MM-DD HH:MM" (um unico texto) em minutos. Retorna 1/0. */
int reserva_ler_datahora(const char *texto, long *minutos);

/* Escreve os minutos como "AAAA-MM-DD HH:MM" em buf (>= TAM_DATAHORA). */
void reserva_formatar(long minutos, char *buf);

/* ---------- Construcao e validacao ---------- */

typedef enum {
    RV_OK = 0,
    RV_ID,            /* id <= 0 */
    RV_INTERVALO,     /* inicio >= fim */
    RV_RECURSO,       /* recurso vazio ou longo demais */
    RV_RESPONSAVEL    /* responsavel longo demais */
} ValidacaoReserva;

/* Monta uma reserva copiando os textos. Textos longos demais NAO sao
   truncados em silencio: a validacao acusa o erro. */
ValidacaoReserva reserva_montar(Reserva *r, int id, const char *recurso,
                                long inicio, long fim, const char *responsavel);

/* Mensagem legivel para um codigo de validacao. */
const char *reserva_erro(ValidacaoReserva v);

/* Imprime uma linha legivel: "#id  recurso  AAAA-MM-DD HH:MM -> HH:MM  responsavel". */
void reserva_imprimir(const Reserva *r);

#endif /* RESERVA_H */