#include "rcmem.h"
#include <stdlib.h>
#include <assert.h>
#include "log.h"
#include "utils.h"

struct rcmem_hdr {
    unsigned counter;
};

struct rcmem {
    struct rcmem_hdr hdr;
    char mem_start;
};

#define rcmem_get_mem(m) (&(m)->mem_start)

extern void *rcmem_alloc(size_t size) {
    size_t total_size;
    if ( ckd_add(&total_size, size, offsetof(struct rcmem, mem_start)) ) return NULL;

    struct rcmem *mem = malloc(total_size);
    if ( mem == NULL ) return NULL;

    mem->hdr.counter = 1;
    return rcmem_get_mem(mem);
}

extern void *rcmem_take(void *mem) {
    struct rcmem *m = containerof(struct rcmem, mem, mem_start);

    if ( ckd_add(&m->hdr.counter, m->hdr.counter, 1u) ) {
        log_msg(LOG_CRITICAL, "Reference counted memory on %p overflowed its reference counter\n", mem);
        assert(0);
    }

    return rcmem_get_mem(m);
}

extern void rcmem_put(void *mem) {
    struct rcmem *m = containerof(struct rcmem, mem, mem_start);

    if (m->hdr.counter == 1) {
        free(m);
        return;
    }

    m->hdr.counter--;
}
