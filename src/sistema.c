#include "sistema.h"
#include <stdlib.h>
#include <string.h>

/* ---------- mapa id -> indice do recurso ----------
   Tabela hash com enderecamento aberto (sondagem linear). Chave 0 marca
   posicao vazia e -1 marca posicao removida (ids validos sao > 0). Mantem o
   fator de carga (ocupadas + removidas) abaixo de 0.5 e cresce dobrando. */

#define VAZIO     0
#define REMOVIDO (-1)

typedef struct {
    int *chaves;
    int *valores;
    int  cap;      /* potencia de 2 */
    int  usados;   /* chaves validas */
    int  sujos;    /* chaves validas + removidas */
} MapaIds;

static unsigned espalhar(int id) {
    unsigned x = (unsigned)id;
    x ^= x >> 16; x *= 0x45d9f3bu; x ^= x >> 16;
    return x;
}

static int mapa_iniciar(MapaIds *m, int cap) {
    m->chaves = calloc((size_t)cap, sizeof(int));
    m->valores = malloc((size_t)cap * sizeof(int));
    if (m->chaves == NULL || m->valores == NULL) {
        free(m->chaves); free(m->valores);
        return 0;
    }
    m->cap = cap;
    m->usados = m->sujos = 0;
    return 1;
}

static int mapa_buscar(const MapaIds *m, int id) {   /* indice da posicao ou -1 */
    unsigned mask = (unsigned)m->cap - 1;
    for (unsigned i = espalhar(id) & mask;; i = (i + 1) & mask) {
        if (m->chaves[i] == VAZIO) return -1;
        if (m->chaves[i] == id) return (int)i;
    }
}

static void mapa_colocar_sem_crescer(MapaIds *m, int id, int valor) {
    unsigned mask = (unsigned)m->cap - 1;
    unsigned i = espalhar(id) & mask;
    while (m->chaves[i] != VAZIO && m->chaves[i] != REMOVIDO) i = (i + 1) & mask;
    if (m->chaves[i] == VAZIO) m->sujos++;
    m->chaves[i] = id;
    m->valores[i] = valor;
    m->usados++;
}

static int mapa_colocar(MapaIds *m, int id, int valor) {
    if (2 * (m->sujos + 1) > m->cap) {               /* rehash */
        MapaIds novo;
        int cap = m->cap;
        while (4 * (m->usados + 1) > cap) cap *= 2;   /* <= 25% apos o rehash */
        if (!mapa_iniciar(&novo, cap)) return 0;
        for (int k = 0; k < m->cap; k++)
            if (m->chaves[k] > 0) mapa_colocar_sem_crescer(&novo, m->chaves[k], m->valores[k]);
        free(m->chaves); free(m->valores);
        *m = novo;
    }
    mapa_colocar_sem_crescer(m, id, valor);
    return 1;
}

static void mapa_remover(MapaIds *m, int pos) {
    m->chaves[pos] = REMOVIDO;
    m->usados--;
}

/* ---------- sistema ---------- */

typedef struct {
    char    nome[TAM_RECURSO];
    Agenda *agenda;
} Recurso;

struct Sistema {
    TipoArvore tipo;
    Recurso   *recursos;
    int        nrec, caprec;
    MapaIds    ids;
    int        maior_id;
    int        total;
};

Sistema *sistema_criar(TipoArvore tipo) {
    Sistema *s = calloc(1, sizeof(Sistema));
    if (s == NULL) return NULL;
    s->tipo = tipo;
    if (!mapa_iniciar(&s->ids, 64)) { free(s); return NULL; }
    return s;
}

void sistema_destruir(Sistema *s) {
    if (s == NULL) return;
    for (int i = 0; i < s->nrec; i++) agenda_destruir(s->recursos[i].agenda);
    free(s->recursos);
    free(s->ids.chaves);
    free(s->ids.valores);
    free(s);
}

TipoArvore sistema_tipo(const Sistema *s) { return s->tipo; }

const char *sistema_nome_arvore(TipoArvore tipo) {
    return tipo == ARVORE_AVL ? "AVL" : "Rubro-Negra";
}

static int achar_recurso(const Sistema *s, const char *nome) {
    for (int i = 0; i < s->nrec; i++)     /* poucos recursos: busca linear basta */
        if (strcmp(s->recursos[i].nome, nome) == 0) return i;
    return -1;
}

static int criar_recurso(Sistema *s, const char *nome) {
    if (s->nrec == s->caprec) {
        int cap = s->caprec ? 2 * s->caprec : 8;
        Recurso *v = realloc(s->recursos, (size_t)cap * sizeof(Recurso));
        if (v == NULL) return -1;
        s->recursos = v;
        s->caprec = cap;
    }
    Agenda *a = s->tipo == ARVORE_AVL ? agenda_criar_avl() : agenda_criar_rb();
    if (a == NULL) return -1;
    Recurso *r = &s->recursos[s->nrec];
    strncpy(r->nome, nome, TAM_RECURSO - 1);
    r->nome[TAM_RECURSO - 1] = '\0';
    r->agenda = a;
    return s->nrec++;
}

ResultadoSistema sistema_reservar(Sistema *s, Reserva r, Reserva *conflito) {
    if (r.id <= 0 || r.inicio >= r.fim || r.recurso[0] == '\0') return SIS_INVALIDA;
    if (mapa_buscar(&s->ids, r.id) >= 0) return SIS_ID_DUPLICADO;

    int idx = achar_recurso(s, r.recurso);
    if (idx < 0) {
        /* so cria o recurso depois de saber que a reserva e valida */
        idx = criar_recurso(s, r.recurso);
        if (idx < 0) return SIS_SEM_MEMORIA;
    }
    switch (agenda_inserir(s->recursos[idx].agenda, r, conflito)) {
    case RESERVA_CONFLITO: return SIS_CONFLITO;
    case RESERVA_INVALIDA: return SIS_INVALIDA;
    case RESERVA_OK:       break;
    }
    if (!mapa_colocar(&s->ids, r.id, idx)) {
        agenda_cancelar(s->recursos[idx].agenda, r.id);   /* desfaz */
        return SIS_SEM_MEMORIA;
    }
    if (r.id > s->maior_id) s->maior_id = r.id;
    s->total++;
    return SIS_OK;
}

int sistema_cancelar(Sistema *s, int id) {
    int pos = mapa_buscar(&s->ids, id);
    if (pos < 0) return 0;
    int idx = s->ids.valores[pos];
    if (!agenda_cancelar(s->recursos[idx].agenda, id)) return 0;
    mapa_remover(&s->ids, pos);
    s->total--;
    return 1;
}

int sistema_existe_id(const Sistema *s, int id) {
    return id > 0 && mapa_buscar(&s->ids, id) >= 0;
}

int sistema_proximo_id(const Sistema *s) { return s->maior_id + 1; }

const Agenda *sistema_agenda(const Sistema *s, const char *recurso) {
    int i = achar_recurso(s, recurso);
    return i < 0 ? NULL : s->recursos[i].agenda;
}

int sistema_num_recursos(const Sistema *s) { return s->nrec; }
const char *sistema_recurso_nome(const Sistema *s, int i) { return s->recursos[i].nome; }
const Agenda *sistema_recurso_agenda(const Sistema *s, int i) { return s->recursos[i].agenda; }
int sistema_total_reservas(const Sistema *s) { return s->total; }