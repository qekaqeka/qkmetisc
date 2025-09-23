#pragma once

#include <stdio.h>
#include <stdarg.h>

#define LOG_DEBUG       0b00000
#define LOG_INFO        0b00001
#define LOG_WARN        0b00010
#define LOG_CRITICAL    0b00011
#define LOG_NO_TIME     0b10000
#define LOG_NO_COLOR    0b00100
#define LOG_NO_BANNER   0b01000

extern void log_set_flags(int flags);
extern int log_get_flags();

extern void log_set_file(FILE *fp);
extern FILE *log_get_file();

[[gnu::format(printf, 2, 3)]]
extern void log_msg(int flags, const char *fmt, ...);
extern void vlog_msg(int flags, const char *fmt, va_list va);

