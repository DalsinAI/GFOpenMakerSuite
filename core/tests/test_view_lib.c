/* openmakeview.library's calls, run on the host as a program makes them:
 * through the jump table's slot at each bias to the library's side and
 * back to the C function. Every answer must be the C function's own.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include <proto/openmakeview.h>

#include "../library/openmakeview_vectors.h"
#include "gs_model.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fails, calls;

static void expect(int ok, const char *what)
{
    if (!ok) {
        printf("fail: %s\n", what);
        fails++;
    }
}

static const APTR table[] = {NULL, NULL, NULL, NULL, OPENMAKEVIEW_VECTORS};
static struct Library *fake = (struct Library *)&fake;
struct Library *OpenMakeViewBase;

APTR om_host_vector(struct Library *base, int bias)
{
    expect(base == fake && bias % 6 == 0 && bias >= 30 && bias <= OPENMAKEVIEW_LAST_BIAS, "a call in the table");
    calls++;
    return table[bias / 6 - 1];
}

#define W 96
#define H 72

int main(void)
{
    static unsigned char a[W * H * 3], b[W * H * 3];
    gs_mesh box;
    gs_cam ca, cb;
    gs_frame fa, fb;
    gs_run ra[W], rb[W];
    int na, nb, y, i, mid = 0, rows = 1;
    OpenMakeViewBase = fake;
    memset(&box, 0, sizeof box);
    expect(sizeof table / sizeof table[0] == OPENMAKEVIEW_LAST_BIAS / 6, "every call has its slot");
    expect(gm_cube(&box, 90, 90, 0, 40, 30, 20) == 0, "a box on the plate");

    gs_cam_home(&ca);
    memset(&cb, 0xff, sizeof cb);
    GS_CamHome(&cb);
    expect(!memcmp(&ca, &cb, sizeof ca), "GS_CamHome");
    gs_cam_orbit(&ca, 0.3f, -0.1f);
    GS_CamOrbit(&cb, 0.3f, -0.1f);
    gs_cam_pan(&ca, 0.05f, 0.02f);
    GS_CamPan(&cb, 0.05f, 0.02f);
    gs_cam_zoom(&ca, 0.8f);
    GS_CamZoom(&cb, 0.8f);
    expect(!memcmp(&ca, &cb, sizeof ca), "GS_CamOrbit, GS_CamPan and GS_CamZoom, floats each way");

    expect(gs_view_frame(&box, &ca, W, H, &fa) == 0 && GS_ViewFrame(&box, &cb, W, H, &fb) == 0 &&
               !memcmp(&fa, &fb, sizeof fa),
           "GS_ViewFrame");
    expect(gs_view_frame_bed(&box, &ca, W, H, 0, 0, GS_BED, GS_BED, &fa) == 0 &&
               GS_ViewFrameBed(&box, &cb, W, H, 0, 0, GS_BED, GS_BED, &fb) == 0 && !memcmp(&fa, &fb, sizeof fa),
           "GS_ViewFrameBed, nine arguments");
    expect(gs_view_solid(&box, &ca, a, W, H) == 0 && GS_ViewSolid(&box, &cb, b, W, H) == 0 && !memcmp(a, b, sizeof a),
           "GS_ViewSolid, the same picture");
    expect(gs_view_bed(&box, &ca, a, W, H) == 0 && GS_ViewBed(&box, &cb, b, W, H) == 0 && !memcmp(a, b, sizeof a),
           "GS_ViewBed, the same picture");
    expect(GS_PenOf(b + (H / 2 * W + W / 2) * 3) == gs_pen_of(a + (H / 2 * W + W / 2) * 3), "GS_PenOf");
    for (y = 0; y < H; y++) {
        na = gs_pen_row(a + y * W * 3, W, ra);
        nb = GS_PenRow(b + y * W * 3, W, rb);
        if (na != nb) rows = 0;
        for (i = 0; rows && i < na; i++) /* field by field: a run's padding is never written */
            if (ra[i].x != rb[i].x || ra[i].n != rb[i].n || ra[i].pen != rb[i].pen) rows = 0;
        mid += nb;
    }
    expect(rows, "GS_PenRow, every row");
    expect(GS_ViewOnGpu() == gs_view_on_gpu(), "GS_ViewOnGpu");
    printf("through the library: a %dx%d plate as %d runs of pens\n", W, H, mid);
    expect(calls == 10 + H, "every GS_ call went through the table");

    gs_mesh_free(&box);
    if (fails) {
        printf("%d failed\n", fails);
        return 1;
    }
    printf("view library ok\n");
    return 0;
}
