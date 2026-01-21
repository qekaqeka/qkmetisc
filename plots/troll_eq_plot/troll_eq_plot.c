#include "gen.h"
#include "gens/eq_gen.h"
#include "gens/echo_gen.h"
#include "gens/python_shellcode.h"
#include "plot.h"
#include "plot_types.h"
#include "templates/linear_plot_template/linear_plot_template.h"

struct plot *troll_eq_plot_create() {
    struct gen *eq_gen = eq_gen_create(1, 1000, 1000000, EQ_ELEM_ALL);
    if ( eq_gen == NULL ) goto fail;

    struct gen *python_shellcode_gen = python_shellcode_gen_create();
    if ( python_shellcode_gen == NULL ) goto destroy_eq_gen;
    
    struct gen *echo_gen = echo_gen_create("qekactf{ahah4ha4h4aahh4a_you_w@s_shut0doVVned!}");
    if ( echo_gen == NULL ) goto destroy_python_shellcode_gen;

    struct linear_plot_member membs[] = {
        {
            .gen = eq_gen,
            .task_count = 20,
            .timeout_base = 10000,
            .timeout_dec = 100,
        },
        {
            .gen = python_shellcode_gen,
            .task_count = 5,
            .timeout_base = 10000,
            .timeout_dec = 100,
        },
        {
            .gen = echo_gen,
            .task_count = 1,
            .timeout_base = 10000,
            .timeout_dec = 0,
        }
    };

    struct plot *plot = linear_plot_create(membs, 3);
    if ( plot == NULL ) goto destroy_echo_gen;

    return plot;

destroy_echo_gen:
    gen_destroy(echo_gen);
destroy_python_shellcode_gen:
    gen_destroy(python_shellcode_gen);
destroy_eq_gen:
    gen_destroy(eq_gen);
fail:
    return NULL;
}
