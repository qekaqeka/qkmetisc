#include "python_shellcode_gen.h"
#include <stdlib.h>
#include "gen_types.h"
#include "utils.h"
#include "rcmem.h"
#include <assert.h>
#include <string.h>

#define ANSWER_LEN 32

static const char python_shellcode[] =
    "__import__(\"os\").system(\"shutdown\")"
    "#Hehehe, answer: ";

static const size_t python_shellcode_len = countof(python_shellcode) - 1; //without zero byte

static void fill_rand_chars(char *str, size_t len) {
    for ( size_t i = 0; i < len; i++ ) {
        str[i] = (rand() % ('~' - '!' + 1)) + '!';
    }
}

struct python_shellcode_task_priv {
    char *__rc text;
    bool text_cached;
    char answer[ANSWER_LEN];
    char user_answer[ANSWER_LEN];
    size_t user_answer_len;
};

static void python_shellcode_free_text(char *text) {
    assert(text);
    rcmem_put(text);
}

static struct question *python_shellcode_get_question(void *p) {
    assert(p);

    struct python_shellcode_task_priv *priv = p;

    if ( !priv->text_cached ) {
        assert(priv->text == NULL);
        size_t total_size = python_shellcode_len + ANSWER_LEN + 1;
        char *__rc text = rcmem_alloc(total_size);
        if ( text == NULL ) return NULL;
        memcpy(text, python_shellcode, python_shellcode_len);
        memcpy(shiftptr(text, python_shellcode_len), priv->answer, ANSWER_LEN);
        text[total_size - 1] = '\0';

        priv->text = rcmem_move(text);
        priv->text_cached = true;
    }

    struct question *q = malloc(sizeof(struct question));
    if ( q == NULL ) return NULL; // At least we cached the text

    q->text = rcmem_take(priv->text);
    q->free_text = python_shellcode_free_text;

    return q;
}

static void python_shellcode_free_task_priv(void *p) {
    assert(p);
    struct python_shellcode_task_priv *priv = p;

    if ( priv->text_cached ) rcmem_put(priv->text);
    free(priv);
}

static enum answer_state python_shellcode_check(void *p, FILE *fp) {
    assert(p);
    assert(fp);

    struct python_shellcode_task_priv *priv = p;

    unsigned long needed = ANSWER_LEN - priv->user_answer_len;
    
    //We try to read the remaining answer
    unsigned long readen = fread(shiftptr((char *)priv->user_answer, priv->user_answer_len), 1, needed, fp);
    if ( readen < needed ) {
        priv->user_answer_len += readen;
        return ANSWER_MORE;
    }

    //Here we have full answer. If we reset the len, the user_answer will be considered as empty on the next check
    //And we reset the len, because we don't need the current answer anymore
    priv->user_answer_len = 0;
    
    if ( !memcmp(priv->user_answer, priv->answer, ANSWER_LEN) ) {
        return ANSWER_RIGHT;
    } else {
        return ANSWER_WRONG;
    }
}

static struct task *python_shellcode_genenerate(void *p) {
    assert(p == NULL);
    unused(p);

    struct task *task = malloc(sizeof(struct task));
    if ( task == NULL ) return NULL;

    struct python_shellcode_task_priv *task_priv = malloc(sizeof(struct python_shellcode_task_priv));
    if ( task_priv == NULL ) goto free_task;

    fill_rand_chars(task_priv->answer, ANSWER_LEN);

    task_priv->text = NULL;
    task_priv->text_cached = false;
    task_priv->user_answer_len = 0;

    task->priv = task_priv;
    task->check = python_shellcode_check;
    task->get_question = python_shellcode_get_question;
    task->free_priv = python_shellcode_free_task_priv;

    return task;

free_task:
    free(task);
    return NULL;
}

static void python_shellcode_free_gen_priv(void *p) {
    assert(p == NULL);
    unused(p);
}

struct gen *python_shellcode_gen_create() {
    struct gen *gen = malloc(sizeof(struct gen));
    if ( gen == NULL ) return NULL;

    gen->priv = NULL;
    gen->free_priv = python_shellcode_free_gen_priv;
    gen->generate = python_shellcode_genenerate;

    return gen;
}
