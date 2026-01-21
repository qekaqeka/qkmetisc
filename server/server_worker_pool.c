#include <assert.h>
#include <stdlib.h>
#include "log.h"
#include "plot.h"
#include "server_worker.h"

struct server_worker_pool {
    struct server_worker **workers;
    unsigned workers_nr;
};

struct server_worker_pool *server_worker_pool_create(unsigned workers_nr) {
    assert(workers_nr > 0);

    struct server_worker_pool *pool = malloc(sizeof(struct server_worker_pool));
    if ( pool == NULL ) return NULL;

    pool->workers = malloc(sizeof(struct server_worker *) * workers_nr);
    pool->workers_nr = workers_nr;

    for ( unsigned i = 0; i < workers_nr; i++ ) {
        pool->workers[i] = server_worker_create();

        if ( pool->workers[i] == NULL ) { // destroy created workers
            for ( unsigned j = 0; j < i; j++ ) {
                server_worker_destroy(pool->workers[j]);
                goto free_pool;
            }
        }
    }

    log_msg(LOG_DEBUG, "Worker pool %p was created\n", pool);
    return pool;

free_pool:
    free(pool);
    return NULL;
}

int server_worker_pool_add_client(struct server_worker_pool *pool, int clientfd, plot_constructor_t pc) {
    assert(pool);
    assert(clientfd > -1);
    assert(pc);

    struct server_worker *laziest_worker = pool->workers[0];
    unsigned min_clients_nr = server_worker_get_clients_nr(pool->workers[0]);
    for ( unsigned i = 1; i < pool->workers_nr; i++ ) {
        unsigned clients_nr = server_worker_get_clients_nr(pool->workers[i]);
        if ( min_clients_nr > clients_nr ) {
            laziest_worker = pool->workers[i];
            min_clients_nr = clients_nr;
        }
    }

    return server_worker_add_client(laziest_worker, clientfd, pc);
}

void server_worker_pool_destroy(struct server_worker_pool *pool) {
    assert(pool);
    for ( unsigned i = 0; i < pool->workers_nr; i++ ) server_worker_destroy(pool->workers[i]);
    free(pool->workers);

    log_msg(LOG_DEBUG, "Worker pool %p was destroyed\n", pool);
    free(pool);
}
