/* The toolpath's picture: the plate alone when there is nothing, each kind
 * in its colour, and only the layers asked for.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gs_model.h"
#include "gs_plan.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define W 320
#define H 240

static int fails;

static void expect(int cond, const char *msg)
{
    if (!cond) {
        fprintf(stderr, "fail: %s\n", msg);
        fails++;
    }
}

/* Pixels unlike the plate's picture, and of those the green ones (brim)
 * and the red ones (skin). */
static int drawn(const unsigned char *a, const unsigned char *plate, int *green, int *red)
{
    int i, n = 0;
    *green = *red = 0;
    for (i = 0; i < W * H; i++) {
        const unsigned char *p = a + i * 3;
        if (!memcmp(p, plate + i * 3, 3)) continue;
        n++;
        if (p[1] > p[0] + 20 && p[1] > p[2] + 20) (*green)++;
        if (p[0] > p[1] + 60 && p[0] > p[2] + 40) (*red)++;
    }
    return n;
}

int main(void)
{
    static unsigned char plate[W * H * 3], one[W * H * 3], all[W * H * 3];
    gs_mesh box, empty;
    gp_opts o;
    gt_path p;
    int n1, nall, g1, gall, r1, rall, k;
    memset(&box, 0, sizeof box);
    memset(&empty, 0, sizeof empty);
    for (k = 1; k < GT_KINDS; k++) expect(gt_colour(k) != 0 && gt_colour(k) != gt_colour(k - 1), "a colour each");
    expect(gt_colour(GT_BRIM) == 0x30a060 && gt_colour(GT_SKIN) == 0xd03040, "brim green, skin red");

    expect(gt_shade(GT_SKIN, 10.f, 10.f) == gt_colour(GT_SKIN) && gt_shade(GT_SKIN, 5.f, 0.f) == gt_colour(GT_SKIN),
           "at the top, or with no height, a kind's own colour");
    expect(gt_shade(GT_SKIN, 0.f, 10.f) == ((0xd0ul * 120 / 256) << 16 | (0x30ul * 120 / 256) << 8 | (0x40ul * 120 / 256)),
           "at the bottom, 120/256 of it");

    gt_init(&p);
    expect(gt_end(&p, 0) == 0 && gt_end(&p, 3) == 0, "nothing drawn of nothing");
    expect(gs_view_bed(&empty, NULL, plate, W, H) == 0 && gt_paint(&p, NULL, 0, one, W, H) == 0, "painted");
    expect(!memcmp(plate, one, sizeof plate), "nothing to draw: the plate alone");

    gp_defaults(&o);
    expect(gm_cube(&box, 0, 0, 0, 30, 30, 10) == 0, "a box");
    gp_place(&box, 220.f, 220.f);
    expect(gp_part(&p, &box, &o) == 0 && p.nlayers == 50, "sliced");
    expect(gt_end(&p, 0) == p.n && gt_end(&p, 1) == p.layer[1] && gt_end(&p, 50) == p.n && gt_end(&p, 99) == p.n,
           "where the layers asked for end");
    {
        gs_frame plate, part;
        gs_cam cam;
        gs_cam_home(&cam);
        expect(gt_frame(&p, &cam, 0, W, H, &plate) == 0 && gt_frame(&p, &cam, 1, W, H, &part) == 0 &&
                   part.s > 4.f * plate.s,
               "framed to the part, it is drawn more than 4 times bigger than on the plate");
    }
    expect(gt_paint(&p, NULL, 1, one, W, H) == 0 && gt_paint(&p, NULL, 0, all, W, H) == 0, "painted again");
    n1 = drawn(one, plate, &g1, &r1);
    nall = drawn(all, plate, &gall, &rall);
    printf("layer 1: %d pixels (%d brim, %d skin); all 50: %d pixels (%d brim, %d skin)\n", n1, g1, r1, nall, gall, rall);
    expect(n1 > 150 && g1 > 30 && r1 > 100, "layer 1: the brim and the bottom");
    expect(nall > n1 && gall < g1, "all of it: more drawn, the brim hidden in part behind the box");
    expect(gt_paint(&p, NULL, 0, NULL, W, H) == -1, "no picture to paint in");
    {
        static unsigned char close[W * H * 3];
        int i, lit = 0, top = 0;
        expect(gt_paint_part(&p, NULL, 0, close, W, H) == 0, "framed to the part");
        for (i = 0; i < W * H; i++) {
            const unsigned char *q = close + i * 3;
            if (q[0] > 60) lit++;
            if (q[0] > 190 && q[1] < 70 && q[2] < 80) top++; /* the top skin, lit as the highest */
        }
        printf("framed to the part: %d pixels, %d of them the top's skin\n", lit, top);
        expect(lit > 10 * nall, "the part fills the picture");
        expect(top > lit / 4, "the top layer in front of the ones below it");
    }
    {
        static unsigned char solid[W * H * 3], over[W * H * 3];
        int i, changed = 0, kept = 0, ink = 0;
        gs_cam cam;
        gs_cam_home(&cam);
        expect(gs_view_bed(&box, &cam, solid, W, H) == 0, "the solid");
        memcpy(over, solid, sizeof over);
        expect(gt_paint_on(&p, &box, &cam, 0, over, W, H) == 0, "the toolpath on the solid");
        expect(gt_paint_on(&p, &box, &cam, 0, NULL, W, H) == -1, "no picture to lay the toolpath on");
        for (i = 0; i < W * H; i++) {
            const unsigned char *a = solid + i * 3, *b = over + i * 3;
            if (!memcmp(a, b, 3)) kept++;
            else {
                changed++;
                if (b[1] > b[0] + 20 || b[0] > 80) ink++;
            }
        }
        printf("on the solid: %d pixels of toolpath, %d of the picture left\n", changed, kept);
        expect(changed > 100 && kept > changed && ink > changed / 2,
               "the lines sit on the solid, and the solid is still there");
    }
    gt_free(&p);
    gs_mesh_free(&box);
    if (fails) {
        printf("%d failed\n", fails);
        return 1;
    }
    printf("toolpath paint ok\n");
    return 0;
}
