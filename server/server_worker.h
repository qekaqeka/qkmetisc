#pragma once

#include "plot.h"

struct server_worker;

[[gnu::malloc]]
struct server_worker *server_worker_create();
int server_worker_add_client(struct server_worker *worker, int clientfd, plot_constructor_t pc);
unsigned server_worker_get_clients_nr(struct server_worker *worker);
void server_worker_destroy(struct server_worker *worker);

struct server_worker_pool;

[[gnu::malloc]]
struct server_worker_pool *server_worker_pool_create(unsigned workers_nr);
int server_worker_pool_add_client(struct server_worker_pool *pool, int clientfd, plot_constructor_t pc);
void server_worker_pool_destroy(struct server_worker_pool *pool);
