#include "echo_gen.h"
#include "gen_types.h"
#include <pthread.h>
#include <string.h>
#include "rcmem.h"
#include <stdlib.h>
#include <stdio.h>

static void echo_gen_question_free_text(char *text) {
    rcmem_put(text);
}

static enum answer_state echo_gen_check(void *priv, FILE *fp) {
    unused(priv);
    unused(fp);

    return ANSWER_RIGHT;
}

static struct question *echo_gen_get_question(void *priv) {
    char *text = priv;

    struct question *res = malloc(sizeof(struct question));
    if ( res == NULL ) return NULL;

    res->text = rcmem_take(text);
    res->free_text = echo_gen_question_free_text;

    return res;
}

static struct task *echo_gen_generate(void *priv) {
    char *text = priv;

    struct task *res = malloc(sizeof(struct task));
    if ( res == NULL ) return NULL;

    res->priv = rcmem_take(text);
    res->free_priv = rcmem_put;
    res->check = echo_gen_check;
    res->get_question = echo_gen_get_question;

    return res;
}

struct gen *echo_gen_create(const char *text) {
    struct gen *res = malloc(sizeof(struct gen));
    if ( res == NULL ) return NULL;

    size_t len = strlen(text);
    res->priv = rcmem_alloc(len + 1);
    if ( res->priv == NULL ) goto free_res;
    memcpy(res->priv, text, len + 1);

    res->free_priv = rcmem_put;
    res->generate = echo_gen_generate;

    return res;

free_res:
    free(res);
    return NULL;
}
