#pragma once

typedef unsigned eq_flags_t;

#define EQ_ELEM_SUM ((eq_flags_t)(1 << 0))
#define EQ_ELEM_SUB ((eq_flags_t)(1 << 1))
#define EQ_ELEM_MUL ((eq_flags_t)(1 << 2))
//No EQ_OP_DIV because we use only integers in th eq
#define EQ_ELEM_OPS ((eq_flags_t)((EQ_ELEM_MUL << 1) - EQ_ELEM_SUM))
#define EQ_ELEM_BRACES ((eq_flags_t)(1 << 3))
#define EQ_ELEM_ALL ((eq_flags_t)((EQ_ELEM_BRACES << 1) - EQ_ELEM_SUM))
