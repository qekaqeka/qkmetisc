#pragma once

#include <stdint.h>
#include <stdio.h>

typedef uint_fast32_t log_flags_t;

#define LOG_DEBUG       ((log_flags_t)0b00000)
#define LOG_INFO        ((log_flags_t)0b00001)
#define LOG_WARN        ((log_flags_t)0b00010)
#define LOG_CRITICAL    ((log_flags_t)0b00011)
#define LOG_NO_TIME     ((log_flags_t)0b10000)
#define LOG_NO_COLOR    ((log_flags_t)0b00100)
#define LOG_NO_BANNER   ((log_flags_t)0b01000)

extern void log_set_flags(log_flags_t flags);
extern log_flags_t log_get_flags();

extern void log_set_file(FILE *fp);
extern FILE *log_get_file();

#ifdef __GNUC__
[[gnu::format(printf, 2, 3)]]
#endif
extern void log_msg(log_flags_t flags, const char *fmt, ...);
extern void vlog_msg(log_flags_t flags, const char *fmt, va_list va);

