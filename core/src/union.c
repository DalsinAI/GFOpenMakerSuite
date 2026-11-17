/* gl_union (gs_layer.h): the material a set of loops makes, as loops that
 * do not cross. Every edge is cut where another crosses it, touches it or
 * runs along it; a piece is kept when the loops wind round at least once
 * just to its left and not just to its right; the kept pieces are joined
 * end to start into loops, turning as far left as they can where several
 * meet, so loops that only touch come out apart. Edges are compared in
 * order of their left ends, and the winding is counted from the edges in
 * the piece's band of height.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gs_layer.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define PARAM 1e-6f /* along an edge, this near an end is the end */
#define LINE 1e-5f  /* mm off a line and still on it */
#define SIDE 2e-4f  /* mm to a piece's side, to count the winding there */

#define LO(a, b) ((a) < (b) ? (a) : (b))
#define HI(a, b) ((a) > (b) ? (a) : (b))

typedef struct { float x0, y0, x1, y1; } edge;
typedef struct { float left; int e; } ordered; /* an edge by its left end */
typedef struct { int e; float t, x, y; } cut;
typedef struct { float x0, y0, x1, y1; } piece;

typedef struct {
    edge *e;
    int ne;
    cut *c;
    int nc, cc;
    int *band, *band_at, nb;
    float y0, bh;
} work;

static int add_cut(work *w, int e, float t, float x, float y)
{
    if (t <= PARAM || t >= 1.f - PARAM) return 0;
    if (w->nc == w->cc) {
        int cap = w->cc ? w->cc * 2 : 256;
        cut *c = (cut *)realloc(w->c, (size_t)cap * sizeof *c);
        if (!c) return -1;
        w->c = c;
        w->cc = cap;
    }
    w->c[w->nc].e = e;
    w->c[w->nc].t = t;
    w->c[w->nc].x = x;
    w->c[w->nc].y = y;
    w->nc++;
    return 0;
}

/* Where along a point p falls, 0 at its start and 1 at its end. */
static float along(const edge *a, float px, float py)
{
    float dx = a->x1 - a->x0, dy = a->y1 - a->y0;
    return ((px - a->x0) * dx + (py - a->y0) * dy) / (dx * dx + dy * dy);
}

/* Edges i and j cut at every point they share, at exactly that point. */
static int meet(work *w, int i, int j)
{
    const edge *a = &w->e[i], *b = &w->e[j];
    float ax = a->x1 - a->x0, ay = a->y1 - a->y0, bx = b->x1 - b->x0, by = b->y1 - b->y0;
    float rx = b->x0 - a->x0, ry = b->y0 - a->y0, den = ax * by - ay * bx;
    float la = sqrtf(ax * ax + ay * ay), lb = sqrtf(bx * bx + by * by), t, u, px, py;
    if (fabsf(den) <= 1e-7f * la * lb) {
        /* Along each other: each cut at the other's ends that lie on it. */
        if (fabsf(rx * ay - ry * ax) > LINE * la) return 0;
        return add_cut(w, i, along(a, b->x0, b->y0), b->x0, b->y0) || add_cut(w, i, along(a, b->x1, b->y1), b->x1, b->y1) ||
               add_cut(w, j, along(b, a->x0, a->y0), a->x0, a->y0) || add_cut(w, j, along(b, a->x1, a->y1), a->x1, a->y1)
                   ? -1 : 0;
    }
    t = (rx * by - ry * bx) / den;
    u = (rx * ay - ry * ax) / den;
    if (t < -PARAM || t > 1.f + PARAM || u < -PARAM || u > 1.f + PARAM) return 0;
    if (t <= PARAM) px = a->x0, py = a->y0;
    else if (t >= 1.f - PARAM) px = a->x1, py = a->y1;
    else if (u <= PARAM) px = b->x0, py = b->y0;
    else if (u >= 1.f - PARAM) px = b->x1, py = b->y1;
    else px = a->x0 + t * ax, py = a->y0 + t * ay;
    return add_cut(w, i, t, px, py) || add_cut(w, j, u, px, py) ? -1 : 0;
}

static int by_left(const void *p, const void *q)
{
    const ordered *a = (const ordered *)p, *b = (const ordered *)q;
    if (a->left != b->left) return a->left < b->left ? -1 : 1;
    return a->e < b->e ? -1 : a->e > b->e;
}

/* How far left the turn from (ix, iy) to (ox, oy) is, rising with the
 * angle from -180 to 180 degrees: no trigonometry. */
static float turn_of(float ix, float iy, float ox, float oy)
{
    float c = ix * oy - iy * ox, d = ix * ox + iy * oy, p = d / (fabsf(c) + fabsf(d) + 1e-30f);
    return c >= 0.f ? 1.f - p : p - 1.f;
}

static int by_edge(const void *p, const void *q)
{
    const cut *a = (const cut *)p, *b = (const cut *)q;
    if (a->e != b->e) return a->e < b->e ? -1 : 1;
    if (a->t != b->t) return a->t < b->t ? -1 : 1;
    if (a->x != b->x) return a->x < b->x ? -1 : 1;
    return a->y < b->y ? -1 : a->y > b->y;
}

/* How many times the loops wind round (px, py). */
static int winding(const work *w, float px, float py)
{
    int b = (int)((py - w->y0) / w->bh), k, n = 0;
    if (b < 0 || b >= w->nb) return 0;
    for (k = w->band_at[b]; k < w->band_at[b + 1]; k++) {
        const edge *e = &w->e[w->band[k]];
        float left = (e->x1 - e->x0) * (py - e->y0) - (px - e->x0) * (e->y1 - e->y0);
        if (e->y0 <= py) {
            if (e->y1 > py && left > 0.f) n++;
        } else if (e->y1 <= py && left < 0.f) n--;
    }
    return n;
}

/* Each edge listed in the bands of height it spans. 0, or -1. */
static int bands(work *w)
{
    float lo = 1e30f, hi = -1e30f;
    int i, b, *fill;
    for (i = 0; i < w->ne; i++) {
        const edge *e = &w->e[i];
        lo = LO(lo, LO(e->y0, e->y1));
        hi = HI(hi, HI(e->y0, e->y1));
    }
    w->nb = w->ne / 4 < 1 ? 1 : w->ne / 4 > 4096 ? 4096 : w->ne / 4;
    w->y0 = lo - 1.f;
    w->bh = (hi - lo + 2.f) / (float)w->nb;
    w->band_at = (int *)calloc((size_t)w->nb + 1, sizeof *w->band_at);
    fill = (int *)calloc((size_t)w->nb, sizeof *fill);
    if (!w->band_at || !fill) {
        free(fill);
        return -1;
    }
    for (i = 0; i < 2; i++) {
        int k;
        for (k = 0; k < w->ne; k++) {
            const edge *e = &w->e[k];
            int b0 = (int)((LO(e->y0, e->y1) - w->y0) / w->bh), b1 = (int)((HI(e->y0, e->y1) - w->y0) / w->bh);
            for (b = b0; b <= b1 && b < w->nb; b++) {
                if (i == 0) w->band_at[b + 1]++;
                else w->band[w->band_at[b] + fill[b]++] = k;
            }
        }
        if (i == 0) {
            for (b = 0; b < w->nb; b++) w->band_at[b + 1] += w->band_at[b];
            w->band = (int *)malloc((size_t)(w->band_at[w->nb] + 1) * sizeof *w->band);
            if (!w->band) {
                free(fill);
                return -1;
            }
        }
    }
    free(fill);
    return 0;
}

/* Keeps the piece from (x0, y0) to (x1, y1) when it bounds the material. */
static void keep(const work *w, piece *p, int *np, float x0, float y0, float x1, float y1)
{
    float dx = x1 - x0, dy = y1 - y0, len = sqrtf(dx * dx + dy * dy), mx = (x0 + x1) * 0.5f, my = (y0 + y1) * 0.5f;
    float nx, ny;
    if (len <= 0.f) return;
    nx = -dy / len * SIDE;
    ny = dx / len * SIDE;
    if (winding(w, mx + nx, my + ny) < 1 || winding(w, mx - nx, my - ny) >= 1) return;
    p[*np].x0 = x0;
    p[*np].y0 = y0;
    p[*np].x1 = x1;
    p[*np].y1 = y1;
    (*np)++;
}

static int by_start(const void *p, const void *q)
{
    const piece *a = (const piece *)p, *b = (const piece *)q;
    if (a->x0 != b->x0) return a->x0 < b->x0 ? -1 : 1;
    if (a->y0 != b->y0) return a->y0 < b->y0 ? -1 : 1;
    if (a->x1 != b->x1) return a->x1 < b->x1 ? -1 : 1;
    return a->y1 < b->y1 ? -1 : a->y1 > b->y1;
}

/* The first piece starting at (x, y) in p sorted by_start, or n. */
static int first_at(const piece *p, int n, float x, float y)
{
    int lo = 0, hi = n;
    while (lo < hi) {
        int mid = (lo + hi) / 2;
        if (p[mid].x0 < x || (p[mid].x0 == x && p[mid].y0 < y)) lo = mid + 1;
        else hi = mid;
    }
    return lo;
}

/* The pieces joined into loops, in out. Where several pieces leave a
 * point, the one turning furthest left; a loop closes when that is the
 * piece it began with. 0, or -1. */
static int join(piece *p, int np, gl_loops *out)
{
    char *used = (char *)calloc((size_t)np + 1, 1);
    float *xy = (float *)malloc(((size_t)np + 1) * 2 * sizeof *xy);
    int s, cap = 0;
    if (!used || !xy) {
        free(used);
        free(xy);
        return -1;
    }
    for (s = 0; s < np; s++) {
        int cur = s, n = 0, closed = 0;
        if (used[s]) continue;
        while (!closed) {
            const piece *c = &p[cur];
            float ix = c->x1 - c->x0, iy = c->y1 - c->y0, best = -10.f;
            int k = first_at(p, np, c->x1, c->y1), next = -1;
            used[cur] = 1;
            xy[n * 2] = c->x0;
            xy[n * 2 + 1] = c->y0;
            n++;
            for (; k < np && p[k].x0 == c->x1 && p[k].y0 == c->y1; k++) {
                float ox = p[k].x1 - p[k].x0, oy = p[k].y1 - p[k].y0, turn;
                if (used[k] && k != s) continue;
                turn = turn_of(ix, iy, ox, oy);
                if (turn > best) {
                    best = turn;
                    next = k;
                }
            }
            if (next < 0) break; /* open: dropped */
            closed = next == s;
            cur = next;
        }
        while (closed && n >= 3) { /* points along a straight line go */
            int i, m = 0;
            for (i = 0; i < n; i++) {
                const float *a = xy + ((i + n - 1) % n) * 2, *b = xy + i * 2, *c = xy + ((i + 1) % n) * 2;
                float ux = b[0] - a[0], uy = b[1] - a[1], vx = c[0] - b[0], vy = c[1] - b[1];
                if (fabsf(ux * vy - uy * vx) <= 1e-9f && ux * vx + uy * vy > 0.f && n - (i - m) > 3) continue;
                xy[m * 2] = b[0];
                xy[m * 2 + 1] = b[1];
                m++;
            }
            if (m == n) break;
            n = m;
        }
        if (closed && n >= 3) {
            gl_loop l;
            l.xy = xy;
            l.n = n;
            l.closed = 1;
            if (fabsf(gl_loop_area(&l)) < 1e-6f) continue;
            if (out->n == cap) {
                gl_loop *g;
                cap = cap ? cap * 2 : 8;
                g = (gl_loop *)realloc(out->l, (size_t)cap * sizeof *g);
                if (!g) break;
                out->l = g;
            }
            l.xy = (float *)malloc((size_t)n * 2 * sizeof *l.xy);
            if (!l.xy) break;
            memcpy(l.xy, xy, (size_t)n * 2 * sizeof *l.xy);
            out->l[out->n++] = l;
        }
    }
    free(used);
    free(xy);
    return s < np ? -1 : 0;
}

int gl_union(const gl_loops *in, gl_loops *out)
{
    work w;
    ordered *order = NULL;
    int i, j, k, np = 0, rc = -1;
    piece *p = NULL;
    gl_loops res;
    memset(&w, 0, sizeof w);
    memset(&res, 0, sizeof res);
    for (i = 0; i < in->n; i++)
        if (in->l[i].closed) w.ne += in->l[i].n;
    w.e = (edge *)malloc(((size_t)w.ne + 1) * sizeof *w.e);
    if (!w.e) goto done;
    w.ne = 0;
    for (i = 0; i < in->n; i++) {
        const gl_loop *l = &in->l[i];
        if (!l->closed || l->n < 3) continue;
        for (j = 0; j < l->n; j++) {
            const float *a = l->xy + j * 2, *b = l->xy + ((j + 1) % l->n) * 2;
            if (a[0] == b[0] && a[1] == b[1]) continue;
            w.e[w.ne].x0 = a[0];
            w.e[w.ne].y0 = a[1];
            w.e[w.ne].x1 = b[0];
            w.e[w.ne].y1 = b[1];
            w.ne++;
        }
    }
    order = (ordered *)malloc(((size_t)w.ne + 1) * sizeof *order);
    if (!order) goto done;
    for (i = 0; i < w.ne; i++) {
        order[i].left = LO(w.e[i].x0, w.e[i].x1);
        order[i].e = i;
    }
    qsort(order, (size_t)w.ne, sizeof *order, by_left);
    for (i = 0; i < w.ne; i++) {
        const edge *a = &w.e[order[i].e];
        float r = HI(a->x0, a->x1) + LINE, lo = LO(a->y0, a->y1) - LINE, hi = HI(a->y0, a->y1) + LINE;
        for (j = i + 1; j < w.ne; j++) {
            const edge *b = &w.e[order[j].e];
            if (LO(b->x0, b->x1) > r) break;
            if (HI(b->y0, b->y1) < lo || LO(b->y0, b->y1) > hi) continue;
            if (meet(&w, order[i].e, order[j].e)) goto done;
        }
    }
    if (w.nc) qsort(w.c, (size_t)w.nc, sizeof *w.c, by_edge);
    if (bands(&w)) goto done;
    p = (piece *)malloc(((size_t)w.ne + (size_t)w.nc + 1) * sizeof *p);
    if (!p) goto done;
    for (i = 0, k = 0; i < w.ne; i++) {
        float x = w.e[i].x0, y = w.e[i].y0;
        for (; k < w.nc && w.c[k].e == i; k++) {
            keep(&w, p, &np, x, y, w.c[k].x, w.c[k].y);
            x = w.c[k].x;
            y = w.c[k].y;
        }
        keep(&w, p, &np, x, y, w.e[i].x1, w.e[i].y1);
    }
    qsort(p, (size_t)np, sizeof *p, by_start);
    for (i = 0, j = 0; i < np; i++) /* a piece along two edges, once */
        if (!j || by_start(&p[j - 1], &p[i])) p[j++] = p[i];
    np = j;
    rc = join(p, np, &res);
done:
    free(w.e);
    free(w.c);
    free(w.band);
    free(w.band_at);
    free(order);
    free(p);
    if (rc) {
        gl_loops_free(&res);
        return -1;
    }
    gl_loops_free(out);
    *out = res;
    return 0;
}
