#include "eq_gen.h"
#include "gen_base_types.h"
#include <stdlib.h>
#include "utils.h"

struct task *eq_gen_generate(void *priv) {
    unused(priv);

    struct task *task = malloc(sizeof(struct task));
    if ( task == NULL ) return NULL;

    //TODO implement eq generation

    return task;
}

static void eq_gen_free_priv(void *priv) {
    unused(priv);
    return;
}

struct gen *eq_gen_create() {
    struct gen* eq_gen = malloc(sizeof(struct gen));
    if ( eq_gen == NULL ) return NULL;

    eq_gen->priv = NULL;
    eq_gen->generate = eq_gen_generate;
    eq_gen->free_priv = eq_gen_free_priv;

    return eq_gen;
}
