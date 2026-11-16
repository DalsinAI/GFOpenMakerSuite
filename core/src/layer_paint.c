/* A layer's walls painted over the plate picture, for the pen drawer: the
 * window's, not the paths', so it is not in openmakepath.library.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gs_layer.h"

#include <math.h>

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
