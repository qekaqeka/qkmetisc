#pragma once

#include "gen.h"
#include "eq_flags.h"

[[gnu::malloc]]
extern struct gen *eq_gen_create(int minlen, int maxlen, unsigned maxnr, eq_flags_t allowed);
