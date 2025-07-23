#pragma once

#include <stdio.h>
#include <time.h>

enum answer_state {
    ANSWER_WRONG,
    ANSWER_MORE,
    ANSWER_RIGHT
};

struct question;

struct task;

struct gen;

//Only successors can create gen object
[[gnu::malloc]]
extern struct task *gen_generate(struct gen *gen);
extern void gen_destroy(struct gen *gen);

[[gnu::malloc]]
extern struct question *task_get_question(struct task *task);
extern enum answer_state task_check(const struct task *task, FILE *answer_stream);
extern void task_destroy(struct task *task);

extern const char *question_get_text(const struct question *q);
extern void question_destroy(struct question *q);
