#include "cli_hash.h"
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include "listc.h"

#define CLI_HASH_T_BITS (sizeof(cli_hash_t) * CHAR_BIT)

struct cli_hash_table_node {
    void *key;
    void *data;

    struct listc ring;
};

struct cli_hash_table {
    size_t space_size;
    cli_hash_key_cmp_t *cmp;

    struct cli_hash_table_node **opts;
};

static inline struct cli_hash_table_node **cli_hash_table_get_nodes(struct cli_hash_table *cht, cli_hash_t hash) {
    return &cht->opts[hash % cht->space_size];
}

struct cli_hash_table *cli_hash_table_create(cli_hash_key_cmp_t *cmp, size_t space_size) {
    struct cli_hash_table *table = malloc(sizeof(struct cli_hash_table));
    if ( !table ) goto fail;

    table->opts = calloc(space_size, sizeof(struct cli_hash_table_node *));   // Initialize array with NULLs
    if ( !table->opts ) goto free_table;

    table->space_size = space_size;
    table->cmp = cmp;

    return table;

free_table:
    free(table);
fail:
    return NULL;
}

void cli_hash_table_destroy(struct cli_hash_table *cht) {
    for ( size_t i = 0; i < cht->space_size; i++ ) {
        if ( cht->opts[i] ) {
            LISTC_FOREACH_START_SAFE(cht->opts[i], ring, iter)
            free(iter);
            LISTC_FOREACH_END_SAFE(cht->opts[i], ring, iter)
        }
    }

    free(cht->opts);
    free(cht);
}

int cli_hash_table_insert(struct cli_hash_table *cht, void *data, void *key, cli_hash_t key_hash) {
    if ( !data ) return -1;

    struct cli_hash_table_node *node = malloc(sizeof(struct cli_hash_table_node));
    if ( !node ) return -1;

    node->data = data;
    node->key = key;
    listc_init(node, ring);

    struct cli_hash_table_node **nodes = cli_hash_table_get_nodes(cht, key_hash);

    if ( *nodes ) {
        LISTC_FOREACH_START(*nodes, ring, iter)
        if ( !cht->cmp(iter->key, key) ) { return -1; }
        LISTC_FOREACH_END(*nodes, ring, iter)
    }

    listc_add_item_back(nodes, node, ring);
    return 0;
}

int cli_hash_table_get(struct cli_hash_table *cht, void *key, cli_hash_t key_hash, void **out) {
    struct cli_hash_table_node *nodes = *cli_hash_table_get_nodes(cht, key_hash);

    if ( !nodes ) return -1;

    LISTC_FOREACH_START(nodes, ring, iter)
    if ( !cht->cmp(iter->key, key) ) {
        if ( out ) *out = iter->data;

        return 0;
    }
    LISTC_FOREACH_END(nodes, ring, iter)

    return -1;
}

int cli_hash_table_remove(struct cli_hash_table *cht, void *key, cli_hash_t key_hash, void **out) {
    struct cli_hash_table_node **nodes = cli_hash_table_get_nodes(cht, key_hash);

    if ( !*nodes ) return -1;

    LISTC_FOREACH_START(*nodes, ring, iter)
    if ( !cht->cmp(iter->key, key) ) {
        listc_remove_item(nodes, iter, ring);
        if ( out ) *out = iter->data;

        free(iter);
        return 0;
    }
    LISTC_FOREACH_END(*nodes, ring, iter)

    return -1;
}
