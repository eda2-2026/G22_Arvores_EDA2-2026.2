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

#endif /* RESERVA_H */
