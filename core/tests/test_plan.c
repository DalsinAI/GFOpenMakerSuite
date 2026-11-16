/* A layer for a printer: the brim on the first, walls on all, skin at the
 * bottom and top, infill between. MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gs_model.h"
#include "gs_plan.h"

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

/* Layer n of m alone: mm of each kind, and whether every move is at z. */
static void layer(const gs_mesh *m, const gp_opts *o, int n, float mm[GT_KINDS], float z, int *flat)
{
    gt_path p;
    int k, i;
    gt_init(&p);
    expect(gp_layer(&p, m, o, n) == 0 && p.nlayers == 1 && p.layer[0] == 0, "a layer");
    for (k = 0; k < GT_KINDS; k++) mm[k] = gt_length(&p, k);
    *flat = p.n > 0;
    for (i = 0; i < p.n; i++)
        if (fabsf(p.m[i].z - z) > 1e-4f) *flat = 0;
    gt_free(&p);
}

int main(void)
{
    gs_mesh box, rod, holed;
    gp_opts o;
    gt_path two;
    float mm[GT_KINDS];
    int flat;
    memset(&box, 0, sizeof box);
    memset(&rod, 0, sizeof rod);
    memset(&holed, 0, sizeof holed);
    expect(gm_cube(&box, 0, 0, 0, 20, 20, 20) == 0 && gm_cylinder(&rod, 10, 10, -1, 6, 22, 32) == 0 &&
               gm_subtract(&holed, &box, &rod) == 0,
           "a cube with a hole");
    gp_defaults(&o);
    expect(o.layer_h == 0.2f && o.line_w == 0.4f && o.walls == 2 && o.skins == 3 && o.infill == 20.f && o.brim == 3,
           "the defaults");
    o.brim = 2;
    expect(gp_layers(&holed, &o) == 100, "100 layers");

    layer(&holed, &o, 1, mm, 0.2f, &flat);
    printf("layer 1: brim %.1f, outer %.1f, inner %.1f, skin %.1f, infill %.1f mm\n", mm[GT_BRIM], mm[GT_OUTER],
           mm[GT_INNER], mm[GT_SKIN], mm[GT_INFILL]);
    expect(flat, "all at 0.2");
    expect(fabsf(mm[GT_BRIM] - 4 * 20.4f - 4 * 21.2f) < 0.01f, "two brim loops round the outside, none in the hole");
    expect(mm[GT_OUTER] > 4 * 19.6f + 37.f && mm[GT_OUTER] < 4 * 19.6f + 40.f, "the outer wall round the outside and the hole");
    expect(mm[GT_INNER] > 4 * 18.8f + 39.f && mm[GT_INNER] < 4 * 18.8f + 42.f, "one inner wall inside each");
    expect(mm[GT_SKIN] > 400.f && mm[GT_INFILL] == 0.f, "the bottom is solid");

    layer(&holed, &o, 50, mm, 10.f, &flat);
    expect(flat && mm[GT_BRIM] == 0.f && mm[GT_SKIN] == 0.f && mm[GT_INFILL] > 50.f && mm[GT_INFILL] < 120.f,
           "the middle: a fifth of the inside, no skin, no brim");
    layer(&holed, &o, 98, mm, 19.6f, &flat);
    expect(flat && mm[GT_SKIN] > 400.f, "three from the top is solid");
    layer(&holed, &o, 97, mm, 19.4f, &flat);
    expect(mm[GT_SKIN] == 0.f, "four from the top is not");

    gt_init(&two);
    expect(gp_layer(&two, &holed, &o, 50) == 0 && gp_layer(&two, &holed, &o, 51) == 0 && two.nlayers == 2,
           "two layers in one path");
    expect(two.m[two.layer[1] - 1].y == two.m[two.layer[1] - 2].y, "one layer's infill along x");
    expect(two.m[two.n - 1].x == two.m[two.n - 2].x, "and the next one's along y");
    gt_free(&two);

    gs_mesh_free(&box);
    gs_mesh_free(&rod);
    gs_mesh_free(&holed);
    if (fails) {
        printf("%d failed\n", fails);
        return 1;
    }
    printf("plan ok\n");
    return 0;
}
