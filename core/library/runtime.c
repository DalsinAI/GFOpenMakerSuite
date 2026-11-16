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

/* ---- snprintf */

typedef struct {
    char *buf;
    size_t len, at;
} out_t;

static void put(out_t *o, char c)
{
    if (o->buf && o->at + 1 < o->len) o->buf[o->at] = c;
    o->at++;
}

static void put_u64(out_t *o, uint64_t v)
{
    char d[24];
    int n = 0;
    do {
        d[n++] = (char)('0' + (int)(v % 10u));
        v /= 10u;
    } while (v);
    while (n) put(o, d[--n]);
}

/* A double to prec places, rounded as the host rounds: on its exact binary
 * value, a tie to even. Integer arithmetic only, so the same on every CPU. */
static void put_fixed(out_t *o, double v, int prec)
{
    union {
        double d;
        uint64_t u;
    } b;
    uint64_t m, ip, frac = 0;
    int e2, e = 0, sticky = 0, i;
    char dig[40];
    if (prec > 30) prec = 30;
    b.d = v;
    e2 = (int)((b.u >> 52) & 0x7ff);
    m = b.u & 0xfffffffffffffull;
    if (b.u >> 63) put(o, '-');
    if (e2 == 0x7ff) {
        put(o, m ? 'n' : 'i');
        put(o, m ? 'a' : 'n');
        put(o, m ? 'n' : 'f');
        return;
    }
    if (e2) m |= 1ull << 52;
    else e2 = 1;
    e2 -= 1075; /* v = m * 2^e2 */
    if (e2 >= 0) {
        ip = e2 < 11 ? m << e2 : 0xffffffffffffffffull; /* beyond 2^63: not ours to print */
    } else if (e2 > -64) {
        e = -e2;
        ip = m >> e;
        frac = m & ((1ull << e) - 1);
    } else {
        ip = 0;
        e = 64;
        frac = m;
    }
    /* Keep the fraction in 60 bits, so ten times it fits; what falls off
     * only tells a tie from just above one. */
    if (e > 60) {
        int drop = e - 60;
        if (drop >= 64) {
            sticky = frac != 0;
            frac = 0;
        } else {
            sticky = (frac & ((1ull << drop) - 1)) != 0;
            frac >>= drop;
        }
        e = 60;
    }
    for (i = 0; i < prec; i++) {
        if (e) {
            frac *= 10u;
            dig[i] = (char)(frac >> e);
            frac &= (1ull << e) - 1;
        } else {
            dig[i] = 0;
        }
    }
    if (e) {
        uint64_t half = 1ull << (e - 1);
        int last = prec ? dig[prec - 1] : (int)(ip & 1u);
        if (frac > half || (frac == half && (sticky || (last & 1)))) {
            for (i = prec - 1; i >= 0; i--) {
                if (++dig[i] < 10) break;
                dig[i] = 0;
            }
            if (i < 0) ip++;
        }
    }
    put_u64(o, ip);
    if (prec) put(o, '.');
    for (i = 0; i < prec; i++) put(o, (char)('0' + dig[i]));
}

int RT(vsnprintf)(char *buf, size_t len, const char *fmt, va_list ap)
{
    out_t o;
    o.buf = buf;
    o.len = len;
    o.at = 0;
    for (; *fmt; fmt++) {
        int prec = -1, lng = 0;
        if (*fmt != '%') {
            put(&o, *fmt);
            continue;
        }
        fmt++;
        if (*fmt == '.') {
            fmt++;
            if (*fmt == '*') {
                prec = va_arg(ap, int);
                fmt++;
            } else {
                prec = 0;
                while (*fmt >= '0' && *fmt <= '9') prec = prec * 10 + (*fmt++ - '0');
            }
        }
        while (*fmt == 'l') {
            lng = 1;
            fmt++;
        }
        switch (*fmt) {
        case 'd':
        case 'i': {
            long v = lng ? va_arg(ap, long) : va_arg(ap, int);
            if (v < 0) {
                put(&o, '-');
                put_u64(&o, (uint64_t)0 - (uint64_t)(int64_t)v);
            } else {
                put_u64(&o, (uint64_t)v);
            }
            break;
        }
        case 'u':
            put_u64(&o, lng ? va_arg(ap, unsigned long) : va_arg(ap, unsigned));
            break;
        case 'c':
            put(&o, (char)va_arg(ap, int));
            break;
        case 's': {
            const char *s = va_arg(ap, const char *);
            if (!s) s = "(null)";
            while (*s && prec--) put(&o, *s++);
            break;
        }
        case 'f':
            put_fixed(&o, va_arg(ap, double), prec < 0 ? 6 : prec);
            break;
        case '%':
            put(&o, '%');
            break;
        case 0:
            fmt--;
            break;
        default:
            put(&o, '%');
            put(&o, *fmt);
            break;
        }
    }
    if (buf && len) buf[o.at < len ? o.at : len - 1] = 0;
    return (int)o.at;
}

int RT(snprintf)(char *buf, size_t len, const char *fmt, ...)
{
    va_list ap;
    int n;
    va_start(ap, fmt);
    n = RT(vsnprintf)(buf, len, fmt, ap);
    va_end(ap);
    return n;
}

/* ---- Numbers from text */

static double ten_to(int n)
{
    double r = 1.0, p = 10.0;
    int neg = n < 0;
    if (neg) n = -n;
    for (; n; n >>= 1, p *= p)
        if (n & 1) r *= p;
    return neg ? 1.0 / r : r;
}

double RT(strtod)(const char *s, char **end)
{
    const char *p = s;
    uint64_t m = 0;
    int digits = 0, ex = 0, neg = 0, any = 0;
    double v;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' || *p == '\f' || *p == '\v') p++;
    if (*p == '+' || *p == '-') neg = *p++ == '-';
    for (; *p >= '0' && *p <= '9'; p++, any = 1) {
        if (digits < 19) {
            m = m * 10u + (uint64_t)(*p - '0');
            if (m) digits++;
        } else {
            ex++;
        }
    }
    if (*p == '.') {
        const char *q = p + 1;
        for (; *q >= '0' && *q <= '9'; q++, any = 1) {
            if (digits < 19) {
                m = m * 10u + (uint64_t)(*q - '0');
                if (m) digits++;
                ex--;
            }
        }
        if (any) p = q;
    }
    if (!any) {
        if (end) *end = (char *)s;
        return 0.0;
    }
    if (*p == 'e' || *p == 'E') {
        const char *q = p + 1;
        int eneg = 0, en = 0, edig = 0;
        if (*q == '+' || *q == '-') eneg = *q++ == '-';
        for (; *q >= '0' && *q <= '9'; q++, edig = 1)
            if (en < 10000) en = en * 10 + (*q - '0');
        if (edig) {
            ex += eneg ? -en : en;
            p = q;
        }
    }
    if (end) *end = (char *)p;
    /* Exact when the digits and the power of ten both are (up to 2^53 and
     * 1e22): one rounding, as the host's. */
    v = (double)m;
    if (ex < 0 && ex >= -22) v /= ten_to(-ex);
    else if (ex < -22) v = v / ten_to(22) / ten_to(-ex - 22);
    else if (ex) v *= ten_to(ex);
    return neg ? -v : v;
}

float RT(strtof)(const char *s, char **end)
{
    return (float)RT(strtod)(s, end);
}
