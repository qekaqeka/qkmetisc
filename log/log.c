#include "log.h"
#include <assert.h>
#include <pthread.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#define LOG_LVL_MASK 0b00011
#define LOG_GET_LVL(flags) ((flags) & LOG_LVL_MASK)

static int log_flags = LOG_WARN;
static pthread_mutex_t log_mtx = PTHREAD_MUTEX_INITIALIZER;

void log_set_flags(int flags) {
    log_flags = flags;
}

int log_get_flags() {
    return log_flags;
}

static FILE *log_fp = NULL;

void log_set_file(FILE *fp) {
    log_fp = fp;
}

FILE *log_get_file() {
    return log_fp;
}

static void log_color(int flags) {
    if ( flags & LOG_NO_COLOR ) return;

    switch ( LOG_GET_LVL(flags) ) {
        case LOG_DEBUG: fputs("\e[1;32m", log_fp); break;
        case LOG_INFO: fputs("\e[1;34m", log_fp); break;
        case LOG_WARN: fputs("\e[1;33m", log_fp); break;
        case LOG_CRITICAL: fputs("\e[1;31m", log_fp); break;
        default: assert(0); return;
    }
}

static void log_clear_color(int flags) {
    if ( flags & LOG_NO_COLOR ) return;

    fputs("\e[0m", log_fp);
}

static void log_banner(int flags) {
    if ( flags & LOG_NO_BANNER ) return;

    switch ( LOG_GET_LVL(flags) ) {
        case LOG_DEBUG: fputs("DEBUG: ", log_fp); break;
        case LOG_INFO: fputs("INFO: ", log_fp); break;
        case LOG_WARN: fputs("WARN: ", log_fp); break;
        case LOG_CRITICAL: fputs("CRITICAL: ", log_fp); break;
        default: assert(0); return;
    }
}

static void log_time(int flags) {
    if ( flags & LOG_NO_TIME ) return;

    char buff[256];

    time_t t;
    time(&t);
    struct tm tm;
    localtime_r(&t ,&tm);

    strftime(buff, sizeof(buff), "[%b %0d %T] ", &tm);
    fputs(buff, log_fp);
}

static void vlog_printf(int flags, const char *fmt, va_list va) {
    assert(log_fp);

    int lvl = flags & LOG_LVL_MASK;
    int main_lvl = log_flags & LOG_LVL_MASK;

    if ( lvl < main_lvl ) return;

    pthread_mutex_lock(&log_mtx);

    log_color(flags);

    log_time(flags);

    log_banner(flags);

    log_clear_color(flags);

    vfprintf(log_fp, fmt, va);

    pthread_mutex_unlock(&log_mtx);
}

void log_msg(int flags, const char *fmt, ...) {
    int e = errno;

    va_list arg;

    va_start(arg, fmt);

    vlog_printf(flags, fmt, arg);

    va_end(arg);

    errno = e;
}

void vlog_msg(int flags, const char *fmt, va_list va) {
    int e = errno;
    vlog_printf(flags, fmt, va);
    errno = e;
}
