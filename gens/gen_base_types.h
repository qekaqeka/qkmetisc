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
    int (*verify)(void *priv, FILE *answer_stream);
    void (*free_priv)(void *priv);
};
