/* openmakeview.library's calls, run on the host as a program makes them:
 * through the jump table's slot at each bias to the library's side and
 * back to the C function. Every answer must be the C function's own.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include <proto/openmakeview.h>

#include "../library/openmakeview_vectors.h"
#include "gs_model.h"
#include "gs_gpu.h"
#include "gs_plan.h"

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

/* The walls and the toolpath drawn: GL_Paint and the GT_ drawers against
 * the C functions. The calls it makes. */
static int paint_case(const gs_mesh *box, const gs_cam *cam)
{
    static unsigned char a[W * H * 3], b[W * H * 3];
    gl_walls wl;
    gp_opts o;
    gt_path p;
    gs_frame fa, fb;
    int k, part, same = 1;
    memset(&wl, 0, sizeof wl);
    expect(gl_cut(box, 0.5f, 10, &wl) == 0 && wl.n > 0, "the box's walls");
    expect(gl_paint(&wl, box, cam, a, W, H) == 0 && GL_Paint(&wl, box, cam, b, W, H) == 0 && !memcmp(a, b, sizeof a),
           "GL_Paint, the same picture");
    gp_defaults(&o);
    o.layer_h = 0.5f;
    gt_init(&p);
    expect(gp_part(&p, box, &o) == 0 && p.nlayers == 40, "the box's toolpath");
    for (k = 0; k < GT_KINDS; k++)
        if (GT_Colour(k) != gt_colour(k) || GT_Shade(k, 3.f, 20.f) != gt_shade(k, 3.f, 20.f)) same = 0;
    expect(same, "GT_Colour and GT_Shade, every kind");
    expect(GT_End(&p, 3) == gt_end(&p, 3) && GT_End(&p, 3) == p.layer[3], "GT_End");
    for (part = 0; part < 2; part++)
        expect(gt_frame(&p, cam, part, W, H, &fa) == 0 && GT_Frame(&p, cam, part, W, H, &fb) == 0 &&
                   !memcmp(&fa, &fb, sizeof fa),
               "GT_Frame, the part and the plate");
    expect(gt_paint(&p, cam, 20, a, W, H) == 0 && GT_Paint(&p, cam, 20, b, W, H) == 0 && !memcmp(a, b, sizeof a),
           "GT_Paint, the same picture");
    expect(gt_paint_part(&p, cam, 0, a, W, H) == 0 && GT_PaintPart(&p, cam, 0, b, W, H) == 0 &&
               !memcmp(a, b, sizeof a),
           "GT_PaintPart, the same picture");
    gt_free(&p);
    gl_walls_free(&wl);
    /* GL_Paint, a GT_Colour and a GT_Shade a kind, two GT_End, two
     * GT_Frame, GT_Paint, GT_PaintPart */
    return 1 + 2 * GT_KINDS + 2 + 2 + 1 + 1;
}

/* The OpenGPU batches: the GS_Gpu calls against the C functions. The
 * calls it makes. */
static int gpu_case(const gs_mesh *box, const gs_cam *cam)
{
    enum { CAP = 60000 };
    static uint32_t va[CAP], vb[CAP];
    gl_walls wl;
    gp_opts o;
    gt_path p;
    int na, nb, plate, ok = 1;
    memset(&wl, 0, sizeof wl);
    na = gs_gpu_vertices(box, cam, W, H, va, CAP);
    nb = GS_GpuVertices(box, cam, W, H, vb, CAP);
    expect(na > 0 && na == nb && !memcmp(va, vb, (size_t)na * 6 * 4), "GS_GpuVertices");
    na = gs_gpu_vertices_plate(box, cam, W, H, va, CAP);
    nb = GS_GpuVerticesPlate(box, cam, W, H, vb, CAP);
    expect(na > 0 && na == nb && !memcmp(va, vb, (size_t)na * 6 * 4), "GS_GpuVerticesPlate");
    expect(gl_cut(box, 0.5f, 10, &wl) == 0, "the walls");
    na = gs_gpu_walls_plate(&wl, box, cam, W, H, va, CAP);
    nb = GS_GpuWallsPlate(&wl, box, cam, W, H, vb, CAP);
    expect(na > 0 && na == nb && !memcmp(va, vb, (size_t)na * 6 * 4), "GS_GpuWallsPlate");
    gp_defaults(&o);
    o.layer_h = 2.f;
    gt_init(&p);
    expect(gp_part(&p, box, &o) == 0, "a coarse toolpath");
    na = gs_gpu_toolpath(&p, cam, 0, 1, W, H, va, CAP);
    nb = GS_GpuToolpath(&p, cam, 0, 1, W, H, vb, CAP);
    expect(na > 0 && na == nb && !memcmp(va, vb, (size_t)na * 6 * 4), "GS_GpuToolpath, eight arguments");
    expect(GS_GpuToolpath(&p, cam, 0, 1, W, H, vb, 6) == -1, "GS_GpuToolpath: too little room");
    na = gs_gpu_stream(va, CAP, 0x100000u, 36, 0x200000u, W * 4, 0x300000u, W, H, box, cam, 1);
    nb = GS_GpuStream(vb, CAP, 0x100000u, 36, 0x200000u, W * 4, 0x300000u, W, H, box, cam, 1);
    expect(na > 0 && na == nb && !memcmp(va, vb, (size_t)na * 4), "GS_GpuStream, twelve arguments");
    for (plate = 0; plate < 2; plate++) {
        na = gs_gpu_stream_plate(va, CAP, 0x100000u, 36, 0x200000u, W * 4, 0x300000u, W, H, box, cam, 1, plate);
        nb = GS_GpuStreamPlate(vb, CAP, 0x100000u, 36, 0x200000u, W * 4, 0x300000u, W, H, box, cam, 1, plate);
        if (na <= 0 || na != nb || memcmp(va, vb, (size_t)na * 4)) ok = 0;
    }
    expect(ok, "GS_GpuStreamPlate, thirteen arguments, part and plate");
    printf("through the library: a stream of %d words\n", nb);
    gt_free(&p);
    gl_walls_free(&wl);
    /* GS_GpuVertices, its plate's, the walls', two GS_GpuToolpath, a stream
     * and two plate streams */
    return 8;
}

int main(void)
{
    static unsigned char a[W * H * 3], b[W * H * 3];
    gs_mesh box;
    gs_cam ca, cb;
    gs_frame fa, fb;
    gs_run ra[W], rb[W];
    int na, nb, y, i, k, mid = 0, rows = 1;
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
    calls = 0;
    k = paint_case(&box, &cb);
    expect(calls == k, "every GL_ and GT_ call went through the table");
    calls = 0;
    k = gpu_case(&box, &cb);
    expect(calls == k, "every GS_Gpu call went through the table");

    gs_mesh_free(&box);
    if (fails) {
        printf("%d failed\n", fails);
        return 1;
    }
    printf("view library ok\n");
    return 0;
}
