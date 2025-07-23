#define _GNU_SOURCE
#include "server.h"
#include "plots/troll_eq_plot.h"
#include "log.h"
#include <signal.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>

int main() {
    log_set_file(stderr);
    log_set_flags(LOG_INFO);

    long cpu_nr = sysconf(_SC_NPROCESSORS_ONLN);

    struct server *server = server_create(cpu_nr);
    if ( server == NULL ) {
        log_msg(LOG_CRITICAL, "Failed to launch server\n");
        exit(EXIT_FAILURE);
    }

    if ( server_add_plot(server, troll_eq_plot_create, 44321) ) {
        log_msg(LOG_CRITICAL, "Failed to add troll_eq_plot\n");
        server_destroy(server);
        exit(EXIT_FAILURE); //OS can do cleanup instead of us, but Valgrind won't like this
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
