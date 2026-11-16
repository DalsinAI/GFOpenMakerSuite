/* The toolpath: moves, layers, loops, joining two, lengths and the box.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gs_toolpath.h"

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

static int near(float a, float b) { return fabsf(a - b) < 1e-4f; }

int main(void)
{
    gt_path a, b;
    float sq[] = {0, 0, 10, 0, 10, 10, 0, 10}, lo[3], hi[3];
    gl_loop square = {sq, 4, 1}, open = {sq, 3, 0};
    gt_init(&a);
    gt_init(&b);
    expect(a.n == 0 && a.nlayers == 0 && gt_length(&a, -1) == 0.f, "an empty path");
    expect(gt_bounds(&a, lo, hi) == -1, "an empty path has no box");

    expect(gt_layer(&a) == 0 && gt_add(&a, GT_TRAVEL, 5, 5, 0.2f, 0) == 0, "a layer and a travel");
    expect(gt_loop(&a, GT_OUTER, &square, 0.2f, 0.4f) == 0, "a square");
    expect(a.n == 6 && a.m[1].kind == GT_TRAVEL && a.m[1].x == 0 && a.m[5].x == 0 && a.m[5].y == 0,
           "a travel to the corner, then round and back to it");
    expect(a.m[2].kind == GT_OUTER && near(a.m[2].w, 0.4f) && near(a.m[2].z, 0.2f), "the walls' kind, width and height");
    expect(near(gt_length(&a, GT_OUTER), 40.f), "40 mm round the square");
    expect(near(gt_length(&a, GT_TRAVEL), sqrtf(50.f)), "the travel to it");

    expect(gt_layer(&b) == 0 && gt_loop(&b, GT_INFILL, &open, 0.4f, 0.4f) == 0, "an open piece");
    expect(b.n == 3 && b.m[0].kind == GT_TRAVEL && b.m[2].kind == GT_INFILL && b.m[2].y == 10, "traced end to end");
    expect(near(gt_length(&b, GT_INFILL), 20.f), "20 mm along it");
    expect(gt_layer(&b) == 0 && gt_add(&b, GT_SKIN, -3, 12, 0.6f, 0.45f) == 0, "a third layer's move");

    expect(gt_append(&a, &b) == 0 && a.n == 10 && a.nlayers == 3, "two paths joined");
    expect(a.layer[0] == 0 && a.layer[1] == 6 && a.layer[2] == 9, "the second path's layers moved along");
    expect(near(gt_length(&a, GT_INFILL), 20.f) && near(gt_length(&a, GT_SKIN), sqrtf(169.f + 4.f + 0.04f)),
           "lengths through the join");
    expect(gt_bounds(&a, lo, hi) == 0 && lo[0] == -3 && lo[1] == 0 && hi[0] == 10 && hi[1] == 12 &&
               near(lo[2], 0.2f) && near(hi[2], 0.6f),
           "the box of what is laid down, travels left out");

    gt_free(&a);
    gt_free(&b);
    expect(a.n == 0 && a.m == NULL && a.layer == NULL && a.nlayers == 0, "freed");
    if (fails) {
        printf("%d failed\n", fails);
        return 1;
    }
    printf("toolpath ok\n");
    return 0;
}
