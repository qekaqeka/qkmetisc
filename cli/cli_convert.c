#include "cli_convert.h"
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdlib.h>

#define CAT(X, Y) X##Y
#define CAT3(X, Y, Z) X##Y##Z

#define DEFINE_strtoX_strict(X, TYPE)                                        \
    static int CAT3(strto, X, _strict)(const char *s, TYPE *out, int base) { \
        char *tmp;                                                           \
        errno = 0;                                                           \
        TYPE res = CAT(strto, X)(s, &tmp, base);                             \
        if ( errno == ERANGE ) return -1;                                    \
        if ( res == 0 && *tmp != '\0' ) return -1;                           \
        *out = res;                                                          \
        return 0;                                                            \
    }

#define DEFINE_strtoX_strict2_int(X, CONV_FUNC, SRC_TYPE, TYPE, MIN, MAX)        \
    static int CAT3(strto, X, _strict)(const char *s, TYPE *out, int base) { \
        char *tmp;                                                           \
        errno = 0;                                                           \
        SRC_TYPE res = CONV_FUNC(s, &tmp, base);                             \
        if ( errno == ERANGE ) return -1;                                    \
        if ( res > MAX || res < MIN ) return -1;                             \
        if ( res == 0 && *tmp != '\0' ) return -1;                           \
        *out = res;                                                          \
        return 0;                                                            \
    }

#define DEFINE_strtoX_strict2_uint(X, CONV_FUNC, SRC_TYPE, TYPE, MAX)        \
    static int CAT3(strto, X, _strict)(const char *s, TYPE *out, int base) { \
        char *tmp;                                                           \
        errno = 0;                                                           \
        SRC_TYPE res = CONV_FUNC(s, &tmp, base);                             \
        if ( errno == ERANGE ) return -1;                                    \
        if ( res > MAX ) return -1;                             \
        if ( res == 0 && *tmp != '\0' ) return -1;                           \
        *out = res;                                                          \
        return 0;                                                            \
    }

DEFINE_strtoX_strict(imax, intmax_t)
DEFINE_strtoX_strict(umax, uintmax_t)
DEFINE_strtoX_strict(l, long int)
DEFINE_strtoX_strict(ll, long long int)
DEFINE_strtoX_strict(ul, long unsigned int)
DEFINE_strtoX_strict(ull, long long unsigned int)

DEFINE_strtoX_strict2_int(i, strtol, long int, int, INT_MIN, INT_MAX)
DEFINE_strtoX_strict2_int(si, strtol, long int, short int, SHRT_MIN, SHRT_MAX)
DEFINE_strtoX_strict2_uint(u, strtoul, unsigned long int, unsigned int, UINT_MAX)
DEFINE_strtoX_strict2_uint(su, strtoul, unsigned long int, unsigned short int, USHRT_MAX)

DEFINE_strtoX_strict2_int(sc, strtol, long int, signed char, SCHAR_MIN, SCHAR_MAX)
DEFINE_strtoX_strict2_uint(uc, strtoul, long int, unsigned char, UCHAR_MAX)
DEFINE_strtoX_strict2_int(c, strtol, long int, char, CHAR_MIN, CHAR_MAX)

#define DEFINE_cli_conver_X(X)                                             \
    int CAT(cli_convert_, X)(const char *s, struct cli_opt_arg *out) { \
        out->data_free = NULL; \
        return CAT3(strto, X, _strict)(s, &out->data.X, 10);                    \
    }

DEFINE_cli_conver_X(imax)
DEFINE_cli_conver_X(umax)
DEFINE_cli_conver_X(su)
DEFINE_cli_conver_X(u)
DEFINE_cli_conver_X(ul)
DEFINE_cli_conver_X(ull)
DEFINE_cli_conver_X(si)
DEFINE_cli_conver_X(i)
DEFINE_cli_conver_X(l)
DEFINE_cli_conver_X(ll)
DEFINE_cli_conver_X(sc)
DEFINE_cli_conver_X(uc)
DEFINE_cli_conver_X(c)
