#pragma once

#include <stdio.h>

enum task_type {
    TASK_INT_ANSWER,
    TASK_STR_ANSWER
};

struct task;

typedef struct qs *(*gen_func_t)(void *);

struct gen;

//Only successors can create gen object
[[gnu::malloc]]
extern struct task *gen_generate(struct gen *gen);
extern void gen_destroy(struct gen *gen);

[[gnu::malloc]]
extern char *task_get_question(struct task *task);
extern int task_verify(const struct task *task, FILE *answer_stream);
extern void task_destroy(struct task *task);
