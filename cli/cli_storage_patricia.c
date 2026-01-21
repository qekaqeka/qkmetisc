#include "cli_storage_patricia.h"
#include "listc.h"
#include <stdlib.h>
#include <stddef.h>

#define INLINE_MAX_LEN sizeof(char *)

struct cli_patricia_node {
    size_t len;

    // if len > INLINE_MAX_LEN - ptr_str is used, otherwise inline_str
    union {
        char *ptr_str;
        char inline_str[INLINE_MAX_LEN];
    } str;

    struct listc ring;

    struct cli_patricia_node *nodes_ring;
};

static char * cli_patricia_node_get_str(struct cli_patricia_node *node) {
    if ( node->len > INLINE_MAX_LEN ) return node->str.ptr_str;

    return node->str.inline_str;
}

static int cli_patricia_node_final(struct cli_patricia_node *node) {
    return !node->nodes_ring;
}

struct cli_storage {
    struct cli_patricia_node *tree;
};

struct cli_storage *cli_storage_create(void) {
    struct cli_storage *cs = malloc(sizeof(struct cli_storage));
    if ( !cs ) return NULL;

    cs->tree = NULL;

    return cs;
}

extern int cli_storage_insert(struct cli_storage *cs, const char *path, void *data);

extern int cli_storage_remove_void(struct cli_storage *cs, const char *path, void **out);
#define cli_storage_remove(cs, path, ptr) cli_storage_remove_void((cs), (path), (void **)&ptr);

extern int cli_storage_get_void(struct cli_storage *cs, const char *path, void **out);
#define cli_storage_get(cs, path, ptr) cli_storage_get_void((cs), (path), (void **)&ptr);

extern void cli_storage_destroy(struct cli_storage *cs);
