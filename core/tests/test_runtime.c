/* The libraries' own C runtime (core/library/runtime.c), built here under
 * rt_ names and held to the host's C library. A shared Amiga library has no
 * start-up code, so it cannot use libnix's malloc, stdio or maths; this is
 * what it uses instead. MIT, Copyright (c) 2026 AmiCradle Team. */
#include "../library/runtime.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fails;

static void expect(int ok, const char *what)
{
    if (!ok) {
        printf("fail: %s\n", what);
        fails++;
    }
}

static int by_int(const void *a, const void *b)
{
    int x = *(const int *)a, y = *(const int *)b;
    return (x > y) - (x < y);
}

typedef struct {
    float key;
    char pad[7];
} odd_t;

static int by_key(const void *a, const void *b)
{
    float x = ((const odd_t *)a)->key, y = ((const odd_t *)b)->key;
    return (x > y) - (x < y);
}

int main(void)
{
    unsigned seed = 12345;
    int i;
    {
        /* Memory: realloc keeps what was there; calloc clears. */
        unsigned char *p = (unsigned char *)rt_malloc(10), *q;
        int *z = (int *)rt_calloc(100, sizeof *z), clear = 1;
        expect(p && z, "memory");
        for (i = 0; p && i < 10; i++) p[i] = (unsigned char)i;
        q = (unsigned char *)rt_realloc(p, 100000);
        expect(q && q[9] == 9 && q[0] == 0, "realloc keeps the bytes");
        p = (unsigned char *)rt_realloc(q, 4);
        expect(p && p[3] == 3, "and when it shrinks");
        for (i = 0; z && i < 100; i++)
            if (z[i]) clear = 0;
        expect(clear, "calloc clears");
        rt_free(p);
        rt_free(z);
        rt_free(NULL);
        p = (unsigned char *)rt_realloc(NULL, 8);
        expect(p != NULL, "realloc of nothing is malloc");
        rt_free(p);
    }
    {
        char a[32] = "hello, world", b[32];
        rt_memset(b, 'x', sizeof b);
        expect(rt_memcpy(b, a, 13) == b && strcmp(b, a) == 0, "memcpy");
        rt_memmove(a + 2, a, 10);
        expect(memcmp(a, "hehello, wor", 12) == 0, "memmove forwards");
        rt_memmove(a, a + 2, 10);
        expect(memcmp(a, "hello, wor", 10) == 0, "and back");
        expect(rt_strlen("abc") == 3 && rt_strcmp("abc", "abd") < 0 && rt_strcmp("b", "a") > 0 &&
                   rt_strcmp("same", "same") == 0, "strlen, strcmp");
        expect(rt_strncmp("solid x", "solid y", 5) == 0 && rt_strncmp("ab", "ac", 2) < 0, "strncmp");
        {
            const char *s = "dir/sub/file.stl";
            expect(rt_strchr(s, '/') == s + 3 && rt_strrchr(s, '/') == s + 7 && rt_strchr(s, '#') == NULL,
                   "strchr and strrchr");
            expect(rt_strchr(s, 0) == s + 16, "strchr finds the end");
            expect(rt_strcspn("speed 300", " \t") == 5 && rt_strcspn("none", " ") == 4, "strcspn");
            expect(rt_memcmp("abc", "abd", 3) < 0 && rt_memcmp("abc", "abc", 3) == 0, "memcmp");
        }
    }
    {
        /* qsort on ints, many alike, and on records of an odd size. */
        int v[1000], w[1000], same = 1;
        odd_t o[257];
        int sorted = 1;
        for (i = 0; i < 1000; i++) {
            seed = seed * 1103515245u + 12345u;
            v[i] = w[i] = (int)(seed >> 16) % 50;
        }
        rt_qsort(v, 1000, sizeof v[0], by_int);
        qsort(w, 1000, sizeof w[0], by_int);
        for (i = 0; i < 1000; i++)
            if (v[i] != w[i]) same = 0;
        expect(same, "qsort sorts as the host's does");
        for (i = 0; i < 257; i++) {
            seed = seed * 1103515245u + 12345u;
            o[i].key = (float)(seed >> 8) / 1000.f;
        }
        rt_qsort(o, 257, sizeof o[0], by_key);
        for (i = 1; i < 257; i++)
            if (o[i - 1].key > o[i].key) sorted = 0;
        expect(sorted, "and records of eleven bytes");
        rt_qsort(o, 0, sizeof o[0], by_key);
        rt_qsort(o, 1, sizeof o[0], by_key);
    }
    if (fails) {
        printf("%d failed\n", fails);
        return 1;
    }
    printf("runtime ok\n");
    return 0;
}
