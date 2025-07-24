#pragma once

#ifndef __GNUC__
#define __typeof__(X) typeof(X)
#endif

#define unused(X) ((void)(X))
#define shiftptr(ptr, off) ((typeof(ptr))((char *)(ptr) + (off)))
#define countof(X) (sizeof(X) / sizeof((X)[0]))
#define containerof(type, ptr, memb) ((type *)(shiftptr((ptr), -offsetof(type, memb))))
#define min(X, Y) ((X) > (Y) ? (Y) : (X))
#define max(X, Y) ((X) > (Y) ? (X) : (Y))
