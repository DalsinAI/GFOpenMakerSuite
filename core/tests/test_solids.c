/* The bench's other solids in C (gs_model.h): sphere, cone, torus, wedge
 * and slot, and the hole tools (bolt circle, hole row, counterbore). Each closed, its outside out, the size asked for, and the
 * volume near the true one for its number of sides.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gs_model.h"

#include <math.h>
#include <stdio.h>

static int failed;

static void expect(int ok, const char *what)
{
    if (!ok) {
        printf("fail: %s\n", what);
        failed++;
    }
}

/* Signed: positive when the outside is out. */
static float volume(const gs_mesh *m)
{
    double v = 0.0;
    int i;
    for (i = 0; i < m->n; i++) {
        const gs_tri *t = &m->t[i];
        v += (double)t->a.x * ((double)t->b.y * t->c.z - (double)t->b.z * t->c.y) -
             (double)t->a.y * ((double)t->b.x * t->c.z - (double)t->b.z * t->c.x) +
             (double)t->a.z * ((double)t->b.x * t->c.y - (double)t->b.y * t->c.x);
    }
    return (float)(v / 6.0);
}

static int near(float a, float b, float eps) { return fabsf(a - b) <= eps; }

static int box_is(const gs_mesh *m, float x0, float y0, float z0, float x1, float y1, float z1)
{
    const float e = 0.02f;
    return near(m->min.x, x0, e) && near(m->min.y, y0, e) && near(m->min.z, z0, e) && near(m->max.x, x1, e) &&
           near(m->max.y, y1, e) && near(m->max.z, z1, e);
}

static void check(const gs_mesh *m, const char *name, float want, float within)
{
    char what[96];
    float v = volume(m);
    snprintf(what, sizeof what, "%s: closed (no open edges)", name);
    expect(m->n > 0 && gm_open_edges(m) == 0, what);
    snprintf(what, sizeof what, "%s: outside out, volume %.0f near %.0f", name, (double)v, (double)want);
    expect(v > 0.f && fabsf(v - want) <= want * within, what);
}

int main(void)
{
    gs_mesh m = {0};
    const float pi = 3.14159265f;

    expect(gm_sphere(&m, 10, 20, 30, 12, 28, 16) == 0, "a sphere");
    check(&m, "sphere", 4.f / 3.f * pi * 12 * 12 * 12, 0.04f);
    expect(box_is(&m, -2, 8, 18, 22, 32, 42), "sphere: about its centre, poles at z 18 and 42");
    expect(gm_sphere(&m, 0, 0, 0, 5, 2, 1) == 0 && gm_open_edges(&m) == 0 && volume(&m) > 0.f,
           "a sphere of too few sides is made with the fewest");

    expect(gm_cone(&m, 0, 0, 2, 12, 24, 32) == 0, "a cone");
    check(&m, "cone", pi * 12 * 12 * 24 / 3.f, 0.02f);
    expect(near(m.min.z, 2, 0.01f) && near(m.max.z, 26, 0.01f) && near(m.max.x, 12, 0.01f), "cone: base at z 2, point 24 up");

    expect(gm_torus(&m, 0, 0, 5, 12, 4, 28, 16) == 0, "a torus");
    check(&m, "torus", 2.f * pi * pi * 12 * 4 * 4, 0.04f);
    expect(near(m.min.z, 1, 0.01f) && near(m.max.z, 9, 0.01f) && near(m.max.x, 16, 0.01f), "torus: about z 5, 16 out");

    expect(gm_wedge(&m, -14, -10, 0, 28, 20, 14) == 0, "a wedge");
    check(&m, "wedge", 28 * 20 * 14 / 2.f, 0.001f);
    expect(box_is(&m, -14, -10, 0, 14, 10, 14), "wedge: in its box");
    expect(m.t != NULL && m.n == 8, "wedge: two ends, two sides, the base and the slope");

    expect(gm_slot(&m, 0, 0, 0, 38, 10, 8, 20) == 0, "a slot along X");
    check(&m, "slot", (28 * 10 + pi * 25) * 8, 0.01f);
    expect(box_is(&m, 0, 0, 0, 38, 10, 8), "slot: in its box");
    expect(gm_slot(&m, 0, 0, 0, 10, 38, 8, 20) == 0 && box_is(&m, 0, 0, 0, 10, 38, 8) && gm_open_edges(&m) == 0 &&
               near(volume(&m), (28 * 10 + pi * 25) * 8, 25.f),
           "a slot along Y when it is longer that way");
    expect(gm_slot(&m, 0, 0, 0, 10, 10, 8, 20) == 0 && box_is(&m, 0, 0, 0, 10, 10, 8) && gm_open_edges(&m) == 0 &&
               near(volume(&m), pi * 25 * 8, 20.f),
           "a slot as long as wide is a round");

    expect(gm_sphere(&m, 0, 0, 0, 0, 28, 16) == -1 && gm_cone(&m, 0, 0, 0, 5, 0, 16) == -1 &&
               gm_torus(&m, 0, 0, 0, 3, 4, 28, 14) == -1 && gm_wedge(&m, 0, 0, 0, 0, 1, 1) == -1 &&
               gm_slot(&m, 0, 0, 0, 5, 5, 0, 20) == -1,
           "no size, or a tube wider than its ring, is refused");
    {
        /* The hole tools: drills to cut from a part, each closed and outside out. */
        gs_mesh blk = {0}, cut = {0};
        float rod = 28.f / 2.f * 9.f * sinf(2.f * pi / 28.f); /* a 28-sided r 3 drill's end */
        expect(gm_bolt_circle(&m, 50, 40, -4, 28, 6, 30, 4) == 0, "a bolt circle");
        check(&m, "bolt circle", 4 * rod * 28, 0.001f);
        expect(box_is(&m, 32, 22, -4, 68, 58, 24), "bolt circle: 4 on a 30 mm circle about 50, 40");
        expect(gm_bolt_circle(&m, 0, 0, 0, 10, 6, 10, 6) == -1, "bolt circle: holes that overlap are refused");
        expect(gm_hole_row(&m, 50, 40, -4, 28, 6, 10, 3) == 0, "a hole row");
        check(&m, "hole row", 3 * rod * 28, 0.001f);
        expect(box_is(&m, 37, 37, -4, 63, 43, 24), "hole row: 3 along X, 10 apart, about 50, 40");
        expect(gm_hole_row(&m, 0, 0, 0, 10, 6, 5, 3) == -1, "hole row: a pitch under the drill is refused");
        expect(gm_counterbore(&m, 50, 40, -4, 28, 6, 11, 5) == 0, "a counterbore");
        check(&m, "counterbore", (23 * 9.f + 5 * 30.25f) * 16.f * sinf(2.f * pi / 32.f), 0.001f);
        expect(box_is(&m, 44.5f, 34.5f, -4, 55.5f, 45.5f, 24), "counterbore: the bore 11 wide at the top");
        expect(gm_counterbore(&m, 0, 0, 0, 10, 6, 6, 3) == -1 && gm_counterbore(&m, 0, 0, 0, 10, 6, 11, 10) == -1,
               "counterbore: as narrow as the drill, or as deep as it is long, is refused");
        expect(gm_bolt_circle(&m, 0, 0, 0, 10, 0, 30, 4) == -1 && gm_hole_row(&m, 0, 0, 0, 0, 6, 10, 3) == -1 &&
                   gm_bolt_circle(&m, 0, 0, 0, 10, 6, 30, 0) == -1 && gm_hole_row(&m, 0, 0, 0, 10, 6, 10, 0) == -1,
               "no drill, no length or no holes is refused");
        /* Cut from a plate 20 thick: the holes come out of it, closed. */
        gm_cube(&blk, 20, 10, 0, 60, 60, 20);
        gm_counterbore(&m, 50, 40, -4, 24.4f, 6, 11, 5.4f); /* the bore from 15 to just over the top */
        expect(gm_subtract(&cut, &blk, &m) == 0 && gm_open_edges(&cut) == 0 &&
                   near(volume(&cut), 72000.f - (15 * 9.f + 5 * 30.25f) * 16.f * sinf(2.f * pi / 32.f), 2.f),
               "a counterbore cut from a plate takes the drill and the bore");
        gm_bolt_circle(&m, 50, 40, -4, 28, 6, 30, 4);
        expect(gm_subtract(&cut, &blk, &m) == 0 && gm_open_edges(&cut) == 0 &&
                   near(volume(&cut), 72000.f - 4 * rod * 20, 2.f),
               "a bolt circle cut from a plate takes four holes");
        gs_mesh_free(&blk);
        gs_mesh_free(&cut);
    }
    {
        /* A part's facts, for Measure: volume, surface, size, triangles. */
        gs_facts f;
        gs_mesh c = {0};
        expect(gm_cube(&c, 100, 100, 8, 20, 10, 5) == 0 && gm_facts(&c, &f) == 0, "a block's facts");
        expect(near(f.volume, 1000.f, 0.01f) && near(f.area, 700.f, 0.01f) && near(f.sx, 20.f, 1e-4f) &&
                   near(f.sy, 10.f, 1e-4f) && near(f.sz, 5.f, 1e-4f) && f.tris == 12,
               "block: 1000 mm3, 700 mm2, 20 x 10 x 5, 12 triangles, far from the origin");
        expect(gm_cylinder(&c, 0, 0, 0, 5, 10, 32) == 0 && gm_facts(&c, &f) == 0 &&
                   near(f.volume, 16.f * 25.f * sinf(2.f * pi / 32.f) * 10.f, 0.05f),
               "cylinder: its polygon's volume");
        gs_mesh_free(&c);
        expect(gm_facts(&c, &f) == 0 && f.volume == 0.f && f.area == 0.f && f.sx == 0.f && f.tris == 0,
               "no part: all nought");
        expect(gm_facts(NULL, &f) == -1 && gm_facts(&c, NULL) == -1, "no mesh or no facts: refused");
    }
    gs_mesh_free(&m);
    if (failed) {
        printf("%d failed\n", failed);
        return 1;
    }
    printf("solids ok\n");
    return 0;
}
