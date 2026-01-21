#ifndef CLI_CONVERT_H
#define CLI_CONVERT_H

#include "cli.h"

extern int cli_convert_imax(const char *s, struct cli_opt_arg *out);
extern int cli_convert_umax(const char *s, struct cli_opt_arg *out);
extern int cli_convert_su(const char *s, struct cli_opt_arg *out);
extern int cli_convert_u(const char *s, struct cli_opt_arg *out);
extern int cli_convert_ul(const char *s, struct cli_opt_arg *out);
extern int cli_convert_ull(const char *s, struct cli_opt_arg *out);
extern int cli_convert_si(const char *s, struct cli_opt_arg *out);
extern int cli_convert_i(const char *s, struct cli_opt_arg *out);
extern int cli_convert_l(const char *s, struct cli_opt_arg *out);
extern int cli_convert_ll(const char *s, struct cli_opt_arg *out);
extern int cli_convert_sc(const char *s, struct cli_opt_arg *out);
extern int cli_convert_uc(const char *s, struct cli_opt_arg *out);
extern int cli_convert_c(const char *s, struct cli_opt_arg *out);
extern int cli_convert_d(const char *s, struct cli_opt_arg *out);
extern int cli_convert_ld(const char *s, struct cli_opt_arg *out);
extern int cli_convert_f(const char *s, struct cli_opt_arg *out);

#endif
