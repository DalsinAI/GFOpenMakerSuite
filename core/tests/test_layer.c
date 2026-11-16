/* The Layers group: walls at one height, a cube, a cube with a hole, and
 * the NASA wrench from the shelf. MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gs_layer.h"
#include "gs_model.h"
#include "gs_view.h"

#include <stdlib.h>
#include <string.h>

#include <math.h>
#include <stdio.h>

static int fails;

static void expect(int cond, const char *msg)
{
    if (!cond) {
        fprintf(stderr, "fail: %s\n", msg);
        fails++;
    }
}

static int near(float a, float b, float tol)
{
    return fabsf(a - b) < tol;
}

int main(void)
{
    gs_mesh box = {0}, rod = {0}, holed = {0}, empty = {0}, wrench;
    gl_walls w = {0};
    char err[160];
    expect(near(gl_height(0.01f), 0.08f, 1e-6f), "height floor is 0.08");
    expect(near(gl_height(5.f), 0.8f, 1e-6f), "height ceiling is 0.8");
    expect(near(gl_height(0.3f), 0.3f, 1e-6f), "0.3 stays");
    expect(near(gl_height(NAN), 0.2f, 1e-6f), "not a number is 0.2");

    expect(gm_cube(&box, 0.f, 0.f, 0.f, 10.f, 10.f, 10.f) == 0, "cube");
    expect(gl_layers(&box, 0.2f) == 50, "10 mm at 0.2 is 50 layers");
    expect(gl_layers(&box, 0.01f) == 125, "a tiny height is clamped to 0.08");
    expect(gl_layers(&empty, 0.2f) == 0, "nothing has no layers");
    expect(gl_cut(&box, 0.2f, 1, &w) == 0, "cut layer 1");
    expect(near(w.z, 0.1f, 1e-4f), "layer 1 cuts mid-layer, at 0.1");
    expect(near(w.length, 40.f, 0.01f), "a 10 mm square is 40 mm of wall");
    expect(w.n >= 4, "four sides at least");
    expect(gl_cut(&box, 0.2f, 999, &w) == 0 && near(w.z, 9.9f, 1e-3f) && near(w.length, 40.f, 0.01f), "past the top is the last layer, at 9.9");
    expect(gl_cut(&empty, 0.2f, 1, &w) == 0 && w.n == 0 && w.length == 0.f, "nothing cuts to nothing");

    expect(gm_cube(&box, 0.f, 0.f, 0.f, 20.f, 20.f, 20.f) == 0, "20 mm cube");
    expect(gm_cylinder(&rod, 10.f, 10.f, -1.f, 6.f, 22.f, 32) == 0, "rod");
    expect(gm_subtract(&holed, &box, &rod) == 0, "hole");
    expect(gl_cut(&holed, 0.5f, 20, &w) == 0, "cut the holed cube");
    expect(near(w.length, 80.f + 64.f * 6.f * sinf(3.14159265f / 32.f), 0.05f), "outside and hole are both wall");
    {
        /* The status line and the blue walls on the plate (the pen drawer's
         * picture; OpenGPU's are in test_gpu). */
        char line[96];
        unsigned char *rgb = (unsigned char *)malloc(160 * 120 * 3);
        gs_cam cam;
        gs_frame f;
        float x0, y0, x1, y1, z;
        int i, blue = 0, outside = 0;
        gl_status(line, (int)sizeof line, &holed, 0.5f, 20, &w);
        printf("%s\n", line);
        expect(strcmp(line, "Layer 20 of 40, 117.6 mm of wall. No infill.") == 0, "the status line");
        gl_status(line, (int)sizeof line, &empty, 0.2f, 1, &w);
        expect(strcmp(line, "No layers: there is no part.") == 0, "and with nothing to cut");
        gs_cam_home(&cam);
        expect(rgb && gs_view_bed(&holed, &cam, rgb, 160, 120) == 0, "the plate");
        for (i = 0; rgb && i < 160 * 120; i++)
            if (rgb[i * 3] == 0 && rgb[i * 3 + 1] == 103 && rgb[i * 3 + 2] == 192) blue++;
        expect(blue == 0, "no blue before the walls");
        gl_cut(&holed, 0.5f, 20, &w);
        expect(rgb && gl_paint(&w, &holed, &cam, rgb, 160, 120) == 0, "paint the walls");
        /* Every blue pixel lies inside where the part's corners land. */
        gs_view_frame_bed(&holed, &cam, 160, 120, 0.f, 0.f, GS_BED, GS_BED, &f);
        x0 = y0 = 1e9f;
        x1 = y1 = -1e9f;
        for (i = 0; i < 8; i++) {
            gs_v c = {(i & 1) ? 20.f : 0.f, (i & 2) ? 20.f : 0.f, (i & 4) ? 20.f : 0.f};
            float x, y;
            gs_view_map(&f, &cam, c, &x, &y, &z);
            if (x < x0) x0 = x;
            if (y < y0) y0 = y;
            if (x > x1) x1 = x;
            if (y > y1) y1 = y;
        }
        for (i = 0; rgb && i < 160 * 120; i++)
            if (rgb[i * 3] == 0 && rgb[i * 3 + 1] == 103 && rgb[i * 3 + 2] == 192) {
                int px = i % 160, py = i / 160;
                blue++;
                if (px < x0 - 2 || px > x1 + 2 || py < y0 - 2 || py > y1 + 2) outside++;
            }
        printf("walls: %d blue pixels, %d outside the part\n", blue, outside);
        expect(blue > 20 && outside == 0, "the walls are blue, on the part");
        free(rgb);
    }

    expect(gs_stl_load(&wrench, "cad/bench/library/nasa-wrench.stl", err, (int)sizeof err) == 0, "wrench loads");
    gs_mesh_repair(&wrench);
    expect(gl_layers(&wrench, 0.2f) == 130, "the wrench is 130 layers at 0.2: 26 mm, no empty layer above it");
    expect(gl_cut(&wrench, 0.2f, 1, &w) == 0, "cut the wrench");
    printf("wrench layer 1 of %d: %d pieces, %.1f mm of wall\n", gl_layers(&wrench, 0.2f), w.n, w.length);
    expect(near(w.z, 0.1f, 1e-4f), "wrench layer 1 cuts at 0.1");
    /* Four triangles have an edge lying in z = 0.1. Each such edge is wall
     * once, not once for each triangle beside it. */
    expect(near(w.length, 441.2f, 0.05f), "441.2 mm of wall on layer 1");

    {
        /* Welded into loops: every wrench piece closes. */
        gl_loops L = {0};
        int k, closed = 0;
        float per = 0.f;
        expect(gl_weld(&w, 1e-3f, &L) == 0, "weld the wrench's layer 1");
        for (k = 0; k < L.n; k++) {
            closed += L.l[k].closed;
            per += gl_loop_length(&L.l[k]);
        }
        printf("wrench layer 1: %d loops, %d open pieces, %.1f mm round them\n", L.n, L.open, per);
        expect(L.n > 0 && closed == L.n && L.open == 0, "the wrench's loops all close");
        expect(near(per, w.length, 0.05f), "and go round all its wall");
        {
            int layer, open = 0, bad = 0, n = gl_layers(&wrench, 0.2f);
            for (layer = 1; layer <= n; layer++) {
                if (gl_cut(&wrench, 0.2f, layer, &w) || gl_weld(&w, 1e-3f, &L)) bad++;
                open += L.open;
            }
            printf("wrench: %d open pieces over %d layers\n", open, n);
            expect(!bad && open == 0, "every wrench layer closes");
        }
        gl_loops_free(&L);
    }
    {
        /* A square, its pieces out of order, some backwards, the ends a
         * little apart: one loop, anticlockwise. Three sides: one open piece. */
        gl_walls sq = {0};
        gl_loops L = {0};
        gl_seg s[4] = {{10.f, 0.f, 10.f, 10.f}, {0.f, 0.f, 10.f, 0.00001f}, {0.f, 10.f, 0.f, 0.f}, {0.f, 10.f, 10.00002f, 10.f}};
        sq.s = s;
        sq.n = 4;
        expect(gl_weld(&sq, 1e-3f, &L) == 0 && L.n == 1 && L.open == 0, "a square is one loop");
        expect(L.n == 1 && L.l[0].n == 4 && L.l[0].closed, "of four corners, closed");
        expect(L.n == 1 && near(gl_loop_area(&L.l[0]), 100.f, 1e-3f), "anticlockwise, 100 mm2");
        sq.n = 3;
        expect(gl_weld(&sq, 1e-3f, &L) == 0 && L.n == 1 && L.open == 1 && !L.l[0].closed && L.l[0].n == 4,
               "three sides are one open piece");
        sq.n = 0;
        expect(gl_weld(&sq, 1e-3f, &L) == 0 && L.n == 0 && L.open == 0, "nothing welds to nothing");
        gl_loops_free(&L);
    }
    {
        /* The holed cube: the outside anticlockwise, the hole clockwise. */
        gl_loops L = {0};
        float a0, a1, hole = 0.5f * 32.f * 36.f * sinf(2.f * 3.14159265f / 32.f);
        expect(gl_cut(&holed, 0.5f, 20, &w) == 0 && gl_weld(&w, 1e-3f, &L) == 0 && L.n == 2 && L.open == 0,
               "the holed cube is two loops");
        a0 = L.n == 2 ? gl_loop_area(&L.l[0]) : 0.f;
        a1 = L.n == 2 ? gl_loop_area(&L.l[1]) : 0.f;
        printf("holed cube: areas %.2f and %.2f\n", a0, a1);
        expect((near(a0, 400.f, 0.01f) && near(a1, -hole, 0.05f)) || (near(a1, 400.f, 0.01f) && near(a0, -hole, 0.05f)),
               "the outside anticlockwise and the hole clockwise");
        gl_loops_free(&L);
    }
    gl_walls_free(&w);
    gs_mesh_free(&box);
    gs_mesh_free(&rod);
    gs_mesh_free(&holed);
    gs_mesh_free(&wrench);
    if (fails) {
        fprintf(stderr, "%d failed\n", fails);
        return 1;
    }
    printf("layer ok\n");
    return 0;
}
