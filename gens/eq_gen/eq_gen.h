#pragma once

#include "gen_base.h"
#include "eq_flags.h"

[[gnu::malloc]]
extern struct gen *eq_gen_create(int minlen, int maxlen, int maxnr, eq_flags_t allowed);
