/* A layer for a printer: the brim on the first, walls on all, skin at the
 * bottom and top, infill between. MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gs_model.h"
#include "gs_plan.h"

#include <math.h>
#include <stddef.h>
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

    {
        /* The more: top and bottom apart, a grid, a skirt and a gap. */
        gp_more x, part;
        gt_path g;
        int i, along_x = 0, along_y = 0;
        gp_more_defaults(&x);
        expect(x.size == (int)sizeof x && x.pattern == GP_LINES && x.top == -1 && x.bottom == -1 && x.skirt == 0 &&
                   x.gap == 0.f,
               "the more's defaults: as gp_opts alone");
        layer(&holed, &o, 1, mm, 0.2f, &flat);
        {
            float plain[GT_KINDS];
            gt_path q;
            int k;
            memcpy(plain, mm, sizeof plain);
            gt_init(&q);
            expect(gp_layer_more(&q, &holed, &o, &x, 1) == 0, "a layer with the more's defaults");
            for (k = 0; k < GT_KINDS; k++) expect(gt_length(&q, k) == plain[k], "the same as without");
            gt_free(&q);
            gt_init(&q);
            expect(gp_layer_more(&q, &holed, &o, NULL, 1) == 0 && gt_length(&q, GT_SKIN) == plain[GT_SKIN], "NULL: the same");
            gt_free(&q);
        }
        x.top = 1;
        x.bottom = 5;
        gt_init(&g);
        expect(gp_layer_more(&g, &holed, &o, &x, 5) == 0 && gt_length(&g, GT_SKIN) > 400.f, "five at the bottom: layer 5 solid");
        gt_free(&g);
        gt_init(&g);
        expect(gp_layer_more(&g, &holed, &o, &x, 6) == 0 && gt_length(&g, GT_SKIN) == 0.f, "but not 6");
        gt_free(&g);
        gt_init(&g);
        expect(gp_layer_more(&g, &holed, &o, &x, 99) == 0 && gt_length(&g, GT_SKIN) == 0.f, "one at the top: 99 not solid");
        gt_free(&g);
        gt_init(&g);
        expect(gp_layer_more(&g, &holed, &o, &x, 100) == 0 && gt_length(&g, GT_SKIN) > 400.f, "100 is");
        gt_free(&g);

        /* A grid: the sparse lines go both ways on every layer, half as
         * close, so about as much as lines. */
        gp_more_defaults(&x);
        x.pattern = GP_GRID;
        gt_init(&g);
        expect(gp_layer_more(&g, &holed, &o, &x, 50) == 0, "a grid layer");
        for (i = 1; i < g.n; i++)
            if (g.m[i].kind == GT_INFILL) {
                if (g.m[i].y == g.m[i - 1].y) along_x++;
                if (g.m[i].x == g.m[i - 1].x) along_y++;
            }
        layer(&holed, &o, 50, mm, 10.f, &flat);
        expect(along_x > 3 && along_y > 3, "infill along x and along y on the one layer");
        expect(gt_length(&g, GT_INFILL) > 0.7f * mm[GT_INFILL] && gt_length(&g, GT_INFILL) < 1.3f * mm[GT_INFILL],
               "about as much infill as lines");
        gt_free(&g);
        gt_init(&g);
        expect(gp_layer_more(&g, &holed, &o, &x, 1) == 0 && gt_length(&g, GT_INFILL) == 0.f,
               "a grid's bottom is only skin");
        {
            gt_path q;
            float lines_skin;
            gt_init(&q);
            gp_layer(&q, &holed, &o, 1);
            lines_skin = gt_length(&q, GT_SKIN);
            gt_free(&q);
            expect(fabsf(gt_length(&g, GT_SKIN) - lines_skin) < 1e-3f, "the same skin as lines");
        }
        gt_free(&g);

        /* A skirt of two, 3 mm out, and the brim set 1 mm out. */
        gp_more_defaults(&x);
        x.skirt = 2;
        x.gap = 3.f;
        o.brim = 0;
        gt_init(&g);
        expect(gp_layer_more(&g, &holed, &o, &x, 1) == 0, "a first layer with a skirt");
        expect(fabsf(gt_length(&g, GT_BRIM) - 4 * (20.f + 2 * 3.2f) - 4 * (20.f + 2 * 3.6f)) < 0.05f,
               "two loops round the outside at 3.2 and 3.6 mm, none in the hole");
        gt_free(&g);
        gt_init(&g);
        expect(gp_layer_more(&g, &holed, &o, &x, 2) == 0 && gt_length(&g, GT_BRIM) == 0.f, "only on the first layer");
        gt_free(&g);
        x.skirt = 0;
        x.gap = 1.f;
        o.brim = 1;
        gt_init(&g);
        expect(gp_layer_more(&g, &holed, &o, &x, 1) == 0 && fabsf(gt_length(&g, GT_BRIM) - 4 * (20.f + 2 * 1.2f)) < 0.05f,
               "a brim 1 mm out");
        gt_free(&g);
        o.brim = 2;

        /* An older program's smaller gp_more: the rest as the defaults. */
        gp_more_defaults(&part);
        part.size = (int)offsetof(gp_more, skirt);
        part.pattern = GP_GRID;
        part.skirt = 9;
        part.gap = 99.f;
        gt_init(&g);
        expect(gp_layer_more(&g, &holed, &o, &part, 1) == 0 &&
                   fabsf(gt_length(&g, GT_BRIM) - 4 * 20.4f - 4 * 21.2f) < 0.01f,
               "a gp_more cut short at its size: no skirt, no gap");
        gt_free(&g);
    }

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
        {
            /* gp_part_more on the wrench: a grid and a skirt. */
            gp_more x;
            gp_more_defaults(&x);
            x.pattern = GP_GRID;
            x.skirt = 1;
            x.gap = 3.f;
            expect(gp_part_more(&all, &wrench, &o, &x) == 0 && all.nlayers == 130 && gt_length(&all, GT_INFILL) > 0.f,
                   "gp_part_more: a grid and a skirt");
        }
        gt_free(&all);
        gs_mesh_free(&wrench);
    }

    {
        /* A 20 mm base, 2 mm tall, and a slab on it from x 10 to 30: 10 mm of
         * that slab hangs past the base. Support is one wall under the hang. */
        gs_mesh base, slab, ledge;
        gp_opts so;
        gp_more x, old;
        gt_path p;
        int i, past;
        float len;
        memset(&base, 0, sizeof base);
        memset(&slab, 0, sizeof slab);
        memset(&ledge, 0, sizeof ledge);
        expect(gm_cube(&base, 0, 0, 0, 20, 20, 2) == 0 && gm_cube(&slab, 10, 5, 2, 20, 10, 2) == 0, "the base and the slab");
        ledge = base;
        base.t = NULL;
        ledge.t = (gs_tri *)realloc(ledge.t, (size_t)(ledge.n + slab.n) * sizeof *ledge.t);
        expect(ledge.t != NULL, "room for both");
        if (ledge.t) {
            memcpy(ledge.t + ledge.n, slab.t, (size_t)slab.n * sizeof *slab.t);
            ledge.n += slab.n;
            ledge.max.x = 30.f;
            ledge.max.z = 4.f;
        }
        gp_defaults(&so);
        so.layer_h = 0.4f;
        so.brim = 0;
        so.walls = 1;
        so.skins = 0;
        so.infill = 0.f;
        gp_more_defaults(&x);
        x.support = 1;
        gt_init(&p);
        expect(gp_layer_more(&p, &ledge, &so, &x, 1) == 0, "the first layer, with support");
        len = gt_length(&p, GT_BRIM);
        past = 1;
        for (i = 0; i < p.n; i++)
            if (p.m[i].kind == GT_BRIM) {
                if (p.m[i].x < 20.f || p.m[i].x > 30.f || p.m[i].y < 4.f || p.m[i].y > 16.f) past = 0;
            }
        expect(len > 30.f && len < 50.f && past, "one wall under the hang, not under the base");
        printf("support: %.1f mm under the ledge\n", len);
        gt_free(&p);
        gt_init(&p);
        expect(gp_layer_more(&p, &ledge, &so, &x, 5) == 0 && gt_length(&p, GT_BRIM) > 30.f, "still there on the last base layer");
        gt_free(&p);
        gt_init(&p);
        expect(gp_layer_more(&p, &ledge, &so, &x, 6) == 0 && gt_length(&p, GT_BRIM) == 0.f, "the slab holds itself: no wall in it");
        gt_free(&p);
        x.support = 0;
        gt_init(&p);
        expect(gp_layer_more(&p, &ledge, &so, &x, 1) == 0 && gt_length(&p, GT_BRIM) == 0.f, "support left off: no wall");
        gt_free(&p);
        gp_more_defaults(&old);
        old.size = (int)offsetof(gp_more, support);
        old.support = 1;
        gt_init(&p);
        expect(gp_layer_more(&p, &ledge, &so, &old, 1) == 0 && gt_length(&p, GT_BRIM) == 0.f,
               "a gp_more cut short of support: none");
        gt_free(&p);
        x.support = 1;
        gt_init(&p);
        expect(gp_layer_more(&p, &box, &so, &x, 10) == 0 && gt_length(&p, GT_BRIM) == 0.f, "a cube overhangs nothing");
        gt_free(&p);
        gs_mesh_free(&ledge);
        gs_mesh_free(&slab);
        gs_mesh_free(&base);
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
