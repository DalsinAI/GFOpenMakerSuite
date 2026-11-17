/* Loops moved into the material (gl_inset in gs_layer.h). Outsides turn
 * anticlockwise and holes clockwise, so the material is always on the
 * left: each edge moves left by d and each corner goes where its two
 * edges' lines meet (a mitre). A very sharp corner's mitre is cut short.
 * An edge the move turns round has closed up and goes. Where the moved
 * loops then cross themselves or each other, gl_union joins them. A d
 * below 0 moves out of the material instead, as a brim does.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gs_layer.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define GAP 1e-4f /* points nearer than this are one */

typedef struct {
    float cx, cy;  /* where the edge starts, before the move */
    float px, py;  /* and after it */
    float ux, uy;  /* along it, a unit */
} edge;

/* Where edge a's moved line meets edge b's: the corner between them. Lines
 * along each other meet at b's start; a mitre past lim from the corner it
 * came from is cut back to lim. */
static void corner(const edge *a, const edge *b, float lim, float *x, float *y)
{
    float cross = a->ux * b->uy - a->uy * b->ux, t, dx, dy, r;
    *x = b->px;
    *y = b->py;
    if (fabsf(cross) < 1e-6f) return;
    t = ((b->px - a->px) * b->uy - (b->py - a->py) * b->ux) / cross;
    *x = a->px + t * a->ux;
    *y = a->py + t * a->uy;
    dx = *x - b->cx;
    dy = *y - b->cy;
    r = sqrtf(dx * dx + dy * dy);
    if (r > lim) {
        *x = b->cx + dx * lim / r;
        *y = b->cy + dy * lim / r;
    }
}

/* src without repeated points, its edges moved left by d, in o. An edge
 * the move turns round goes, and its neighbours meet instead, until none
 * is turned round. 0, 1 when no room is left, or -1. */
static int move(const gl_loop *src, float d, gl_loop *o)
{
    edge *e = (edge *)malloc((size_t)src->n * sizeof *e);
    float *m = (float *)malloc((size_t)src->n * 2 * sizeof *m), lim = 3.f * fabsf(d);
    int n = 0, i, k, turned = 1;
    if (!e || !m) {
        free(e);
        free(m);
        return -1;
    }
    for (i = 0; i < src->n; i++) {
        const float *p = src->xy + i * 2;
        if (n && fabsf(p[0] - e[n - 1].cx) < GAP && fabsf(p[1] - e[n - 1].cy) < GAP) continue;
        e[n].cx = p[0];
        e[n].cy = p[1];
        n++;
    }
    if (n > 1 && fabsf(e[0].cx - e[n - 1].cx) < GAP && fabsf(e[0].cy - e[n - 1].cy) < GAP) n--;
    for (i = 0; i < n; i++) {
        const edge *q = &e[(i + 1) % n];
        float dx = q->cx - e[i].cx, dy = q->cy - e[i].cy, len = sqrtf(dx * dx + dy * dy);
        e[i].ux = dx / len;
        e[i].uy = dy / len;
        e[i].px = e[i].cx - d * e[i].uy;
        e[i].py = e[i].cy + d * e[i].ux;
    }
    while (turned && n >= 3) {
        turned = 0;
        for (i = 0; i < n; i++) corner(&e[(i + n - 1) % n], &e[i], lim, &m[i * 2], &m[i * 2 + 1]);
        for (i = 0; i < n; i++) {
            k = (i + 1) % n;
            if ((m[k * 2] - m[i * 2]) * e[i].ux + (m[k * 2 + 1] - m[i * 2 + 1]) * e[i].uy < 0.f) {
                memmove(e + i, e + i + 1, (size_t)(n - i - 1) * sizeof *e);
                n--;
                turned = 1;
                break;
            }
        }
    }
    free(e);
    o->xy = m;
    o->n = n;
    o->closed = 1;
    return n < 3;
}

int gl_inset(const gl_loops *in, float d, gl_loops *out)
{
    gl_loops moved;
    int i;
    gl_loops_free(out);
    if (!in->n) return 0;
    out->l = (gl_loop *)calloc((size_t)in->n, sizeof *out->l);
    if (!out->l) return -1;
    for (i = 0; i < in->n; i++) {
        const gl_loop *l = &in->l[i];
        gl_loop *o = &out->l[out->n];
        float a, b;
        int r;
        if (!l->closed || l->n < 3) continue;
        r = move(l, d, o);
        if (r < 0) {
            gl_loops_free(out);
            return -1;
        }
        a = gl_loop_area(l);
        b = r ? 0.f : gl_loop_area(o);
        /* No room: it turned inside out, or moved the wrong way (in, the
         * area to the left falls; out, it rises). */
        if (r || (a > 0.f) != (b > 0.f) || (d > 0.f ? b >= a : b <= a) || fabsf(b) < GAP) {
            free(o->xy);
            memset(o, 0, sizeof *o);
            continue;
        }
        out->n++;
    }
    /* Walls that met: one material, as loops that do not cross. */
    moved = *out;
    memset(out, 0, sizeof *out);
    i = gl_union(&moved, out);
    gl_loops_free(&moved);
    return i;
}
