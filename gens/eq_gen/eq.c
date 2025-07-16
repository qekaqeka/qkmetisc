#include "eq.h"
#include <errno.h>
#include <inttypes.h>
#include <stdckdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"

enum eq_elem_type {
    EQ_OP,
    EQ_NR,
    EQ_BR
};

enum eq_op {
    EQ_OP_SUM,
    EQ_OP_SUB,
    EQ_OP_MUL
};

enum eq_br {
    EQ_BR_L,
    EQ_BR_R
};

struct eq_elem {
    union {
        enum eq_op op;
        int nr;
        enum eq_br br;
    } data;
    enum eq_elem_type type;
    struct list eq_elem_chain;
};

struct eq {
    struct eq_elem *eq_elems;
};

typedef unsigned eq_elem_gen_flags_t;

#define EQ_GEN_OP_SUM ((eq_elem_gen_flags_t)(1 << 0))
#define EQ_GEN_OP_SUB ((eq_elem_gen_flags_t)(1 << 1))
#define EQ_GEN_OP_MUL ((eq_elem_gen_flags_t)(1 << 2))
#define EQ_GEN_OP_ALL ((eq_elem_gen_flags_t)((EQ_GEN_OP_MUL << 1) - EQ_GEN_OP_SUM))
#define EQ_GEN_BR_L ((eq_elem_gen_flags_t)(1 << 3))
#define EQ_GEN_BR_R ((eq_elem_gen_flags_t)(1 << 4))
#define EQ_GEN_NR ((eq_elem_gen_flags_t)(1 << 5))

static struct eq_elem *eq_elem_gen(eq_elem_gen_flags_t flags, int max_nr) {
    if ( max_nr <= 0 ) max_nr = RAND_MAX;

    int popcount = __builtin_popcount(flags);
    if ( popcount == 0 ) {
        assert(0);
        return NULL;
    }

    int index = rand() % popcount;
    eq_elem_gen_flags_t mask = flags;

    for ( int i = 0; i < index; i++ ) {
        int one_index = __builtin_ffs(mask);
        mask &= ~(1 << (one_index - 1));   // clear bit
    }

    eq_elem_gen_flags_t chosen = (1 << (__builtin_ffs(mask) - 1));

    struct eq_elem *elem = malloc(sizeof(struct eq_elem));
    if ( elem == NULL ) return NULL;
    list_init(elem, eq_elem_chain);

    switch ( chosen ) {
        case EQ_GEN_OP_SUM:
            elem->data.op = EQ_OP_SUM;
            elem->type = EQ_OP;
            break;
        case EQ_GEN_OP_SUB:
            elem->data.op = EQ_OP_SUB;
            elem->type = EQ_OP;
            break;
        case EQ_GEN_OP_MUL:
            elem->data.op = EQ_OP_MUL;
            elem->type = EQ_OP;
            break;
        case EQ_GEN_BR_L:
            elem->data.br = EQ_BR_L;
            elem->type = EQ_BR;
            break;
        case EQ_GEN_BR_R:
            elem->data.br = EQ_BR_R;
            elem->type = EQ_BR;
            break;
        case EQ_GEN_NR:
            elem->data.nr = rand() % max_nr;
            elem->type = EQ_NR;
            break;
        default:
            assert(0);
            free(elem);
            return NULL;
    }

    return elem;
}

static int rand_eq_len(int minlen, int maxlen) {
    minlen = minlen <= 0 ? 1 : minlen;
    maxlen = maxlen <= 0 ? RAND_MAX : maxlen;

    if ( minlen > maxlen ) return 0;

    int chain_len = (rand() % (maxlen - minlen + 1)) + minlen;
    // chain_len can be only odd
    if ( chain_len % 2 == 0 ) {
        if ( chain_len != maxlen ) {
            chain_len--;
        } else if ( chain_len != minlen ) {
            chain_len++;
        } else {
            return 0;
        }
    }

    return chain_len;
}

[[gnu::const]]
static eq_elem_gen_flags_t eq_flags_to_gen_flags(eq_flags_t flags) {
    eq_elem_gen_flags_t res = 0;

    if ( (flags & EQ_ELEM_OPS) == 0 ) {   // no ops specified
        return 0;
    }

    if ( flags & EQ_ELEM_SUM ) res |= EQ_GEN_OP_SUM;
    if ( flags & EQ_ELEM_SUB ) res |= EQ_GEN_OP_SUB;
    if ( flags & EQ_ELEM_MUL ) res |= EQ_GEN_OP_MUL;
    if ( flags & EQ_ELEM_BRACES ) res |= EQ_GEN_BR_L | EQ_GEN_BR_R;

    return res;
}

struct eq *eq_generate(int minlen, int maxlen, int maxnr, eq_flags_t allowed_ops) {
    eq_elem_gen_flags_t allowed = eq_flags_to_gen_flags(allowed_ops);
    if ( allowed == 0 ) return NULL;

    int chain_len = rand_eq_len(minlen, maxlen);
    if ( chain_len == 0 ) return NULL;

    struct eq *eq = malloc(sizeof(struct eq));
    if ( eq == NULL ) return NULL;

    struct eq_elem *chain = NULL;
    struct eq_elem *tail = NULL;
    eq_elem_gen_flags_t flags = 0;

    int br_l_avail = chain_len / 2;
    int br_r_avail = 0;
    int op_avail = chain_len / 2;
    int nr_avail = chain_len / 2 + 1;
    bool br_l_possible = br_l_avail > 0;
    bool br_r_possible = false;
    bool op_possible = false;
    bool nr_possible = true;

    for ( int i = 0; i < chain_len; i++ ) {
        if ( br_l_possible ) flags |= EQ_GEN_BR_L;
        if ( br_r_possible ) flags |= EQ_GEN_BR_R;
        if ( op_possible ) flags |= EQ_GEN_OP_ALL;
        flags &= allowed;

        // nr is always allowed
        if ( nr_possible ) flags |= EQ_GEN_NR;

        struct eq_elem *new = eq_elem_gen(flags, maxnr);
        if ( new == NULL ) goto free_eq_chain;

        if ( new->type == EQ_OP ) {
            op_avail--;
            br_l_possible = br_l_avail > 0;
            br_r_possible = false;
            op_possible = false;
            nr_possible = nr_avail > 0;
        } else if ( new->type == EQ_BR ) {
            if ( new->data.br == EQ_BR_L ) {
                br_l_avail--;
                br_r_avail++;
                nr_avail--;
                op_avail--;   // reserve space for right brace
                br_l_possible = br_l_avail > 0;
                br_r_possible = false;
                op_possible = false;
                nr_possible = nr_avail > 0;
            } else if ( new->data.br == EQ_BR_R ) {
                br_r_avail--;
                br_l_possible = false;
                br_r_possible = br_r_avail > 0;
                op_possible = op_avail > 0;
                nr_possible = false;
            } else {
                assert(0);
            }
        } else if ( new->type == EQ_NR ) {
            nr_avail--;
            br_l_avail--;
            br_l_possible = false;
            br_r_possible = br_r_avail > 0;
            op_possible = op_avail > 0;
            nr_possible = false;
        } else {
            assert(0);
        }
        flags = 0;

        if ( tail == NULL ) {
            chain = new;
            tail = new;
        } else {
            list_add_tail(tail, new, eq_elem_chain);
            tail = new;
        }
    }

    eq->eq_elems = chain;

    return eq;

free_eq_chain:
    if ( chain ) {
        list_foreach_safe(chain, eq_elem_chain, iter) { free(iter); }
    }
    free(eq);
    return NULL;
}

/*
 * EXPRESSION:
 *  EXPRESSION + TERM
 *  EXPRESSION - TERM
 *  TERM
 *
 * TERM:
 *  TERM * PRIMARY
 *  PRIMARY
 *
 * PRIMARY:
 *  NUMBER
 *  ( EXPRESSION )
 */

static bool eq_get_primary(struct eq_elem **chain, intmax_t *out);
static bool eq_get_term(struct eq_elem **chain, intmax_t *out);
static bool eq_get_expression(struct eq_elem **chain, intmax_t *out);

static bool eq_get_primary(struct eq_elem **chain, intmax_t *out) {
    assert(chain);
    struct eq_elem *elem = *chain;

    if ( elem == NULL ) {
        errno = EILSEQ;
        return false;
    }

    if ( elem->type == EQ_NR ) {
        *chain = list_get_next(elem, eq_elem_chain);
        *out = (intmax_t)elem->data.nr;
        return true;
    } else if ( elem->type == EQ_BR ) {
        if ( elem->data.br != EQ_BR_L ) {
            errno = EILSEQ;
            return false;
        }

        *chain = list_get_next(elem, eq_elem_chain);
        intmax_t res;
        if ( eq_get_expression(chain, &res) ) {
            elem = *chain;
            if ( elem == NULL ) {
                errno = EILSEQ;
                return false;
            }
            if ( elem->type != EQ_BR || elem->data.br != EQ_BR_R ) {
                errno = EILSEQ;
                return false;
            }
            *chain = list_get_next(elem, eq_elem_chain);
            *out = res;
            return true;
        } else {
            return false;
        }
    } else {
        errno = EILSEQ;
        return false;
    }
}

static bool eq_get_term(struct eq_elem **chain, intmax_t *out) {
    assert(chain);

    intmax_t left, right;

    if ( !eq_get_primary(chain, &left) ) return false;

    struct eq_elem *elem = *chain;

    while ( true ) {
        if ( elem == NULL ) {
            *out = left;
            return true;
        }

        if ( elem->type == EQ_OP ) {
            if ( elem->data.op == EQ_OP_MUL ) {
                *chain = list_get_next(elem, eq_elem_chain);
                if ( !eq_get_primary(chain, &right) ) return false;
                if ( ckd_mul(&left, left, right) ) {
                    errno = ERANGE;
                    return false;
                }
                elem = *chain;
            } else {
                *out = left;
                return true;
            }
        } else if ( elem->type == EQ_BR ) {
            assert(elem->data.br == EQ_BR_R);
            *out = left;
            return true;
        } else {
            errno = EILSEQ;
            return false;
        }
    }
}

static bool eq_get_expression(struct eq_elem **chain, intmax_t *out) {
    assert(chain);

    intmax_t left, right;

    if ( !eq_get_term(chain, &left) ) return false;

    struct eq_elem *elem = *chain;

    while ( true ) {
        if ( elem == NULL ) {
            *out = left;
            return true;
        }

        if ( elem->type == EQ_OP ) {
            if ( elem->data.op == EQ_OP_SUM ) {
                *chain = list_get_next(elem, eq_elem_chain);
                if ( !eq_get_term(chain, &right) ) return false;
                if ( ckd_add(&left, left, right) ) {
                    errno = ERANGE;
                    return false;
                }
                elem = *chain;
            } else if ( elem->data.op == EQ_OP_SUB ) {
                *chain = list_get_next(elem, eq_elem_chain);
                if ( !eq_get_term(chain, &right) ) return false;
                if ( ckd_sub(&left, left, right) ) {
                    errno = ERANGE;
                    return false;
                }
                elem = *chain;
            } else {
                *out = left;
                return true;
            }
        } else if ( elem->type == EQ_BR ) {
            assert(elem->data.br == EQ_BR_R);
            *out = left;
            return true;
        } else {
            errno = EILSEQ;
            return false;
        }
    }
}

static size_t int_digit_nr(int val) {
    if ( val == 0 ) return 1;

    size_t len = 0;
    while ( val != 0 ) {
        val /= 10;
        len++;   // Overflow is impossible
    }

    return len;
}

static size_t eq_calc_str_size(const struct eq *eq) {
    size_t res = 1;   // 1 for zero byte

    struct eq_elem *chain = eq->eq_elems;

    list_foreach(chain, eq_elem_chain, iter) {
        if ( iter->type == EQ_BR ) {
            if ( ckd_add(&res, res, 1) ) goto fail;
        } else if ( iter->type == EQ_OP ) {
            if ( ckd_add(&res, res, 1) ) goto fail;
        } else if ( iter->type == EQ_NR ) {
            if ( iter->data.nr < 0 ) {
                assert(0);   // Negative values unsupported
                goto fail;
            } else {
                size_t intlen = int_digit_nr(iter->data.nr);
                if ( ckd_add(&res, res, intlen) ) goto fail;
            }
        } else {
            assert(0);
            goto fail;
        }
    }

    return res;

fail:
    return 0;
}

static size_t eq_elem_print(const struct eq_elem *elem, char *buf) {
    if ( elem->type == EQ_BR ) {
        if ( elem->data.br == EQ_BR_L ) {
            buf[0] = '(';
        } else if ( elem->data.br == EQ_BR_R ) {
            buf[0] = ')';
        } else {
            assert(0);
            return 0;
        }
        return 1;
    } else if ( elem->type == EQ_NR ) {
        assert(elem->data.nr >= 0);
        sprintf(buf, "%d", elem->data.nr);
        return int_digit_nr(elem->data.nr);
    } else if ( elem->type == EQ_OP ) {
        switch ( elem->data.op ) {
            case EQ_OP_SUM: buf[0] = '+'; return 1;
            case EQ_OP_SUB: buf[0] = '-'; return 1;
            case EQ_OP_MUL: buf[0] = '*'; return 1;
            default: assert(0); return 0;
        }
    } else {
        assert(0);
        return 0;
    }
}

bool eq_print(const struct eq *eq, char *buffer) {
    assert(eq);
    assert(eq->eq_elems);
    size_t index = 0;
    struct eq_elem *chain = eq->eq_elems;
    list_foreach(chain, eq_elem_chain, iter) {
        size_t incr;
        incr = eq_elem_print(iter, shiftptr(buffer, index));
        if ( incr == 0 ) return false;
        index += incr;
    }

    return true;
}

size_t eq_print_buffer_size(const struct eq *eq) {
    return eq_calc_str_size(eq);
}

bool eq_solve(const struct eq *eq, intmax_t *out) {
    assert(eq);
    assert(eq->eq_elems);
    struct eq_elem *chain = eq->eq_elems;
    return eq_get_expression(&chain, out);
}

void eq_destroy(struct eq *eq) {
    if ( eq == NULL ) return;

    assert(eq->eq_elems);

    list_foreach_safe(eq->eq_elems, eq_elem_chain, iter) free(iter);
    free(eq);
}
