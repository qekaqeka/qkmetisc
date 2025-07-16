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

struct question *task_get_question(struct task *task) {
    assert(task);
    assert(task->get_question);
    return task->get_question(task->priv);
}

void task_free_question(struct task *task, struct question *question) {
    assert(task);
    assert(task->free_question);
    return task->free_question(question);
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
    return q->text;
}

time_t question_get_timeout(const struct question *q) {
    return q->timeout;
}
