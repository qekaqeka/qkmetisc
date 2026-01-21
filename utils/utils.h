#pragma once

#define typeof(X) __typeof__(X)

#include <assert.h>

#define unused(X) ((void)(X))
#define shiftptr(ptr, off) ((typeof(ptr))((char *)(ptr) + (off)))
#define countof(X) (sizeof(X) / sizeof((X)[0]))
#define containerof(type, ptr, memb) ((type *)(shiftptr((ptr), -offsetof(type, memb))))
#define min(X, Y) ((X) > (Y) ? (Y) : (X))
#define max(X, Y) ((X) > (Y) ? (X) : (Y))

#ifdef __GNUC__
#define ckd_add(r, a, b) __builtin_add_overflow((a), (b), (r))
#define ckd_sub(r, a, b) __builtin_sub_overflow((a), (b), (r))
#define ckd_mul(r, a, b) __builtin_mul_overflow((a), (b), (r))
#else
#error "Man... So... You need to implement this on your own"
#endif
