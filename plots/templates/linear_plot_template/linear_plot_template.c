#include "linear_plot_template.h"
#include <stdlib.h>
#include "list.h"
#include "utils.h"
#include "plot.h"
#include <errno.h>

struct linear_plot_priv {
    struct linear_plot_member *members;
    unsigned long members_count;
    unsigned long cur_member_index;
    unsigned cur_task_nr;
};

static void linear_plot_priv_destroy(void *priv) {
    struct linear_plot_priv *p = priv;


    //gens of all previous gens were already destroyed
    for ( unsigned long i = p->cur_member_index; i < p->members_count; i++ ) {
        gen_destroy(p->members[i].gen);
    }

    free(p->members);
    free(p);
}

static struct plot_task *linear_plot_get_task(void *priv) {
    struct linear_plot_priv *p = priv;

    if ( p->cur_member_index == p->members_count ) {
        errno = ENOTASK;
        return NULL;
    }

    struct linear_plot_member *cur_member = &p->members[p->cur_member_index];

    assert(cur_member->task_count > p->cur_task_nr);

    struct task *task = gen_generate(cur_member->gen);
    if ( task == NULL ) return NULL;

    struct plot_task *pt = malloc(sizeof(struct plot_task));
    if ( pt == NULL ) goto destroy_task;

    pt->gen_task = task;
    pt->msec_timemout = cur_member->timeout_base - cur_member->task_count * cur_member->timeout_dec;

    assert(pt->msec_timemout >= cur_member->timeout_base); //overflow check

    p->cur_task_nr++;
    if ( cur_member->task_count == p->cur_task_nr ) {
        gen_destroy(cur_member->gen);
        p->cur_member_index++;
        p->cur_task_nr = 0;
    }

    return pt;

destroy_task:
    task_destroy(task);
    return NULL;
}

struct plot *linear_plot_create(struct linear_plot_member *membs, unsigned long members_count) {
    struct plot *lp = malloc(sizeof(struct plot));
    if ( lp == NULL ) return NULL;

    struct linear_plot_priv *lpp = malloc(sizeof(struct linear_plot_priv));
    if ( lpp == NULL ) goto free_lp;

    size_t members_size;
    if ( ckd_mul(&members_size, sizeof(struct linear_plot_member), members_count) ) goto free_lpp;
    lpp->members = malloc(members_size);
    if ( lpp->members == NULL ) goto free_lpp;

    for ( unsigned long i = 0; i < members_count; i++ ) {
        lpp->members[i].gen = membs->gen; //move the gens
        lpp->members[i].task_count = membs->task_count;
        lpp->members[i].timeout_base = membs->timeout_base;
        lpp->members[i].timeout_dec = membs->timeout_dec;
    }
    lpp->members_count = members_count;
    lpp->cur_member_index = 0;
    lpp->cur_task_nr = 0;

    lp->priv = lpp;
    lp->destroy_priv = linear_plot_priv_destroy;
    lp->get_task = linear_plot_get_task;

    return lp;

free_lpp:
    free(lpp);
free_lp:
    free(lp);
    return NULL;
}
