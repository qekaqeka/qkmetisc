#include "eq_gen.h"
#include <stdlib.h>
#include "eq.h"
#include "gen_types.h"
#include <stdio.h>
#include <assert.h>
#include "rcmem.h"
#include <stdbool.h>
#include <string.h>

struct eq_gen_priv {
    int minlen;
    int maxlen;
    mpz_t maxnr;
    eq_flags_t allowed;
};

struct eq_task_priv {
    struct eq *eq;
    char *__rc text_rc;
    bool text_cached;
    mpz_t answer;
    bool answer_cached;
};


static void eq_free_text(char *text) {
    rcmem_put(text);
}

static struct question *eq_get_question(void *p) { 
    struct eq_task_priv *priv = p;

    if ( !priv->text_cached ) {
        assert(priv->text_rc == NULL);
        size_t size = eq_print_buffer_size(priv->eq);
        if ( size == 0 ) return NULL;
        char * __rc text = rcmem_alloc(size);
        text[size - 1] = '\0';
        if ( text == NULL ) return NULL;
        if ( !eq_print(priv->eq, text) ) {
            rcmem_put(text);
            return NULL;
        }

        priv->text_rc = rcmem_move(text);
        priv->text_cached = true;
    }

    struct question *q = malloc(sizeof(struct question));
    if ( q == NULL ) return NULL; //At least we cached the text

    q->text = rcmem_take(priv->text_rc);
    q->free_text = eq_free_text;
    return q;
}

static void eq_free(void *priv) {
    struct eq_task_priv *p = priv;
    mpz_clear(p->answer);
    eq_destroy(p->eq);
    if ( p->text_cached ) rcmem_put(p->text_rc);
    free(p);
}

static enum answer_state eq_check(void *p, FILE *fp) {
    assert(p);
    assert(fp);
    struct eq_task_priv *priv = p;

    if ( !priv->answer_cached ) {
        if ( !eq_solve(priv->eq, priv->answer) ) return ANSWER_WRONG;
        priv->answer_cached = true;
    }

    mpz_t answer;
    mpz_init(answer);
    int r = mpz_inp_str(answer, fp, 10);
    if ( r == 0 ) {
        mpz_clear(answer);
        return ANSWER_WRONG;
    }

    int cmp = mpz_cmp(answer, priv->answer);
    mpz_clear(answer);

    if ( cmp == 0 ) {
        return ANSWER_RIGHT;
    } else {
        return ANSWER_WRONG;
    }
}
static struct task *eq_gen_generate(void *priv) {
    struct eq_gen_priv *priv_ = priv;

    struct task *task = malloc(sizeof(struct task));
    if ( task == NULL ) return NULL;

    struct eq_task_priv *tpriv = malloc(sizeof(struct eq_task_priv));
    if ( tpriv == NULL ) goto free_task;

    tpriv->eq = eq_generate(priv_->minlen, priv_->maxlen, priv_->maxnr, priv_->allowed);
    if ( tpriv->eq == NULL ) goto free_tpriv;
    mpz_init(tpriv->answer);
    tpriv->answer_cached = false;
    tpriv->text_rc = NULL;
    tpriv->text_cached = false;

    task->priv = tpriv;
    task->get_question = eq_get_question;
    task->free_priv = eq_free;
    task->check = eq_check;

    return task;

free_tpriv:
    free(tpriv);
free_task:
    free(task);
    return NULL;
}

static void eq_gen_free_priv(void *priv) {
    struct eq_gen_priv *p = priv;
    mpz_clear(p->maxnr);
    free(priv);
    return;
}

struct gen *eq_gen_create(int minlen, int maxlen, unsigned maxnr, eq_flags_t allowed) {
    struct gen *eq_gen = malloc(sizeof(struct gen));
    if ( eq_gen == NULL ) return NULL;

    struct eq_gen_priv *priv = malloc(sizeof(struct eq_gen_priv));
    if ( priv == NULL ) goto free_eq_gen;

    priv->minlen = minlen;
    priv->maxlen = maxlen;
    mpz_init_set_ui(priv->maxnr, maxnr);
    priv->allowed = allowed;

    eq_gen->priv = priv;
    eq_gen->generate = eq_gen_generate;
    eq_gen->free_priv = eq_gen_free_priv;

    return eq_gen;

free_eq_gen:
    free(eq_gen);
    return NULL;
}
