#pragma once

#include <stdio.h>
#include <stdarg.h>

#define LOG_DEBUG       0b0000
#define LOG_INFO        0b0001
#define LOG_WARN        0b0010
#define LOG_CRITICAL    0b0011
#define LOG_COLOR    0b0100
#define LOG_BANNER   0b1000

extern void log_set_flags(int flags);
extern int log_get_flags();

extern void log_set_file(FILE *fp);
extern FILE *log_get_file();

[[gnu::format(printf, 2, 3)]]
extern void log_msg(int flags, const char *fmt, ...);
extern void vlog_msg(int flags, const char *fmt, va_list va);

