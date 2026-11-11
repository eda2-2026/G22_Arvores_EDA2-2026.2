#ifndef GERADOR_H
#define GERADOR_H

#include <stdint.h>
#include "reserva.h"

/* Gerador de reservas aleatorias, usado pelo executavel 'gerador' (que grava
   CSV) e pelo benchmark (que gera direto em memoria).

   As reservas sao geradas SEM sobreposicao por construcao: cada recurso tem
   uma linha do tempo percorrida em ordem (dias uteis, 08:00-20:00), com
   intervalos de 0-60 min entre reservas e duracao de 30 min a 2 h, em
   multiplos de 15 min. Assim todas sao aceitas pela agenda e a arvore cresce
   exatamente n elementos — se sortessemos intervalos ao acaso, quase todos
   conflitariam quando n cresce e o benchmark mediria arvores pequenas. */

/* PRNG proprio (xorshift32): o rand() do Windows so vai ate 32767, o que
   estragaria tanto a distribuicao quanto a reprodutibilidade entre sistemas. */
uint32_t gerador_aleatorio(uint32_t *estado);
uint32_t gerador_semente(uint32_t semente);   /* estado inicial valido (!= 0) */

/* Inicio da linha do tempo: segunda-feira 2026-01-05 08:00. */
long gerador_base(void);

typedef enum {
    ORDEM_CRONOLOGICA,   /* inicio crescente — o caso real e o pior para rotacoes */
    ORDEM_ALEATORIA      /* embaralhada (Fisher-Yates) */
} OrdemGeracao;

/* Preenche v[0..n-1] com reservas de ids 1..n distribuidas entre
   'num_recursos' recursos ("Sala1", "Sala2", ...). Retorna o fim da ultima
   reserva gerada (limite superior da linha do tempo). */
long gerador_reservas(Reserva *v, int n, int num_recursos, OrdemGeracao ordem,
                      uint32_t semente);

/* Embaralha v (Fisher-Yates). */
void gerador_embaralhar(Reserva *v, int n, uint32_t *estado);

#endif /* GERADOR_H */