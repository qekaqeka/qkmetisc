#pragma once

#include <assert.h>
#include <stddef.h>
#include <stdbool.h>
#include "utils.h"

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

static inline void list_init_list(struct list *list) {
    assert(list);
    list->prev = NULL;
    list->next = NULL;
}

#define list_init(ptr, mmbr) list_init_list(&(ptr)->mmbr)

#define list_init_head_tail(ptr, head, tail) \
    ({                                       \
        (ptr)->head = NULL;                  \
        (ptr)->tail = NULL;                  \
    })

static inline void list_add_before_list(struct list *list, struct list *item) {
    assert(list);
    assert(item);
    item->next = list;
    item->prev = list->prev;
    if ( list->prev != NULL ) list->prev->next = item;
    list->prev = item;
}

#define list_add_before(ptr, item, mmbr) list_add_before_list(&(ptr)->mmbr, &(item)->mmbr)

static inline void list_add_after_lift(struct list *list, struct list *item) {
    assert(list);
    assert(item);
    item->prev = list;
    item->next = list->next;
    if ( list->next != NULL ) list->next->prev = item;
    list->next = item;
}

#define list_add_after(ptr, item, mmbr) list_add_after_list(&(ptr)->mmbr, &(item)->mmbr)

static inline void list_add_head_list(struct list *list, struct list *item) {
    assert(list);
    assert(item);
    item->next = list;
    assert(list->prev == NULL);
    list->prev = item;
}

#define list_add_head(head, item, list_mmbr) list_add_head_list(&(head)->list_mmbr, &item->list_mmbr)

#define list_add_item_front(head, tail, item, mmbr) \
    ({                                                   \
        if ( *(head) == NULL ) {                     \
            *(head) = item;                          \
            *(tail) = item;                          \
        } else {                                         \
            list_add_head(*(head), item, mmbr);      \
            *(head) = item;                          \
        }                                                \
    })

static inline void list_add_tail_list(struct list *list, struct list *item) {
    assert(list);
    assert(item);
    item->prev = list;
    assert(list->next == NULL);
    list->next = item;
}

#define list_add_tail(tail, item, list_mmbr) list_add_tail_list(&(tail)->list_mmbr, &item->list_mmbr)

#define list_add_item_back(head, tail, item, mmbr) \
    ({                                                  \
        if ( *(head) == NULL ) {                    \
            *(head) = item;                         \
            *(tail) = item;                         \
        } else {                                        \
            list_add_tail(*(tail), item, mmbr);     \
            *(tail) = item;                         \
        }                                               \
    })

static inline void list_unlink_list(struct list *item) {
    assert(item);
    if ( item->prev != NULL ) item->prev->next = item->next;
    if ( item->next != NULL ) item->next->prev = item->prev;
    list_init_list(item);
}


#define list_unlink(ptr, mmbr) list_unlink_list(&(ptr)->mmbr)

#define list_remove_item(head, tail, item, mmbr)              \
    ({                                                             \
        if ( *(head) == *(tail) && *(head) == (item) ) { \
            *(head) = NULL;                                    \
            *(tail) = NULL;                                    \
        } else if ( *(head) == (item) ) {                        \
            *(head) = list_get_next((item), mmbr);               \
        } else if ( *(tail) == (item) ) {                        \
            *tail = list_get_prev((item), mmbr);               \
        } else {                                                          \
            list_unlink((item), mmbr);  \
        } \
    })

static inline bool list_is_empty_list(struct list *list) {
    assert(list);
    return list->prev == NULL && list->next == NULL;
}

#define list_is_empty(ptr, mmbr) list_is_empty_list(&(ptr)->mmbr)

#define list_no_items(head, tail) (*(head) == NULL && (assert(*(tail) == NULL), true))

#define list_foreach(ptr, list_member, ITER_NAME) \
    for ( typeof(ptr) ITER_NAME = (ptr); ITER_NAME != NULL; ITER_NAME = list_get_next(ITER_NAME, list_member) )
#define list_foreach_safe(ptr, list_member, ITER_NAME)                                                             \
    for ( typeof(ptr) ITER_NAME = (ptr), ITER_NAME##_safe_ = list_get_next((ptr), list_member); ITER_NAME != NULL; \
          (ITER_NAME = ITER_NAME##_safe_) ? (ITER_NAME##_safe_ = list_get_next(ITER_NAME##_safe_, list_member)) : 0 )
#define list_foreach_back(ptr, list_member, ITER_NAME) \
    for ( typeof(ptr) ITER_NAME = (ptr); ITER_NAME != NULL; ITER_NAME = list_get_prev(ITER_NAME, list_member) )
#define list_foreach_back_safe(ptr, list_member, ITER_NAME)                                                        \
    for ( typeof(ptr) ITER_NAME = (ptr), ITER_NAME##_safe_ = list_get_prev((ptr), list_member); ITER_NAME != NULL; \
          (ITER_NAME = ITER_NAME##_safe_) ? (ITER_NAME##_safe_ = list_get_prev(ITER_NAME##_safe_, list_member)) : 0 )
