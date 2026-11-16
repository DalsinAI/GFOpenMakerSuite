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

    {
        gs_mesh wrench;
        gt_path all;
        float lo[3], hi[3];
        char err[96];
        int empty = 0, i;
        expect(gs_stl_load(&wrench, "core/tests/data/nasa-wrench.stl", err, (int)sizeof err) == 0, "the wrench");
        gs_mesh_repair(&wrench);
        gp_place(&wrench, 220.f, 220.f);
        expect(wrench.min.z == 0.f && fabsf(wrench.min.x + wrench.max.x - 220.f) < 1e-3f &&
                   fabsf(wrench.min.y + wrench.max.y - 220.f) < 1e-3f,
               "placed on the bed, in the middle");
        gt_init(&all);
        expect(gp_part(&all, &wrench, &o) == 0 && all.nlayers == 130, "130 layers");
        for (i = 0; i < all.nlayers; i++)
            if ((i + 1 < all.nlayers ? all.layer[i + 1] : all.n) - all.layer[i] < 4) empty++;
        expect(!empty, "something on every layer");
        expect(gt_length(&all, GT_BRIM) > 0.f && gt_length(&all, GT_OUTER) > 0.f && gt_length(&all, GT_INNER) > 0.f &&
                   gt_length(&all, GT_SKIN) > 0.f && gt_length(&all, GT_INFILL) > 0.f,
               "brim, walls, skin and infill");
        expect(gt_bounds(&all, lo, hi) == 0 && lo[0] > 0.f && hi[0] < 220.f && fabsf(lo[2] - 0.2f) < 1e-4f &&
                   fabsf(hi[2] - 26.f) < 1e-3f,
               "on the bed, from 0.2 up to 26");
        printf("wrench: %d moves; walls %.0f, skin %.0f, infill %.0f mm\n", all.n,
               gt_length(&all, GT_OUTER) + gt_length(&all, GT_INNER), gt_length(&all, GT_SKIN), gt_length(&all, GT_INFILL));
        gt_free(&all);
        gs_mesh_free(&wrench);
    }

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
