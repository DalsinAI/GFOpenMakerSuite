/* The bench's other solids in C (gs_model.h): sphere, cone, torus, wedge
 * and slot. Each closed, its outside out, the size asked for, and the
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
    gs_mesh_free(&m);
    if (failed) {
        printf("%d failed\n", failed);
        return 1;
    }
    printf("solids ok\n");
    return 0;
}
