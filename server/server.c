#define _GNU_SOURCE
#include <errno.h>
#include "server.h"
#include <assert.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include "listc.h"
#include "log.h"
#include "server_worker.h"

struct plot_socket {
    int sockfd;
    plot_constructor_t pc;
    unsigned short port;
    struct listc plot_socket_ring;
    bool active;
};

struct server {
    int epfd;
    pthread_t main_thread;
    struct plot_socket *plot_sockets;
    pthread_mutex_t plot_sockets_mtx;

    struct plot_socket *inactive_plot_sockets;
    pthread_mutex_t inactive_plot_sockets_mtx;
    struct server_worker_pool *pool;
};

static void server_cleanup_inactive_plot_sockets(struct server *server) {
    pthread_mutex_lock(&server->inactive_plot_sockets_mtx);
    if ( server->inactive_plot_sockets != NULL ) {
        struct plot_socket *iter = server->inactive_plot_sockets;
        do {
            struct plot_socket *next = listc_get_next(iter, plot_socket_ring);
            assert(iter->sockfd == -1);
            free(iter);
            iter = next;
        } while ( iter != server->inactive_plot_sockets );
        server->inactive_plot_sockets = NULL;
    }
    pthread_mutex_unlock(&server->inactive_plot_sockets_mtx);
}

[[noreturn]]
static void *server_main_thread_loop(void *arg) {
    struct server *serv = arg;
    struct epoll_event ev;

    while ( true ) {
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

        /*
         * ATTENTION!!!
         * epoll_wait doesn't have to be a cancellation point. It is now, but it could change in the future
         * ATTENTION!!!
         */
retry:
        int res = epoll_wait(serv->epfd, &ev, 1, -1);
        if ( res == -1 ) {
            if ( errno == EINTR ) goto retry;
        }
        assert(res != -1);

        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
        pthread_mutex_lock(&serv->plot_sockets_mtx);

        struct plot_socket *ps = ev.data.ptr;

        if ( !ps->active ) {   // Plot socket is going to destroy
            server_cleanup_inactive_plot_sockets(serv);
            pthread_mutex_unlock(&serv->plot_sockets_mtx);
            continue;
        }

        // If we did cleanup before ps active check, we'd cleared our ps if it is inactive, which cause to UAF
        server_cleanup_inactive_plot_sockets(serv);

        assert(ev.events == EPOLLIN);

        int clientfd = accept(ps->sockfd, NULL, 0);
        if ( clientfd == -1 ) {
            log_msg(LOG_WARN, "Failed to accept connection\n");
            pthread_mutex_unlock(&serv->plot_sockets_mtx);
            continue;
        }

        if ( server_worker_pool_add_client(serv->pool, clientfd, ps->pc) ) {
            log_msg(LOG_WARN, "Failed to bind client\n");
        }

        log_msg(LOG_INFO, "New connection\n");
        pthread_mutex_unlock(&serv->plot_sockets_mtx);
    }

    assert(0);
    pthread_exit(NULL);
}

extern struct server *server_create(unsigned wq_workers_nr) {
    struct server *server = malloc(sizeof(struct server));
    if ( server == NULL ) return NULL;

    server->pool = server_worker_pool_create(wq_workers_nr);
    if ( server->pool == NULL ) goto free_server;

    server->plot_sockets = NULL;
    if ( pthread_mutex_init(&server->plot_sockets_mtx, NULL) ) goto destroy_pool;

    server->inactive_plot_sockets = NULL;
    if ( pthread_mutex_init(&server->inactive_plot_sockets_mtx, NULL) ) goto destroy_mtx;

    server->epfd = epoll_create1(0);
    if ( server->epfd == -1 ) goto destroy_inactive_mtx;

    if ( pthread_create(&server->main_thread, NULL, server_main_thread_loop, server) ) goto close_epfd;


    return server;

close_epfd:
    close(server->epfd);
destroy_inactive_mtx:
    pthread_mutex_destroy(&server->inactive_plot_sockets_mtx);
destroy_mtx:
    pthread_mutex_destroy(&server->plot_sockets_mtx);
destroy_pool:
    server_worker_pool_destroy(server->pool);
free_server:
    free(server);
    return NULL;
}

extern int server_add_plot(struct server *server, plot_constructor_t pc, unsigned short port) {
    struct plot_socket *ps = malloc(sizeof(struct plot_socket));
    if ( ps == NULL ) return -1;

    ps->active = true;
    ps->port = port;
    ps->pc = pc;
    listc_init(ps, plot_socket_ring);
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if ( sockfd == -1 ) goto free_ps;

    struct sockaddr_in sin = {
        .sin_addr = {INADDR_ANY},
        .sin_port = htons(port),
        .sin_family = AF_INET
    };

    if ( bind(sockfd, &sin, sizeof(sin)) ) goto close_sockfd;

    if ( listen(sockfd, SOMAXCONN) ) goto close_sockfd;

    ps->sockfd = sockfd;

    pthread_mutex_lock(&server->plot_sockets_mtx);

    struct epoll_event ev = {
        .data.ptr = ps,
        .events = EPOLLIN
    };

    if ( epoll_ctl(server->epfd, EPOLL_CTL_ADD, ps->sockfd, &ev) ) {
        pthread_mutex_unlock(&server->plot_sockets_mtx);
        goto close_sockfd;
    }

    listc_add_item_back(server, plot_sockets, ps, plot_socket_ring);
    pthread_mutex_unlock(&server->plot_sockets_mtx);

    return 0;

close_sockfd:
    close(sockfd);
free_ps:
    free(ps);
    return -1;
}

extern int server_remove_plot(struct server *server, unsigned short port) {
    assert(server);

    pthread_mutex_lock(&server->plot_sockets_mtx);
    if ( server->plot_sockets != NULL ) {
        struct plot_socket *iter = server->plot_sockets;
        do {
            if ( iter->port == port ) {
                epoll_ctl(server->epfd, EPOLL_CTL_DEL, iter->sockfd, NULL);
                close(iter->sockfd);
                iter->sockfd = -1;
                iter->active = false;
                listc_remove_item(server, plot_sockets, iter, plot_socket_ring);

                pthread_mutex_lock(&server->inactive_plot_sockets_mtx);
                listc_add_item_back(server, inactive_plot_sockets, iter, plot_socket_ring);
                pthread_mutex_unlock(&server->inactive_plot_sockets_mtx);

                pthread_mutex_unlock(&server->plot_sockets_mtx);
                return 0;
            }
        } while ( iter != server->plot_sockets );
    }
    pthread_mutex_unlock(&server->plot_sockets_mtx);

    return -1;
}

extern void server_destroy(struct server *server) {
    assert(server);

    pthread_cancel(server->main_thread);
    pthread_join(server->main_thread, NULL);
    //Now we have an exclusive access to this object

    server_worker_pool_destroy(server->pool);
    
    server_cleanup_inactive_plot_sockets(server);

    pthread_mutex_destroy(&server->plot_sockets_mtx);
    pthread_mutex_destroy(&server->inactive_plot_sockets_mtx);

    close(server->epfd);

    if ( server->plot_sockets != NULL ) {
        struct plot_socket *iter = server->plot_sockets;

        do {
            struct plot_socket *next = listc_get_next(iter, plot_socket_ring);
            close(iter->sockfd);
            free(iter);
            iter = next;
        } while ( iter != server->plot_sockets );
    }

    free(server);
}
