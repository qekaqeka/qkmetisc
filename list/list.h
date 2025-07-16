#pragma once

#include <stddef.h>
#include "utils.h"
#include <assert.h>

struct list {
    struct list *prev;
    struct list *next;
};

static inline void *_list_get_prev_(void *ptr, ptrdiff_t list_off) {
    assert(ptr);
    struct list *list = shiftptr(ptr, list_off);

    if ( list->prev == NULL ) return NULL;

    return shiftptr(list->prev, -list_off);
}

#define list_get_prev(ptr, list_member) ((typeof(ptr))_list_get_prev_((ptr), offsetof(typeof(*ptr), list_member)))

static inline void *_list_get_next_(void *ptr, ptrdiff_t list_off) {
    assert(ptr);
    struct list *list = shiftptr(ptr, list_off);

    if ( list->next == NULL ) return NULL;

    return shiftptr(list->next, -list_off);
}

#define list_get_next(ptr, list_member) ((typeof(ptr))_list_get_next_((ptr), offsetof(typeof(*ptr), list_member)))

static inline void _list_init_(struct list *list) {
    assert(list);
    list->prev = NULL;
    list->next = NULL;
}

#define list_init(ptr, mmbr) _list_init_(&(ptr)->mmbr)

static inline void _list_add_before_(struct list *list, struct list *item) {
    assert(list);
    assert(item);
    item->next = list;
    item->prev = list->prev;
    if ( list->prev != NULL ) list->prev->next = item;
    list->prev = item;
}

#define list_add_before(ptr, item, mmbr) _list_add_before_(&(ptr)->mmbr, &(item)->mmbr)

static inline void _list_add_after_(struct list *list, struct list *item) {
    assert(list);
    assert(item);
    item->prev = list;
    item->next = list->next;
    if ( list->next != NULL ) list->next->prev = item;
    list->next = item;
}

#define list_add_after(ptr, item, mmbr) _list_add_after_(&(ptr)->mmbr, &(item)->mmbr)

static inline void _list_add_head_(struct list *list, struct list *item) {
    assert(list);
    assert(item);
    item->next = list;
    assert(list->prev == NULL);
    list->prev = item;
}

#define list_add_head(ptr, item, mmbr) _list_add_head_(&(ptr)->mmbr, &(item)->mmbr)

static inline void _list_add_tail_(struct list *list, struct list *item) {
    assert(list);
    assert(item);
    item->prev = list;
    assert(list->next == NULL);
    list->next = item;
}

#define list_add_tail(ptr, item, mmbr) _list_add_tail_(&(ptr)->mmbr, &(item)->mmbr)

static inline void _list_unlink_(struct list *item) {
    assert(item);
    if ( item->prev != NULL ) item->prev->next = item->next;
    if ( item->next != NULL ) item->next->prev = item->prev;
}

#define list_unlink(ptr, mmbr) _list_unlink_(&(ptr)->mmbr)

static inline bool _list_is_empty_(struct list *list) { 
    assert(list);
    return list->prev == NULL && list->next == NULL;
}

#define list_is_empty(ptr, mmbr) _list_is_empty_(&(ptr)->mmbr)

#define list_foreach(ptr, list_member, ITER_NAME) \
    for ( typeof(ptr) ITER_NAME = (ptr); ITER_NAME != NULL; ITER_NAME = list_get_next(ITER_NAME, list_member) )
#define list_foreach_safe(ptr, list_member, ITER_NAME)                                                             \
    for ( typeof(ptr) ITER_NAME = (ptr), ITER_NAME##_safe_ = list_get_next((ptr), list_member); ITER_NAME != NULL; \
          (ITER_NAME = ITER_NAME##_safe_) ? (ITER_NAME##_safe_ = list_get_next(ITER_NAME##_safe_, list_member)) : 0)
#define list_foreach_back(ptr, list_member, ITER_NAME) \
    for ( typeof(ptr) ITER_NAME = (ptr); ITER_NAME != NULL; ITER_NAME = list_get_prev(ITER_NAME, list_member) )
#define list_foreach_back_safe(ptr, list_member, ITER_NAME)                                                        \
    for ( typeof(ptr) ITER_NAME = (ptr), ITER_NAME##_safe_ = list_get_prev((ptr), list_member); ITER_NAME != NULL; \
          (ITER_NAME = ITER_NAME##_safe_) ? (ITER_NAME##_safe_ = list_get_prev(ITER_NAME##_safe_, list_member)) : 0)
