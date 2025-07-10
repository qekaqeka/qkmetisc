#include "gen_base.h"
#include <stdlib.h>
#include <assert.h>

struct gen {
    void *priv;
    struct task *(*generate)(void *priv);
    void (*free_priv)(void *priv);
};

struct task {
    void *priv;
    char *(*get_question)(void *priv);
    int (*verify)(void *priv, FILE *answer_stream);
    void (*free_priv)(void *priv);
};

struct task *gen_generate(struct gen *gen) {
    assert(gen);
    assert(gen->generate);
    return gen->generate(gen->priv);
}

void gen_destroy(struct gen *gen) {
    assert(gen);
    assert(gen->free_priv);
    gen->free_priv(gen->priv);
    free(gen);
}

char *task_get_question(struct task *task) {
    assert(task);
    assert(task->get_question);
    return task->get_question(task->priv);
}

int task_verify(const struct task *task, FILE *answer_stream) {
    assert(task);
    assert(answer_stream);
    assert(task->verify);
    return task->verify(task->priv, answer_stream);
}

void task_destroy(struct task *task) {
    assert(task);
    assert(task->free_priv);
    task->free_priv(task->priv);
    free(task);
}
