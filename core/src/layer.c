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

/* A line two pixels wide, with no depth test: the walls are a cut through
 * the part, so they show through what is in front of them. */
static void line2(unsigned char *rgb, int w, int h, float x0, float y0, float x1, float y1)
{
    float dx = x1 - x0, dy = y1 - y0;
    int steps = (int)(fabsf(dx) > fabsf(dy) ? fabsf(dx) : fabsf(dy)), i, k;
    if (steps < 1) steps = 1;
    for (i = 0; i <= steps; i++) {
        float t = (float)i / (float)steps;
        int x = (int)(x0 + dx * t), y = (int)(y0 + dy * t);
        for (k = 0; k < 2; k++) {
            int px = x + (fabsf(dx) > fabsf(dy) ? 0 : k), py = y + (fabsf(dx) > fabsf(dy) ? k : 0);
            unsigned char *p;
            if (px < 0 || py < 0 || px >= w || py >= h) continue;
            p = rgb + ((size_t)py * (size_t)w + (size_t)px) * 3;
            p[0] = 0;
            p[1] = 103;
            p[2] = 192;
        }
    }
}

int gl_paint(const gl_walls *wl, const gs_mesh *m, const gs_cam *cam, unsigned char *rgb, int width, int height)
{
    gs_frame f;
    int i;
    if (!wl || !cam || !rgb) return -1;
    if (gs_view_frame_bed(m, cam, width, height, 0.f, 0.f, GS_BED, GS_BED, &f)) return -1;
    for (i = 0; i < wl->n; i++) {
        gs_v a = {wl->s[i].x0, wl->s[i].y0, wl->z}, b = {wl->s[i].x1, wl->s[i].y1, wl->z};
        float ax, ay, az, bx, by, bz;
        gs_view_map(&f, cam, a, &ax, &ay, &az);
        gs_view_map(&f, cam, b, &bx, &by, &bz);
        line2(rgb, width, height, ax, ay, bx, by);
    }
    return 0;
}
