#pragma once

#include <assert.h>
#include <stddef.h>
#include "utils.h"

struct listc {
    struct listc *prev;
    struct listc *next;
};

static inline void *_listc_get_prev_(void *ptr, ptrdiff_t listc_off) {
    assert(ptr);
    struct listc *listc = shiftptr(ptr, listc_off);

    return shiftptr(listc->prev, -listc_off);
}

#define listc_get_prev(ptr, listc_member) ((typeof(ptr))_listc_get_prev_((ptr), offsetof(typeof(*ptr), listc_member)))

static inline void *_listc_get_next_(void *ptr, ptrdiff_t listc_off) {
    assert(ptr);
    struct listc *listc = shiftptr(ptr, listc_off);

    return shiftptr(listc->next, -listc_off);
}

#define listc_get_next(ptr, listc_member) ((typeof(ptr))_listc_get_next_((ptr), offsetof(typeof(*ptr), listc_member)))

static inline void listc_init_listc(struct listc *listc) {
    assert(listc);
    listc->prev = listc;
    listc->next = listc;
}

#define listc_init(ptr, mmbr) listc_init_listc(&(ptr)->mmbr)

static inline void listc_add_before_listc(struct listc *listc, struct listc *item) {
    assert(listc);
    assert(item);
    item->next = listc;
    item->prev = listc->prev;
    listc->prev->next = item;
    listc->prev = item;
}

#define listc_add_before(ptr, item, mmbr) listc_add_before_listc(&(ptr)->mmbr, &(item)->mmbr)

#define listc_add_item_front(ptr, mmbr, item, listc_mmbr)      \
    do {                                                       \
        if ( (ptr)->mmbr == NULL ) {                           \
            (ptr)->mmbr = (item);                              \
        } else {                                               \
            listc_add_before((ptr)->mmbr, (item), listc_mmbr); \
            (ptr)->mmbr = (item);                              \
        }                                                      \
    } while ( 0 )

static inline void listc_add_after_listc(struct listc *listc, struct listc *item) {
    assert(listc);
    assert(item);
    item->prev = listc;
    item->next = listc->next;
    listc->next->prev = item;
    listc->next = item;
}

#define listc_add_after(ptr, item, mmbr) listc_add_after_listc(&(ptr)->mmbr, &(item)->mmbr)

#define listc_add_item_back(ptr, mmbr, item, listc_mmbr)       \
    do {                                                       \
        if ( (ptr)->mmbr == NULL ) {                           \
            (ptr)->mmbr = (item);                              \
        } else {                                               \
            listc_add_before((ptr)->mmbr, (item), listc_mmbr); \
        }                                                      \
    } while ( 0 )

static inline void listc_unlink_listc(struct listc *item) {
    assert(item);
    item->prev->next = item->next;
    item->next->prev = item->prev;
    listc_init_listc(item);
}

#define listc_unlink(ptr, mmbr) listc_unlink_listc(&(ptr)->mmbr)

static inline bool listc_is_empty_listc(struct listc *listc) {
    assert(listc);
    return listc->prev == listc;
}

#define listc_is_empty(ptr, mmbr) listc_is_empty_listc(&(ptr)->mmbr)

#define listc_remove_item(ptr, mmbr, item, listc_mmbr)            \
    do {                                                          \
        if ( (ptr)->mmbr == NULL ) break;                         \
        if ( (ptr)->mmbr == (item) ) {                            \
            if ( listc_is_empty((item), listc_mmbr) ) {           \
                (ptr)->mmbr = NULL;                               \
            } else {                                              \
                (ptr)->mmbr = listc_get_next((item), listc_mmbr); \
            }                                                     \
        }                                                         \
        listc_unlink((item), listc_mmbr);                         \
    } while ( 0 )

/*
#define listc_foreach(ptr, listc_member, ITER_NAME) \
    for ( typeof(ptr) ITER_NAME = (ptr); ITER_NAME != NULL; ITER_NAME = listc_get_next(ITER_NAME, listc_member) )
#define listc_foreach_safe(ptr, listc_member, ITER_NAME)                                                             \
    for ( typeof(ptr) ITER_NAME = (ptr), ITER_NAME##_safe_ = listc_get_next((ptr), listc_member); ITER_NAME != NULL; \
          (ITER_NAME = ITER_NAME##_safe_) ? (ITER_NAME##_safe_ = listc_get_next(ITER_NAME##_safe_, listc_member)) : 0 )
#define listc_foreach_back(ptr, listc_member, ITER_NAME) \
    for ( typeof(ptr) ITER_NAME = (ptr); ITER_NAME != NULL; ITER_NAME = listc_get_prev(ITER_NAME, listc_member) )
#define listc_foreach_back_safe(ptr, listc_member, ITER_NAME)                                                        \
    for ( typeof(ptr) ITER_NAME = (ptr), ITER_NAME##_safe_ = listc_get_prev((ptr), listc_member); ITER_NAME != NULL; \
          (ITER_NAME = ITER_NAME##_safe_) ? (ITER_NAME##_safe_ = listc_get_prev(ITER_NAME##_safe_, listc_member)) : 0 )
*/
