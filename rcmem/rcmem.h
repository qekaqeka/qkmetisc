#pragma once

#include <stddef.h>
#include "utils.h"

#define __rc
#define rcmem_move(mem) ({ \
    typeof(mem) tmp___ = (mem); \
    (mem) = NULL; \
    tmp___; \
})

extern void *rcmem_alloc(size_t size);
extern void *rcmem_take(void *mem);
extern void rcmem_put(void *mem);
