#pragma once

#include <stddef.h>

#define __rc

extern void *rcmem_alloc(size_t size);
extern void *rcmem_take(void *mem);
extern void rcmem_put(void *mem);
