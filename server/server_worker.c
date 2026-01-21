#include "server_worker.h"
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/fcntl.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include "bstream.h"
#include "listc.h"
#include "log.h"

struct client {
    int sockfd;
    struct listc client_ring;
    struct client_timer *timers;
    struct server_worker_event *event;

    struct plot *plot;
    struct plot_task *current_task;
    struct question *current_question;
    struct bstream *send_stream;
};

struct client_timer {
    int timerfd;
    struct client *client;
    struct listc timers_ring;
    struct server_worker_event *event;
};

struct server_worker {
    int epfd;
    pthread_t thread;
    struct client *clients;
    unsigned clients_nr;
    pthread_mutex_t clients_mtx;
};

enum server_worker_event_type {
    SWET_TIMER,
    SWET_CLIENT
};

struct server_worker_event {
    union {
        struct client_timer *timer;
        struct client *client;
    } data;
    enum server_worker_event_type type;
};

static enum answer_state plot_task_check_fd(struct plot_task *pt, int fd) {
    assert(fd > -1);
    assert(pt);
    int new_fd = dup(fd);
    if ( new_fd == -1 ) return ANSWER_WRONG;

    FILE *fp = fdopen(new_fd, "r");
    if ( fp == NULL ) {
        int e = errno;
        close(new_fd);
        errno = e;
        return ANSWER_WRONG;
    }

    enum answer_state res = plot_task_check(pt, fp);
    fclose(fp);   // new_fd is also closed by this call
    return res;
}

static void client_timer_destroy(struct server_worker *worker, struct client_timer *timer) {
    assert(timer);

    epoll_ctl(worker->epfd, EPOLL_CTL_DEL, timer->timerfd, NULL);
    close(timer->timerfd);
    listc_remove_item(&timer->client->timers, timer, timers_ring);
    free(timer->event);
    free(timer);
}

static int client_timer_setup(struct server_worker *worker, struct client *client, unsigned long msec_timeout) {
    assert(msec_timeout > 0);

    struct client_timer *timer = malloc(sizeof(struct client_timer));
    if ( timer == NULL ) return -1;

    struct server_worker_event *event = malloc(sizeof(struct server_worker_event));
    if ( event == NULL ) goto free_timer;

    event->data.timer = timer;
    event->type = SWET_TIMER;
    timer->event = event;

    timer->timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
    if ( timer->timerfd == -1 ) goto free_event;

    timer->client = client;

    struct epoll_event ev = {.data.ptr = timer->event, .events = EPOLLONESHOT | EPOLLIN};

    if ( epoll_ctl(worker->epfd, EPOLL_CTL_ADD, timer->timerfd, &ev) ) goto close_timerfd;

    listc_init(timer, timers_ring);
    listc_add_item_back(&client->timers, timer, timers_ring);

    time_t secs = msec_timeout / 1000;
    unsigned long nsecs = (msec_timeout % 1000) * 1000000;

    struct itimerspec its = {
        .it_interval = {.tv_nsec = 0,   .tv_sec = 0     },
        .it_value = {.tv_sec = secs, .tv_nsec = nsecs}
    };

    timerfd_settime(timer->timerfd, 0, &its, NULL);

    return 0;

close_timerfd:
    close(timer->timerfd);
free_event:
    free(event);
free_timer:
    free(timer);
    return -1;
}

static struct client *client_create(struct server_worker *wr, int clientfd, plot_constructor_t pc) {
    struct client *cl = malloc(sizeof(struct client));
    if ( cl == NULL ) return NULL;

    cl->plot = pc();
    if ( cl->plot == NULL ) goto free_cl;

    cl->send_stream = bstream_create();
    if ( cl->send_stream == NULL ) goto destroy_plot;

    int old_fl = fcntl(clientfd, F_GETFL);
    fcntl(clientfd, F_SETFL, O_NONBLOCK);   // this action doesn't raise error
    cl->sockfd = clientfd;

    listc_init(cl, client_ring);
    cl->current_question = NULL;
    cl->current_task = NULL;
    cl->event = NULL;
    cl->timers = NULL;

    
    // Event creation
    struct server_worker_event *event = malloc(sizeof(struct server_worker_event));
    if ( event == NULL ) goto destroy_cl;

    cl->event = event;
    event->data.client = cl;
    event->type = SWET_CLIENT;

    struct epoll_event ev = {.events = EPOLLRDHUP, .data.ptr = event};

    pthread_mutex_lock(&wr->clients_mtx);

    if ( epoll_ctl(wr->epfd, EPOLL_CTL_ADD, cl->sockfd, &ev) ) {
        pthread_mutex_unlock(&wr->clients_mtx);
        goto free_event;
    }
    listc_add_item_back(&wr->clients, cl, client_ring);
    wr->clients_nr++;

    pthread_mutex_unlock(&wr->clients_mtx);

    log_msg(LOG_DEBUG, "Client %p was created on worker %p\n", cl, wr);
    return cl;

free_event:
    free(event);
destroy_cl:
    bstream_destroy(cl->send_stream);
    fcntl(clientfd, F_SETFL, old_fl);
destroy_plot:
    plot_destroy(cl->plot);
free_cl:
    free(cl);
    return NULL;
}

static void client_disconnect(struct server_worker *wr, struct client *cl) {
    pthread_mutex_lock(&wr->clients_mtx);
    epoll_ctl(wr->epfd, EPOLL_CTL_DEL, cl->sockfd, NULL);
    listc_remove_item(&wr->clients, cl, client_ring);
    wr->clients_nr--;
    pthread_mutex_unlock(&wr->clients_mtx);

    close(cl->sockfd);
    free(cl->event);
    plot_destroy(cl->plot);
    bstream_destroy(cl->send_stream);
    if ( cl->current_question ) question_destroy(cl->current_question);
    if ( cl->current_task ) plot_task_destroy(cl->current_task);

    while ( cl->timers ) client_timer_destroy(wr, cl->timers);

    log_msg(LOG_DEBUG, "Client %p was destroyed\n", cl);
    free(cl);
}

static void client_send_task_text(struct client *client, const char *text, unsigned long timeout) {
    struct bstream *bst = client->send_stream;

    if ( timeout != 0 ) {
        char buf[256];
        snprintf(buf, sizeof(buf), "\nAnswer (timeout %lu msec): ", timeout);

        bstream_write_borrow(bst, text, strlen(text));
        bstream_write_mem(bst, buf, strlen(buf)); //We can't borrow from stack
    } else {
        bstream_write_borrow(bst, text, strlen(text));
        const char extra_text[] = "\nAnswer (no timeout): ";
        bstream_write_borrow(bst, extra_text, sizeof(extra_text));
    }
}

static int client_next_task(struct server_worker *worker, struct client *client) {
    assert(worker);
    assert(client);

    bstream_flush(client->send_stream);

    if ( client->current_task != NULL )   // Possibly we haven't given any task for this client
        plot_task_destroy(client->current_task);

    client->current_task = NULL;

    struct plot_task *new_task = plot_get_task(client->plot);
    if ( new_task == NULL ) {
        if ( errno != ENOTASK ) { log_msg(LOG_WARN, "Failed to create task for client\n"); }
        return -1;
    }

    struct question *q = plot_task_get_question(new_task);
    if ( q == NULL ) goto destroy_plot_task;

    struct epoll_event ev = {
        .data.ptr = client->event,
        .events = EPOLLIN | EPOLLOUT | EPOLLRDHUP
    };   // Poll for EPOLLOUT now

    // Notice that we only modify epoll entry, it must be added before this
    if ( epoll_ctl(worker->epfd, EPOLL_CTL_MOD, client->sockfd, &ev) ) goto destroy_question;

    client->current_task = new_task;
    client->current_question = q;
    const char *text = question_get_text(q);

    client_send_task_text(client, text, plot_task_get_timeout_msec(new_task));

    return 0;

destroy_question:
    question_destroy(q);
destroy_plot_task:
    plot_task_destroy(new_task);
    return -1;
}

static int client_send_text(struct server_worker *worker, struct client *client) {
    assert(worker);
    assert(client);

    if ( client->current_question == NULL ) return 0;   // No questions for client now

    struct bstream *bst = client->send_stream;

    ssize_t written = bstream_read_fd(bst, client->sockfd, bstream_len(bst));
    if ( written == 0 ) log_msg(LOG_WARN, "Failed to send data to clientfd %d\n", client->sockfd);

    if ( bstream_len(bst) == 0 ) {
        // bstream_flush(client->send_stream);   // We don't want to flush the buffer because it alredy has zero len
        question_destroy(client->current_question); //We can destroy question, because now bst doesn't borrow anything
        client->current_question = NULL;

        struct epoll_event ev = {.data.ptr = client->event, .events = EPOLLIN | EPOLLRDHUP};

        if ( epoll_ctl(worker->epfd, EPOLL_CTL_MOD, client->sockfd, &ev) )   // We aren't interested in writting now
            return -1;


        assert(client->current_task);
        unsigned long timeout = plot_task_get_timeout_msec(client->current_task);
        if ( timeout != 0 ) client_timer_setup(worker, client, timeout);
    }

    return 0;
}

[[noreturn]]
static void *worker_handler(void *arg) {
    signal(SIGPIPE, SIG_IGN);   // Block SIGPIPE, otherwise, writting to closed socket will crash program
    struct server_worker *w = arg;

    struct epoll_event ev;

    while ( true ) {
        int res;
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

        /*
         * ATTENTION!!!
         * epoll_wait doesn't have to be a cancellation point. It is now, but it could change in the future
         * ATTENTION!!!
         */
retry:
        res = epoll_wait(w->epfd, &ev, 1, -1);
        if ( res == -1 ) {
            if ( errno == EINTR ) goto retry;
        }
        assert(res != -1);

        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);


        struct server_worker_event *event = ev.data.ptr;

        if ( event->type == SWET_TIMER ) {
            struct client_timer *timer = event->data.timer;
            struct client *client = timer->client;

            client_timer_destroy(w, timer);

            assert(client->current_task);
            if ( plot_task_check_fd(client->current_task, client->sockfd) != ANSWER_RIGHT ) {
                log_msg(LOG_DEBUG, "Client %p was disconnected because of the timeout\n", client);
                client_disconnect(w, client);
            } else {
                if ( client_next_task(w, client) ) {
                    client_disconnect(w, client);
                    continue;
                }
            }
        } else if ( event->type == SWET_CLIENT ) {
            struct client *client = event->data.client;

            if ( ev.events & EPOLLIN ) {
                if ( client->current_task != NULL ) {
                    enum answer_state res = plot_task_check_fd(client->current_task, client->sockfd);

                    if ( res == ANSWER_WRONG ) {
                        log_msg(LOG_DEBUG, "Client %p was disconnected because of wrong answer\n", client);
                        client_disconnect(w, client);
                        continue;
                    } else if ( res == ANSWER_MORE ) {
                        continue;
                    } else if ( res == ANSWER_RIGHT ) {
                        if ( client_next_task(w, client) ) {
                            client_disconnect(w, client);
                            continue;
                        }
                    } else {
                        assert(0);
                    }
                }
            }

            if ( ev.events & EPOLLOUT ) {
                if ( client_send_text(w, client) ) {
                    log_msg(LOG_DEBUG, "Client %p was disconnected because of sending problem\n", client);
                    client_disconnect(w, client);
                    continue;
                }
            }

            if ( ev.events & EPOLLRDHUP ) {
                log_msg(LOG_DEBUG, "Client %p disconnected\n", client);
                client_disconnect(w, client);
                continue;
            }
        } else {
            assert(0);
        }
    }
}

struct server_worker *server_worker_create() {
    struct server_worker *worker = malloc(sizeof(struct server_worker));
    if ( worker == NULL ) return NULL;

    worker->clients_nr = 0;

    worker->clients = NULL;

    worker->epfd = epoll_create1(0);
    if ( worker->epfd == -1 ) goto free_worker;

    if ( pthread_mutex_init(&worker->clients_mtx, NULL) ) goto close_epfd;

    if ( pthread_create(&worker->thread, NULL, worker_handler, worker) ) goto destroy_mtx;

    log_msg(LOG_DEBUG, "Worker %p was created\n", worker);
    return worker;

destroy_mtx:
    pthread_mutex_destroy(&worker->clients_mtx);
close_epfd:
    close(worker->epfd);
free_worker:
    free(worker);
    return NULL;
}

int server_worker_add_client(struct server_worker *worker, int clientfd, plot_constructor_t pc) {
    assert(worker);

    struct client *client = client_create(worker, clientfd, pc);
    if ( client == NULL ) return -1;

    if ( client_next_task(worker, client) ) {
        client_disconnect(worker, client);
        return -1;
    }

    return 0;
}

unsigned server_worker_get_clients_nr(struct server_worker *worker) {
    assert(worker);


    pthread_mutex_lock(&worker->clients_mtx);   // We can use atomics instead of this lock
    unsigned res = worker->clients_nr;
    pthread_mutex_unlock(&worker->clients_mtx);
    return res;
}

void server_worker_destroy(struct server_worker *worker) {
    assert(worker);

    pthread_cancel(worker->thread);
    pthread_join(worker->thread, NULL);

    close(worker->epfd);

    while ( worker->clients != NULL ) { client_disconnect(worker, worker->clients); }

    pthread_mutex_destroy(&worker->clients_mtx);

    log_msg(LOG_DEBUG, "Worker %p was destroyed\n", worker);
    free(worker);
}
