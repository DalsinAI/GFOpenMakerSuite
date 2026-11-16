/* The OpenGPU batch for the cube with a hole. The library is not here:
 * this draws the same vertices the batch names, with the greater-depth
 * test the batch asks for, and checks the hole is still a hole.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gs_gpu.h"
#include "gs_model.h"
#include "gs_layer.h"

#include <gfopengpu/stream.h>
#include <gfopengpu/stream3d.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fails;

static void expect(int cond, const char *msg)
{
    if (!cond) {
        fprintf(stderr, "fail: %s\n", msg);
        fails++;
    }
}

static void putpix(unsigned char *rgb, int w, int h, int x, int y, int r, int g, int b)
{
    unsigned char *p;
    if (x < 0 || y < 0 || x >= w || y >= h) return;
    p = rgb + ((size_t)y * (size_t)w + (size_t)x) * 3;
    p[0] = (unsigned char)r;
    p[1] = (unsigned char)g;
    p[2] = (unsigned char)b;
}

/* The batch's rule: a larger z wins. z is the vertex word, not the painter's float. */
static void fill(unsigned char *rgb, uint32_t *zb, int w, int h,
                 float ax, float ay, uint32_t az, float bx, float by, uint32_t bz,
                 float cx, float cy, uint32_t cz, uint32_t colour)
{
    int minx, miny, maxx, maxy, x, y;
    float area, inv;
    minx = (int)ax;
    if ((int)bx < minx) minx = (int)bx;
    if ((int)cx < minx) minx = (int)cx;
    miny = (int)ay;
    if ((int)by < miny) miny = (int)by;
    if ((int)cy < miny) miny = (int)cy;
    maxx = (int)ax;
    if ((int)bx > maxx) maxx = (int)bx;
    if ((int)cx > maxx) maxx = (int)cx;
    maxy = (int)ay;
    if ((int)by > maxy) maxy = (int)by;
    if ((int)cy > maxy) maxy = (int)cy;
    if (minx < 0) minx = 0;
    if (miny < 0) miny = 0;
    if (maxx >= w) maxx = w - 1;
    if (maxy >= h) maxy = h - 1;
    area = (bx - ax) * (cy - ay) - (cx - ax) * (by - ay);
    if (area > -0.5f && area < 0.5f) return;
    inv = 1.f / area;
    for (y = miny; y <= maxy; y++) {
        for (x = minx; x <= maxx; x++) {
            float px = (float)x + 0.5f, py = (float)y + 0.5f;
            float w0 = ((bx - px) * (cy - py) - (cx - px) * (by - py)) * inv;
            float w1 = ((cx - px) * (ay - py) - (ax - px) * (cy - py)) * inv;
            float w2 = 1.f - w0 - w1;
            float zf;
            uint32_t z;
            int i;
            if (w0 < 0.f || w1 < 0.f || w2 < 0.f) continue;
            zf = w0 * (float)az + w1 * (float)bz + w2 * (float)cz;
            z = (uint32_t)zf;
            i = y * w + x;
            if (z <= zb[i]) continue;
            zb[i] = z;
            putpix(rgb, w, h, x, y, (int)((colour >> 16) & 255), (int)((colour >> 8) & 255), (int)(colour & 255));
        }
    }
}

static int sample(const unsigned char *rgb, int w, int h, float x, float y)
{
    int ix = (int)x, iy = (int)y;
    const unsigned char *p;
    if (ix < 0 || iy < 0 || ix >= w || iy >= h) return 0;
    p = rgb + ((size_t)iy * (size_t)w + (size_t)ix) * 3;
    return p[0] > 40 || p[1] > 40 || p[2] > 50;
}

int main(void)
{
    gs_mesh box, peg, hole;
    gs_cam cam;
    gs_frame frame;
    uint32_t *verts, stream[512];
    unsigned char *gpu, *cpu;
    uint32_t *zb;
    int nv, nw, i, coloured = 0;
    int saw_cmp = 0, saw_z = 0, saw_count = 0;
    float sx, sy, sz;
    const int w = 480, h = 360;
    memset(&box, 0, sizeof box);
    memset(&peg, 0, sizeof peg);
    memset(&hole, 0, sizeof hole);
    expect(gm_cube(&box, 0, 0, 0, 20, 20, 20) == 0, "cube");
    expect(gm_cylinder(&peg, 10, 10, -1, 6, 22, 32) == 0, "cylinder");
    expect(gm_subtract(&hole, &box, &peg) == 0, "subtract");
    gs_cam_home(&cam);
    verts = (uint32_t *)malloc((size_t)hole.n * 3u * 6u * sizeof *verts);
    expect(verts != NULL, "vertices");
    nv = verts ? gs_gpu_vertices(&hole, &cam, w, h, verts, hole.n * 3) : -1;
    expect(nv == hole.n * 3, "every triangle was packed");
    nw = gs_gpu_stream(stream, 512, 0x1000, nv, 0x2000, w * 4, 0x3000, w, h, &hole, &cam, 1);
    expect(nw > 0, "stream");
    fprintf(stderr, "vertices %d  stream %d words\n", nv, nw);
    for (i = 0; i < nw && stream[i]; ) {
        uint32_t hdr = stream[i];
        int op = (int)(hdr >> 16);
        int words = (int)(hdr & 0xFFFF);
        if (words < 1 || i + words > nw) break;
        if (op == OGPU_OP_RENDER3D) {
            saw_cmp = stream[i + 2] == 5;
        } else if (op == OGPU_OP_TRIANGLES) {
            saw_z = (stream[i + 9] & OGPU_LAY_Z) != 0;
            saw_count = stream[i + 2] == (uint32_t)nv && stream[i + 3] == 24;
        }
        i += words;
    }
    expect(saw_cmp, "z compare is Warp3D GREATER (5)");
    expect(saw_z, "triangles carry a depth");
    expect(saw_count, "the batch names every vertex");

    gpu = (unsigned char *)malloc((size_t)w * (size_t)h * 3);
    cpu = (unsigned char *)malloc((size_t)w * (size_t)h * 3);
    zb = (uint32_t *)calloc((size_t)w * (size_t)h, sizeof *zb);
    expect(gpu && cpu && zb, "pictures");
    if (gpu && cpu && zb && nv > 0) {
        for (i = 0; i < w * h; i++) {
            gpu[i * 3] = 20;
            gpu[i * 3 + 1] = 22;
            gpu[i * 3 + 2] = 28;
        }
        for (i = 0; i < nv; i += 3) {
            float ax = (float)(int32_t)verts[(i) * 6] / 65536.f;
            float ay = (float)(int32_t)verts[(i) * 6 + 1] / 65536.f;
            float bx = (float)(int32_t)verts[(i + 1) * 6] / 65536.f;
            float by = (float)(int32_t)verts[(i + 1) * 6 + 1] / 65536.f;
            float cx = (float)(int32_t)verts[(i + 2) * 6] / 65536.f;
            float cy = (float)(int32_t)verts[(i + 2) * 6 + 1] / 65536.f;
            fill(gpu, zb, w, h, ax, ay, verts[i * 6 + 5], bx, by, verts[(i + 1) * 6 + 5],
                 cx, cy, verts[(i + 2) * 6 + 5], verts[i * 6 + 2]);
        }
        for (i = 0; i < w * h * 3; i += 3)
            if (gpu[i] > 40 || gpu[i + 2] > 40) coloured++;
        expect(coloured > 2000, "the batch covers the picture");
        expect(gs_view_solid(&hole, &cam, cpu, w, h) == 0, "cpu picture");
        expect(gs_view_frame(&hole, &cam, w, h, &frame) == 0, "frame");
        gs_view_map(&frame, &cam, (gs_v){2.f, 2.f, 10.f}, &sx, &sy, &sz);
        expect(sample(cpu, w, h, sx, sy), "cpu: a corner is solid");
        expect(sample(gpu, w, h, sx, sy), "batch: a corner is solid");
        {
            int face = 0, agree = 0, cpu_n = 0;
            for (i = 0; i < w * h; i++) {
                const unsigned char *c = cpu + i * 3;
                const unsigned char *g = gpu + i * 3;
                int c_face = c[0] > 40 || c[2] > 50;
                int g_face = g[0] > 40 || g[2] > 50;
                if (c_face) {
                    cpu_n++;
                    /* The bed grid is in the painter and not in this raster. */
                    if (c[0] < 90 && c[1] > 60 && c[2] > 70) continue;
                    face++;
                    if (g_face) agree++;
                }
            }
            fprintf(stderr, "painter faces %d  matched %d\n", face, agree);
            expect(face > 2000, "the painter drew the solid");
            expect(agree * 100 > face * 90, "the batch matches the painter");
            (void)cpu_n;
        }
        {
            FILE *f = fopen("build/hole.json", "w");
            expect(f != NULL, "hole.json");
            if (f) {
                fprintf(f, "{\"n\":%d,\"tris\":[", hole.n);
                for (i = 0; i < hole.n; i++) {
                    gs_v a = hole.t[i].a, b = hole.t[i].b, c = hole.t[i].c;
                    fprintf(f, "%s[%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f]",
                            i ? "," : "", a.x, a.y, a.z, b.x, b.y, b.z, c.x, c.y, c.z);
                }
                fprintf(f, "]}");
                fclose(f);
            }
            expect(gm_stl_write(&hole, "build/hole.stl") == 0, "public stl");
        }
    }
    {
        /* The Layers group's walls: two triangles a piece of wall, Clear's
         * blue, nearest of all so they show through the part, on screen. */
        gl_walls wl;
        uint32_t *wv;
        int k, cap, blue = 1, near = 1, onscreen = 1;
        memset(&wl, 0, sizeof wl);
        expect(gl_cut(&hole, 0.5f, 20, &wl) == 0 && wl.n > 4, "walls to draw");
        cap = wl.n * 6;
        wv = (uint32_t *)malloc((size_t)cap * 6u * sizeof *wv);
        nv = wv ? gs_gpu_walls_plate(&wl, &hole, &cam, w, h, wv, cap) : -1;
        expect(nv == wl.n * 6, "six vertices a piece of wall");
        for (k = 0; k < nv; k++) {
            int32_t x = (int32_t)wv[k * 6], y = (int32_t)wv[k * 6 + 1];
            if (wv[k * 6 + 2] != 0xFF0067C0u) blue = 0;
            if (wv[k * 6 + 5] != 0xFFFFFFFFu) near = 0;
            if (x < -65536 || y < -65536 || x > (w + 1) * 65536 || y > (h + 1) * 65536) onscreen = 0;
        }
        expect(blue, "the walls are #0067c0");
        expect(near, "the walls are nearest");
        expect(onscreen, "the walls are in the picture");
        expect(gs_gpu_walls_plate(&wl, &hole, &cam, w, h, wv, 5) == -1, "too small a buffer is refused");
        free(wv);
        gl_walls_free(&wl);
    }
    {
        /* The pen drawer's runs: a row as runs of one pen, to fill with a
         * blit each (RectFill), not a WritePixel a pixel. */
        unsigned char row[8 * 3] = {20, 22, 28, 20, 22, 28, 90, 90, 90, 90, 90, 90,
                                    0, 103, 192, 200, 200, 200, 20, 22, 28, 20, 22, 28};
        gs_run runs[8];
        unsigned char *pic = (unsigned char *)malloc((size_t)w * (size_t)h * 3);
        gs_run *rr = (gs_run *)malloc((size_t)w * sizeof *rr);
        int n = gs_pen_row(row, 8, runs), y, total = 0, cover = 1, same = 1, apart = 1;
        expect(n == 5, "five runs in the row");
        expect(n == 5 && runs[0].x == 0 && runs[0].n == 2 && runs[0].pen == 0 && runs[1].x == 2 && runs[1].n == 2 &&
                   runs[1].pen == 1 && runs[2].pen == 3 && runs[3].pen == 2 && runs[4].x == 6 && runs[4].n == 2,
               "the runs are where the pens change");
        expect(gs_pen_of(row) == 0 && gs_pen_of(row + 6) == 1 && gs_pen_of(row + 12) == 3 && gs_pen_of(row + 15) == 2,
               "background, grey, the walls' blue, light");
        expect(pic && rr && gs_view_bed(&hole, &cam, pic, w, h) == 0, "the plate for the pens");
        for (y = 0; pic && rr && y < h; y++) {
            const unsigned char *line = pic + (size_t)y * (size_t)w * 3;
            int k, x = 0;
            n = gs_pen_row(line, w, rr);
            total += n;
            for (k = 0; k < n; k++) {
                int j;
                if (rr[k].x != x || rr[k].n < 1) cover = 0;
                if (k && rr[k].pen == rr[k - 1].pen) apart = 0;
                for (j = 0; j < rr[k].n && rr[k].x + j < w; j++)
                    if (gs_pen_of(line + (rr[k].x + j) * 3) != rr[k].pen) same = 0;
                x += rr[k].n;
            }
            if (x != w) cover = 0;
        }
        printf("pens: %d runs for %d pixels\n", total, w * h);
        expect(cover, "the runs cover every row, once");
        expect(same, "each run is one pen");
        expect(apart, "and the next run another");
        expect(total > 0 && total * 20 < w * h, "twenty pixels a blit or more");
        free(rr);
        free(pic);
    }
    free(verts);
    free(gpu);
    free(cpu);
    free(zb);
    gs_mesh_free(&box);
    gs_mesh_free(&peg);
    gs_mesh_free(&hole);
    if (fails) {
        fprintf(stderr, "%d failed\n", fails);
        return 1;
    }
    printf("gpu ok\n");
    return 0;
}
