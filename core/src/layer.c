/* The walls of a part at one height: each triangle the plane crosses gives
 * one piece of wall. MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gs_layer.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

float gl_height(float h)
{
    if (!(h == h)) return 0.2f;
    if (h < GL_MIN_H) return GL_MIN_H;
    if (h > GL_MAX_H) return GL_MAX_H;
    return h;
}

int gl_layers(const gs_mesh *m, float h)
{
    float tall = m->n ? m->max.z - m->min.z : 0.f;
    int n;
    if (tall <= 0.f) return 0;
    n = (int)ceilf(tall / gl_height(h) - 1e-4f);
    return n < 1 ? 1 : n;
}

float gl_layer_z(const gs_mesh *m, float h, int layer)
{
    int n = gl_layers(m, h);
    if (layer > n) layer = n;
    if (layer < 1) layer = 1;
    float z = m->min.z + gl_height(h) * ((float)layer - 0.5f);
    return z > m->max.z ? m->max.z : z; /* a short top layer stops at the top */
}

/* An edge is crossed when its ends are on two sides. A point on the plane
 * counts as above, so a triangle gives none or two. */
static int cross(gs_v p, gs_v q, float z, float *x, float *y)
{
    float t;
    if ((p.z < z) == (q.z < z)) return 0;
    t = (z - p.z) / (q.z - p.z);
    *x = p.x + t * (q.x - p.x);
    *y = p.y + t * (q.y - p.y);
    return 1;
}

int gl_cut(const gs_mesh *m, float h, int layer, gl_walls *w)
{
    int i;
    gl_walls_free(w);
    if (!gl_layers(m, h)) return 0;
    w->z = gl_layer_z(m, h, layer);
    for (i = 0; i < m->n; i++) {
        gs_v v[3];
        float px[2], py[2];
        int e, k = 0;
        v[0] = m->t[i].a;
        v[1] = m->t[i].b;
        v[2] = m->t[i].c;
        for (e = 0; e < 3 && k < 2; e++)
            k += cross(v[e], v[(e + 1) % 3], w->z, &px[k], &py[k]);
        if (k < 2) continue;
        if (w->n == w->cap) {
            int c = w->cap ? w->cap * 2 : 64;
            gl_seg *s = (gl_seg *)realloc(w->s, (size_t)c * sizeof *s);
            if (!s) return -1;
            w->s = s;
            w->cap = c;
        }
        w->s[w->n++] = (gl_seg){px[0], py[0], px[1], py[1]};
        w->length += sqrtf((px[1] - px[0]) * (px[1] - px[0]) + (py[1] - py[0]) * (py[1] - py[0]));
    }
    return 0;
}

void gl_walls_free(gl_walls *w)
{
    free(w->s);
    memset(w, 0, sizeof *w);
}

void gl_status(char *buf, int len, const gs_mesh *m, float h, int layer, const gl_walls *w)
{
    int n = gl_layers(m, h);
    if (!buf || len <= 0) return;
    if (!n) {
        snprintf(buf, (size_t)len, "No layers: there is no part.");
        return;
    }
    if (layer > n) layer = n;
    if (layer < 1) layer = 1;
    snprintf(buf, (size_t)len, "Layer %d of %d, %.1f mm of wall. No infill.", layer, n, (double)w->length);
}
