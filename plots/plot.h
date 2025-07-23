#pragma once

#include <stdio.h>
#include "gen.h"

struct plot;

#define ENOTASK 666

typedef struct plot *(*plot_constructor_t)();

struct plot_task;

extern struct plot_task *plot_get_task(struct plot *plot);
extern void plot_destroy(struct plot *plot);

extern struct question *plot_task_get_question(struct plot_task *pt);
extern enum answer_state plot_task_check(const struct plot_task *pt, FILE *fp);
extern unsigned long plot_task_get_timeout_msec(const struct plot_task *pt);
extern void plot_task_destroy(struct plot_task *pt);
