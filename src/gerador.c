#include "gerador.h"
#include <stdio.h>
#include <stdlib.h>

#define ABRE    (8 * 60)    /* 08:00 */
#define FECHA   (20 * 60)   /* 20:00 */

static const char *RESPONSAVEIS[] = {
    "Anna Clara", "Esdras", "Monitoria", "Coordenacao", "Grupo de estudos",
    "Aula pratica", "Manutencao", "Reuniao de equipe", "Banca", "Visitante"
};
#define NRESP ((int)(sizeof RESPONSAVEIS / sizeof RESPONSAVEIS[0]))

uint32_t gerador_aleatorio(uint32_t *estado) {
    uint32_t x = *estado;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return *estado = x;
}

uint32_t gerador_semente(uint32_t semente) {
    uint32_t x = semente * 2654435761u + 0x9E3779B9u;
    return x ? x : 1u;
}

static uint32_t entre(uint32_t *st, uint32_t a, uint32_t b) {   /* [a, b] */
    return a + gerador_aleatorio(st) % (b - a + 1);
}

/* 1970-01-01 foi quinta-feira; (dia + 3) % 7 da 0 = segunda ... 6 = domingo */
static int dia_util(long dia) { return (dia + 3) % 7 < 5; }

long gerador_base(void) {
    long dia = 20458;                 /* 2026-01-05, segunda-feira */
    return dia * 1440 + ABRE;
}

void gerador_embaralhar(Reserva *v, int n, uint32_t *st) {
    for (int i = n - 1; i > 0; i--) {
        int j = (int)(gerador_aleatorio(st) % (uint32_t)(i + 1));
        Reserva t = v[i]; v[i] = v[j]; v[j] = t;
    }
}

long gerador_reservas(Reserva *v, int n, int num_recursos, OrdemGeracao ordem,
                      uint32_t semente) {
    if (num_recursos < 1) num_recursos = 1;
    uint32_t st = gerador_semente(semente);
    long *relogio = malloc((size_t)num_recursos * sizeof(long));
    if (relogio == NULL) return -1;
    for (int k = 0; k < num_recursos; k++) relogio[k] = gerador_base();

    long fim_max = gerador_base();
    for (int i = 0; i < n; i++) {
        int k = i % num_recursos;                       /* recurso da vez */
        long gap = 15L * entre(&st, 0, 4);             /* 0..60 min livre */
        long dur = 15L * entre(&st, 2, 8);             /* 30 min .. 2 h   */
        long t = relogio[k] + gap;
        /* nao atravessa o fechamento: pula para 08:00 do proximo dia util */
        while (t % 1440 + dur > FECHA || t % 1440 < ABRE || !dia_util(t / 1440)) {
            long dia = t / 1440 + (t % 1440 >= ABRE ? 1 : 0);
            while (!dia_util(dia)) dia++;
            t = dia * 1440 + ABRE;
        }
        Reserva *r = &v[i];
        r->id = i + 1;
        snprintf(r->recurso, TAM_RECURSO, "Sala%d", k + 1);
        r->inicio = t;
        r->fim = t + dur;
        snprintf(r->responsavel, TAM_RESPONSAVEL, "%s",
                 RESPONSAVEIS[gerador_aleatorio(&st) % NRESP]);
        relogio[k] = r->fim;
        if (r->fim > fim_max) fim_max = r->fim;
    }
    free(relogio);
    if (ordem == ORDEM_ALEATORIA) gerador_embaralhar(v, n, &st);
    return fim_max;
}