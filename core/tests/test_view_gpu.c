/* The view as OpenGPU draws it, built the same for every app (core/ui's
 * omv_scene): the toolpath once sliced, the part before, or nothing.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gs_gpu.h"
#include "gs_model.h"
#include "gs_plan.h"
#include "omview_scene.h"

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

int main(void)
{
    gs_mesh box;
    gt_path p;
    gp_opts o;
    gs_cam cam;
    uint32_t *v, *s, *want;
    int need, words, nv, cap, i;
    memset(&box, 0, sizeof box);
    gt_init(&p);
    gs_cam_home(&cam);
    expect(omv_scene_need(NULL, NULL) == 0, "nothing: no room needed");
    expect(omv_scene(NULL, NULL, &cam, 0, 1, W, H, NULL, 0, NULL, 0, 0x1000, W * 4, 0x9000) == -1,
           "nothing to draw: -1, the pens draw the empty view");

    expect(gm_cube(&box, 0, 0, 0, 30, 30, 10) == 0, "a box");
    gp_place(&box, 220.f, 220.f);
    need = omv_scene_need(&box, NULL);
    expect(need == box.n * 3, "the part: three vertices a triangle");
    cap = need + 1024;
    v = calloc((size_t)cap * 6 + 600000, 4);
    s = calloc(4096, 4);
    want = calloc((size_t)cap * 6 + 600000, 4);
    words = omv_scene(&box, NULL, &cam, 0, 1, W, H, v, need, s, 4096, 0x1000, W * 4, 0x9000);
    expect(words > 0, "the part's stream");
    nv = gs_gpu_vertices_plate(&box, &cam, W, H, want, need);
    expect(nv == need && !memcmp(v, want, (size_t)nv * 24), "the part's vertices, on the plate, as GFOpenMake draws them");
    expect(omv_scene(&box, NULL, &cam, 0, 1, W, H, v, need - 1, s, 4096, 0x1000, W * 4, 0x9000) == -1,
           "too little room: -1");
    expect(omv_scene(&box, NULL, &cam, 0, 1, W, H, v, need, s, 4, 0x1000, W * 4, 0x9000) == -1,
           "too short a stream: -1");
    i = omv_scene(&box, NULL, &cam, 0, 0, W, H, v, need, s, 4096, 0x1000, W * 4, 0x9000);
    expect(i > 0 && i < words, "no grid: a shorter stream");

    gp_defaults(&o);
    expect(gp_part(&p, &box, &o) == 0 && p.nlayers == 50, "sliced");
    need = omv_scene_need(&box, &p);
    expect(need == p.n * 6 + 6, "the toolpath: room for two triangles a move");
    free(v);
    free(want);
    v = calloc((size_t)need * 6, 4);
    want = calloc((size_t)need * 6, 4);
    words = omv_scene(&box, &p, &cam, 0, 1, W, H, v, need, s, 4096, 0x1000, W * 4, 0x9000);
    nv = gs_gpu_toolpath(&p, &cam, 0, 1, W, H, want, need);
    expect(words > 0 && nv > 3 && !memcmp(v, want, (size_t)nv * 24),
           "sliced: the toolpath's vertices, fitted to the part, as gt_paint_part frames it");
    i = omv_scene(&box, &p, &cam, 0, 0, W, H, v, need, s, 4096, 0x1000, W * 4, 0x9000);
    expect(i == words, "sliced: no grid either way");
    nv = gs_gpu_toolpath(&p, &cam, 3, 1, W, H, want, need);
    expect(omv_scene(&box, &p, &cam, 3, 1, W, H, v, need, s, 4096, 0x1000, W * 4, 0x9000) > 0 &&
               !memcmp(v, want, (size_t)nv * 24),
           "layers 1 to 3: those layers' moves");
    gt_free(&p);
    gs_mesh_free(&box);
    free(v);
    free(want);
    free(s);
    if (fails) {
        printf("%d failed\n", fails);
        return 1;
    }
    printf("view gpu ok\n");
    return 0;
}
