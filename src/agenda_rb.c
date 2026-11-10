#include <stdlib.h>
#include "agenda_impl.h"

typedef enum { VERMELHO, PRETO } Cor;

typedef struct NoRB {
    Reserva r;
    Cor cor;
    struct NoRB *esq, *dir, *pai;
} NoRB;

typedef struct {
    NoRB *raiz;
    NoRB *nil;      /* sentinela (preto) compartilhado pelas folhas e pela raiz->pai */
    int   tamanho;
    long  rotacoes;
} ArvoreRB;

/* ---------- rotacoes (CLRS) ---------- */

static void rot_esq(ArvoreRB *t, NoRB *x) {
    NoRB *y = x->dir;
    x->dir = y->esq;
    if (y->esq != t->nil) y->esq->pai = x;
    y->pai = x->pai;
    if (x->pai == t->nil)        t->raiz = y;
    else if (x == x->pai->esq)   x->pai->esq = y;
    else                         x->pai->dir = y;
    y->esq = x;
    x->pai = y;
    t->rotacoes++;
}

static void rot_dir(ArvoreRB *t, NoRB *x) {
    NoRB *y = x->esq;
    x->esq = y->dir;
    if (y->dir != t->nil) y->dir->pai = x;
    y->pai = x->pai;
    if (x->pai == t->nil)        t->raiz = y;
    else if (x == x->pai->dir)   x->pai->dir = y;
    else                         x->pai->esq = y;
    y->dir = x;
    x->pai = y;
    t->rotacoes++;
}

/* ---------- insercao (CLRS) ---------- */

static void inserir_fixup(ArvoreRB *t, NoRB *z) {
    while (z->pai->cor == VERMELHO) {
        if (z->pai == z->pai->pai->esq) {
            NoRB *y = z->pai->pai->dir;              /* tio */
            if (y->cor == VERMELHO) {                /* caso 1: tio vermelho */
                z->pai->cor = PRETO;
                y->cor = PRETO;
                z->pai->pai->cor = VERMELHO;
                z = z->pai->pai;
            } else {
                if (z == z->pai->dir) {              /* caso 2 */
                    z = z->pai;
                    rot_esq(t, z);
                }
                z->pai->cor = PRETO;                 /* caso 3 */
                z->pai->pai->cor = VERMELHO;
                rot_dir(t, z->pai->pai);
            }
        } else {                                     /* espelho */
            NoRB *y = z->pai->pai->esq;
            if (y->cor == VERMELHO) {
                z->pai->cor = PRETO;
                y->cor = PRETO;
                z->pai->pai->cor = VERMELHO;
                z = z->pai->pai;
            } else {
                if (z == z->pai->esq) {
                    z = z->pai;
                    rot_dir(t, z);
                }
                z->pai->cor = PRETO;
                z->pai->pai->cor = VERMELHO;
                rot_esq(t, z->pai->pai);
            }
        }
    }
    t->raiz->cor = PRETO;
}

static NoRB *novo_no(ArvoreRB *t, Reserva r) {
    NoRB *no = malloc(sizeof(NoRB));
    if (no == NULL) return NULL;
    no->r = r;
    no->cor = VERMELHO;
    no->esq = no->dir = no->pai = t->nil;
    return no;
}

static int inserir_rb(ArvoreRB *t, Reserva r) {
    NoRB *z = novo_no(t, r);
    if (z == NULL) return 0;
    NoRB *y = t->nil, *x = t->raiz;
    while (x != t->nil) {
        y = x;
        if (z->r.inicio < x->r.inicio) x = x->esq;
        else                           x = x->dir;
    }
    z->pai = y;
    if (y == t->nil)                   t->raiz = z;
    else if (z->r.inicio < y->r.inicio) y->esq = z;
    else                               y->dir = z;
    inserir_fixup(t, z);
    return 1;
}

/* ---------- remocao (CLRS) ---------- */

static void transplantar(ArvoreRB *t, NoRB *u, NoRB *v) {
    if (u->pai == t->nil)        t->raiz = v;
    else if (u == u->pai->esq)   u->pai->esq = v;
    else                         u->pai->dir = v;
    v->pai = u->pai;
}

static NoRB *minimo(ArvoreRB *t, NoRB *x) {
    while (x->esq != t->nil) x = x->esq;
    return x;
}

static void remover_fixup(ArvoreRB *t, NoRB *x) {
    while (x != t->raiz && x->cor == PRETO) {
        if (x == x->pai->esq) {
            NoRB *w = x->pai->dir;                   /* irmao */
            if (w->cor == VERMELHO) {                /* caso 1 */
                w->cor = PRETO;
                x->pai->cor = VERMELHO;
                rot_esq(t, x->pai);
                w = x->pai->dir;
            }
            if (w->esq->cor == PRETO && w->dir->cor == PRETO) {  /* caso 2 */
                w->cor = VERMELHO;
                x = x->pai;
            } else {
                if (w->dir->cor == PRETO) {          /* caso 3 */
                    w->esq->cor = PRETO;
                    w->cor = VERMELHO;
                    rot_dir(t, w);
                    w = x->pai->dir;
                }
                w->cor = x->pai->cor;                /* caso 4 */
                x->pai->cor = PRETO;
                w->dir->cor = PRETO;
                rot_esq(t, x->pai);
                x = t->raiz;
            }
        } else {                                     /* espelho */
            NoRB *w = x->pai->esq;
            if (w->cor == VERMELHO) {
                w->cor = PRETO;
                x->pai->cor = VERMELHO;
                rot_dir(t, x->pai);
                w = x->pai->esq;
            }
            if (w->dir->cor == PRETO && w->esq->cor == PRETO) {
                w->cor = VERMELHO;
                x = x->pai;
            } else {
                if (w->esq->cor == PRETO) {
                    w->dir->cor = PRETO;
                    w->cor = VERMELHO;
                    rot_esq(t, w);
                    w = x->pai->esq;
                }
                w->cor = x->pai->cor;
                x->pai->cor = PRETO;
                w->esq->cor = PRETO;
                rot_dir(t, x->pai);
                x = t->raiz;
            }
        }
    }
    x->cor = PRETO;
}

static void remover_no(ArvoreRB *t, NoRB *z) {
    NoRB *y = z;
    NoRB *x;
    Cor cor_original = y->cor;
    if (z->esq == t->nil) {
        x = z->dir;
        transplantar(t, z, z->dir);
    } else if (z->dir == t->nil) {
        x = z->esq;
        transplantar(t, z, z->esq);
    } else {
        y = minimo(t, z->dir);
        cor_original = y->cor;
        x = y->dir;
        if (y->pai == z) {
            x->pai = y;                              /* vale ate para o sentinela */
        } else {
            transplantar(t, y, y->dir);
            y->dir = z->dir;
            y->dir->pai = y;
        }
        transplantar(t, z, y);
        y->esq = z->esq;
        y->esq->pai = y;
        y->cor = z->cor;
    }
    free(z);
    if (cor_original == PRETO) remover_fixup(t, x);
}

/* ---------- busca de conflito (O(altura)) ----------
   Reservas nunca se sobrepoem => totalmente ordenadas por inicio e por fim.
   Nova [i, f) conflita com [s, e) sse s < f && i < e. */
static NoRB *achar_conflito(ArvoreRB *t, long i, long f) {
    NoRB *no = t->raiz;
    while (no != t->nil) {
        if (no->r.inicio < f && i < no->r.fim) return no;
        if (f <= no->r.inicio) no = no->esq;
        else                   no = no->dir;
    }
    return NULL;
}

static NoRB *achar_por_id(ArvoreRB *t, NoRB *no, int id) {
    if (no == t->nil) return NULL;
    if (no->r.id == id) return no;
    NoRB *e = achar_por_id(t, no->esq, id);
    if (e) return e;
    return achar_por_id(t, no->dir, id);
}

static void listar_no(const ArvoreRB *t, const NoRB *no, long i, long f,
                      void (*visita)(const Reserva *, void *), void *ctx) {
    if (no == t->nil) return;
    if (no->r.inicio >= f) { listar_no(t, no->esq, i, f, visita, ctx); return; }
    listar_no(t, no->esq, i, f, visita, ctx);
    if (i < no->r.fim) visita(&no->r, ctx);
    listar_no(t, no->dir, i, f, visita, ctx);
}

static int altura_no(const ArvoreRB *t, const NoRB *no) {
    if (no == t->nil) return 0;
    int e = altura_no(t, no->esq), d = altura_no(t, no->dir);
    return 1 + (e > d ? e : d);
}

static void destruir_subarvore(ArvoreRB *t, NoRB *no) {
    if (no == t->nil) return;
    destruir_subarvore(t, no->esq);
    destruir_subarvore(t, no->dir);
    free(no);
}

/* ---------- operacoes da interface ---------- */

static ResultadoReserva rb_inserir(void *self, Reserva r, Reserva *conflito) {
    ArvoreRB *t = self;
    if (r.inicio >= r.fim) return RESERVA_INVALIDA;
    NoRB *c = achar_conflito(t, r.inicio, r.fim);
    if (c) {
        if (conflito) *conflito = c->r;
        return RESERVA_CONFLITO;
    }
    if (!inserir_rb(t, r)) return RESERVA_INVALIDA;
    t->tamanho++;
    return RESERVA_OK;
}

static int rb_cancelar(void *self, int id) {
    ArvoreRB *t = self;
    NoRB *no = achar_por_id(t, t->raiz, id);
    if (no == NULL) return 0;
    remover_no(t, no);
    t->tamanho--;
    return 1;
}

static int rb_buscar_conflito(const void *self, long inicio, long fim, Reserva *conflito) {
    ArvoreRB *t = (ArvoreRB *)self;
    NoRB *c = achar_conflito(t, inicio, fim);
    if (c == NULL) return 0;
    if (conflito) *conflito = c->r;
    return 1;
}

static void rb_listar_periodo(const void *self, long inicio, long fim,
                              void (*visita)(const Reserva *, void *), void *ctx) {
    const ArvoreRB *t = self;
    listar_no(t, t->raiz, inicio, fim, visita, ctx);
}

static long rb_proximo_livre(const void *self, long apartir, long duracao) {
    ArvoreRB *t = (ArvoreRB *)self;
    long inicio = apartir;
    for (;;) {
        NoRB *c = achar_conflito(t, inicio, inicio + duracao);
        if (c == NULL) return inicio;
        inicio = c->r.fim;
    }
}

static int  rb_tamanho(const void *self)  { return ((const ArvoreRB *)self)->tamanho; }
static int  rb_altura(const void *self)   { const ArvoreRB *t = self; return altura_no(t, t->raiz); }
static long rb_rotacoes(const void *self) { return ((const ArvoreRB *)self)->rotacoes; }

static void rb_destruir(void *self) {
    ArvoreRB *t = self;
    destruir_subarvore(t, t->raiz);
    free(t->nil);
    free(t);
}

static const AgendaOps OPS_RB = {
    rb_inserir, rb_cancelar, rb_buscar_conflito, rb_listar_periodo,
    rb_proximo_livre, rb_tamanho, rb_altura, rb_rotacoes, rb_destruir
};

Agenda *agenda_criar_rb(void) {
    ArvoreRB *t = malloc(sizeof(ArvoreRB));
    if (t == NULL) return NULL;
    t->nil = malloc(sizeof(NoRB));
    if (t->nil == NULL) { free(t); return NULL; }
    t->nil->cor = PRETO;
    t->nil->esq = t->nil->dir = t->nil->pai = t->nil;
    t->raiz = t->nil;
    t->tamanho = 0;
    t->rotacoes = 0;
    return agenda_envolver(&OPS_RB, t);
}
