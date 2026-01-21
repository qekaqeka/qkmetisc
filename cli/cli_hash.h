#ifndef CLI_HASH_H
#define CLI_HASH_H

#include <limits.h>
#include <stddef.h>
#include "cli.h"

// cli_hast_t MUST be unsigned. See cli_hash_from_str
typedef unsigned long long cli_hash_t;

struct cli_hash_table;

typedef int cli_hash_key_cmp_t(void *lkey, void *rkey);

extern struct cli_hash_table *cli_hash_table_create(cli_hash_key_cmp_t *cmp, size_t space_size);
extern void cli_hash_table_destroy(struct cli_hash_table *cht);

extern int cli_hash_table_insert(struct cli_hash_table *cht, void *key, void *data, cli_hash_t key_hash);

extern int cli_hash_table_get(struct cli_hash_table *cht, void *key, cli_hash_t key_hash, void **out);
extern int cli_hash_table_remove(struct cli_hash_table *cht, void *key, cli_hash_t key_hash, void **out);

#endif
