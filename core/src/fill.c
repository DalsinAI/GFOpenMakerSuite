/* Skin and infill (gt_fill in gs_toolpath.h): each line across the layer
 * is a list of runs, from where it crosses the loops (even-odd). The runs
 * where every deciding layer has material too are sparse; the rest is
 * skin. MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gs_toolpath.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    float *x;      /* runs: a start and an end each, in order */
    int n, cap;    /* floats */
} runs;

static int put(runs *r, float x)
{
    if (r->n == r->cap) {
        int c = r->cap ? r->cap * 2 : 32;
        float *g = (float *)realloc(r->x, (size_t)c * sizeof *g);
        if (!g) return -1;
        r->x = g;
        r->cap = c;
    }
    r->x[r->n++] = x;
    return 0;
}

static int by_value(const void *a, const void *b)
{
    float d = *(const float *)a - *(const float *)b;
    return d < 0.f ? -1 : d > 0.f;
}

/* The runs of the line at v (y, or x when cross) inside L. 0, or -1. */
static int line(const gl_loops *L, float v, int cross, runs *out)
{
    int i, j, a = cross ? 1 : 0, b = cross ? 0 : 1;
    out->n = 0;
    for (i = 0; i < L->n; i++) {
        const gl_loop *l = &L->l[i];
        if (!l->closed) continue;
        for (j = 0; j < l->n; j++) {
            const float *p = l->xy + j * 2, *q = l->xy + ((j + 1) % l->n) * 2;
            if ((p[b] > v) == (q[b] > v)) continue;
            if (put(out, p[a] + (v - p[b]) * (q[a] - p[a]) / (q[b] - p[b]))) return -1;
        }
    }
    qsort(out->x, (size_t)(out->n / 2 * 2), sizeof *out->x, by_value);
    out->n = out->n / 2 * 2;
    return 0;
}

/* out = a and b (keep 1), or a without b (keep 0). Runs in order. */
static int join(const runs *a, const runs *b, int keep, runs *out)
{
    int i, j;
    out->n = 0;
    for (i = 0; i < a->n; i += 2) {
        float s = a->x[i], e = a->x[i + 1];
        if (keep) {
            for (j = 0; j < b->n; j += 2) {
                float s2 = b->x[j] > s ? b->x[j] : s, e2 = b->x[j + 1] < e ? b->x[j + 1] : e;
                if (e2 > s2 && (put(out, s2) || put(out, e2))) return -1;
            }
            continue;
        }
        for (j = 0; j < b->n && s < e; j += 2) {
            if (b->x[j + 1] <= s || b->x[j] >= e) continue;
            if (b->x[j] > s && (put(out, s) || put(out, b->x[j]))) return -1;
            s = b->x[j + 1];
        }
        if (e > s && (put(out, s) || put(out, e))) return -1;
    }
    return 0;
}

/* The run at float index at of skin and sparse merged in order (neither
 * overlaps the other), and its kind. */
static const float *pick(const runs *skin, const runs *sparse, int at, int *kind)
{
    int a = 0, b = 0;
    for (;;) {
        int from_skin = b >= sparse->n || (a < skin->n && skin->x[a] < sparse->x[b]);
        if (a + b == at) {
            *kind = from_skin ? GT_SKIN : GT_INFILL;
            return from_skin ? &skin->x[a] : &sparse->x[b];
        }
        if (from_skin) a += 2;
        else b += 2;
    }
}

/* One move along the line at v from u0 to u1, a travel to its start first. */
static int run(gt_path *p, int kind, float v, float u0, float u1, int cross, float z, float w)
{
    if (u1 - u0 < 1e-3f && u0 - u1 < 1e-3f) return 0;
    if (cross) return gt_add(p, GT_TRAVEL, v, u0, z, 0.f) || gt_add(p, kind, v, u1, z, w);
    return gt_add(p, GT_TRAVEL, u0, v, z, 0.f) || gt_add(p, kind, u1, v, z, w);
}

/* skins 0: the sparse runs alone. */
static int fill(gt_path *p, const gl_loops *region, const gl_loops *const *solid, int nsolid, float z, float w,
                int every, int cross, int skins)
{
    runs in = {0}, keep = {0}, t = {0}, skin = {0}, sparse = {0};
    float lo = 1e30f, hi = -1e30f;
    int i, j, k, k0, k1, flip = 0, rc = 0;
    for (i = 0; i < region->n; i++)
        for (j = 0; j < region->l[i].n; j++) {
            float v = region->l[i].xy[j * 2 + (cross ? 0 : 1)];
            lo = v < lo ? v : lo;
            hi = v > hi ? v : hi;
        }
    if (lo > hi || w <= 0.f) return 0;
    if (every < 1) every = 1;
    k0 = (int)ceilf(lo / w - 0.5f);
    k1 = (int)floorf(hi / w - 0.5f);
    for (k = k0; k <= k1 && !rc; k++) {
        float v = ((float)k + 0.5f) * w;
        int sparse_line = ((k % every) + every) % every == 0, n;
        rc = line(region, v, cross, &in);
        /* keep: where every deciding layer has material as well */
        keep.n = 0;
        for (i = 0; i < in.n && !rc; i++) rc = put(&keep, in.x[i]);
        for (j = 0; j < nsolid && !rc && keep.n; j++) {
            if (!solid[j]) {
                keep.n = 0;
                break;
            }
            rc = line(solid[j], v, cross, &t) || join(&keep, &t, 1, &sparse);
            keep.n = 0;
            for (i = 0; i < sparse.n && !rc; i++) rc = put(&keep, sparse.x[i]);
        }
        if (rc || join(&in, &keep, 0, &skin)) {
            rc = -1;
            break;
        }
        /* skin and sparse runs merged in order along the line, back and forth */
        if (!sparse_line) keep.n = 0;
        if (!skins) skin.n = 0;
        n = skin.n + keep.n;
        for (i = 0; i < n && !rc; i += 2) {
            int at = flip ? n - 2 - i : i, kind;
            const float *r = pick(&skin, &keep, at, &kind);
            rc = flip ? run(p, kind, v, r[1], r[0], cross, z, w) : run(p, kind, v, r[0], r[1], cross, z, w);
        }
        if (n) flip = !flip;
    }
    free(in.x);
    free(keep.x);
    free(t.x);
    free(skin.x);
    free(sparse.x);
    return rc ? -1 : 0;
}

int gt_fill(gt_path *p, const gl_loops *region, const gl_loops *const *solid, int nsolid, float z, float w,
            int every, int cross)
{
    return fill(p, region, solid, nsolid, z, w, every, cross, 1);
}

int gt_fill_grid(gt_path *p, const gl_loops *region, const gl_loops *const *solid, int nsolid, float z, float w,
                 int every, int cross)
{
    if (every < 1) every = 1;
    return fill(p, region, solid, nsolid, z, w, 2 * every, cross, 1) ||
                   fill(p, region, solid, nsolid, z, w, 2 * every, !cross, 0)
               ? -1
               : 0;
}
