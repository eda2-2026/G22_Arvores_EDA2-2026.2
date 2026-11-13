/* reservas — aplicacao de linha de comando do sistema de reservas.

   uso: reservas [-a avl|rb] [-e] [arquivo.csv]
     -a   arvore usada nas agendas (padrao: avl)
     -e   ecoa cada comando lido (util para rodar um roteiro: reservas -e < demo.txt)

   Digite 'ajuda' no prompt para ver os comandos. */

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "csv.h"
#include "sistema.h"

#define TAM_CMD    512
#define MAX_TOKENS 32

static void ajuda(void) {
    puts("Comandos (datas AAAA-MM-DD, horas HH:MM; use aspas para nomes com espaco):\n"
         "  reservar <recurso> <data> <inicio> <fim|+min> [responsavel...]\n"
         "      cria a reserva; em conflito mostra quem conflita e sugere o proximo horario livre\n"
         "      ex.: reservar Sala1 2026-11-16 14:00 15:30 Anna Clara\n"
         "           reservar Sala1 2026-11-16 23:00 +120 Plantao   (atravessa a meia-noite)\n"
         "  cancelar <id>\n"
         "  conflito <recurso> <data> <inicio> <fim|+min>   verifica sem reservar\n"
         "  listar <recurso|*> <data> [data_final]          agenda do(s) dia(s), em ordem\n"
         "  livre <recurso> <data> <hora> <duracao_min>     proximo horario livre a partir de data/hora\n"
         "  recursos                                        recursos, quantidade, altura e rotacoes\n"
         "  carregar <arquivo.csv>    |   salvar <arquivo.csv>\n"
         "  ajuda   |   sair");
}

/* ---------- leitura de comandos ---------- */

/* Quebra a linha em tokens separados por espaco; "texto com espaco" vira um token. */
static int tokenizar(char *linha, char **tok, int max) {
    int n = 0;
    char *p = linha;
    while (n < max) {
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
        if (*p == '\0') break;
        if (*p == '"') {
            tok[n++] = ++p;
            while (*p && *p != '"') p++;
        } else {
            tok[n++] = p;
            while (*p && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') p++;
        }
        if (*p == '\0') break;
        *p++ = '\0';
    }
    return n;
}

static int ler_inteiro(const char *txt, long minimo, long *v) {
    char *fim;
    long x = strtol(txt, &fim, 10);
    if (fim == txt || *fim != '\0' || x < minimo || x > INT_MAX) return 0;
    *v = x;
    return 1;
}

/* data + hora inicial + (HH:MM do mesmo dia | +minutos) -> [ini, fim) */
static int ler_intervalo(const char *data, const char *hini, const char *hfim,
                         long *ini, long *fim) {
    if (!reserva_minutos(data, hini, ini)) {
        printf("  data/hora invalida: %s %s (use AAAA-MM-DD HH:MM)\n", data, hini);
        return 0;
    }
    if (hfim[0] == '+') {
        long dur;
        if (!ler_inteiro(hfim + 1, 1, &dur)) {
            printf("  duracao invalida: %s (use +minutos, ex.: +90)\n", hfim);
            return 0;
        }
        *fim = *ini + dur;
        return 1;
    }
    if (!reserva_minutos(data, hfim, fim)) {
        printf("  hora final invalida: %s (use HH:MM ou +minutos)\n", hfim);
        return 0;
    }
    if (*fim <= *ini) {
        printf("  o fim (%s) deve ser depois do inicio (%s); para atravessar a "
               "meia-noite use +minutos\n", hfim, hini);
        return 0;
    }
    return 1;
}

static void juntar(char **tok, int n, char *buf, size_t tam) {
    buf[0] = '\0';
    for (int i = 0; i < n; i++) {
        if (i) strncat(buf, " ", tam - strlen(buf) - 1);
        strncat(buf, tok[i], tam - strlen(buf) - 1);
    }
}

static void imprimir_sugestao(const Agenda *a, long ini, long dur) {
    long t = agenda_proximo_livre(a, ini, dur);
    if (t < 0) return;
    char ti[TAM_DATAHORA], tf[TAM_DATAHORA];
    reserva_formatar(t, ti);
    reserva_formatar(t + dur, tf);
    printf("  sugestao: proximo horario livre com a mesma duracao: %s -> %s\n", ti, tf);
}

/* ---------- comandos ---------- */

static void cmd_reservar(Sistema *s, char **tok, int n) {
    if (n < 5) { puts("  uso: reservar <recurso> <data> <inicio> <fim|+min> [responsavel...]"); return; }
    long ini, fim;
    if (!ler_intervalo(tok[2], tok[3], tok[4], &ini, &fim)) return;
    char resp[TAM_CMD];
    juntar(tok + 5, n - 5, resp, sizeof resp);

    Reserva r, conf;
    ValidacaoReserva v = reserva_montar(&r, sistema_proximo_id(s), tok[1], ini, fim, resp);
    if (v != RV_OK) { printf("  reserva invalida: %s\n", reserva_erro(v)); return; }

    switch (sistema_reservar(s, r, &conf)) {
    case SIS_OK:
        printf("  reserva #%d criada:\n", r.id);
        reserva_imprimir(&r);
        break;
    case SIS_CONFLITO:
        puts("  CONFLITO com a reserva:");
        reserva_imprimir(&conf);
        imprimir_sugestao(sistema_agenda(s, r.recurso), ini, fim - ini);
        break;
    case SIS_ID_DUPLICADO: puts("  id ja existe");     break;
    case SIS_SEM_MEMORIA:  puts("  sem memoria");      break;
    case SIS_INVALIDA:     puts("  reserva invalida"); break;
    }
}

static void cmd_cancelar(Sistema *s, char **tok, int n) {
    long id;
    if (n != 2 || !ler_inteiro(tok[1], 1, &id)) { puts("  uso: cancelar <id>"); return; }
    if (sistema_cancelar(s, (int)id)) printf("  reserva #%ld cancelada\n", id);
    else                              printf("  nao existe reserva #%ld\n", id);
}

static void cmd_conflito(Sistema *s, char **tok, int n) {
    if (n != 5) { puts("  uso: conflito <recurso> <data> <inicio> <fim|+min>"); return; }
    long ini, fim;
    if (!ler_intervalo(tok[2], tok[3], tok[4], &ini, &fim)) return;
    const Agenda *a = sistema_agenda(s, tok[1]);
    Reserva c;
    if (a == NULL || !agenda_buscar_conflito(a, ini, fim, &c)) {
        puts("  livre: nenhum conflito nesse horario");
        return;
    }
    puts("  ocupado: conflita com");
    reserva_imprimir(&c);
    imprimir_sugestao(a, ini, fim - ini);
}

typedef struct { int n; } CtxListar;

static void listar_uma(const Reserva *r, void *ctx) {
    ((CtxListar *)ctx)->n++;
    reserva_imprimir(r);
}

static int listar_recurso(const Sistema *s, int i, long ini, long fim) {
    CtxListar c = {0};
    printf("  [%s]\n", sistema_recurso_nome(s, i));
    agenda_listar_periodo(sistema_recurso_agenda(s, i), ini, fim, listar_uma, &c);
    if (c.n == 0) puts("    (nenhuma reserva no periodo)");
    return c.n;
}

static void cmd_listar(Sistema *s, char **tok, int n) {
    if (n != 3 && n != 4) { puts("  uso: listar <recurso|*> <data> [data_final]"); return; }
    long ini, fim;
    if (!reserva_minutos(tok[2], "00:00", &ini) ||
        !reserva_minutos(n == 4 ? tok[3] : tok[2], "00:00", &fim)) {
        puts("  data invalida (use AAAA-MM-DD)");
        return;
    }
    fim += 1440;                          /* inclui o dia final inteiro */
    if (fim <= ini) { puts("  a data final deve ser igual ou posterior a inicial"); return; }

    int total = 0;
    if (strcmp(tok[1], "*") == 0) {
        if (sistema_num_recursos(s) == 0) puts("  (nenhum recurso cadastrado)");
        for (int i = 0; i < sistema_num_recursos(s); i++) total += listar_recurso(s, i, ini, fim);
    } else {
        int achou = 0;
        for (int i = 0; i < sistema_num_recursos(s); i++)
            if (strcmp(sistema_recurso_nome(s, i), tok[1]) == 0) {
                total += listar_recurso(s, i, ini, fim);
                achou = 1;
            }
        if (!achou) { printf("  recurso '%s' nao tem reservas\n", tok[1]); return; }
    }
    printf("  %d reserva(s)\n", total);
}

static void cmd_livre(Sistema *s, char **tok, int n) {
    long ini, dur;
    if (n != 5 || !ler_inteiro(tok[4], 1, &dur)) {
        puts("  uso: livre <recurso> <data> <hora> <duracao_min>");
        return;
    }
    if (!reserva_minutos(tok[2], tok[3], &ini)) { puts("  data/hora invalida"); return; }
    const Agenda *a = sistema_agenda(s, tok[1]);
    long t = a ? agenda_proximo_livre(a, ini, dur) : ini;
    if (t < 0) { puts("  nenhum horario livre encontrado"); return; }
    char ti[TAM_DATAHORA], tf[TAM_DATAHORA];
    reserva_formatar(t, ti);
    reserva_formatar(t + dur, tf);
    printf("  proximo horario livre em %s: %s -> %s\n", tok[1], ti, tf);
}

static void cmd_recursos(const Sistema *s) {
    printf("  arvore: %s | %d reserva(s) em %d recurso(s)\n",
           sistema_nome_arvore(sistema_tipo(s)), sistema_total_reservas(s),
           sistema_num_recursos(s));
    if (sistema_num_recursos(s) == 0) return;
    printf("  %-20s %10s %8s %10s\n", "recurso", "reservas", "altura", "rotacoes");
    for (int i = 0; i < sistema_num_recursos(s); i++) {
        const Agenda *a = sistema_recurso_agenda(s, i);
        printf("  %-20s %10d %8d %10ld\n", sistema_recurso_nome(s, i),
               agenda_tamanho(a), agenda_altura(a), agenda_rotacoes(a));
    }
}

static void cmd_carregar(Sistema *s, char **tok, int n) {
    if (n != 2) { puts("  uso: carregar <arquivo.csv>"); return; }
    ResumoCsv r;
    if (!csv_carregar(s, tok[1], stdout, &r)) return;
    printf("  %s: %d linha(s), %d aceita(s), %d rejeitada(s) (%d por conflito)\n",
           tok[1], r.lidas, r.aceitas, r.rejeitadas, r.conflitos);
}

static void cmd_salvar(Sistema *s, char **tok, int n) {
    if (n != 2) { puts("  uso: salvar <arquivo.csv>"); return; }
    int q = csv_salvar(s, tok[1]);
    if (q < 0) printf("  nao foi possivel gravar '%s'\n", tok[1]);
    else       printf("  %d reserva(s) gravada(s) em %s\n", q, tok[1]);
}

/* ---------- laço principal ---------- */

int main(int argc, char **argv) {
    TipoArvore tipo = ARVORE_AVL;
    int eco = 0;
    const char *arquivo = NULL;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-a") == 0 && i + 1 < argc) {
            i++;
            if (strcmp(argv[i], "avl") == 0)     tipo = ARVORE_AVL;
            else if (strcmp(argv[i], "rb") == 0) tipo = ARVORE_RB;
            else { fprintf(stderr, "arvore desconhecida: %s (use avl ou rb)\n", argv[i]); return 1; }
        } else if (strcmp(argv[i], "-e") == 0) {
            eco = 1;
        } else if (argv[i][0] != '-' && arquivo == NULL) {
            arquivo = argv[i];
        } else {
            fprintf(stderr, "uso: reservas [-a avl|rb] [-e] [arquivo.csv]\n");
            return 1;
        }
    }

    Sistema *s = sistema_criar(tipo);
    if (s == NULL) { fprintf(stderr, "sem memoria\n"); return 1; }
    printf("Sistema de Reservas - agendas em arvore %s. Digite 'ajuda'.\n",
           sistema_nome_arvore(tipo));
    if (arquivo) {
        char *tok[2] = {"carregar", (char *)arquivo};
        cmd_carregar(s, tok, 2);
    }

    char linha[TAM_CMD];
    char *tok[MAX_TOKENS];
    for (;;) {
        printf("> ");
        fflush(stdout);
        if (fgets(linha, sizeof linha, stdin) == NULL) { putchar('\n'); break; }
        if (strchr(linha, '\n') == NULL && !feof(stdin)) {   /* linha longa: descarta o resto */
            int c;
            while ((c = getchar()) != EOF && c != '\n') {}
            puts("  comando longo demais");
            continue;
        }
        if (eco) fputs(linha, stdout);
        int n = tokenizar(linha, tok, MAX_TOKENS);
        if (n == 0 || tok[0][0] == '#') continue;

        const char *c = tok[0];
        if      (strcmp(c, "reservar") == 0) cmd_reservar(s, tok, n);
        else if (strcmp(c, "cancelar") == 0) cmd_cancelar(s, tok, n);
        else if (strcmp(c, "conflito") == 0) cmd_conflito(s, tok, n);
        else if (strcmp(c, "listar") == 0)   cmd_listar(s, tok, n);
        else if (strcmp(c, "livre") == 0)    cmd_livre(s, tok, n);
        else if (strcmp(c, "recursos") == 0) cmd_recursos(s);
        else if (strcmp(c, "carregar") == 0) cmd_carregar(s, tok, n);
        else if (strcmp(c, "salvar") == 0)   cmd_salvar(s, tok, n);
        else if (strcmp(c, "ajuda") == 0)    ajuda();
        else if (strcmp(c, "sair") == 0)     break;
        else printf("  comando desconhecido: '%s' (digite 'ajuda')\n", c);
    }
    sistema_destruir(s);
    return 0;
}