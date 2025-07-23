#pragma once

#include "plot.h"

struct server;

extern struct server *server_create(unsigned wq_workers_nr);
extern int server_add_plot(struct server *server, plot_constructor_t pc, unsigned short port);
extern int server_remove_plot(struct server *server, unsigned short port);
extern void server_destroy(struct server *server);
