/* Skin and infill: lines across a layer, solid where the layers above or
 * below run out. MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gs_toolpath.h"

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

static int near(float a, float b) { return fabsf(a - b) < 1e-3f; }

/* Moves of one kind, and how many are not along x (or y, when cross). */
static int count(const gt_path *p, int kind, int cross, int *off)
{
    int i, n = 0;
    *off = 0;
    for (i = 1; i < p->n; i++) {
        if (p->m[i].kind != kind) continue;
        n++;
        if (cross ? p->m[i].x != p->m[i - 1].x : p->m[i].y != p->m[i - 1].y) (*off)++;
    }
    return n;
}

int main(void)
{
    /* 0.4 to 9.6 square, anticlockwise; a 2 mm square hole, clockwise. */
    float sq[] = {0.4f, 0.4f, 9.6f, 0.4f, 9.6f, 9.6f, 0.4f, 9.6f};
    float hole[] = {4, 4, 4, 6, 6, 6, 6, 4};
    float half[] = {0, 0, 5, 0, 5, 10, 0, 10};
    gl_loop ls[] = {{sq, 4, 1}, {hole, 4, 1}}, hl = {half, 4, 1};
    gl_loops region = {ls, 1, 0}, holed = {ls, 2, 0}, left = {&hl, 1, 0};
    const gl_loops *all[] = {&region, &region}, *top[] = {&region, NULL}, *step[] = {&left, &region};
    gt_path p;
    int off;
    gt_init(&p);

    expect(gt_fill(&p, &region, top, 2, 1.f, 0.4f, 5, 0) == 0, "a top layer");
    expect(count(&p, GT_SKIN, 0, &off) == 23 && off == 0 && count(&p, GT_INFILL, 0, &off) == 0, "23 lines of skin along x");
    expect(near(gt_length(&p, GT_SKIN), 23 * 9.2f), "each 9.2 mm");
    expect(p.m[1].x > p.m[0].x && p.m[3].x < p.m[2].x, "back and forth");
    gt_free(&p);

    expect(gt_fill(&p, &region, all, 2, 1.f, 0.4f, 5, 0) == 0, "a layer in the middle");
    expect(count(&p, GT_SKIN, 0, &off) == 0 && count(&p, GT_INFILL, 0, &off) == 4, "only every 5th line: 4 of them");
    expect(near(gt_length(&p, GT_INFILL), 4 * 9.2f) && near(p.m[1].y, 2.2f), "on the bed's grid, at y 2.2, 4.2 ...");
    gt_free(&p);

    expect(gt_fill(&p, &region, all, 0, 1.f, 0.4f, 1, 1) == 0, "no skin asked for, every line, across");
    expect(count(&p, GT_INFILL, 1, &off) == 23 && off == 0 && near(gt_length(&p, GT_INFILL), 23 * 9.2f), "23 lines along y");
    gt_free(&p);

    expect(gt_fill(&p, &region, step, 2, 1.f, 0.4f, 5, 0) == 0, "the layer below covers the left half only");
    expect(near(gt_length(&p, GT_SKIN), 23 * 4.6f) && near(gt_length(&p, GT_INFILL), 4 * 4.6f),
           "skin over the right half, infill over the left");
    gt_free(&p);

    expect(gt_fill(&p, &holed, top, 2, 1.f, 0.4f, 5, 0) == 0, "round a hole");
    expect(near(gt_length(&p, GT_SKIN), 23 * 9.2f - 5 * 2.f), "the 5 lines through it stop at it");
    gt_free(&p);

    if (fails) {
        printf("%d failed\n", fails);
        return 1;
    }
    printf("fill ok\n");
    return 0;
}
