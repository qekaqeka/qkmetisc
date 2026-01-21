#pragma once
#include "plot_types.h"

struct linear_plot_member {
    struct gen *gen;
    unsigned task_count;
    unsigned long timeout_base;
    unsigned long timeout_dec;
};

extern struct plot *linear_plot_create(struct linear_plot_member *membs, unsigned long members_count);
