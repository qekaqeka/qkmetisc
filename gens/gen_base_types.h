#pragma once

#include <stdio.h>
#include <time.h>

struct gen {
    void *priv;
    struct task *(*generate)(void *priv);
    void (*free_priv)(void *priv);
};

struct question {
    time_t timeout;
    char *text;
};

struct task {
    void *priv;
    struct question *(*get_question)(void *priv);
    void (*free_question)(struct question *q);
    enum answer_state (*check)(void *priv, FILE *answer_stream);
    void (*free_priv)(void *priv);
};

