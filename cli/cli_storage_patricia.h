#ifndef CLI_STORAGE_PATRICIA_H
#define CLI_STORAGE_PATRICIA_H

struct cli_storage;

extern struct cli_storage *cli_storage_create(void);
extern int cli_storage_insert(struct cli_storage *cs, const char *path, void *data);

extern int cli_storage_remove_void(struct cli_storage *cs, const char *path, void **out);
#define cli_storage_remove(cs, path, ptr) cli_storage_remove_void((cs), (path), (void **)&ptr);

extern int cli_storage_get_void(struct cli_storage *cs, const char *path, void **out);
#define cli_storage_get(cs, path, ptr) cli_storage_get_void((cs), (path), (void **)&ptr);

extern void cli_storage_destroy(struct cli_storage *cs);

#endif
