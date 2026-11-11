#include "reserva.h"
#include <stdio.h>
#include <string.h>

static int total = 0, passaram = 0;
static void checa(const char *nome, int cond) {
    total++;
    if (cond) { passaram++; printf("  [ok]     %s\n", nome); }
    else      {             printf("  [FALHOU] %s\n", nome); }
}

int main(void) {
    printf("== Testes do modelo Reserva (data/hora e validacao) ==\n");
    long m;
    char buf[TAM_DATAHORA];

    checa("epoca = 0 minutos", reserva_minutos("1970-01-01", "00:00", &m) && m == 0);
    checa("1970-01-02 01:30 = 1440+90",
          reserva_minutos("1970-01-02", "01:30", &m) && m == 1530);

    /* ida e volta em datas variadas, inclusive bissextos e virada de ano */
    const char *datas[] = {"2026-11-16 14:00", "2024-02-29 23:59", "2000-02-29 00:00",
                           "2026-12-31 23:59", "2027-01-01 00:00", "2100-03-01 07:05"};
    int ida_volta = 1;
    for (int i = 0; i < 6; i++) {
        if (!reserva_ler_datahora(datas[i], &m)) ida_volta = 0;
        reserva_formatar(m, buf);
        if (strcmp(buf, datas[i]) != 0) ida_volta = 0;
    }
    checa("ler -> formatar devolve o mesmo texto", ida_volta);

    /* minutos consecutivos e monotonicos ao longo de varios anos */
    long antes = -1; int mono = 1;
    for (int dia = 0; dia < 3000; dia += 7) {
        long t = (long)dia * 1440 + 20000000L;
        reserva_formatar(t, buf);
        if (!reserva_ler_datahora(buf, &m) || m != t || m <= antes) mono = 0;
        antes = m;
    }
    checa("formatar -> ler e identidade (3000 dias)", mono);

    checa("rejeita 29/02 em ano nao bissexto", !reserva_minutos("2026-02-29", "10:00", &m));
    checa("rejeita 1900-02-29 (secular)", !reserva_minutos("1900-02-29", "10:00", &m));
    checa("rejeita 31/04", !reserva_minutos("2026-04-31", "10:00", &m));
    checa("rejeita mes 13", !reserva_minutos("2026-13-01", "10:00", &m));
    checa("rejeita hora 24:00", !reserva_minutos("2026-11-16", "24:00", &m));
    checa("rejeita minuto 60", !reserva_minutos("2026-11-16", "10:60", &m));
    checa("rejeita lixo apos a data", !reserva_minutos("2026-11-16x", "10:00", &m));
    checa("rejeita formato errado", !reserva_minutos("16/11/2026", "10:00", &m));
    checa("rejeita texto com sobra", !reserva_ler_datahora("2026-11-16 10:00 x", &m));

    Reserva r;
    checa("montar valida", reserva_montar(&r, 1, "Sala1", 10, 20, "Ana") == RV_OK
                           && r.id == 1 && strcmp(r.recurso, "Sala1") == 0);
    checa("montar: id <= 0", reserva_montar(&r, 0, "Sala1", 10, 20, "") == RV_ID);
    checa("montar: inicio >= fim", reserva_montar(&r, 1, "Sala1", 20, 20, "") == RV_INTERVALO);
    checa("montar: recurso vazio", reserva_montar(&r, 1, "", 10, 20, "") == RV_RECURSO);
    char longo[100];
    memset(longo, 'x', 99); longo[99] = '\0';
    checa("montar: recurso longo nao trunca", reserva_montar(&r, 1, longo, 10, 20, "") == RV_RECURSO);
    checa("montar: responsavel longo", reserva_montar(&r, 1, "S", 10, 20, longo) == RV_RESPONSAVEL);

    printf("\n== Resumo: %d/%d passaram ==\n", passaram, total);
    if (passaram == total) { printf("RESULTADO: SUCESSO\n"); return 0; }
    printf("RESULTADO: FALHA\n");
    return 1;
}