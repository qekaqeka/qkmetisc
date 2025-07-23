#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include "gen.h"
#include "gens/eq_gen.h"
#include "gens/python_shellcode.h"
#include "plot.h"
#include "plot_types.h"

#define EQ_GEN_CALL_NR 25
#define PYTHON_SHELLCODE_GEN_CALL_NR 5
#define MSEC_IN_SEC 1000

struct troll_eq_plot_priv {
    struct gen *eq_gen;
    int eq_gen_used;
    struct gen *python_shellcode_gen;
    int python_shellcode_gen_used;
};

static struct plot_task *troll_eq_get_task(void *p) {
    assert(p);

    struct troll_eq_plot_priv *priv = p;

    if ( priv->eq_gen_used < EQ_GEN_CALL_NR ) {
        struct plot_task *pt = malloc(sizeof(struct plot_task));
        if ( pt == NULL ) return NULL;

        pt->gen_task = gen_generate(priv->eq_gen);
        if ( pt->gen_task == NULL ) {
            free(pt);
            return NULL;
        }

        pt->msec_timemout = 15 * MSEC_IN_SEC / (priv->eq_gen_used + 1);
        priv->eq_gen_used++;
        return pt;

    } else if ( priv->python_shellcode_gen_used < PYTHON_SHELLCODE_GEN_CALL_NR ) {
        struct plot_task *pt = malloc(sizeof(struct plot_task));
        if ( pt == NULL ) return NULL;

        pt->gen_task = gen_generate(priv->python_shellcode_gen);
        if ( pt->gen_task == NULL ) {
            free(pt);
            return NULL;
        }

        pt->msec_timemout = 2 * MSEC_IN_SEC / (priv->eq_gen_used + 1);
        priv->python_shellcode_gen_used++;
        return pt;

    } else {
        errno = ENOTASK;
        return NULL;
    }
}

static void troll_eq_destroy_priv(void *p) {
    assert(p);

    struct troll_eq_plot_priv *priv = p;

    gen_destroy(priv->eq_gen);
    gen_destroy(priv->python_shellcode_gen);
    free(priv);
}

struct plot *troll_eq_plot_create() {
    struct plot *plot = malloc(sizeof(struct plot));
    if ( plot == NULL ) return NULL;

    struct troll_eq_plot_priv *priv = malloc(sizeof(struct troll_eq_plot_priv));
    if ( priv == NULL ) goto free_plot;

    struct gen *eq_gen = eq_gen_create(1, 25, 11, EQ_ELEM_ALL);
    if ( eq_gen == NULL ) goto free_priv;

    struct gen *python_shellcode_gen = python_shellcode_gen_create();
    if ( python_shellcode_gen == NULL ) goto destroy_eq_gen;

    priv->eq_gen = eq_gen;
    priv->eq_gen_used = 0;
    priv->python_shellcode_gen = python_shellcode_gen;
    priv->python_shellcode_gen_used = 0;

    plot->priv = priv;
    plot->get_task = troll_eq_get_task;
    plot->destroy_priv = troll_eq_destroy_priv;

    return plot;

destroy_eq_gen:
    gen_destroy(eq_gen);
free_priv:
    free(priv);
free_plot:
    free(plot);
    return NULL;
}
