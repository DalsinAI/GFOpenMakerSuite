/* A printer's layer (gs_plan.h). MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gs_plan.h"

#include <stdlib.h>
#include <string.h>

#define MOST_SKINS 8

void gp_defaults(gp_opts *o)
{
    o->layer_h = 0.2f;
    o->line_w = 0.4f;
    o->walls = 2;
    o->skins = 3;
    o->infill = 20.f;
    o->brim = 3;
}

void gp_more_defaults(gp_more *x)
{
    memset(x, 0, sizeof *x);
    x->size = (int)sizeof *x;
    x->pattern = GP_LINES;
    x->top = x->bottom = -1;
}

/* x as far as its size, the rest the defaults. */
static void more_of(const gp_more *x, gp_more *to)
{
    gp_more_defaults(to);
    if (x && x->size > (int)sizeof x->size)
        memcpy(&to->pattern, &x->pattern, (size_t)(x->size < (int)sizeof *x ? x->size : (int)sizeof *x) - sizeof x->size);
}

static int held(int v, int lo, int hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}

int gp_layers(const gs_mesh *m, const gp_opts *o)
{
    return gl_layers(m, o->layer_h);
}

/* Layer n's loops, or 1 when there is no layer n. 0, or -1. */
static int loops(const gs_mesh *m, float h, int n, gl_loops *out)
{
    gl_walls w;
    int rc;
    if (n < 1 || n > gl_layers(m, h)) return 1;
    memset(&w, 0, sizeof w);
    rc = gl_cut(m, h, n, &w) || gl_weld(&w, 1e-3f, out) ? -1 : 0;
    gl_walls_free(&w);
    return rc;
}

/* Each closed loop of in moved by d, as kind; outsides only when outer. */
static int ring(gt_path *p, const gl_loops *in, float d, int kind, int outer, float z, float w)
{
    gl_loops o;
    int i, rc;
    memset(&o, 0, sizeof o);
    rc = gl_inset(in, d, &o);
    for (i = 0; i < o.n && !rc; i++)
        if (!outer || gl_loop_area(&o.l[i]) > 0.f) rc = gt_loop(p, kind, &o.l[i], z, w);
    gl_loops_free(&o);
    return rc ? -1 : 0;
}

/* src's closed loops onto dst. flip reverses them, so a solid becomes a hole. */
static int add_loops(gl_loops *dst, const gl_loops *src, int flip)
{
    int i, k;
    if (!src) return 0;
    for (i = 0; i < src->n; i++) {
        const gl_loop *s = &src->l[i];
        gl_loop *nl, *d;
        float *xy;
        if (!s->closed || s->n < 3 || !s->xy) continue;
        nl = (gl_loop *)realloc(dst->l, (size_t)(dst->n + 1) * sizeof *nl);
        if (!nl) return -1;
        dst->l = nl;
        d = &dst->l[dst->n];
        memset(d, 0, sizeof *d);
        xy = (float *)malloc((size_t)s->n * 2 * sizeof *xy);
        if (!xy) return -1;
        for (k = 0; k < s->n; k++) {
            int f = flip ? s->n - 1 - k : k;
            xy[k * 2] = s->xy[f * 2];
            xy[k * 2 + 1] = s->xy[f * 2 + 1];
        }
        d->xy = xy;
        d->n = s->n;
        d->closed = 1;
        dst->n++;
    }
    return 0;
}

/* What of layer k the layer under it does not hold at 45 degrees: further
 * out than one layer height. Empty when k is held. */
static int overhang(const gs_mesh *m, float h, int k, gl_loops *out)
{
    gl_loops upper, lower, grown, bag;
    int rc;
    memset(&upper, 0, sizeof upper);
    memset(&lower, 0, sizeof lower);
    memset(&grown, 0, sizeof grown);
    memset(&bag, 0, sizeof bag);
    memset(out, 0, sizeof *out);
    rc = loops(m, h, k, &upper);
    if (rc > 0) rc = 0;
    if (!rc && upper.n) rc = loops(m, h, k - 1, &lower);
    if (rc > 0) rc = 0;
    if (!rc && upper.n) rc = gl_inset(&lower, -h, &grown);
    if (!rc && upper.n) rc = add_loops(&bag, &upper, 0) || add_loops(&bag, &grown, 1) || gl_union(&bag, out);
    gl_loops_free(&upper);
    gl_loops_free(&lower);
    gl_loops_free(&grown);
    gl_loops_free(&bag);
    if (rc) gl_loops_free(out);
    return rc ? -1 : 0;
}

/* One wall, and a line every four widths, at this layer under every overhang
 * above it that the part here does not already hold. The wall is a brim line
 * and the fill is infill: gc_opts has a speed for each kind, and a new kind
 * would change that struct. */
static int support_wall(gt_path *p, const gs_mesh *m, float h, float w, int n, float z, const gl_loops *part)
{
    int layers = gl_layers(m, h), k, rc = 0;
    gl_loops acc, one, bag, merged, need;
    memset(&acc, 0, sizeof acc);
    memset(&need, 0, sizeof need);
    for (k = n + 1; k <= layers && !rc; k++) {
        memset(&one, 0, sizeof one);
        rc = overhang(m, h, k, &one);
        if (!rc && one.n) {
            memset(&bag, 0, sizeof bag);
            memset(&merged, 0, sizeof merged);
            rc = add_loops(&bag, &acc, 0) || add_loops(&bag, &one, 0) || gl_union(&bag, &merged);
            gl_loops_free(&bag);
            gl_loops_free(&acc);
            acc = merged;
            if (rc) memset(&acc, 0, sizeof acc);
        }
        gl_loops_free(&one);
    }
    if (!rc && acc.n) {
        memset(&bag, 0, sizeof bag);
        rc = add_loops(&bag, &acc, 0) || add_loops(&bag, part, 1) || gl_union(&bag, &need);
        gl_loops_free(&bag);
        if (!rc && need.n) rc = ring(p, &need, w * 0.5f, GT_BRIM, 1, z, w) || gt_fill(p, &need, NULL, 0, z, w, 4, 0);
    }
    gl_loops_free(&acc);
    gl_loops_free(&need);
    return rc ? -1 : 0;
}

int gp_layer(gt_path *p, const gs_mesh *m, const gp_opts *o, int n)
{
    return gp_layer_more(p, m, o, NULL, n);
}

int gp_layer_more(gt_path *p, const gs_mesh *m, const gp_opts *o, const gp_more *more, int n)
{
    float h = gl_height(o->layer_h), w = o->line_w, z = (float)n * h;
    int walls = held(o->walls, 1, 8), skins = held(o->skins, 0, MOST_SKINS), below, above, ns = 0;
    int every = o->infill <= 0.f ? 1 << 20 : o->infill >= 100.f ? 1 : (int)(100.f / o->infill + 0.5f);
    gl_loops L, region, near[2 * MOST_SKINS];
    const gl_loops *solid[2 * MOST_SKINS];
    int i, k, r, rc = 0;
    gp_more x;
    more_of(more, &x);
    below = x.bottom < 0 ? skins : held(x.bottom, 0, MOST_SKINS);
    above = x.top < 0 ? skins : held(x.top, 0, MOST_SKINS);
    memset(&L, 0, sizeof L);
    memset(&region, 0, sizeof region);
    memset(near, 0, sizeof near);
    if (gt_layer(p)) return -1;
    r = loops(m, h, n, &L);
    if (r) return r < 0 ? -1 : 0;
    /* the skirt, out to in; the brim, from the part out; then the walls,
     * from the inside out */
    for (k = x.skirt; n == 1 && k >= 1 && !rc; k--) rc = ring(p, &L, -x.gap - w * ((float)k - 0.5f), GT_BRIM, 1, z, w);
    for (k = 1; n == 1 && k <= o->brim && !rc; k++) rc = ring(p, &L, -x.gap - w * ((float)k - 0.5f), GT_BRIM, 1, z, w);
    for (k = walls - 1; k >= 0 && !rc; k--) rc = ring(p, &L, w * ((float)k + 0.5f), k ? GT_INNER : GT_OUTER, 0, z, w);
    /* skin where the layers above or below run out, infill elsewhere */
    for (i = 0; i < below + above && !rc; i++) {
        r = loops(m, h, i < below ? n - 1 - i : n + 1 + (i - below), &near[ns]);
        solid[ns] = r ? NULL : &near[ns];
        ns++;
        rc = r < 0;
    }
    if (!rc) rc = gl_inset(&L, w * (float)walls, &region);
    if (!rc && x.pattern == GP_GRID) rc = gt_fill_grid(p, &region, solid, ns, z, w, every, n & 1);
    else if (!rc) rc = gt_fill(p, &region, solid, ns, z, w, every, n & 1);
    if (!rc && x.support) rc = support_wall(p, m, h, w, n, z, &L);
    for (i = 0; i < ns; i++) gl_loops_free(&near[i]);
    gl_loops_free(&region);
    gl_loops_free(&L);
    return rc ? -1 : 0;
}

static void shift(gs_v *v, float dx, float dy, float dz)
{
    v->x += dx;
    v->y += dy;
    v->z += dz;
}

void gp_place(gs_mesh *m, float bed_x, float bed_y)
{
    float dx = bed_x * 0.5f - (m->min.x + m->max.x) * 0.5f, dy = bed_y * 0.5f - (m->min.y + m->max.y) * 0.5f,
          dz = -m->min.z;
    int i;
    for (i = 0; i < m->n; i++) {
        shift(&m->t[i].a, dx, dy, dz);
        shift(&m->t[i].b, dx, dy, dz);
        shift(&m->t[i].c, dx, dy, dz);
    }
    m->min.x += dx;
    m->max.x += dx;
    m->min.y += dy;
    m->max.y += dy;
    m->min.z = 0.f;
    m->max.z += dz;
}

int gp_part(gt_path *p, const gs_mesh *m, const gp_opts *o)
{
    return gp_part_more(p, m, o, NULL);
}

int gp_part_more(gt_path *p, const gs_mesh *m, const gp_opts *o, const gp_more *x)
{
    int n, layers = gp_layers(m, o);
    gt_free(p);
    for (n = 1; n <= layers; n++)
        if (gp_layer_more(p, m, o, x, n)) return -1;
    return 0;
}
