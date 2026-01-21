#ifndef CLI_H
#define CLI_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

typedef uint_fast32_t cli_flags_t;

// If you add new flags, you should update cli_check_flags function
#define CLI_STRICT ((cli_flags_t)0x00000001)
#define CLI_AUTO_HELP ((cli_flags_t)0x00000002)

enum cli_opt_error_type {
    CLI_OPT_ERROR_UNKNOWN_OPT, // opt
    CLI_OPT_ERROR_OPT_NO_ARG, // opt
    CLI_OPT_ERROR_OPT_FAILED_TO_PARSE_ARG, // opt, arg
    CLI_OPT_ERROR_OPT_REPEAT_NO_APPEND, // opt
    CLI_OPT_ERROR_REQUIRED_NOT_PRESENT // opt long name, opt short name str
};

typedef void cli_print_opt_error_t(enum cli_opt_error_type type, char *val1, char *val2);

struct cli;

extern struct cli *cli_create(cli_flags_t flags, cli_print_opt_error_t *func);
extern void cli_destroy(struct cli *cli);
extern size_t cli_print_help(struct cli *cli, FILE *fp, const char *app_name);

union cli_opt_arg_data {
    void *ptr;
    intmax_t imax;
    uintmax_t umax;
    short unsigned int su;
    unsigned int u;
    unsigned long int ul;
    unsigned long long int ull;
    short int si;
    int i;
    long int l;
    long long int ll;
    signed char sc;
    unsigned char uc;
    char c;
    double d;
    long double ld;
    float f;
};

struct cli_opt_arg {
    union cli_opt_arg_data data;
    void (*data_free)(union cli_opt_arg_data *data);
};

typedef int cli_opt_arg_parser_t(const char *s, struct cli_opt_arg *out);

typedef uint_fast32_t cli_opt_flags_t;

// If you add new flags, you should update cli_opt_check_flags function
#define CLI_OPT_REQUIRED ((cli_opt_flags_t)0x00000001)
#define CLI_OPT_HAS_ARG ((cli_opt_flags_t)0x00000002)
#define CLI_OPT_APPEND ((cli_opt_flags_t)0x00000004)

#define CLI_OPT_NO_SHORT_NAME '\0'

struct cli_opt {
    char *id;
    char *long_name;
    unsigned char short_name;
    cli_opt_flags_t flags;
    cli_opt_arg_parser_t *parser;

    char *description;
};

extern int cli_add_opts(struct cli *cli, const struct cli_opt *args, size_t opts_nr);
extern int cli_remove_opt(struct cli *cli, char *id);

struct cli_match;

extern struct cli_match *cli_match(struct cli *cli, int argc, char **argv);
extern void cli_match_destroy(struct cli_match *match);

extern struct cli_opt_arg *cli_match_get_arg(struct cli_match *match, char *id);
extern int cli_match_get_args(struct cli_match *match, char *id, struct cli_opt_arg **out);
extern int cli_match_get_args_nr(struct cli_match *match, char *id, size_t *out);
#endif
