#include "log.h"
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#ifdef LOG_MT_SAFETY
#include <pthread.h>
#endif

#ifndef __STDC_NO_ATOMICS__
#include <stdatomic.h>
#define LOG_ATOMIC
#endif

#define LOG_LVL_MASK (LOG_DEBUG | LOG_INFO | LOG_WARN | LOG_CRITICAL)
#define LOG_GET_LVL(flags) ((flags) & LOG_LVL_MASK)
#define LOG_GET_OPTS(flags) ((flags) & ~(LOG_LVL_MASK))

#ifdef LOG_ATOMIC
static _Atomic log_flags_t log_flags = LOG_WARN;
#else
static log_flags_t log_flags = LOG_WARN;
#endif

#ifdef LOG_MT_SAFETY
static pthread_mutex_t log_mtx = PTHREAD_MUTEX_INITIALIZER;
#define log_lock() pthread_mutex_lock(&log_mtx)
#define log_unlock() pthread_mutex_unlock(&log_mtx)
#else
#define log_lock() \
    do {           \
    } while ( 0 )
#define log_unlock() \
    do {             \
    } while ( 0 )
#endif

void log_set_flags(log_flags_t flags) {
#ifdef LOG_ATOMIC
    log_flags = flags;
#else
    log_lock();
    log_flags = flags;
    log_unlock();
#endif
}

log_flags_t log_get_flags() {
#ifdef LOG_ATOMIC
    log_flags_t flags = log_flags;
#else
    log_lock();
    log_flags_t flags = log_flags;
    log_unlock();
#endif

    return flags;
}

#ifdef LOG_ATOMIC
static _Atomic(FILE *) log_fp = NULL;
#else
static FILE *log_fp = NULL;
#endif

void log_set_file(FILE *fp) {
#ifdef LOG_ATOMIC
    log_fp = fp;
#else
    log_lock();
    log_fp = fp;
    log_unlock();
#endif
}

FILE *log_get_file() {
#ifdef LOG_ATOMIC
    FILE *fp = log_fp;
#else
    log_lock();
    FILE *fp = log_fp;
    log_unlock();
#endif

    return fp;
}

static const char DEBUG_COLOR[] = "\e[1;32m";
static const char INFO_COLOR[] = "\e[1;34m";
static const char WARN_COLOR[] = "\e[1;33m";
static const char CRITICAL_COLOR[] = "\e[1;31m";
static const char CLEAR_COLOR[] = "\e[0m";

static const char DEBUG_BANNER[] = "DEBUG: ";
static const char INFO_BANNER[] = "INFO: ";
static const char WARN_BANNER[] = "WARN: ";
static const char CRITICAL_BANNER[] = "CRITICAL: ";

#ifdef LOG_PRINT_ONCE

#define shiftptr(ptr, off) ((void *)(((char *)(ptr)) + (off)))

// Copy literal to string only if there enough space for the whole literal
// Return the number of chars which were copied ( 0 or sizeof(literal) - 1)
#define strcpy_literal_safe(s, s_size, l) ((sizeof(l) > (s_size)) ? 0 : (memcpy((s), (l), sizeof(l)), (sizeof(l) - 1)))

static size_t log_color_print(log_flags_t flags, char *buff, size_t left) {
    if ( flags & LOG_NO_COLOR )
        return 0;

    switch ( LOG_GET_LVL(flags) ) {
        case LOG_DEBUG: return strcpy_literal_safe(buff, left, DEBUG_COLOR);
        case LOG_INFO: return strcpy_literal_safe(buff, left, INFO_COLOR);
        case LOG_WARN: return strcpy_literal_safe(buff, left, WARN_COLOR);
        case LOG_CRITICAL: return strcpy_literal_safe(buff, left, CRITICAL_COLOR);
        default: assert(0); return 0;
    }
}

static size_t log_clear_color_print(log_flags_t flags, char *buff, size_t left) {
    if ( flags & LOG_NO_COLOR )
        return 0;

    return strcpy_literal_safe(buff, left, CLEAR_COLOR);
}

static size_t log_banner_print(log_flags_t flags, char *buff, size_t left) {
    if ( flags & LOG_NO_BANNER )
        return 0;

    switch ( LOG_GET_LVL(flags) ) {
        case LOG_DEBUG: return strcpy_literal_safe(buff, left, DEBUG_BANNER);
        case LOG_INFO: return strcpy_literal_safe(buff, left, INFO_BANNER);
        case LOG_WARN: return strcpy_literal_safe(buff, left, WARN_BANNER);
        case LOG_CRITICAL: return strcpy_literal_safe(buff, left, CRITICAL_BANNER);
        default: assert(0); return 0;
    }
}

static size_t log_time_print(log_flags_t flags, char *buff, size_t left) {
    if ( flags & LOG_NO_TIME )
        return 0;

    time_t t;
    time(&t);
    struct tm tm;
    localtime_r(&t, &tm);

    return strftime(buff, left, "[%b %0d %T] ", &tm);
}

#else   // LOG_PRINT_ONCE

static void log_color(log_flags_t flags) {
    if ( flags & LOG_NO_COLOR )
        return;

    switch ( LOG_GET_LVL(flags) ) {
        case LOG_DEBUG: fputs(DEBUG_COLOR, log_fp); break;
        case LOG_INFO: fputs(INFO_COLOR, log_fp); break;
        case LOG_WARN: fputs(WARN_COLOR, log_fp); break;
        case LOG_CRITICAL: fputs(CRITICAL_COLOR, log_fp); break;
        default: assert(0); return;
    }
}

static void log_clear_color(log_flags_t flags) {
    if ( flags & LOG_NO_COLOR )
        return;

    fputs(CLEAR_COLOR, log_fp);
}

static void log_banner(log_flags_t flags) {
    if ( flags & LOG_NO_BANNER )
        return;

    switch ( LOG_GET_LVL(flags) ) {
        case LOG_DEBUG: fputs(DEBUG_BANNER, log_fp); break;
        case LOG_INFO: fputs(INFO_BANNER, log_fp); break;
        case LOG_WARN: fputs(WARN_BANNER, log_fp); break;
        case LOG_CRITICAL: fputs(CRITICAL_BANNER, log_fp); break;
        default: assert(0); return;
    }
}

static void log_time(log_flags_t flags) {
    if ( flags & LOG_NO_TIME )
        return;

    char buf[256];

    time_t t;
    time(&t);
    struct tm tm;
    localtime_r(&t, &tm);

    strftime(buf, sizeof(buf), "[%b %0d %T] ", &tm);
    fputs(buf, log_fp);
}

#endif   // LOG_PRINT_ONCE

#ifdef LOG_PRINT_ONCE
static void vlog_printf(log_flags_t flags, const char *fmt, va_list va) {
#ifndef LOG_ATOMIC
    log_lock();
#endif
    log_flags_t lf = log_flags;
    FILE *fp = log_fp;

    int lvl = LOG_GET_LVL(flags);

    int main_lvl = LOG_GET_LVL(lf);
    flags |= LOG_GET_OPTS(lf);

    if ( lvl < main_lvl ) {
#ifndef LOG_ATOMIC
        log_unlock();
#endif
        return;
    }

    char log_line_base[LOG_PRINT_LINE_LEN + 1];
    char *log_line = log_line_base;
    size_t left = sizeof(log_line_base);
    size_t res;

    res = log_color_print(flags, log_line, left);
    left -= res;
    log_line = shiftptr(log_line, res);
    res = log_time_print(flags, log_line, left);
    left -= res;
    log_line = shiftptr(log_line, res);
    res = log_banner_print(flags, log_line, left);
    left -= res;
    log_line = shiftptr(log_line, res);
    res = log_clear_color_print(flags, log_line, left);
    left -= res;
    log_line = shiftptr(log_line, res);

    vsnprintf(log_line, left, fmt, va);

    fputs(log_line_base, fp);

#ifndef LOG_ATOMIC
    log_unlock();
#endif
}

#else   // LOG_PRINT_ONCE

static void vlog_printf(log_flags_t flags, const char *fmt, va_list va) {
#ifndef LOG_ATOMIC
    log_lock();
#endif
    log_flags_t lf = log_flags;
    FILE *fp = log_fp;

    int lvl = LOG_GET_LVL(flags);

    int main_lvl = LOG_GET_LVL(lf);
    flags |= LOG_GET_OPTS(lf);

    if ( lvl < main_lvl ) {
#ifndef LOG_ATOMIC
        log_unlock();
#endif
        return;
    }

#ifdef LOG_ATOMIC
    log_lock();
#endif
    log_color(flags);
    log_time(flags);
    log_banner(flags);
    log_clear_color(flags);
    vfprintf(fp, fmt, va);

    log_unlock();
}

#endif   // LOG_PRINT_ONCE

void log_msg(log_flags_t flags, const char *fmt, ...) {
    int e = errno;

    va_list arg;
    va_start(arg, fmt);
    vlog_printf(flags, fmt, arg);
    va_end(arg);

    errno = e;
}

void vlog_msg(log_flags_t flags, const char *fmt, va_list va) {
    int e = errno;
    vlog_printf(flags, fmt, va);
    errno = e;
}
