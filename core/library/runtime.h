/* The C runtime the OpenMake libraries are built with. A shared Amiga
 * library has no start-up code, so it cannot take libnix's malloc, stdio or
 * maths: these stand in, on exec.library alone. Built for the host test
 * (OM_RT_HOST) every name gets rt_ in front, to sit beside the host's.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef OM_RUNTIME_H
#define OM_RUNTIME_H

#include <stdarg.h>
#include <stddef.h>

#ifdef OM_RT_HOST
#define RT(name) rt_##name
#else
#define RT(name) name
#endif

void *RT(malloc)(size_t n);
void *RT(calloc)(size_t n, size_t size);
void *RT(realloc)(void *p, size_t n);
void RT(free)(void *p);

void *RT(memcpy)(void *dst, const void *src, size_t n);
void *RT(memmove)(void *dst, const void *src, size_t n);
void *RT(memset)(void *dst, int c, size_t n);
int RT(memcmp)(const void *a, const void *b, size_t n);
size_t RT(strlen)(const char *s);
int RT(strcmp)(const char *a, const char *b);
int RT(strncmp)(const char *a, const char *b, size_t n);
char *RT(strchr)(const char *s, int c);
char *RT(strrchr)(const char *s, int c);
size_t RT(strcspn)(const char *s, const char *reject);

void RT(qsort)(void *base, size_t n, size_t size, int (*cmp)(const void *, const void *));

/* %d %i %u %c %s %% and %f, %.Nf and %.*f; l before d, i or u. */
int RT(vsnprintf)(char *buf, size_t len, const char *fmt, va_list ap);
int RT(snprintf)(char *buf, size_t len, const char *fmt, ...);
double RT(strtod)(const char *s, char **end);
float RT(strtof)(const char *s, char **end);

#endif
