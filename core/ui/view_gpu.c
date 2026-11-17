/* omview_scene.h: the view as an OpenGPU batch, for every app.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "omview_scene.h"

#include "gs_gpu.h"

#include <string.h>

int omv_scene_need(const gs_mesh *part, const gt_path *path)
{
    if (path && path->n > 0) return path->n * 6 + 6;
    return part ? part->n * 3 : 0;
}

int omv_scene(const gs_mesh *part, const gt_path *path, const gs_cam *cam, int upto, int grid, int w, int h,
              uint32_t *verts, int vert_cap, uint32_t *stream, int stream_cap, uint32_t picture, int bpr,
              uint32_t depth)
{
    gs_mesh none;
    int nv, sliced = path && path->n > 0;
    if (!verts || !stream || w <= 0 || h <= 0) return -1;
    if (sliced) {
        nv = gs_gpu_toolpath(path, cam, upto, 1, w, h, verts, vert_cap);
        grid = 0; /* fitted to the part: the plate's grid would not line up */
    } else if (part && part->n > 0) {
        nv = gs_gpu_vertices_plate(part, cam, w, h, verts, vert_cap);
    } else {
        return -1;
    }
    if (nv < 3) return -1;
    if (!part) {
        memset(&none, 0, sizeof none);
        part = &none;
    }
    return gs_gpu_stream_plate(stream, stream_cap, (uint32_t)(uintptr_t)verts, nv, picture, bpr, depth, w, h, part,
                               cam, grid, 1);
}
