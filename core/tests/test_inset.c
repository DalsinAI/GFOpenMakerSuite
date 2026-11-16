/* Walls inside walls: a layer's loops moved into the material.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gs_layer.h"
#include "gs_model.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static int fails;

static void expect(int cond, const char *msg)
{
    if (!cond) {
        fprintf(stderr, "fail: %s\n", msg);
        fails++;
    }
}

static int near(float a, float b, float tol) { return fabsf(a - b) < tol; }

/* The loops of layer 1 of m at 0.2 mm. */
static void loops_of(const gs_mesh *m, gl_loops *L)
{
    gl_walls w;
    memset(&w, 0, sizeof w);
    expect(gl_cut(m, 0.2f, 1, &w) == 0 && gl_weld(&w, 1e-3f, L) == 0, "cut and welded");
    gl_walls_free(&w);
}

int main(void)
{
    gs_mesh box, rod, holed, wrench;
    gl_loops L, in, gone;
    float slim[] = {0, 0, 10, 0, 10, 1, 0, 1}, a = 6.f * cosf(3.14159265f / 32.f) + 0.2f, before = 0.f, after = 0.f;
    gl_loops thin = {NULL, 1, 0};
    gl_loop bar = {slim, 4, 1};
    char err[96];
    int i;
    memset(&box, 0, sizeof box);
    memset(&rod, 0, sizeof rod);
    memset(&holed, 0, sizeof holed);
    memset(&L, 0, sizeof L);
    memset(&in, 0, sizeof in);
    memset(&gone, 0, sizeof gone);
    expect(gm_cube(&box, 0, 0, 0, 20, 20, 20) == 0 && gm_cylinder(&rod, 10, 10, -1, 6, 22, 32) == 0 &&
               gm_subtract(&holed, &box, &rod) == 0,
           "a cube with a hole");
    loops_of(&holed, &L);
    expect(L.n == 2, "two loops");

    expect(gl_inset(&L, 0.2f, &in) == 0 && in.n == 2 && in.open == 0, "both moved in by 0.2");
    for (i = 0; i < in.n; i++) {
        float s = gl_loop_area(&in.l[i]);
        expect(in.l[i].closed, "still closed");
        if (s > 0.f)
            expect(near(s, 19.6f * 19.6f, 0.01f), "the outside shrinks to 19.6 square");
        else
            expect(near(-s, 32.f * a * a * tanf(3.14159265f / 32.f), 0.05f), "the hole grows 0.2 all round");
    }
    expect(gl_inset(&L, 10.5f, &gone) == 0 && gone.n == 1 && gl_loop_area(&gone.l[0]) < 0.f,
           "10.5 closes the outside up, and only the hole is left");

    thin.l = &bar;
    gl_loops_free(&gone);
    expect(gl_inset(&thin, 0.6f, &gone) == 0 && gone.n == 0, "a 1 mm bar has no room for 0.6 a side");
    expect(gl_inset(&thin, 0.4f, &gone) == 0 && gone.n == 1 && near(gl_loop_area(&gone.l[0]), 9.2f * 0.2f, 1e-3f),
           "but has for 0.4");

    expect(gs_stl_load(&wrench, "core/tests/data/nasa-wrench.stl", err, (int)sizeof err) == 0, "the wrench");
    gs_mesh_repair(&wrench);
    gl_loops_free(&L);
    loops_of(&wrench, &L);
    expect(gl_inset(&L, 0.2f, &in) == 0 && in.n == L.n && L.n == 6, "every loop of the wrench's layer 1 moved in");
    for (i = 0; i < L.n; i++) {
        before += gl_loop_area(&L.l[i]);
        after += gl_loop_area(&in.l[i]);
    }
    printf("wrench layer 1: %.1f mm2 inside its walls, %.1f mm2 inside 0.2 mm in\n", before, after);
    expect(after < before && after > before * 0.8f, "a little less inside");

    gl_loops_free(&L);
    gl_loops_free(&in);
    gl_loops_free(&gone);
    gs_mesh_free(&box);
    gs_mesh_free(&rod);
    gs_mesh_free(&holed);
    gs_mesh_free(&wrench);
    if (fails) {
        printf("%d failed\n", fails);
        return 1;
    }
    printf("inset ok\n");
    return 0;
}
