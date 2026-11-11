
#include "csv.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int total = 0, passaram = 0;
static void checa(const char *nome, int cond) {
    total++;
    if (cond) { passaram++; printf("  [ok]     %s\n", nome); }
    else      {             printf("  [FALHOU] %s\n", nome); }
}

#define ARQ  "test_tmp_reservas.csv"
#define ARQ2 "test_tmp_salvo.csv"

static void escrever(const char *caminho, const char *conteudo) {
    FILE *f = fopen(caminho, "wb");
    fputs(conteudo, f);
    fclose(f);
}

static void contar(const Reserva *r, void *ctx) { (void)r; (*(int *)ctx)++; }

static void testar_dividir(void) {
    char l1[] = "1, Sala 1 ,\"Silva, Ana\",\"diz \"\"oi\"\"\",";
    char *c[8];
    int n = csv_dividir(l1, ',', c, 8);
    checa("dividir: 5 campos (inclui vazio no fim)", n == 5);
    checa("dividir: apara espacos", n == 5 && strcmp(c[1], "Sala 1") == 0);
    checa("dividir: virgula dentro de aspas", n == 5 && strcmp(c[2], "Silva, Ana") == 0);
    checa("dividir: aspas escapadas", n == 5 && strcmp(c[3], "diz \"oi\"") == 0);
    checa("dividir: campo vazio", n == 5 && c[4][0] == '\0');
    char l2[] = "a;\"sem fechar";
    checa("dividir: aspas sem fechar -> -1", csv_dividir(l2, ';', c, 8) == -1);
}

static void testar_sistema(void) {
    Sistema *s = sistema_criar(ARVORE_RB);
    Reserva r, conf;
    reserva_montar(&r, 10, "Sala1", 600, 660, "A");
    checa("sistema: reserva ok", sistema_reservar(s, r, NULL) == SIS_OK);
    reserva_montar(&r, 10, "Sala2", 600, 660, "B");
    checa("sistema: id duplicado (mesmo em outro recurso)",
          sistema_reservar(s, r, NULL) == SIS_ID_DUPLICADO);
    reserva_montar(&r, 11, "Sala2", 600, 660, "B");
    checa("sistema: mesmo horario em outro recurso nao conflita",
          sistema_reservar(s, r, NULL) == SIS_OK);
    reserva_montar(&r, 12, "Sala1", 630, 700, "C");
    checa("sistema: conflito no mesmo recurso",
          sistema_reservar(s, r, &conf) == SIS_CONFLITO && conf.id == 10);
    reserva_montar(&r, 13, "Sala1", 660, 720, "D");
    checa("sistema: encostado [fim, ...) nao conflita", sistema_reservar(s, r, NULL) == SIS_OK);
    checa("sistema: proximo id", sistema_proximo_id(s) == 14);
    checa("sistema: total 3", sistema_total_reservas(s) == 3);
    checa("sistema: cancelar existente", sistema_cancelar(s, 10) == 1);
    checa("sistema: cancelar de novo devolve 0", sistema_cancelar(s, 10) == 0);
    checa("sistema: id cancelado volta a ser livre", !sistema_existe_id(s, 10));
    reserva_montar(&r, 12, "Sala1", 630, 660, "C");
    checa("sistema: horario liberado aceita nova reserva", sistema_reservar(s, r, NULL) == SIS_OK);
    checa("sistema: 2 recursos", sistema_num_recursos(s) == 2);

    /* muitos ids: forca o crescimento do mapa e remocoes com lapides */
    int ok = 1;
    for (int i = 1000; i < 21000; i++) {
        reserva_montar(&r, i, "Massa", (long)i * 100, (long)i * 100 + 50, "");
        if (sistema_reservar(s, r, NULL) != SIS_OK) ok = 0;
    }
    for (int i = 1000; i < 21000; i += 2) if (sistema_cancelar(s, i) != 1) ok = 0;
    for (int i = 1000; i < 21000; i++) if (sistema_existe_id(s, i) != (i % 2)) ok = 0;
    checa("sistema: 20000 ids, metade cancelada, mapa consistente", ok);
    sistema_destruir(s);
}

static void testar_carga(void) {
    escrever(ARQ,
        "\xEF\xBB\xBFresponsavel;fim;inicio;recurso;id\r\n"     /* BOM, CRLF, ordem trocada */
        "# comentario\r\n"
        "Ana;2026-11-16 09:00;2026-11-16 08:00;Sala1;1\r\n"
        "\r\n"
        "\"Silva; Joao\";2026-11-16T10:00;2026-11-16T09:00;Sala1;2\r\n"   /* encosta: ok */
        "Bia;2026-11-16 09:30;2026-11-16 08:30;Sala1;3\r\n"              /* conflito c/ 1 */
        "Caio;2026-11-16 09:30;2026-11-16 08:30;Sala2;4\r\n"             /* outro recurso */
        "Duda;2026-11-16 12:00;2026-11-16 11:00;Sala1;1\r\n"             /* id repetido */
        "Eva;2026-11-16 12:00;2026-11-31 11:00;Sala1;5\r\n"              /* data invalida */
        "Fabi;2026-11-16 11:00;2026-11-16 12:00;Sala1;6\r\n"             /* fim < inicio */
        "Gil;2026-11-16 13:00;2026-11-16 12:00;;7\r\n"                   /* sem recurso */
        "Hugo;2026-11-16 13:00\r\n"                                       /* faltam campos */
        "Ivo;2026-11-16 15:00;2026-11-16 14:00;Sala1;abc\r\n"            /* id invalido */
        "Juca;2026-11-16 16:00;2026-11-16 15:00;Sala1;\r\n");            /* id automatico */

    Sistema *s = sistema_criar(ARVORE_AVL);
    ResumoCsv r;
    printf("     (relatorio da carga abaixo)\n");
    int ok = csv_carregar(s, ARQ, stdout, &r);
    checa("carga: arquivo lido", ok == 1);
    checa("carga: 11 linhas de dados", r.lidas == 11);
    checa("carga: 4 aceitas", r.aceitas == 4);
    checa("carga: 7 rejeitadas, 1 por conflito", r.rejeitadas == 7 && r.conflitos == 1);
    checa("carga: aspas com separador preservadas", sistema_existe_id(s, 2));
    checa("carga: id automatico = maior + 1", sistema_existe_id(s, 5));

    /* salvar -> recarregar produz o mesmo conjunto */
    checa("salvar: grava 4 reservas", csv_salvar(s, ARQ2) == 4);
    Sistema *s2 = sistema_criar(ARVORE_RB);
    ResumoCsv r2;
    csv_carregar(s2, ARQ2, NULL, &r2);
    int n1 = 0, n2 = 0;
    agenda_listar_periodo(sistema_agenda(s2, "Sala1"), 0, 100000000L, contar, &n1);
    agenda_listar_periodo(sistema_agenda(s2, "Sala2"), 0, 100000000L, contar, &n2);
    checa("salvar -> carregar: 4 aceitas, nenhuma rejeitada", r2.aceitas == 4 && r2.rejeitadas == 0);
    checa("salvar -> carregar: 3 em Sala1 e 1 em Sala2", n1 == 3 && n2 == 1);

    escrever(ARQ, "nome,quando\nx,y\n");
    checa("cabecalho sem colunas obrigatorias e recusado", csv_carregar(s2, ARQ, NULL, NULL) == 0);
    checa("arquivo inexistente e recusado", csv_carregar(s2, "nao_existe.csv", NULL, NULL) == 0);

    sistema_destruir(s);
    sistema_destruir(s2);
    remove(ARQ);
    remove(ARQ2);
}

int main(void) {
    printf("== Testes de CSV e do Sistema (varios recursos) ==\n");
    testar_dividir();
    testar_sistema();
    testar_carga();
    printf("\n== Resumo: %d/%d passaram ==\n", passaram, total);
    if (passaram == total) { printf("RESULTADO: SUCESSO\n"); return 0; }
    printf("RESULTADO: FALHA\n");
    return 1;
}