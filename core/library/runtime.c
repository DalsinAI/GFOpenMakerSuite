/* The OpenMake libraries' C runtime: see runtime.h.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "runtime.h"

#include <stdint.h>

#ifdef OM_RT_HOST
#include <stdlib.h>
#define RAW_ALLOC(n) malloc(n)
#define RAW_FREE(p) free(p)
#else
#include <exec/memory.h>
#include <proto/exec.h>
extern struct ExecBase *SysBase;
#define RAW_ALLOC(n) AllocVec((ULONG)(n), MEMF_ANY)
#define RAW_FREE(p) FreeVec(p)
#endif

/* ---- Memory: a size before each block, so realloc knows how much to keep */

typedef union {
    size_t size;
    double align;
} head_t;

void *RT(malloc)(size_t n)
{
    head_t *h;
    if (!n) n = 1;
    if (n > ((size_t)-1) / 2) return NULL;
    h = (head_t *)RAW_ALLOC(n + sizeof *h);
    if (!h) return NULL;
    h->size = n;
    return h + 1;
}

void RT(free)(void *p)
{
    if (p) RAW_FREE((head_t *)p - 1);
}

void *RT(calloc)(size_t n, size_t size)
{
    void *p;
    if (size && n > ((size_t)-1) / 2 / size) return NULL;
    p = RT(malloc)(n * size);
    if (p) RT(memset)(p, 0, n * size);
    return p;
}

void *RT(realloc)(void *p, size_t n)
{
    void *q;
    size_t had;
    if (!p) return RT(malloc)(n);
    had = ((head_t *)p - 1)->size;
    q = RT(malloc)(n);
    if (!q) return NULL;
    RT(memcpy)(q, p, had < n ? had : n);
    RT(free)(p);
    return q;
}

/* ---- Strings */

void *RT(memcpy)(void *dst, const void *src, size_t n)
{
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    while (n--) *d++ = *s++;
    return dst;
}

void *RT(memmove)(void *dst, const void *src, size_t n)
{
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    if (d <= s || d >= s + n) return RT(memcpy)(dst, src, n);
    while (n--) d[n] = s[n];
    return dst;
}

void *RT(memset)(void *dst, int c, size_t n)
{
    unsigned char *d = (unsigned char *)dst;
    while (n--) *d++ = (unsigned char)c;
    return dst;
}

int RT(memcmp)(const void *a, const void *b, size_t n)
{
    const unsigned char *x = (const unsigned char *)a, *y = (const unsigned char *)b;
    for (; n; n--, x++, y++)
        if (*x != *y) return *x - *y;
    return 0;
}

size_t RT(strlen)(const char *s)
{
    const char *p = s;
    while (*p) p++;
    return (size_t)(p - s);
}

int RT(strcmp)(const char *a, const char *b)
{
    while (*a && *a == *b) a++, b++;
    return (unsigned char)*a - (unsigned char)*b;
}

int RT(strncmp)(const char *a, const char *b, size_t n)
{
    for (; n; n--, a++, b++) {
        if (*a != *b) return (unsigned char)*a - (unsigned char)*b;
        if (!*a) return 0;
    }
    return 0;
}

char *RT(strchr)(const char *s, int c)
{
    for (;; s++) {
        if (*s == (char)c) return (char *)s;
        if (!*s) return NULL;
    }
}

char *RT(strrchr)(const char *s, int c)
{
    const char *found = NULL;
    for (;; s++) {
        if (*s == (char)c) found = s;
        if (!*s) return (char *)found;
    }
}

size_t RT(strcspn)(const char *s, const char *reject)
{
    size_t n = 0;
    for (; s[n]; n++)
        if (RT(strchr)(reject, s[n])) break;
    return n;
}

/* ---- qsort: a heap sort, so no recursion on a caller's small stack */

static void swap(unsigned char *a, unsigned char *b, size_t size)
{
    while (size--) {
        unsigned char t = *a;
        *a++ = *b;
        *b++ = t;
    }
}

static void sift(unsigned char *base, size_t root, size_t n, size_t size, int (*cmp)(const void *, const void *))
{
    for (;;) {
        size_t child = root * 2 + 1;
        if (child >= n) return;
        if (child + 1 < n && cmp(base + child * size, base + (child + 1) * size) < 0) child++;
        if (cmp(base + root * size, base + child * size) >= 0) return;
        swap(base + root * size, base + child * size, size);
        root = child;
    }
}

void RT(qsort)(void *base, size_t n, size_t size, int (*cmp)(const void *, const void *))
{
    unsigned char *b = (unsigned char *)base;
    size_t i;
    if (n < 2) return;
    for (i = n / 2; i-- > 0;) sift(b, i, n, size, cmp);
    for (i = n - 1; i > 0; i--) {
        swap(b, b + i * size, size);
        sift(b, 0, i, size, cmp);
    }
}
