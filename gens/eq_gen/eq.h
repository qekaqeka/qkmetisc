#pragma once

#include <stddef.h>
#include <stdint.h>
#include "eq_flags.h"

struct eq;

struct eq *eq_generate(int minlen, int maxlen, int maxnr, eq_flags_t allowed_ops);
bool eq_print(const struct eq *eq, char *buffer);
size_t eq_print_buffer_size(const struct eq *eq);
bool eq_solve(const struct eq* eq, intmax_t *out);
void eq_destroy(struct eq* eq);
