#include "gen.h"
#include "gen_types.h"
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

struct question *task_get_question(struct task *task) {
    assert(task);
    assert(task->get_question);
    return task->get_question(task->priv);
}

enum answer_state task_check(const struct task *task, FILE *answer_stream) {
    assert(task);
    assert(answer_stream);
    assert(task->check);
    return task->check(task->priv, answer_stream);
}

void task_destroy(struct task *task) {
    assert(task);
    assert(task->free_priv);
    task->free_priv(task->priv);
    free(task);
}

const char *question_get_text(const struct question *q) {
    assert(q);
    return q->text;
}

void question_destroy(struct question *q) {
    assert(q);
    assert(q->free_text);
    q->free_text(q->text);
    free(q);
}
