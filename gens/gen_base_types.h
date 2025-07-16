#pragma once

#include <stdio.h>

struct gen {
    void *priv;
    struct task *(*generate)(void *priv);
    void (*free_priv)(void *priv);
};

struct task {
    void *priv;
    char *(*get_question)(void *priv);
    void (*free_question)(char *q);
    enum answer_state (*check)(void *priv, FILE *answer_stream);
    void (*free_priv)(void *priv);
};

enum answer_state {
    ANSWER_WRONG,
    ANSWER_MORE,
    ANSWER_RIGHT
};
