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

int gp_layer(gt_path *p, const gs_mesh *m, const gp_opts *o, int n)
{
    float h = gl_height(o->layer_h), w = o->line_w, z = (float)n * h;
    int walls = o->walls < 1 ? 1 : o->walls > 8 ? 8 : o->walls;
    int skins = o->skins < 0 ? 0 : o->skins > MOST_SKINS ? MOST_SKINS : o->skins;
    int every = o->infill <= 0.f ? 1 << 20 : o->infill >= 100.f ? 1 : (int)(100.f / o->infill + 0.5f);
    gl_loops L, region, near[2 * MOST_SKINS];
    const gl_loops *solid[2 * MOST_SKINS];
    int i, k, r, rc = 0;
    memset(&L, 0, sizeof L);
    memset(&region, 0, sizeof region);
    memset(near, 0, sizeof near);
    if (gt_layer(p)) return -1;
    r = loops(m, h, n, &L);
    if (r) return r < 0 ? -1 : 0;
    /* the brim, from the part out; then the walls, from the inside out */
    for (k = 1; n == 1 && k <= o->brim && !rc; k++) rc = ring(p, &L, -w * ((float)k - 0.5f), GT_BRIM, 1, z, w);
    for (k = walls - 1; k >= 0 && !rc; k--) rc = ring(p, &L, w * ((float)k + 0.5f), k ? GT_INNER : GT_OUTER, 0, z, w);
    /* skin where the layers above or below run out, infill elsewhere */
    for (i = 0; i < skins && !rc; i++) {
        r = loops(m, h, n - 1 - i, &near[2 * i]);
        solid[2 * i] = r ? NULL : &near[2 * i];
        rc = r < 0;
        r = loops(m, h, n + 1 + i, &near[2 * i + 1]);
        solid[2 * i + 1] = r ? NULL : &near[2 * i + 1];
        rc = rc || r < 0;
    }
    if (!rc) rc = gl_inset(&L, w * (float)walls, &region);
    if (!rc) rc = gt_fill(p, &region, solid, 2 * skins, z, w, every, n & 1);
    for (i = 0; i < 2 * skins; i++) gl_loops_free(&near[i]);
    gl_loops_free(&region);
    gl_loops_free(&L);
    return rc ? -1 : 0;
}
