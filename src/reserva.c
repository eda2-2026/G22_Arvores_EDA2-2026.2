#include "reserva.h"
#include <stdio.h>
#include <string.h>

/* ---------- calendario ----------
   dias_desde_epoca: numero de dias de 1970-01-01 ate a data. 
   Usamos aritmetica pura em vez de
   mktime() para nao depender de fuso horario nem de horario de verao. */

static int bissexto(long a) { return (a % 4 == 0 && a % 100 != 0) || a % 400 == 0; }

static int dias_no_mes(long a, int m) {
    static const int d[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    return (m == 2 && bissexto(a)) ? 29 : d[m - 1];
}

static long dias_desde_epoca(long a, int m, int d) {
    a -= m <= 2;
    long era = (a >= 0 ? a : a - 399) / 400;
    long yoe = a - era * 400;                                   /* [0, 399]   */
    long doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;  /* [0, 365]   */
    long doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;           /* [0, 146096] */
    return era * 146097 + doe - 719468;
}

static void data_de_dias(long z, long *a, int *m, int *d) {
    z += 719468;
    long era = (z >= 0 ? z : z - 146096) / 146097;
    long doe = z - era * 146097;
    long yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    long doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    long mp  = (5 * doy + 2) / 153;
    *d = (int)(doy - (153 * mp + 2) / 5 + 1);
    *m = (int)(mp < 10 ? mp + 3 : mp - 9);
    *a = yoe + era * 400 + (*m <= 2);
}

/* ---------- conversoes ---------- */

int reserva_minutos(const char *data, const char *hora, long *minutos) {
    int a, m, d, h, mi;
    char resto;
    if (data == NULL || hora == NULL) return 0;
    /* %c no final pega lixo depois do campo (ex.: "2026-11-16x") */
    if (sscanf(data, "%4d-%2d-%2d%c", &a, &m, &d, &resto) != 3) return 0;
    if (sscanf(hora, "%2d:%2d%c", &h, &mi, &resto) != 2) return 0;
    if (a < 1970 || a > 9999 || m < 1 || m > 12) return 0;
    if (d < 1 || d > dias_no_mes(a, m)) return 0;
    if (h < 0 || h > 23 || mi < 0 || mi > 59) return 0;
    *minutos = dias_desde_epoca(a, m, d) * 1440L + h * 60L + mi;
    return 1;
}

int reserva_ler_datahora(const char *texto, long *minutos) {
    char data[16], hora[8], resto;
    if (texto == NULL) return 0;
    if (sscanf(texto, " %15s %7s %c", data, hora, &resto) != 2) return 0;
    return reserva_minutos(data, hora, minutos);
}

void reserva_formatar(long minutos, char *buf) {
    long dias = minutos / 1440, rest = minutos % 1440;
    if (rest < 0) { rest += 1440; dias--; }
    long a; int m, d;
    data_de_dias(dias, &a, &m, &d);
    if (a < 0) a = 0;
    if (a > 9999) a = 9999;
    /* buffer folgado para o compilador nao acusar truncamento; o texto
       final sempre tem 16 caracteres */
    char tmp[64];
    snprintf(tmp, sizeof tmp, "%04ld-%02d-%02d %02ld:%02ld", a, m, d, rest / 60, rest % 60);
    memcpy(buf, tmp, TAM_DATAHORA - 1);
    buf[TAM_DATAHORA - 1] = '\0';
}

/* ---------- construcao / validacao ---------- */

ValidacaoReserva reserva_montar(Reserva *r, int id, const char *recurso,
                                long inicio, long fim, const char *responsavel) {
    if (recurso == NULL) recurso = "";
    if (responsavel == NULL) responsavel = "";
    if (id <= 0) return RV_ID;
    if (inicio >= fim) return RV_INTERVALO;
    size_t lr = strlen(recurso), lp = strlen(responsavel);
    if (lr == 0 || lr >= TAM_RECURSO) return RV_RECURSO;
    if (lp >= TAM_RESPONSAVEL) return RV_RESPONSAVEL;

    memset(r, 0, sizeof *r);
    r->id = id;
    memcpy(r->recurso, recurso, lr + 1);
    r->inicio = inicio;
    r->fim = fim;
    memcpy(r->responsavel, responsavel, lp + 1);
    return RV_OK;
}

const char *reserva_erro(ValidacaoReserva v) {
    switch (v) {
    case RV_OK:          return "ok";
    case RV_ID:          return "id deve ser um inteiro positivo";
    case RV_INTERVALO:   return "o fim deve ser posterior ao inicio";
    case RV_RECURSO:     return "recurso vazio ou com mais de 39 caracteres";
    case RV_RESPONSAVEL: return "responsavel com mais de 59 caracteres";
    }
    return "erro desconhecido";
}

void reserva_imprimir(const Reserva *r) {
    char ini[TAM_DATAHORA], fim[TAM_DATAHORA];
    reserva_formatar(r->inicio, ini);
    reserva_formatar(r->fim, fim);
    /* se termina no mesmo dia, mostra so a hora final */
    const char *fim_txt = strncmp(ini, fim, 10) == 0 ? fim + 11 : fim;
    printf("  #%-5d %-14s %s -> %-16s %s\n", r->id, r->recurso, ini, fim_txt,
           r->responsavel);
}