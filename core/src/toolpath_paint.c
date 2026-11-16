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

/* From (ax, ay, az) to (bx, by, bz) in colour c, scaled by shade (0 to 256). */
static void line(unsigned char *rgb, float *zb, int w, int h, const float *a, const float *b, unsigned long c, int shade)
{
    float dx = b[0] - a[0], dy = b[1] - a[1];
    int n = (int)((dx < 0 ? -dx : dx) > (dy < 0 ? -dy : dy) ? (dx < 0 ? -dx : dx) : (dy < 0 ? -dy : dy)) + 1, i;
    for (i = 0; i <= n; i++) {
        float t = (float)i / (float)n, z = a[2] + (b[2] - a[2]) * t;
        int x = (int)(a[0] + dx * t + 0.5f), y = (int)(a[1] + dy * t + 0.5f), k;
        unsigned char *p;
        if (x < 0 || y < 0 || x >= w || y >= h) continue;
        k = y * w + x;
        if (z < zb[k]) continue;
        zb[k] = z;
        p = rgb + k * 3;
        p[0] = (unsigned char)(((c >> 16) & 255) * (unsigned long)shade / 256);
        p[1] = (unsigned char)(((c >> 8) & 255) * (unsigned long)shade / 256);
        p[2] = (unsigned char)((c & 255) * (unsigned long)shade / 256);
    }
}

int gt_paint(const gt_path *p, const gs_cam *cam, int upto, unsigned char *rgb, int w, int h)
{
    gs_mesh none;
    gs_cam home;
    gs_frame f;
    float *zb, top = 0.f, at[3], was[3];
    int i, end = p->n;
    if (!rgb || w < 4 || h < 4) return -1;
    if (!cam) {
        gs_cam_home(&home);
        cam = &home;
    }
    memset(&none, 0, sizeof none);
    if (gs_view_bed(&none, cam, rgb, w, h) || gs_view_frame_bed(&none, cam, w, h, 0.f, 0.f, GS_BED, GS_BED, &f)) return -1;
    if (upto > 0 && upto < p->nlayers) end = p->layer[upto];
    for (i = 0; i < end; i++)
        if (p->m[i].z > top) top = p->m[i].z;
    zb = (float *)malloc((size_t)w * (size_t)h * sizeof *zb);
    if (!zb) return -1;
    for (i = 0; i < w * h; i++) zb[i] = -1e30f;
    for (i = 0; i < end; i++) {
        const gt_move *m = &p->m[i];
        gs_v v;
        v.x = m->x;
        v.y = m->y;
        v.z = m->z;
        gs_view_map(&f, cam, v, &at[0], &at[1], &at[2]);
        if (i && m->kind != GT_TRAVEL)
            line(rgb, zb, w, h, was, at, gt_colour(m->kind), top > 0.f ? 120 + (int)(136.f * m->z / top) : 256);
        memcpy(was, at, sizeof at);
    }
    free(zb);
    return 0;
}
