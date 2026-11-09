#include <stdlib.h>
#include "agenda_impl.h"

typedef struct NoAVL {
    Reserva r;
    struct NoAVL *esq, *dir;
    int altura;
} NoAVL;

typedef struct {
    NoAVL *raiz;
    int    tamanho;
    long   rotacoes;
} ArvoreAVL;

/* ---------- utilitarios ---------- */

static int altura_no(const NoAVL *no) { return no ? no->altura : 0; }

static int max_int(int a, int b) { return a > b ? a : b; }

static void atualiza_altura(NoAVL *no) {
    no->altura = 1 + max_int(altura_no(no->esq), altura_no(no->dir));
}

static int fator(const NoAVL *no) {
    return altura_no(no->esq) - altura_no(no->dir);
}

static NoAVL *novo_no(Reserva r) {
    NoAVL *no = malloc(sizeof(NoAVL));
    if (no == NULL) return NULL;
    no->r = r;
    no->esq = no->dir = NULL;
    no->altura = 1;
    return no;
}

/* ---------- rotacoes (cada rotacao simples incrementa o contador) ---------- */

static NoAVL *rot_dir(NoAVL *y, long *rot) {
    NoAVL *x = y->esq;
    NoAVL *t = x->dir;
    x->dir = y;
    y->esq = t;
    atualiza_altura(y);
    atualiza_altura(x);
    (*rot)++;
    return x;
}

static NoAVL *rot_esq(NoAVL *x, long *rot) {
    NoAVL *y = x->dir;
    NoAVL *t = y->esq;
    y->esq = x;
    x->dir = t;
    atualiza_altura(x);
    atualiza_altura(y);
    (*rot)++;
    return y;
}

static NoAVL *rebalancear(NoAVL *no, long *rot) {
    int b = fator(no);
    if (b > 1) {                       /* esquerda pesada */
        if (fator(no->esq) < 0) no->esq = rot_esq(no->esq, rot);  /* caso EsqDir */
        return rot_dir(no, rot);
    }
    if (b < -1) {                      /* direita pesada */
        if (fator(no->dir) > 0) no->dir = rot_dir(no->dir, rot);  /* caso DirEsq */
        return rot_esq(no, rot);
    }
    return no;
}

/* ---------- conflito (busca em O(altura)) ----------
   Como as reservas nunca se sobrepoem, elas ficam totalmente ordenadas por
   inicio (e por fim). Uma nova [i, f) conflita com [s, e) sse s < f && i < e. */
static const NoAVL *achar_conflito(const NoAVL *no, long i, long f) {
    while (no) {
        if (no->r.inicio < f && i < no->r.fim) return no;  /* sobrepoe */
        if (f <= no->r.inicio) no = no->esq;               /* nova toda a esquerda */
        else                   no = no->dir;
    }
    return NULL;
}

/* ---------- insercao ---------- */

static NoAVL *inserir_no(NoAVL *no, Reserva r, long *rot) {
    if (no == NULL) return novo_no(r);
    if (r.inicio < no->r.inicio) no->esq = inserir_no(no->esq, r, rot);
    else                         no->dir = inserir_no(no->dir, r, rot);
    atualiza_altura(no);
    return rebalancear(no, rot);
}

/* ---------- remocao por inicio (chave) ---------- */

static NoAVL *min_no(NoAVL *no) {
    while (no->esq) no = no->esq;
    return no;
}

static NoAVL *remover_no(NoAVL *no, long inicio, long *rot) {
    if (no == NULL) return NULL;
    if (inicio < no->r.inicio) {
        no->esq = remover_no(no->esq, inicio, rot);
    } else if (inicio > no->r.inicio) {
        no->dir = remover_no(no->dir, inicio, rot);
    } else {
        if (no->esq == NULL || no->dir == NULL) {
            NoAVL *filho = no->esq ? no->esq : no->dir;
            free(no);
            return filho;                       /* pode ser NULL */
        }
        NoAVL *suc = min_no(no->dir);           /* menor da subarvore direita */
        no->r = suc->r;
        no->dir = remover_no(no->dir, suc->r.inicio, rot);
    }
    atualiza_altura(no);
    return rebalancear(no, rot);
}

/* ---------- percurso por periodo (em ordem) ---------- */

static void listar_no(const NoAVL *no, long i, long f,
                      void (*visita)(const Reserva *, void *), void *ctx) {
    if (no == NULL) return;
    if (no->r.inicio >= f) {               /* este e tudo a direita comecam >= f */
        listar_no(no->esq, i, f, visita, ctx);
        return;
    }
    listar_no(no->esq, i, f, visita, ctx);
    if (i < no->r.fim) visita(&no->r, ctx); /* intersecta [i, f) */
    listar_no(no->dir, i, f, visita, ctx);
}

/* ---------- busca por id (para o cancelamento) ---------- */

static const NoAVL *achar_por_id(const NoAVL *no, int id) {
    if (no == NULL) return NULL;
    if (no->r.id == id) return no;
    const NoAVL *e = achar_por_id(no->esq, id);
    if (e) return e;
    return achar_por_id(no->dir, id);
}

static void destruir_no(NoAVL *no) {
    if (no == NULL) return;
    destruir_no(no->esq);
    destruir_no(no->dir);
    free(no);
}

/* ---------- operacoes da interface (AgendaOps) ---------- */

static ResultadoReserva avl_inserir(void *self, Reserva r, Reserva *conflito) {
    ArvoreAVL *t = self;
    if (r.inicio >= r.fim) return RESERVA_INVALIDA;
    const NoAVL *c = achar_conflito(t->raiz, r.inicio, r.fim);
    if (c) {
        if (conflito) *conflito = c->r;
        return RESERVA_CONFLITO;
    }
    t->raiz = inserir_no(t->raiz, r, &t->rotacoes);
    t->tamanho++;
    return RESERVA_OK;
}

static int avl_cancelar(void *self, int id) {
    ArvoreAVL *t = self;
    const NoAVL *no = achar_por_id(t->raiz, id);
    if (no == NULL) return 0;
    long inicio = no->r.inicio;
    t->raiz = remover_no(t->raiz, inicio, &t->rotacoes);
    t->tamanho--;
    return 1;
}

static int avl_buscar_conflito(const void *self, long inicio, long fim, Reserva *conflito) {
    const ArvoreAVL *t = self;
    const NoAVL *c = achar_conflito(t->raiz, inicio, fim);
    if (c == NULL) return 0;
    if (conflito) *conflito = c->r;
    return 1;
}

static void avl_listar_periodo(const void *self, long inicio, long fim,
                               void (*visita)(const Reserva *, void *), void *ctx) {
    const ArvoreAVL *t = self;
    listar_no(t->raiz, inicio, fim, visita, ctx);
}

static long avl_proximo_livre(const void *self, long apartir, long duracao) {
    const ArvoreAVL *t = self;
    long inicio = apartir;
    for (;;) {
        const NoAVL *c = achar_conflito(t->raiz, inicio, inicio + duracao);
        if (c == NULL) return inicio;      /* cabe em [inicio, inicio+duracao) */
        inicio = c->r.fim;                 /* pula para o fim do conflito */
    }
}

static int  avl_tamanho(const void *self)  { return ((const ArvoreAVL *)self)->tamanho; }
static int  avl_altura(const void *self)   { return altura_no(((const ArvoreAVL *)self)->raiz); }
static long avl_rotacoes(const void *self) { return ((const ArvoreAVL *)self)->rotacoes; }

static void avl_destruir(void *self) {
    ArvoreAVL *t = self;
    destruir_no(t->raiz);
    free(t);
}

static const AgendaOps OPS_AVL = {
    avl_inserir, avl_cancelar, avl_buscar_conflito, avl_listar_periodo,
    avl_proximo_livre, avl_tamanho, avl_altura, avl_rotacoes, avl_destruir
};

Agenda *agenda_criar_avl(void) {
    ArvoreAVL *t = malloc(sizeof(ArvoreAVL));
    if (t == NULL) return NULL;
    t->raiz = NULL;
    t->tamanho = 0;
    t->rotacoes = 0;
    return agenda_envolver(&OPS_AVL, t);
}
