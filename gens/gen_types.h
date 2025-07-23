#pragma once

#include <stdio.h>
#include <time.h>

struct gen {
    void *priv;
    struct task *(*generate)(void *priv);
    void (*free_priv)(void *priv);
};

struct task {
    void *priv;
    struct question *(*get_question)(void *priv);
    enum answer_state (*check)(void *priv, FILE *answer_stream);
    void (*free_priv)(void *priv);
};

struct question {
    char *text;
    void (*free_text)(char *text);
};
