#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <utils.h>
#include "cli.h"
#include "cli_convert.h"
#include "log.h"
#include "plots/troll_eq_plot.h"
#include "server.h"

int main(int argc, char **argv) {
    log_set_file(stderr);

    unsigned workers_nr = sysconf(_SC_NPROCESSORS_ONLN);
    short unsigned port = 0;
    int log_lvl = LOG_WARN;

    log_set_flags(log_lvl);

    struct cli *cli = cli_create(CLI_STRICT | CLI_AUTO_HELP, NULL);
    if ( !cli ) {
        log_msg(LOG_CRITICAL, "Failed to init cli\n");
        exit(EXIT_FAILURE);
    }

    struct cli_opt opts[] = {
        {
         .id = "log_lvl",
         .long_name = "log-lvl",
         .short_name = 'l',
         .flags = CLI_OPT_HAS_ARG,
         .parser = cli_convert_i,
         .description = "Verbosity level",
         },
        {
         .id = "port",
         .long_name = "port",
         .short_name = 'p',
         .flags = CLI_OPT_HAS_ARG,
         .parser = cli_convert_su,
         .description = "Port listen to",
         },
        {
         .id = "workers_nr",
         .long_name = "workers-nr",
         .short_name = 'w',
         .flags = CLI_OPT_HAS_ARG,
         .parser = cli_convert_u,
         .description = "Number of worker threads",
         }
    };

    if ( cli_add_opts(cli, opts, countof(opts)) ) {
        log_msg(LOG_CRITICAL, "Failed to add opts\n");
        cli_destroy(cli);
        exit(EXIT_FAILURE);
    }

    struct cli_match *m = cli_match(cli, argc - 1, argv + 1);
    if ( !m ) {
        log_msg(LOG_CRITICAL, "Failed to parse args\n");
        cli_destroy(cli);
        exit(EXIT_FAILURE);
    }

    struct cli_opt_arg *arg;
    arg = cli_match_get_arg(m, "log_lvl");
    if ( arg ) { log_lvl = arg->data.i; }
    arg = cli_match_get_arg(m, "port");
    if ( arg ) { port = arg->data.su; }
    arg = cli_match_get_arg(m, "workers_nr");
    if ( arg ) { workers_nr = arg->data.u; }

    cli_match_destroy(m);
    cli_remove_opt(cli, "port");
    cli_destroy(cli);

    log_set_flags(log_lvl);

    struct server *server = server_create(workers_nr);
    if ( server == NULL ) {
        log_msg(LOG_CRITICAL, "Failed to launch server\n");
        exit(EXIT_FAILURE);
    }

    if ( server_add_plot(server, troll_eq_plot_create, port) ) {
        log_msg(LOG_CRITICAL, "Failed to add troll_eq_plot\n");
        server_destroy(server);
        exit(EXIT_FAILURE);   // OS can do cleanup instead of us, but Valgrind won't like this
    }

    sigset_t ss;
    sigemptyset(&ss);
    sigaddset(&ss, SIGINT);
    sigprocmask(SIG_BLOCK, &ss, NULL);
    log_msg(LOG_INFO, "Server launched, press Ctrl+C to exit\n");
    int sig;
    sigwait(&ss, &sig);
    assert(sig == SIGINT);

    log_msg(LOG_INFO, "Shutting down the server\n");
    server_destroy(server);
    log_msg(LOG_INFO, "The server was shut down\n");
    exit(EXIT_SUCCESS);
}
