#include "cli.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include "cli_hash.h"
#include "listc.h"

#ifdef __STDC_ALLOC_LIB__
    #define __STDC_WANT_LIB_EXT2__ 1
    #include <string.h>
    #undef __STDC_WANT_LIB_EXT2__
static char *cli_strdup(const char *s) {
    return strdup(s);
}
#else
    #include <string.h>
static char *cli_strdup(const char *s) {
    size_t len = strlen(s);

    char *res = malloc(len + 1);   // Strlen returns SIZE_MAX - 1 at most
    if ( !res ) return NULL;

    memcpy(res, s, len + 1);

    return res;
}
#endif

struct cli_hash_str {
    char *str;
    cli_hash_t hash;
};

// polynomial rolling hash function
static cli_hash_t cli_hash_from_str(const char *s) {
    cli_hash_t hash = 0;

    // The smallest prime number which bigger that number of characters in [a-z][A-Z][0-9]\- alphabet
    const cli_hash_t p = 67;
    cli_hash_t pc = 1;

    while ( *s != '\0' ) {
        // We don't worry about overflow, because with unsigned is just wrapping around
        hash += (cli_hash_t)*s * pc;
        pc *= p;
        s++;
    }

    return hash;
}

static void cli_hash_str_from_str(struct cli_hash_str *out, char *str) {
    out->str = str;
    out->hash = cli_hash_from_str(str);
}

static int cli_hash_str_cmp(const struct cli_hash_str *lhs, const struct cli_hash_str *rhs) {
    if ( lhs->hash == rhs->hash ) {
        if ( !strcmp(lhs->str, rhs->str) ) return 0;
    }

    return -1;
}

static int cli_hash_str_cmp_void_wrapper(void *lhs, void *rhs) {
    return cli_hash_str_cmp(lhs, rhs);
}

static void cli_hash_str_init_empty(struct cli_hash_str *hs) {
    hs->str = NULL;
    hs->hash = 0;
}

static int cli_hash_str_is_empty(struct cli_hash_str *hs) {
    return hs->str == NULL && hs->hash == 0;
}

struct cli_opt_node;

#define CLI_OPT_INTERNAL ((cli_opt_flags_t)0x80000000)
typedef int cli_internal_handler_t(struct cli *cli, struct cli_opt_node *node);

struct cli_opt_internal {
    char *id;
    char *long_name;

    cli_opt_flags_t flags;

    unsigned char short_name;

    cli_internal_handler_t *handler;

    char *description;
};

struct cli_opt_node {
    struct cli_hash_str id;

    struct cli_hash_str long_name;

    cli_opt_flags_t flags;

    unsigned char short_name;

    union {
        cli_opt_arg_parser_t *parser;
        cli_internal_handler_t *handler;
    } funcs;

    char *description;

    struct listc ring;
};

#define CLI_OPTS_BY_ID_HT_SIZE 256u
#define CLI_OPTS_BY_LONG_NAME_HT_SIZE 256u
struct cli {
    struct cli_opt_node *optional_opts;
    struct cli_opt_node *required_opts;

    struct cli_hash_table *opts_by_id_ht;
    struct cli_hash_table *opts_by_long_name_ht;

    struct cli_opt_node **opts_by_short_name_arr;

    cli_flags_t flags;

    cli_print_opt_error_t *print_opt_error;
};

// We need to unlink the arg on our own befire calling this func
static void cli_opt_node_destroy(struct cli_opt_node *node) {
    free(node->id.str);
    free(node->long_name.str);
    free(node->description);
    free(node);
}

static int cli_opt_check_flags(cli_opt_flags_t flags) {
    if ( flags & ~CLI_OPT_HAS_ARG & ~CLI_OPT_REQUIRED ) return -1;

    return 0;
}

static int cli_opt_check(const struct cli_opt *opt) {
    if ( opt->short_name == CLI_OPT_NO_SHORT_NAME && opt->long_name == NULL ) return -1;

    if ( opt->id == NULL ) return -1;

    if ( !isgraph(opt->short_name) && opt->short_name != CLI_OPT_NO_SHORT_NAME ) return -1;

    return cli_opt_check_flags(opt->flags);
}

static struct cli_opt_node *cli_opt_node_from_opt(const struct cli_opt *src) {
    struct cli_opt_node *node = malloc(sizeof(struct cli_opt_node));
    if ( !node ) goto fail;

    char *id_copy = cli_strdup(src->id);
    if ( !id_copy ) goto free_node;
    cli_hash_str_from_str(&node->id, id_copy);

    if ( src->long_name ) {
        char *long_name_copy = cli_strdup(src->long_name);
        if ( !long_name_copy ) goto free_id;
        cli_hash_str_from_str(&node->long_name, long_name_copy);
    } else {
        cli_hash_str_init_empty(&node->long_name);
    }

    node->short_name = src->short_name;
    node->flags = src->flags;
    node->description = cli_strdup(src->description);
    if ( !node->description ) goto free_long_name;

    if ( node->flags & CLI_OPT_HAS_ARG ) { node->funcs.parser = src->parser; }

    listc_init(node, ring);

    return node;

free_long_name:
    free(node->long_name.str);
free_id:
    free(node->id.str);
free_node:
    free(node);
fail:
    return NULL;
}

static struct cli_opt_node *cli_opt_node_from_opt_internal(const struct cli_opt_internal *src) {
    assert(src->flags & CLI_OPT_INTERNAL);

    struct cli_opt_node *node = malloc(sizeof(struct cli_opt_node));
    if ( !node ) goto fail;

    char *id_copy = cli_strdup(src->id);
    if ( !id_copy ) goto free_node;
    cli_hash_str_from_str(&node->id, id_copy);

    if ( src->long_name ) {
        char *long_name_copy = cli_strdup(src->long_name);
        if ( !long_name_copy ) goto free_id;
        cli_hash_str_from_str(&node->long_name, long_name_copy);
    } else {
        cli_hash_str_init_empty(&node->long_name);
    }

    node->short_name = src->short_name;
    node->flags = src->flags;
    node->description = cli_strdup(src->description);
    if ( !node->description ) goto free_long_name;

    node->funcs.handler = src->handler;

    listc_init(node, ring);

    return node;

free_long_name:
    free(node->long_name.str);
free_id:
    free(node->id.str);
free_node:
    free(node);
fail:
    return NULL;
}

static int cli_opt_node_has_arg(struct cli_opt_node *node) {
    return node->flags & CLI_OPT_HAS_ARG;
}

static int cli_check_flags(cli_flags_t flags) {
    if ( flags & ~(CLI_STRICT | CLI_AUTO_HELP) ) return -1;

    return 0;
}

static void cli_print_opt_error(enum cli_opt_error_type type, char *val1, char *val2) {
    switch ( type ) {
        case CLI_OPT_ERROR_UNKNOWN_OPT: fprintf(stderr, "Unknown option %s\n", val1); break;
        case CLI_OPT_ERROR_OPT_NO_ARG: fprintf(stderr, "Option %s must have arg, but it doesn't\n", val1); break;
        case CLI_OPT_ERROR_OPT_FAILED_TO_PARSE_ARG:
            fprintf(stderr, "Failed to parse arg %s for option %s\n", val2, val1);
            break;
        case CLI_OPT_ERROR_OPT_REPEAT_NO_APPEND:
            fprintf(stderr, "Option %s mustn't appear more than 1 time\n", val1);
            break;
        case CLI_OPT_ERROR_REQUIRED_NOT_PRESENT:
            if ( val1 == NULL ) {
                fprintf(stderr, "Option %s is required\n", val2);
            } else {
                fprintf(stderr, "Option %s ( %s ) is required\n", val2, val1);
            }
            break;
    }
}

static char * const auto_help_id = "help_id_internal";
static char * const auto_help_long = "help";
static const char auto_help_short = 'h';

static int cli_add_auto_help(struct cli *cli);

struct cli *cli_create(cli_flags_t flags, cli_print_opt_error_t *func) {
    if ( cli_check_flags(flags) ) goto fail;

    struct cli *cli = malloc(sizeof(struct cli));
    if ( !cli ) goto fail;

    cli->opts_by_id_ht = cli_hash_table_create(cli_hash_str_cmp_void_wrapper, CLI_OPTS_BY_ID_HT_SIZE);
    if ( !cli->opts_by_id_ht ) goto free_cli;

    cli->opts_by_long_name_ht = cli_hash_table_create(cli_hash_str_cmp_void_wrapper, CLI_OPTS_BY_LONG_NAME_HT_SIZE);
    if ( !cli->opts_by_long_name_ht ) goto destroy_opts_by_id_ht;

    cli->opts_by_short_name_arr = calloc(UCHAR_MAX, sizeof(struct cli_opt_node *));
    if ( !cli->opts_by_short_name_arr ) goto destroy_opts_by_long_name_ht;

    cli->optional_opts = NULL;
    cli->required_opts = NULL;

    cli->flags = flags;
    if ( func == NULL ) {
        cli->print_opt_error = cli_print_opt_error;
    } else {
        cli->print_opt_error = func;
    }

    if ( cli->flags & CLI_AUTO_HELP ) {
        if ( cli_add_auto_help(cli) ) goto destroy_opts_by_long_name_ht;
    }

    return cli;

destroy_opts_by_long_name_ht:
    cli_hash_table_destroy(cli->opts_by_long_name_ht);
destroy_opts_by_id_ht:
    cli_hash_table_destroy(cli->opts_by_id_ht);
free_cli:
    free(cli);
fail:
    return NULL;
}

void cli_destroy(struct cli *cli) {
    if ( cli->optional_opts ) {
        LISTC_FOREACH_START_SAFE(cli->optional_opts, ring, iter)
        cli_opt_node_destroy(iter);
        LISTC_FOREACH_END_SAFE(cli->optional_opts, ring, iter)
    }
    if ( cli->required_opts ) {
        LISTC_FOREACH_START_SAFE(cli->required_opts, ring, iter)
        cli_opt_node_destroy(iter);
        LISTC_FOREACH_END_SAFE(cli->required_opts, ring, iter)
    }

    /*
     * We can just destroy the tables, because their elements were already destroyed in last foreach loops and these
     * tables don't care about destroy their object on their own
     */
    cli_hash_table_destroy(cli->opts_by_id_ht);
    cli_hash_table_destroy(cli->opts_by_long_name_ht);

    free(cli->opts_by_short_name_arr);
    free(cli);
}

static size_t cli_print_node_name(struct cli_opt_node *node, FILE *fp) {
    if ( node->short_name != CLI_OPT_NO_SHORT_NAME ) {
        size_t printed = 0;
        printed += fprintf(fp, "-%c", node->short_name);

        if ( !cli_hash_str_is_empty(&node->long_name) ) { return printed + fprintf(fp, " --%s", node->long_name.str); }
    }

    if ( !cli_hash_str_is_empty(&node->long_name) ) { return fprintf(fp, "--%s", node->long_name.str); }

    assert(0);   // We must have at least one name
    return 0;
}

static size_t cli_print_node_short(struct cli_opt_node *node, FILE *fp) {
    size_t printed = cli_print_node_name(node, fp);

    if ( cli_opt_node_has_arg(node) ) { printed += fputs(" <ARG>", fp); }

    return printed;
}

static size_t cli_print_node_long(struct cli_opt_node *node, FILE *fp) {
    assert(node->description);   // Must have description

    size_t printed = cli_print_node_name(node, fp);
    printed += fprintf(fp, " - %s", node->description);

    return printed;
}

size_t cli_print_help(struct cli *cli, FILE *fp, const char *app_name) {
    if ( !cli->optional_opts && !cli->required_opts ) return 0;

    size_t printed = fprintf(fp, "Usage: %s ", app_name);

    // Short help
    if ( cli->optional_opts ) {
        LISTC_FOREACH_START(cli->optional_opts, ring, iter)
        printed += fputs("[ ", fp);
        printed += cli_print_node_short(iter, fp);
        printed += fputs(" ] ", fp);
        LISTC_FOREACH_END(cli->optional_opts, ring, iter)
    }

    if ( cli->required_opts ) {
        LISTC_FOREACH_START(cli->required_opts, ring, iter)
        printed += cli_print_node_short(iter, fp);
        printed += fputs(" ", fp);
        LISTC_FOREACH_END(cli->required_opts, ring, iter)
    }
    printed += fputs("\n\n\t", fp);

    // Long help
    if ( cli->optional_opts ) {
        LISTC_FOREACH_START(cli->optional_opts, ring, iter)
        if ( iter->description ) {
            printed += cli_print_node_long(iter, fp);
            printed += fputs("\n\t", fp);
        }
        LISTC_FOREACH_END(cli->optional_opts, ring, iter)
    }

    if ( cli->required_opts ) {
        LISTC_FOREACH_START(cli->required_opts, ring, iter)
        if ( iter->description ) {
            printed += cli_print_node_long(iter, fp);
            printed += fputs("\n\t", fp);
        }
        LISTC_FOREACH_END(cli->required_opts, ring, iter)
    }

    printed += fputs("\n", fp);

    return printed;
}

static int cli_add_opt_node(struct cli *cli, struct cli_opt_node *node) {
    if ( node->short_name != CLI_OPT_NO_SHORT_NAME ) {
        if ( !cli->opts_by_short_name_arr[node->short_name] ) {
            cli->opts_by_short_name_arr[node->short_name] = node;
        } else {
            goto fail;
        }
    }

    if ( cli_hash_table_insert(cli->opts_by_id_ht, node, &node->id, node->id.hash) ) goto remove_from_by_short_name_arr;

    if ( !cli_hash_str_is_empty(&node->long_name) ) {
        if ( cli_hash_table_insert(cli->opts_by_long_name_ht, node, &node->long_name, node->long_name.hash) )
            goto remove_from_by_id_ht;
    }

    if ( node->flags & CLI_OPT_REQUIRED ) {
        listc_add_item_back(&cli->required_opts, node, ring);
    } else {
        listc_add_item_back(&cli->optional_opts, node, ring);
    }

    return 0;

remove_from_by_id_ht:
    cli_hash_table_remove(cli->opts_by_id_ht, &node->id, node->id.hash, NULL);
remove_from_by_short_name_arr:
    cli->opts_by_short_name_arr[node->short_name] = NULL;
fail:
    return -1;
}

static int cli_add_opt(struct cli *cli, const struct cli_opt *opt) {
    if ( cli_opt_check(opt) ) goto fail;

    struct cli_opt_node *node = cli_opt_node_from_opt(opt);
    if ( !node ) goto fail;
    
    if ( cli_add_opt_node(cli, node) ) goto destroy_node;

    return 0;

destroy_node:
    cli_opt_node_destroy(node);
fail:
    return -1;
}

static int cli_add_opt_internal(struct cli *cli, const struct cli_opt_internal *opt) {
    struct cli_opt_node *node = cli_opt_node_from_opt_internal(opt);
    if ( !node ) goto fail;
    
    if ( cli_add_opt_node(cli, node) ) goto destroy_node;

    return 0;

destroy_node:
    cli_opt_node_destroy(node);
fail:
    return -1;
}

// check that user don't try to use internal stuff
static int cli_opt_user_check(const struct cli_opt *opt) {
    return opt->flags & CLI_OPT_INTERNAL;
}

int cli_add_opts(struct cli *cli, const struct cli_opt *opts, size_t opts_nr) {
    for ( size_t i = 0; i < opts_nr; i++ ) {
        const struct cli_opt *opt = &opts[i];
        if ( cli_opt_user_check(opt) || cli_add_opt(cli, opt) ) {
            for ( size_t j = 0; j < i; j++ ) {
                cli_remove_opt(cli, opts[j].id);
            }
            return -1;
        }
    }

    return 0;
}

static int cli_auto_help_handler(struct cli *cli, struct cli_opt_node *node) {
    (void)node;

    cli_print_help(cli, stdout, "<app path>");

    exit(EXIT_SUCCESS);
}

static int cli_add_auto_help(struct cli *cli) {
    struct cli_opt_internal opt = {
        .id = auto_help_id,
        .long_name = auto_help_long,
        .short_name = auto_help_short,
        .handler = cli_auto_help_handler,
        .flags = CLI_OPT_INTERNAL,
        .description = "show this text",
    };

    return cli_add_opt_internal(cli, &opt);
}

int cli_remove_opt(struct cli *cli, char *id) {
    struct cli_hash_str id_hs;
    cli_hash_str_from_str(&id_hs, id);

    void *data;
    if ( cli_hash_table_get(cli->opts_by_id_ht, &id_hs, id_hs.hash, &data) ) return -1;

    struct cli_opt_node *node = data;

    if ( node->flags & CLI_OPT_INTERNAL ) return -1;

    if ( node->flags & CLI_OPT_REQUIRED ) {
        listc_remove_item(&cli->required_opts, node, ring);
    } else {
        listc_remove_item(&cli->optional_opts, node, ring);
    }

    cli_hash_table_remove(cli->opts_by_id_ht, &node->id, node->id.hash, NULL);

    if ( !cli_hash_str_is_empty(&node->long_name) )
        cli_hash_table_remove(cli->opts_by_long_name_ht, &node->long_name, node->long_name.hash, NULL);

    cli_opt_node_destroy(node);
    return 0;
}

struct cli_match_arg_node {
    struct cli_opt_arg arg;

    struct listc ring;
};

static void cli_opt_arg_destruct(struct cli_opt_arg *arg) {
    if ( arg->data_free ) arg->data_free(&arg->data);
}

static void cli_match_arg_node_destroy(struct cli_match_arg_node *node) {
    cli_opt_arg_destruct(&node->arg);
    free(node);
}


struct cli_match_node {
    struct cli_hash_str id;

    struct cli_match_arg_node *args;
    size_t args_nr;

    struct listc ring;
};

static void cli_match_node_destroy(struct cli_match_node *node) {
    if ( node->args ) {
        LISTC_FOREACH_START_SAFE(node->args, ring, iter)
        cli_match_arg_node_destroy(iter);
        LISTC_FOREACH_END_SAFE(node->args, ring, iter)
    }

    free(node->id.str);
    free(node);
}

#define CLI_MATCH_NODES_BY_ID_HT_SIZE 256u
struct cli_match {
    struct cli_match_node *nodes;

    struct cli_hash_table *nodes_by_id_ht;
};

static struct cli_match *cli_match_create(void) {
    struct cli_match *match = malloc(sizeof(struct cli_match));
    if ( !match ) goto fail;

    match->nodes_by_id_ht = cli_hash_table_create(cli_hash_str_cmp_void_wrapper, CLI_MATCH_NODES_BY_ID_HT_SIZE);
    if ( !match->nodes_by_id_ht ) goto free_match;

    match->nodes = NULL;

    return match;

free_match:
    free(match);
fail:
    return NULL;
}

static void cli_opt_arg_copy(struct cli_opt_arg *dest, struct cli_opt_arg *src) {
    dest->data = src->data;
    dest->data_free = src->data_free;
}

static int cli_match_node_add_arg(struct cli_match_node *node, struct cli_opt_arg *arg) {
    struct cli_match_arg_node *arg_node = malloc(sizeof(struct cli_match_arg_node));
    if ( !arg_node ) return -1;

    cli_opt_arg_copy(&arg_node->arg, arg);

    listc_init(arg_node, ring);
    listc_add_item_back(&node->args, arg_node, ring);

    node->args_nr++;

    return 0;
}

static int cli_match_add_node(struct cli_match *match, struct cli_hash_str *id, struct cli_opt_arg *arg) {
    struct cli_match_node *node = malloc(sizeof(struct cli_match_node));
    if ( !node ) goto fail;

    node->id.str = cli_strdup(id->str);
    if ( !node->id.str ) goto free_node;
    node->id.hash = id->hash;

    node->args_nr = 0;

    if ( cli_hash_table_insert(match->nodes_by_id_ht, node, &node->id, node->id.hash) ) goto free_id;

    node->args = NULL;

    if ( arg ) {
        if ( cli_match_node_add_arg(node, arg) ) goto remove_from_ht;
    }

    listc_init(node, ring);
    listc_add_item_back(&match->nodes, node, ring);

    return 0;

remove_from_ht:
    cli_hash_table_remove(match->nodes_by_id_ht, id, id->hash, NULL);
free_id:
    free(node->id.str);
free_node:
    free(node);
fail:
    return -1;
}

static int opt_is_long(const char *opt) {
    return !strncmp(opt, "--", 2);
}

static char *long_name_from_long_opt(char *opt) {
    return opt + 2;
}

static char short_name_from_short_opt(char *opt) {
    return opt[1];
}

static int cli_print_error(struct cli *cli, enum cli_opt_error_type type, char *val1, char *val2) {
    cli->print_opt_error(type, val1, val2);
    return cli->flags & CLI_STRICT;
}

static int cli_match_check_required(struct cli *cli, struct cli_match *match) {
    if ( cli->required_opts ) {
        LISTC_FOREACH_START(cli->required_opts, ring, iter)
        // If we didn't find option from the required options list, it's error
        if ( cli_hash_table_get(match->nodes_by_id_ht, &iter->id, iter->id.hash, NULL) ) {
            char short_name_str[] = {iter->short_name, '\0'};

            cli_print_error(cli, CLI_OPT_ERROR_REQUIRED_NOT_PRESENT, iter->long_name.str, short_name_str);

            return -1;
        }
        LISTC_FOREACH_END(cli->required_opts, ring, iter)
    }

    return 0;
}

static int cli_handle_internal(struct cli *cli, struct cli_opt_node *node) {
    assert(node->flags & CLI_OPT_INTERNAL);
    return node->funcs.handler(cli, node);
}

struct cli_match *cli_match(struct cli *cli, int argc, char **argv) {
    struct cli_match *match = cli_match_create();
    if ( !match ) goto fail;

    (void)argc;

    struct cli_hash_str hs;
    for ( ; *argv; argv++ ) {
        char *opt = *argv;

        struct cli_opt_node *opt_node;

        if ( opt_is_long(opt) ) {
            char *long_name = long_name_from_long_opt(opt);
            cli_hash_str_from_str(&hs, long_name);

            void *data = NULL;
            if ( cli_hash_table_get(cli->opts_by_long_name_ht, &hs, hs.hash, &data) ) {
                if ( cli_print_error(cli, CLI_OPT_ERROR_UNKNOWN_OPT, opt, NULL) ) goto destroy_match;
            }

            opt_node = data;
        } else {
            if ( opt[0] != '-' ) {
                if ( cli_print_error(cli, CLI_OPT_ERROR_UNKNOWN_OPT, opt, NULL) ) goto destroy_match;
            }

            unsigned char short_name = short_name_from_short_opt(opt);

            opt_node = cli->opts_by_short_name_arr[short_name];
        }

        if ( !opt_node ) {
            if ( cli_print_error(cli, CLI_OPT_ERROR_UNKNOWN_OPT, opt, NULL) ) goto destroy_match;
            continue;
        }

        if ( opt_node->flags & CLI_OPT_INTERNAL ) {
            if ( cli_handle_internal(cli, opt_node) ) goto destroy_match;
        }

        int opt_node_has_arg = cli_opt_node_has_arg(opt_node);


        struct cli_opt_arg arg;

        if ( opt_node_has_arg ) {
            argv++;

            char *arg_str = *argv;

            if ( !arg_str ) {
                if ( cli_print_error(cli, CLI_OPT_ERROR_OPT_NO_ARG, opt, NULL) ) goto destroy_match;
                continue;
            }

            if ( opt_node->funcs.parser(arg_str, &arg) ) {
                if ( cli_print_error(cli, CLI_OPT_ERROR_OPT_FAILED_TO_PARSE_ARG, opt, arg_str) ) goto destroy_match;
                continue;
            }
        }

        void *data;

        if ( !cli_hash_table_get(match->nodes_by_id_ht, &opt_node->id, opt_node->id.hash, &data) ) {
            if ( opt_node->flags & CLI_OPT_APPEND ) {
                struct cli_match_node *node = data;

                if ( cli_match_node_add_arg(node, &arg) ) goto destroy_match;

                continue;   // We added arg, out work is done
            } else {
                if ( cli_print_error(cli, CLI_OPT_ERROR_OPT_REPEAT_NO_APPEND, opt, NULL) ) {
                    if ( opt_node_has_arg ) cli_opt_arg_destruct(&arg);
                    goto destroy_match;
                }

                continue;
            }
        }

        if ( cli_match_add_node(match, &opt_node->id, &arg) ) {
            if ( opt_node_has_arg ) cli_opt_arg_destruct(&arg);
            goto destroy_match;
        }
    }

    if ( cli_match_check_required(cli, match) ) goto destroy_match;

    return match;

destroy_match:
    cli_match_destroy(match);
fail:
    return NULL;
}

void cli_match_destroy(struct cli_match *match) {
    if ( match->nodes ) {
        LISTC_FOREACH_START_SAFE(match->nodes, ring, iter)
        cli_match_node_destroy(iter);
        LISTC_FOREACH_END_SAFE(match->nodes, ring, iter)
    }

    cli_hash_table_destroy(match->nodes_by_id_ht);

    free(match);
}

extern struct cli_opt_arg *cli_match_get_arg(struct cli_match *match, char *id) {
    struct cli_hash_str hs;
    cli_hash_str_from_str(&hs, id);

    void *data;
    if ( cli_hash_table_get(match->nodes_by_id_ht, &hs, hs.hash, &data) ) return NULL;

    struct cli_match_node *node = data;

    if ( node->args_nr != 1 ) return NULL;

    return &node->args->arg;
}

int cli_match_get_args(struct cli_match *match, char *id, struct cli_opt_arg **out) {
    struct cli_hash_str hs;
    cli_hash_str_from_str(&hs, id);

    void *data;
    if ( cli_hash_table_get(match->nodes_by_id_ht, &hs, hs.hash, &data) ) return -1;

    struct cli_match_node *node = data;

    LISTC_FOREACH_START(node->args, ring, iter)
    *out = &iter->arg;
    out++;
    LISTC_FOREACH_END(node->args, ring, iter)

    return 0;
}

int cli_match_get_args_nr(struct cli_match *match, char *id, size_t *out) {
    struct cli_hash_str hs;
    cli_hash_str_from_str(&hs, id);

    void *data;
    if ( cli_hash_table_get(match->nodes_by_id_ht, &hs, hs.hash, &data) ) return -1;

    struct cli_match_node *node = data;

    *out = node->args_nr;

    return 0;
}
