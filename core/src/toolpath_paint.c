/* The toolpath's picture (gt_paint in gs_toolpath.h), over gs_view_bed's
 * plate: each move a line, nearer in front (a depth for each pixel).
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gs_toolpath.h"

#include <stdlib.h>
#include <string.h>

unsigned long gt_colour(int kind)
{
    static const unsigned long c[GT_KINDS] = {
        0x000000, /* travel: not drawn */
        0xe8742a, /* outer wall */
        0xf0c040, /* inner walls */
        0xd03040, /* skin */
        0xb08050, /* infill */
        0x30a060, /* brim */
        0x0067c0  /* a laser's or a mill's line: Clear's blue */
    };
    return kind >= 0 && kind < GT_KINDS ? c[kind] : 0;
}

unsigned long gt_shade(int kind, float z, float top)
{
    unsigned long c = gt_colour(kind), s = top > 0.f ? 120ul + (unsigned long)(136.f * z / top) : 256ul;
    if (s > 256ul) s = 256ul;
    return (((c >> 16) & 255) * s / 256) << 16 | (((c >> 8) & 255) * s / 256) << 8 | ((c & 255) * s / 256);
}

int gt_end(const gt_path *p, int upto)
{
    return upto > 0 && upto < p->nlayers ? p->layer[upto] : p->n;
}

/* From (ax, ay, az) to (bx, by, bz) in colour c. */
static void line(unsigned char *rgb, float *zb, int w, int h, const float *a, const float *b, unsigned long c)
{
    float dx = b[0] - a[0], dy = b[1] - a[1];
    int n = (int)((dx < 0 ? -dx : dx) > (dy < 0 ? -dy : dy) ? (dx < 0 ? -dx : dx) : (dy < 0 ? -dy : dy)) + 1, i;
    for (i = 0; i <= n; i++) {
        float t = (float)i / (float)n, z = a[2] + (b[2] - a[2]) * t;
        int x = (int)(a[0] + dx * t + 0.5f), y = (int)(a[1] + dy * t + 0.5f), k;
        unsigned char *p;
        if (x < 0 || y < 0 || x >= w || y >= h) continue;
        k = y * w + x;
        if (z < zb[k]) continue; /* nearer is larger */
        zb[k] = z;
        p = rgb + k * 3;
        p[0] = (unsigned char)((c >> 16) & 255);
        p[1] = (unsigned char)((c >> 8) & 255);
        p[2] = (unsigned char)(c & 255);
    }
}

/* f refitted so the moves (not travels) fill the picture, as gs_view_frame
 * fits a mesh: 18 pixels round, cam's zoom and pan after. */
static void fit_moves(const gt_path *p, const gs_cam *cam, gs_frame *f)
{
    float x0 = 1e30f, y0 = 1e30f, x1 = -1e30f, y1 = -1e30f, s;
    int i;
    for (i = 0; i < p->n; i++) {
        gs_v v;
        float x, y, z, rx, ry;
        if (p->m[i].kind == GT_TRAVEL) continue;
        v.x = p->m[i].x;
        v.y = p->m[i].y;
        v.z = p->m[i].z;
        gs_view_map(f, cam, v, &x, &y, &z);
        rx = (x - f->ox) / f->s; /* back to the view's own units */
        ry = ((float)f->h - y - f->oy) / f->s;
        x0 = rx < x0 ? rx : x0;
        x1 = rx > x1 ? rx : x1;
        y0 = ry < y0 ? ry : y0;
        y1 = ry > y1 ? ry : y1;
    }
    if (x1 - x0 < 0.01f || y1 - y0 < 0.01f) return;
    s = (float)(f->w - 36) / (x1 - x0);
    if ((float)(f->h - 36) / (y1 - y0) < s) s = (float)(f->h - 36) / (y1 - y0);
    s *= cam->dist;
    f->ox = ((float)f->w - (x1 - x0) * s) * 0.5f - x0 * s + cam->panx * (x1 - x0) * s;
    f->oy = ((float)f->h - (y1 - y0) * s) * 0.5f - y0 * s + cam->pany * (y1 - y0) * s;
    f->s = s;
}

int gt_frame(const gt_path *p, const gs_cam *cam, int part, int w, int h, gs_frame *f)
{
    gs_mesh none;
    float lo[3], hi[3];
    memset(&none, 0, sizeof none);
    if (part && gt_bounds(p, lo, hi) == 0) {
        if (gs_view_frame_bed(&none, cam, w, h, lo[0], lo[1], hi[0], hi[1], f)) return -1;
        fit_moves(p, cam, f);
        return 0;
    }
    return gs_view_frame_bed(&none, cam, w, h, 0.f, 0.f, GS_BED, GS_BED, f);
}

/* part: framed to the moves over a plain background, else the plate. */
static int strokes(const gt_path *p, const gs_cam *cam, const gs_frame *f, int end, unsigned char *rgb, int w, int h)
{
    float *zb, top = 0.f, at[3], was[3];
    int i;
    zb = (float *)malloc((size_t)w * (size_t)h * sizeof *zb);
    if (!zb) return -1;
    for (i = 0; i < end; i++)
        if (p->m[i].z > top) top = p->m[i].z;
    for (i = 0; i < w * h; i++) zb[i] = -1e30f;
    for (i = 0; i < end; i++) {
        const gt_move *m = &p->m[i];
        gs_v v;
        v.x = m->x;
        v.y = m->y;
        v.z = m->z;
        gs_view_map(f, cam, v, &at[0], &at[1], &at[2]);
        if (i && m->kind != GT_TRAVEL) line(rgb, zb, w, h, was, at, gt_shade(m->kind, m->z, top));
        memcpy(was, at, sizeof at);
    }
    free(zb);
    return 0;
}

static int paint(const gt_path *p, const gs_cam *cam, int upto, unsigned char *rgb, int w, int h, int part)
{
    gs_mesh none;
    gs_cam home;
    gs_frame f;
    float lo[3], hi[3];
    int i, end = gt_end(p, upto);
    if (!rgb || w < 4 || h < 4) return -1;
    if (!cam) {
        gs_cam_home(&home);
        cam = &home;
    }
    memset(&none, 0, sizeof none);
    if (gs_view_bed(&none, cam, rgb, w, h) || gt_frame(p, cam, part, w, h, &f)) return -1;
    if (part && gt_bounds(p, lo, hi) == 0)
        for (i = 0; i < w * h * 3; i += 3) {
            rgb[i] = 20;
            rgb[i + 1] = 22;
            rgb[i + 2] = 28;
        }
    return strokes(p, cam, &f, end, rgb, w, h);
}

int gt_paint(const gt_path *p, const gs_cam *cam, int upto, unsigned char *rgb, int w, int h)
{
    return paint(p, cam, upto, rgb, w, h, 0);
}

int gt_paint_part(const gt_path *p, const gs_cam *cam, int upto, unsigned char *rgb, int w, int h)
{
    return paint(p, cam, upto, rgb, w, h, 1);
}

int gt_paint_on(const gt_path *p, const gs_mesh *solid, const gs_cam *cam, int upto, unsigned char *rgb, int w, int h)
{
    gs_mesh none;
    gs_cam home;
    gs_frame f;
    if (!p || !rgb || w < 4 || h < 4) return -1;
    if (!cam) {
        gs_cam_home(&home);
        cam = &home;
    }
    if (!solid) {
        memset(&none, 0, sizeof none);
        solid = &none;
    }
    /* The solid's frame, the plate, so a line lands on the face it was cut from. */
    if (gs_view_frame_bed(solid, cam, w, h, 0.f, 0.f, GS_BED, GS_BED, &f)) return -1;
    return strokes(p, cam, &f, gt_end(p, upto), rgb, w, h);
}
