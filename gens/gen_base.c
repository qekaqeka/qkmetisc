#include "gen_base.h"
#include "gen_base_types.h"
#include <stdlib.h>
#include <assert.h>

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

void task_free_question(struct task *task, char *question) {
    assert(task);
    assert(task->free_question);
    return task->free_question(question);
}

bool task_verify(const struct task *task, FILE *answer_stream) {
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
