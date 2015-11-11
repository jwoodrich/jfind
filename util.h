#ifndef _UTIL_H
#define _UTIL_H

#define CALLOC(N,I,S) calloc_or_die(N, I, S)
#ifdef DEBUG
#define DEBUGOUT(S,...) printf(S,##__VA_ARGS__)
#else
#define DEBUGOUT(S,...)
#endif

#include <stddef.h>

void *calloc_or_die(char *name, size_t nitems, size_t size);

#endif

