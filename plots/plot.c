#include "plot.h"
#include "plot_types.h"
#include <assert.h>
#include "gen.h"
#include <stdlib.h>

struct plot_task *plot_get_task(struct plot *plot) {
    assert(plot);
    assert(plot->get_task);
    return plot->get_task(plot->priv);
}

void plot_destroy(struct plot *plot) {
    assert(plot);
    assert(plot->destroy_priv);
    plot->destroy_priv(plot->priv);
    free(plot);
}

struct question *plot_task_get_question(struct plot_task *pt) {
    assert(pt);
    assert(pt->gen_task);
    return task_get_question(pt->gen_task);
}

enum answer_state plot_task_check(const struct plot_task *pt, FILE *fp) {
    assert(pt);
    assert(fp);
    assert(pt->gen_task);
    return task_check(pt->gen_task, fp);
}

unsigned long plot_task_get_timeout_msec(const struct plot_task *pt) {
    assert(pt);
    return pt->msec_timemout;
}

void plot_task_destroy(struct plot_task *pt) {
    assert(pt);
    task_destroy(pt->gen_task);
    free(pt);
}
