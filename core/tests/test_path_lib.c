/* openmakepath.library's calls, run on the host as a program makes them:
 * through the jump table's slot at each bias to the library's side and
 * back to the C function. Every answer must be the C function's own.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include <proto/openmakepath.h>

#include "../library/openmakepath_vectors.h"
#include "gs_model.h"

#include <stdio.h>
#include <string.h>

static int fails, calls;

static void expect(int ok, const char *what)
{
    if (!ok) {
        printf("fail: %s\n", what);
        fails++;
    }
}

static const APTR table[] = {NULL, NULL, NULL, NULL, OPENMAKEPATH_VECTORS};
static struct Library *fake = (struct Library *)&fake;
struct Library *OpenMakePathBase;

APTR om_host_vector(struct Library *base, int bias)
{
    expect(base == fake && bias % 6 == 0 && bias >= 30 && bias <= OPENMAKEPATH_LAST_BIAS, "a call in the table");
    calls++;
    return table[bias / 6 - 1];
}

/* Two toolpaths, move for move the same (field by field: a move's padding
 * is never written). */
static int same_path(const gt_path *a, const gt_path *b)
{
    int i;
    if (a->n != b->n)
        return 0;
    for (i = 0; i < a->n; i++)
        if (a->m[i].x != b->m[i].x || a->m[i].y != b->m[i].y || a->m[i].z != b->m[i].z || a->m[i].w != b->m[i].w ||
            a->m[i].kind != b->m[i].kind)
            return 0;
    return a->nlayers == b->nlayers &&
           (!a->nlayers || memcmp(a->layer, b->layer, (size_t)a->nlayers * sizeof *a->layer) == 0);
}

/* Two sets of loops, point for point the same. */
static int same_loops(const gl_loops *a, const gl_loops *b)
{
    int i;
    if (a->n != b->n)
        return 0;
    for (i = 0; i < a->n; i++)
        if (a->l[i].n != b->l[i].n || memcmp(a->l[i].xy, b->l[i].xy, (size_t)a->l[i].n * 2 * sizeof(float)))
            return 0;
    return 1;
}

/* The wall loops inset, walked as a toolpath and filled: GL_Inset and the
 * GT_ calls against the C functions. The calls it makes. */
static int toolpath_case(const gl_loops *walls)
{
    gl_loops ia, ib;
    gt_path pa, pb, joined;
    const gl_loops *solid[2];
    float mina[3], maxa[3], minb[3], maxb[3];
    int i;
    memset(&ia, 0, sizeof ia);
    memset(&ib, 0, sizeof ib);
    expect(gl_inset(walls, 0.6f, &ia) == 0 && GL_Inset(walls, 0.6f, &ib) == 0 && ia.n == 2 && same_loops(&ia, &ib),
           "GL_Inset gives the same loops");
    {
        gl_loops ta, tb;
        memset(&ta, 0, sizeof ta);
        memset(&tb, 0, sizeof tb);
        expect(gl_tabs(&walls->l[0], 4, 0.5f, &ta) == 0 && GL_Tabs(&walls->l[0], 4, 0.5f, &tb) == 0 &&
                   ta.n == 4 && same_loops(&ta, &tb),
               "GL_Tabs (1.4) gives the same cuts");
        gl_loops_free(&ta);
        gl_loops_free(&tb);
    }
    gt_init(&pa);
    GT_Init(&pb);
    expect(pb.n == 0 && pb.m == NULL && pb.nlayers == 0, "GT_Init");
    solid[0] = walls;
    solid[1] = NULL;
    expect(gt_layer(&pa) == 0 && GT_Layer(&pb) == 0, "GT_Layer");
    for (i = 0; i < ia.n; i++)
        expect(gt_loop(&pa, GT_OUTER, &ia.l[i], 10.f, 0.4f) == 0 && GT_Loop(&pb, GT_OUTER, &ib.l[i], 10.f, 0.4f) == 0,
               "GT_Loop");
    expect(gt_add(&pa, GT_TRAVEL, 1.5f, 2.5f, 10.f, 0) == 0 && GT_Add(&pb, GT_TRAVEL, 1.5f, 2.5f, 10.f, 0) == 0,
           "GT_Add, four floats");
    expect(gt_fill(&pa, &ia, solid, 2, 10.f, 0.4f, 5, 1) == 0 && GT_Fill(&pb, &ib, solid, 2, 10.f, 0.4f, 5, 1) == 0,
           "GT_Fill, eight arguments");
    expect(same_path(&pa, &pb) && pb.n > 50, "the same moves");
    expect(gt_fill_grid(&pa, &ia, solid, 2, 10.f, 0.4f, 5, 1) == 0 && GT_FillGrid(&pb, &ib, solid, 2, 10.f, 0.4f, 5, 1) == 0 &&
               same_path(&pa, &pb),
           "GT_FillGrid, eight arguments (1.3)");
    expect(GT_Length(&pb, GT_SKIN) == gt_length(&pa, GT_SKIN) && GT_Length(&pb, GT_SKIN) > 0, "GT_Length");
    expect(gt_bounds(&pa, mina, maxa) == 0 && GT_Bounds(&pb, minb, maxb) == 0 && !memcmp(mina, minb, sizeof mina) &&
               !memcmp(maxa, maxb, sizeof maxa),
           "GT_Bounds");
    GT_Init(&joined);
    expect(GT_Append(&joined, &pb) == 0 && GT_Append(&joined, &pb) == 0 && joined.n == 2 * pb.n &&
               joined.nlayers == 2,
           "GT_Append");
    printf("through the library: %d moves, %.1f mm of skin\n", pb.n, (double)GT_Length(&pb, GT_SKIN));
    GT_Free(&joined);
    GT_Free(&pb);
    expect(pb.n == 0 && pb.m == NULL && pb.layer == NULL, "GT_Free");
    gt_free(&pa);
    i = ia.n;
    gl_loops_free(&ia);
    gl_loops_free(&ib);
    /* GL_Inset, GL_Tabs, GT_Init, GT_Layer, a GT_Loop a loop, GT_Add, GT_Fill, three
     * GT_Length, GT_Bounds, GT_Init, two GT_Append, two GT_Free, GT_FillGrid */
    return 16 + i;
}

/* The holed cube placed on a bed and sliced: the GP_ calls against the C
 * functions. The calls it makes. */
static int plan_case(const gs_mesh *part)
{
    gs_mesh ca, cb;
    gp_opts oa, ob;
    gt_path pa, pb;
    int n;
    memset(&ca, 0, sizeof ca);
    memset(&cb, 0, sizeof cb);
    expect(gm_cube(&ca, 0, 0, 0, 20, 20, 20) == 0 && gm_cube(&cb, 0, 0, 0, 20, 20, 20) == 0, "two cubes");
    gp_place(&ca, 220.f, 180.f);
    GP_Place(&cb, 220.f, 180.f);
    expect(ca.n == cb.n && !memcmp(ca.t, cb.t, (size_t)ca.n * sizeof *ca.t) && cb.min.z == 0 && cb.min.x == 100,
           "GP_Place, two floats");
    gp_defaults(&oa);
    memset(&ob, 0xff, sizeof ob);
    GP_Defaults(&ob);
    expect(!memcmp(&oa, &ob, sizeof oa), "GP_Defaults");
    oa.layer_h = ob.layer_h = 0.5f;
    n = GP_Layers(part, &ob);
    expect(n == gp_layers(part, &oa) && n == 40, "GP_Layers");
    gt_init(&pa);
    gt_init(&pb);
    expect(gp_layer(&pa, part, &oa, 2) == 0 && GP_Layer(&pb, part, &ob, 2) == 0 && same_path(&pa, &pb) &&
               pb.nlayers == 1,
           "GP_Layer");
    expect(gp_part(&pa, part, &oa) == 0 && GP_Part(&pb, part, &ob) == 0 && same_path(&pa, &pb) && pb.nlayers == n,
           "GP_Part");
    {
        /* 1.3: the more, a grid and a skirt. */
        gp_more xa, xb;
        gp_more_defaults(&xa);
        memset(&xb, 0xff, sizeof xb);
        GP_MoreDefaults(&xb);
        expect(!memcmp(&xa, &xb, sizeof xa), "GP_MoreDefaults");
        xa.pattern = xb.pattern = GP_GRID;
        xa.skirt = xb.skirt = 2;
        xa.gap = xb.gap = 3.f;
        xa.top = xb.top = 1;
        expect(gp_layer_more(&pa, part, &oa, &xa, 1) == 0 && GP_LayerMore(&pb, part, &ob, &xb, 1) == 0 &&
                   same_path(&pa, &pb) && GT_Length(&pb, GT_BRIM) > 0.f,
               "GP_LayerMore, five arguments");
        expect(gp_part_more(&pa, part, &oa, &xa) == 0 && GP_PartMore(&pb, part, &ob, &xb) == 0 && same_path(&pa, &pb) &&
                   pb.nlayers == n,
               "GP_PartMore");
    }
    printf("through the library: %d layers, %d moves\n", pb.nlayers, pb.n);
    gt_free(&pa);
    gt_free(&pb);
    gs_mesh_free(&ca);
    gs_mesh_free(&cb);
    return 5 + 4; /* and the three _More and GT_Length */
}

/* Two sets of walls, piece for piece the same. */
static int same_walls(const gl_walls *a, const gl_walls *b)
{
    return a->n == b->n && a->z == b->z && a->length == b->length &&
           (!a->n || memcmp(a->s, b->s, (size_t)a->n * sizeof *a->s) == 0);
}

int main(void)
{
    gs_mesh box, rod, holed;
    gl_walls a, b;
    gl_loops la, lb;
    char sa[96], sb[96];
    int i, same = 1, toolpath_calls = 0;
    OpenMakePathBase = fake;
    memset(&box, 0, sizeof box);
    memset(&rod, 0, sizeof rod);
    memset(&holed, 0, sizeof holed);
    memset(&a, 0, sizeof a);
    memset(&b, 0, sizeof b);
    memset(&la, 0, sizeof la);
    memset(&lb, 0, sizeof lb);
    expect(sizeof table / sizeof table[0] == OPENMAKEPATH_LAST_BIAS / 6, "every call has its slot");
    expect(gm_cube(&box, 0, 0, 0, 20, 20, 20) == 0 && gm_cylinder(&rod, 10, 10, -1, 6, 22, 32) == 0 &&
               gm_subtract(&holed, &box, &rod) == 0,
           "the holed cube");

    expect(GL_Height(0.01f) == gl_height(0.01f) && GL_Height(0.33f) == 0.33f && GL_Height(9.f) == 0.8f,
           "GL_Height, a float each way");
    expect(GL_Layers(&holed, 0.2f) == gl_layers(&holed, 0.2f) && GL_Layers(&holed, 0.2f) == 100, "GL_Layers");
    expect(GL_LayerZ(&holed, 0.2f, 7) == gl_layer_z(&holed, 0.2f, 7), "GL_LayerZ");

    expect(gl_cut(&holed, 0.5f, 20, &a) == 0 && GL_Cut(&holed, 0.5f, 20, &b) == 0 && same_walls(&a, &b),
           "GL_Cut gives the same walls");
    gl_status(sa, (int)sizeof sa, &holed, 0.5f, 20, &a);
    GL_Status(sb, (int)sizeof sb, &holed, 0.5f, 20, &b);
    expect(strcmp(sa, sb) == 0, "GL_Status, six arguments");
    printf("through the library: %s\n", sb);

    expect(gl_weld(&a, 1e-3f, &la) == 0 && GL_Weld(&b, 1e-3f, &lb) == 0 && la.n == lb.n && la.n == 2 && la.open == lb.open,
           "GL_Weld gives the same loops");
    for (i = 0; i < la.n && i < lb.n; i++) {
        if (la.l[i].n != lb.l[i].n || memcmp(la.l[i].xy, lb.l[i].xy, (size_t)la.l[i].n * 2 * sizeof(float)))
            same = 0;
        if (GL_LoopArea(&lb.l[i]) != gl_loop_area(&la.l[i]) || GL_LoopLength(&lb.l[i]) != gl_loop_length(&la.l[i]))
            same = 0;
    }
    expect(same, "point for point, and GL_LoopArea and GL_LoopLength agree");
    toolpath_calls = toolpath_case(&lb) + plan_case(&holed);
    GL_LoopsFree(&lb);
    GL_WallsFree(&b);
    expect(lb.n == 0 && lb.l == NULL && b.n == 0 && b.s == NULL, "GL_LoopsFree and GL_WallsFree");
    expect(calls == 15 + toolpath_calls, "every GL_ call went through the table");

    gl_loops_free(&la);
    gl_walls_free(&a);
    gs_mesh_free(&box);
    gs_mesh_free(&rod);
    gs_mesh_free(&holed);
    if (fails) {
        printf("%d failed\n", fails);
        return 1;
    }
    printf("path library ok\n");
    return 0;
}
