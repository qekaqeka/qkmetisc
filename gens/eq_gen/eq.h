#pragma once

#include <stddef.h>
#include <stdint.h>
#include "eq_flags.h"
#include "gmp.h"

struct eq;

struct eq *eq_generate(int minlen, int maxlen, const mpz_t max, eq_flags_t allowed_ops);
int eq_print(const struct eq *eq, char *buffer);
size_t eq_print_buffer_size(const struct eq *eq);
int eq_solve(const struct eq* eq, mpz_t out);
void eq_destroy(struct eq* eq);
