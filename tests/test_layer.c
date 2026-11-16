/* The Layers group: walls at one height, a cube, a cube with a hole, and
 * the NASA wrench from the shelf. MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gs_layer.h"
#include "gs_model.h"

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
    expect(near(w.z, 0.2f, 1e-4f), "layer 1 cuts at 0.2");
    expect(near(w.length, 40.f, 0.01f), "a 10 mm square is 40 mm of wall");
    expect(w.n >= 4, "four sides at least");
    expect(gl_cut(&box, 0.2f, 999, &w) == 0 && near(w.z, 10.f, 1e-4f) && near(w.length, 40.f, 0.01f), "past the top is the last layer, at the top");
    expect(gl_cut(&empty, 0.2f, 1, &w) == 0 && w.n == 0 && w.length == 0.f, "nothing cuts to nothing");

    expect(gm_cube(&box, 0.f, 0.f, 0.f, 20.f, 20.f, 20.f) == 0, "20 mm cube");
    expect(gm_cylinder(&rod, 10.f, 10.f, -1.f, 6.f, 22.f, 32) == 0, "rod");
    expect(gm_subtract(&holed, &box, &rod) == 0, "hole");
    expect(gl_cut(&holed, 0.5f, 20, &w) == 0, "cut the holed cube");
    expect(near(w.length, 80.f + 64.f * 6.f * sinf(3.14159265f / 32.f), 0.05f), "outside and hole are both wall");

    expect(gs_stl_load(&wrench, "preview/library/nasa-wrench.stl", err, (int)sizeof err) == 0, "wrench loads");
    gs_mesh_repair(&wrench);
    expect(gl_layers(&wrench, 0.2f) == 130, "the wrench is 130 layers at 0.2");
    expect(gl_cut(&wrench, 0.2f, 1, &w) == 0, "cut the wrench");
    printf("wrench layer 1 of %d: %d pieces, %.1f mm of wall\n", gl_layers(&wrench, 0.2f), w.n, w.length);
    expect(near(w.z, 0.2f, 1e-4f), "wrench layer 1 cuts at 0.2");
    expect(near(w.length, 443.2f, 0.05f), "about 443 mm of wall on layer 1");

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
