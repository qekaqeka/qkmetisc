#pragma once

#include "gen.h"

struct plot {
    void *priv;
    struct plot_task *(*get_task)(void *priv);
    void (*destroy_priv)(void *priv);
};

struct plot_task {
    struct task *gen_task;
    unsigned long msec_timemout;
};
