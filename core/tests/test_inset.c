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

/* How many of L's loops turn each way, and their areas added up. */
static void count(const gl_loops *L, int *outs, int *holes, float *out_area, float *hole_area)
{
    int i;
    *outs = *holes = 0;
    *out_area = *hole_area = 0.f;
    for (i = 0; i < L->n; i++) {
        float a = gl_loop_area(&L->l[i]);
        if (a > 0.f) {
            (*outs)++;
            *out_area += a;
        } else {
            (*holes)++;
            *hole_area -= a;
        }
    }
}

/* Walls that meet: a shape that pinches apart, holes that merge, a hole
 * that opens to the outside, and the union under them. */
static void meeting(void)
{
    /* Two 10 mm squares joined by a neck 1 mm wide and 4 long. */
    static float bell[] = {0, 0, 10, 0, 10, 4.5f, 14, 4.5f, 14, 0, 24, 0, 24, 10, 14, 10, 14, 5.5f, 10, 5.5f, 10, 10, 0, 10};
    /* A 30 by 10 plate with two 4 mm square holes 2 mm apart (clockwise). */
    static float plate[] = {0, 0, 30, 0, 30, 10, 0, 10};
    static float hole_a[] = {9, 3, 9, 7, 13, 7, 13, 3}, hole_b[] = {15, 3, 15, 7, 19, 7, 19, 3};
    /* A 20 mm square with a 4 mm hole 1 mm in from its right side. */
    static float sq[] = {0, 0, 20, 0, 20, 20, 0, 20}, near_edge[] = {15, 8, 15, 12, 19, 12, 19, 8};
    /* Two squares overlapping, both anticlockwise: one outside. */
    static float s1[] = {0, 0, 10, 0, 10, 10, 0, 10}, s2[] = {5, 5, 15, 5, 15, 15, 5, 15};
    gl_loop one[] = {{bell, 12, 1}}, two[] = {{plate, 4, 1}, {hole_a, 4, 1}, {hole_b, 4, 1}};
    gl_loop three[] = {{sq, 4, 1}, {near_edge, 4, 1}}, four[] = {{s1, 4, 1}, {s2, 4, 1}};
    gl_loops in1 = {one, 1, 0}, in2 = {two, 3, 0}, in3 = {three, 2, 0}, in4 = {four, 2, 0}, o;
    int outs, holes;
    float oa, ha;
    memset(&o, 0, sizeof o);

    expect(gl_inset(&in1, 0.2f, &o) == 0 && o.n == 1, "0.2 in, the neck still joins the two squares");
    count(&o, &outs, &holes, &oa, &ha);
    expect(outs == 1 && near(oa, 9.6f * 9.6f * 2.f + 4.4f * 0.6f, 0.01f), "one outside round both and the neck");
    expect(gl_inset(&in1, 0.6f, &o) == 0 && o.n == 2, "0.6 in, the 1 mm neck pinches apart: two loops");
    count(&o, &outs, &holes, &oa, &ha);
    expect(outs == 2 && holes == 0 && near(oa, 8.8f * 8.8f * 2.f, 0.01f), "two 8.8 mm squares, both anticlockwise");

    expect(gl_inset(&in2, 0.8f, &o) == 0 && o.n == 3, "0.8 in, the holes 2 mm apart stay apart");
    expect(gl_inset(&in2, 1.2f, &o) == 0 && o.n == 2, "1.2 in, they merge: an outside and one hole");
    count(&o, &outs, &holes, &oa, &ha);
    expect(outs == 1 && holes == 1, "the outside anticlockwise, the merged hole clockwise");
    expect(near(oa, 27.6f * 7.6f, 0.01f), "the outside is 27.6 by 7.6");
    expect(near(ha, 6.4f * 6.4f * 2.f - 0.4f * 6.4f, 0.01f), "the hole is the two grown holes' union");

    expect(gl_inset(&in3, 0.8f, &o) == 0 && o.n == 1, "0.8 in, the hole 1 mm from the side opens to the outside");
    count(&o, &outs, &holes, &oa, &ha);
    expect(outs == 1 && near(oa, 18.4f * 18.4f - 5.f * 5.6f, 0.01f), "one notched outside, no hole");

    expect(gl_union(&in4, &o) == 0 && o.n == 1, "two overlapping squares are one outside");
    count(&o, &outs, &holes, &oa, &ha);
    expect(outs == 1 && near(oa, 175.f, 0.01f), "of 175 mm2");
    expect(gl_inset(&in4, -0.5f, &o) == 0 && o.n == 1, "their brims merge too");
    {
        /* Corners touching: two loops still. An island in a hole: kept. */
        static float t1[] = {0, 0, 5, 0, 5, 5, 0, 5}, t2[] = {5, 5, 10, 5, 10, 10, 5, 10};
        static float big[] = {0, 0, 20, 0, 20, 20, 0, 20}, hole[] = {5, 5, 5, 15, 15, 15, 15, 5};
        static float isle[] = {8, 8, 12, 8, 12, 12, 8, 12};
        gl_loop touch[] = {{t1, 4, 1}, {t2, 4, 1}}, nest[] = {{big, 4, 1}, {hole, 4, 1}, {isle, 4, 1}};
        gl_loops in5 = {touch, 2, 0}, in6 = {nest, 3, 0};
        expect(gl_union(&in5, &o) == 0 && o.n == 2, "squares touching at a corner stay two loops");
        count(&o, &outs, &holes, &oa, &ha);
        expect(outs == 2 && near(oa, 50.f, 1e-3f), "both anticlockwise, 25 mm2 each");
        expect(gl_union(&in6, &o) == 0 && o.n == 3, "an outside, its hole, and an island in the hole");
        count(&o, &outs, &holes, &oa, &ha);
        expect(outs == 2 && holes == 1 && near(oa, 416.f, 1e-3f) && near(ha, 100.f, 1e-3f), "each where it was");
    }
    gl_loops_free(&o);
}

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
    expect(gl_inset(&L, 10.5f, &gone) == 0 && gone.n == 0,
           "10.5 closes the outside up: a hole with nothing round it is no material, so nothing is left");

    gl_loops_free(&gone);
    expect(gl_inset(&L, -0.2f, &gone) == 0 && gone.n == 2, "moved out by 0.2, for a brim");
    for (i = 0; i < gone.n; i++) {
        float s = gl_loop_area(&gone.l[i]), h = 6.f * cosf(3.14159265f / 32.f) - 0.2f;
        if (s > 0.f)
            expect(near(s, 20.4f * 20.4f, 0.01f), "the outside grows to 20.4 square");
        else
            expect(near(-s, 32.f * h * h * tanf(3.14159265f / 32.f), 0.05f), "the hole shrinks 0.2 all round");
    }
    expect(gl_inset(&L, -6.5f, &gone) == 0 && gone.n == 1 && gl_loop_area(&gone.l[0]) > 400.f,
           "6.5 out closes the hole up, and only the outside is left");

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

    meeting();
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
