/* The pieces of a layer's wall welded into loops, as the slicer's stitch
 * does: ends within a tolerance are one point (sorted by x, joined by
 * union-find), then each point's pieces are followed round. A loop held
 * by an odd number of others is a hole, and turns clockwise.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gs_layer.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    float x, y;
    int i; /* the end: piece i / 2, its start when even */
} end_t;

static int by_x(const void *a, const void *b)
{
    float d = ((const end_t *)a)->x - ((const end_t *)b)->x;
    return d < 0.f ? -1 : d > 0.f;
}

static int root(int *up, int i)
{
    while (up[i] != i) i = up[i] = up[up[i]];
    return i;
}

static int put(gl_loop *l, int *cap, float x, float y)
{
    if (l->n == *cap) {
        int c = *cap ? *cap * 2 : 16;
        float *p = (float *)realloc(l->xy, (size_t)c * 2 * sizeof *p);
        if (!p) return -1;
        l->xy = p;
        *cap = c;
    }
    l->xy[l->n * 2] = x;
    l->xy[l->n * 2 + 1] = y;
    l->n++;
    return 0;
}

float gl_loop_area(const gl_loop *l)
{
    float a = 0.f;
    int i;
    for (i = 0; i < l->n; i++) {
        const float *p = l->xy + i * 2, *q = l->xy + ((i + 1) % l->n) * 2;
        a += p[0] * q[1] - q[0] * p[1];
    }
    return a * 0.5f;
}

float gl_loop_length(const gl_loop *l)
{
    float s = 0.f;
    int i, last = l->closed ? l->n : l->n - 1;
    for (i = 0; i < last; i++) {
        const float *p = l->xy + i * 2, *q = l->xy + ((i + 1) % l->n) * 2;
        s += sqrtf((q[0] - p[0]) * (q[0] - p[0]) + (q[1] - p[1]) * (q[1] - p[1]));
    }
    return s;
}

static void reverse(gl_loop *l, int from)
{
    int a = from, b = l->n - 1;
    for (; a < b; a++, b--) {
        float x = l->xy[a * 2], y = l->xy[a * 2 + 1];
        l->xy[a * 2] = l->xy[b * 2];
        l->xy[a * 2 + 1] = l->xy[b * 2 + 1];
        l->xy[b * 2] = x;
        l->xy[b * 2 + 1] = y;
    }
}

/* Even-odd: is (x, y) inside l? */
static int inside(const gl_loop *l, float x, float y)
{
    int i, in = 0;
    for (i = 0; i < l->n; i++) {
        const float *p = l->xy + i * 2, *q = l->xy + ((i + 1) % l->n) * 2;
        if ((p[1] > y) != (q[1] > y) && x < p[0] + (y - p[1]) / (q[1] - p[1]) * (q[0] - p[0])) in = !in;
    }
    return in;
}

void gl_loops_free(gl_loops *out)
{
    int i;
    for (i = 0; out->l && i < out->n; i++) free(out->l[i].xy);
    free(out->l);
    memset(out, 0, sizeof *out);
}

/* Follows unused pieces from point at, adding their far ends to l, until
 * none is left there or the loop comes back to stop. */
static int follow(gl_loop *l, int *cap, int at, int stop, const int *node, const int *first, const int *list,
                  unsigned char *used, const gl_walls *w, int *back)
{
    *back = 0;
    for (;;) {
        int k, next = -1;
        for (k = first[at]; k < first[at + 1]; k++) {
            int e = list[k];
            if (!used[e / 2]) {
                next = e;
                break;
            }
        }
        if (next < 0) return 0;
        used[next / 2] = 1;
        {
            int far = next ^ 1;
            const gl_seg *s = &w->s[far / 2];
            at = node[far];
            if (at == stop) {
                *back = 1;
                return 0;
            }
            if (put(l, cap, (far & 1) ? s->x1 : s->x0, (far & 1) ? s->y1 : s->y0)) return -1;
        }
    }
}

int gl_weld(const gl_walls *w, float tol, gl_loops *out)
{
    int ne = w->n * 2, i, j, nn = 0, rc = -1, lcap = 0;
    end_t *e = NULL;
    int *up = NULL, *node = NULL, *first = NULL, *list = NULL;
    unsigned char *used = NULL;
    gl_loops_free(out);
    if (!w->n) return 0;
    e = (end_t *)malloc((size_t)ne * sizeof *e);
    up = (int *)malloc((size_t)ne * sizeof *up);
    node = (int *)malloc((size_t)ne * sizeof *node);
    first = (int *)calloc((size_t)ne + 1, sizeof *first);
    list = (int *)malloc((size_t)ne * sizeof *list);
    used = (unsigned char *)calloc((size_t)w->n, 1);
    if (!e || !up || !node || !first || !list || !used) goto done;
    for (i = 0; i < w->n; i++) {
        e[i * 2] = (end_t){w->s[i].x0, w->s[i].y0, i * 2};
        e[i * 2 + 1] = (end_t){w->s[i].x1, w->s[i].y1, i * 2 + 1};
    }
    qsort(e, (size_t)ne, sizeof *e, by_x);
    for (i = 0; i < ne; i++) up[i] = i;
    for (i = 0; i < ne; i++)
        for (j = i + 1; j < ne && e[j].x - e[i].x <= tol; j++)
            if (fabsf(e[j].y - e[i].y) <= tol) up[root(up, j)] = root(up, i);
    /* Number the points, and list each one's piece ends. */
    for (i = 0; i < ne; i++) up[i] = root(up, i);
    for (i = 0; i < ne; i++) first[i] = -1; /* first[] numbers the roots for now */
    for (i = 0; i < ne; i++) {
        if (first[up[i]] < 0) first[up[i]] = nn++;
        node[e[i].i] = first[up[i]];
    }
    memset(first, 0, ((size_t)ne + 1) * sizeof *first);
    for (i = 0; i < ne; i++) first[node[i] + 1]++;
    for (i = 0; i < nn; i++) first[i + 1] += first[i];
    {
        int *fill = (int *)malloc((size_t)(nn + 1) * sizeof *fill);
        if (!fill) goto done;
        memcpy(fill, first, (size_t)(nn + 1) * sizeof *fill);
        for (i = 0; i < ne; i++) list[fill[node[i]]++] = i;
        free(fill);
    }
    /* A piece whose ends are one point is no wall. */
    for (i = 0; i < w->n; i++)
        if (node[i * 2] == node[i * 2 + 1]) used[i] = 1;
    for (i = 0; i < w->n; i++) {
        gl_loop l = {0};
        int cap = 0, back, start = i * 2;
        if (used[i]) continue;
        used[i] = 1;
        if (put(&l, &cap, w->s[i].x0, w->s[i].y0) || put(&l, &cap, w->s[i].x1, w->s[i].y1)) {
            free(l.xy);
            goto done;
        }
        if (follow(&l, &cap, node[start + 1], node[start], node, first, list, used, w, &back)) {
            free(l.xy);
            goto done;
        }
        l.closed = back;
        if (!back) {
            /* Open: go the other way from the first point, then turn it so
             * it runs from that end. */
            reverse(&l, 0);
            if (follow(&l, &cap, node[start], -1, node, first, list, used, w, &back)) {
                free(l.xy);
                goto done;
            }
            out->open++;
        }
        if (out->n == lcap) {
            int c = lcap ? lcap * 2 : 8;
            gl_loop *p = (gl_loop *)realloc(out->l, (size_t)c * sizeof *p);
            if (!p) {
                free(l.xy);
                goto done;
            }
            out->l = p;
            lcap = c;
        }
        out->l[out->n++] = l;
    }
    /* Anticlockwise round the part, clockwise round a hole. */
    for (i = 0; i < out->n; i++) {
        gl_loop *l = &out->l[i];
        int depth = 0;
        float a;
        if (!l->closed) continue;
        for (j = 0; j < out->n; j++)
            if (j != i && out->l[j].closed && inside(&out->l[j], l->xy[0], l->xy[1])) depth++;
        a = gl_loop_area(l);
        if ((depth & 1) ? a > 0.f : a < 0.f) reverse(l, 0);
    }
    rc = 0;
done:
    free(e);
    free(up);
    free(node);
    free(first);
    free(list);
    free(used);
    if (rc) gl_loops_free(out);
    return rc;
}

/* Holding tabs on a closed loop: `count` gaps of `width` mm, equally spaced,
 * each centred half a step along the perimeter so a square's four tabs sit
 * mid-edge, not on a corner. Replaces out with the remaining open cuts.
 * count 0 or width 0 leaves the loop closed, uncut. 0, or -1 out of memory
 * or a bad argument, or -2 if the tabs do not fit (each tab plus 0.5 mm of
 * cut, times the count, must fit the perimeter). */
#define GL_TAB_GAP 0.5f

static int loops_add(gl_loops *out, int *cap, gl_loop l)
{
    if (out->n == *cap) {
        int c = *cap ? *cap * 2 : 4;
        gl_loop *p = (gl_loop *)realloc(out->l, (size_t)c * sizeof *p);
        if (!p) {
            free(l.xy);
            return -1;
        }
        out->l = p;
        *cap = c;
    }
    out->l[out->n++] = l;
    return 0;
}

static int copy_loop(const gl_loop *in, gl_loops *out)
{
    gl_loop l = {0};
    int cap = 0, i;
    for (i = 0; i < in->n; i++)
        if (put(&l, &cap, in->xy[i * 2], in->xy[i * 2 + 1])) {
            free(l.xy);
            return -1;
        }
    l.closed = in->closed;
    cap = 0;
    return loops_add(out, &cap, l);
}

static void point_at(const gl_loop *l, float s, float *x, float *y)
{
    int i, last = l->closed ? l->n : l->n - 1;
    float acc = 0.f;
    if (s <= 0.f || last < 1) {
        *x = l->xy[0];
        *y = l->xy[1];
        return;
    }
    for (i = 0; i < last; i++) {
        float x0 = l->xy[i * 2], y0 = l->xy[i * 2 + 1];
        float x1 = l->xy[((i + 1) % l->n) * 2], y1 = l->xy[((i + 1) % l->n) * 2 + 1];
        float dx = x1 - x0, dy = y1 - y0, len = sqrtf(dx * dx + dy * dy);
        if (acc + len >= s || i == last - 1) {
            float t = len > 0.f ? (s - acc) / len : 0.f;
            if (t < 0.f) t = 0.f;
            if (t > 1.f) t = 1.f;
            *x = x0 + t * dx;
            *y = y0 + t * dy;
            return;
        }
        acc += len;
    }
    *x = l->xy[0];
    *y = l->xy[1];
}

/* Vertices with cumulative length in (s0, s1), plus the ends. s0 < s1, both in [0, P]. */
static int emit_span(const gl_loop *l, float s0, float s1, gl_loop *out, int *cap)
{
    int i, last = l->closed ? l->n : l->n - 1;
    float acc = 0.f, x, y;
    if (s1 - s0 < 1e-7f) return 0;
    point_at(l, s0, &x, &y);
    if (put(out, cap, x, y)) return -1;
    for (i = 0; i < last; i++) {
        float x0 = l->xy[i * 2], y0 = l->xy[i * 2 + 1];
        float x1 = l->xy[((i + 1) % l->n) * 2], y1 = l->xy[((i + 1) % l->n) * 2 + 1];
        float dx = x1 - x0, dy = y1 - y0, len = sqrtf(dx * dx + dy * dy);
        float mid = acc + len;
        if (mid > s0 + 1e-7f && mid < s1 - 1e-7f) {
            if (put(out, cap, x1, y1)) return -1;
        }
        acc = mid;
    }
    point_at(l, s1, &x, &y);
    if (out->n == 0 || fabsf(out->xy[(out->n - 1) * 2] - x) > 1e-7f ||
        fabsf(out->xy[(out->n - 1) * 2 + 1] - y) > 1e-7f) {
        if (put(out, cap, x, y)) return -1;
    }
    return 0;
}

typedef struct {
    float a, b;
} gl_span;

static int by_span(const void *p, const void *q)
{
    float d = ((const gl_span *)p)->a - ((const gl_span *)q)->a;
    return d < 0.f ? -1 : d > 0.f;
}

static int emit_walk(const gl_loop *l, float s0, float s1, float P, gl_loops *out, int *ocap)
{
    gl_loop seg = {0};
    int cap = 0;
    if (s1 < s0) s1 += P;
    if (s1 - s0 < 1e-7f) return 0;
    if (s1 <= P + 1e-7f) {
        if (emit_span(l, s0, s1 > P ? P : s1, &seg, &cap)) {
            free(seg.xy);
            return -1;
        }
    } else {
        if (emit_span(l, s0, P, &seg, &cap) || emit_span(l, 0.f, s1 - P, &seg, &cap)) {
            free(seg.xy);
            return -1;
        }
    }
    if (seg.n < 2) {
        free(seg.xy);
        return 0;
    }
    seg.closed = 0;
    return loops_add(out, ocap, seg);
}

int gl_tabs(const gl_loop *in, int count, float width, gl_loops *out)
{
    float P;
    gl_span *tab = NULL;
    int ntab = 0, i, ocap = 0, rc;
    gl_loops_free(out);
    if (!in || in->n < 3 || !in->closed) return -1;
    if (count < 0 || width < 0.f) return -1;
    if (count == 0 || width == 0.f) return copy_loop(in, out);
    P = gl_loop_length(in);
    if (P <= 0.f) return -1;
    if ((float)count * (width + GL_TAB_GAP) > P + 1e-5f) return -2;
    tab = (gl_span *)malloc((size_t)count * 2 * sizeof *tab);
    if (!tab) return -1;
    for (i = 0; i < count; i++) {
        float c = ((float)i + 0.5f) * P / (float)count;
        float a = c - width * 0.5f, b = c + width * 0.5f;
        if (a >= 0.f && b <= P) {
            tab[ntab++] = (gl_span){a, b};
        } else if (a < 0.f && b <= P) {
            tab[ntab++] = (gl_span){0.f, b};
            tab[ntab++] = (gl_span){P + a, P};
        } else if (a >= 0.f && b > P) {
            tab[ntab++] = (gl_span){a, P};
            tab[ntab++] = (gl_span){0.f, b - P};
        } else {
            /* a tab longer than the loop: already refused by the fit check. */
            free(tab);
            return -2;
        }
    }
    qsort(tab, (size_t)ntab, sizeof *tab, by_span);
    {
        int w = 0;
        for (i = 0; i < ntab; i++) {
            if (w && tab[i].a <= tab[w - 1].b + 1e-7f) {
                if (tab[i].b > tab[w - 1].b) tab[w - 1].b = tab[i].b;
            } else
                tab[w++] = tab[i];
        }
        ntab = w;
    }
    rc = 0;
    if (ntab == 0)
        rc = copy_loop(in, out);
    else {
        /* One open cut from the end of each tab to the start of the next
         * (wrapping from the last tab back to the first). */
        for (i = 0; i < ntab; i++) {
            float s0 = tab[i].b, s1 = tab[(i + 1) % ntab].a;
            if (i == ntab - 1) s1 += P;
            if (emit_walk(in, s0, s1, P, out, &ocap)) {
                rc = -1;
                break;
            }
        }
    }
    free(tab);
    if (rc) gl_loops_free(out);
    return rc;
}
