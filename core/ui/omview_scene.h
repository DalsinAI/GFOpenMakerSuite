/* The view as an OpenGPU batch, the same for every app: the toolpath once
 * sliced (fitted to the part, no grid), else the part on the plate with
 * the grid. Portable: the Amiga side (omview_gpu.h) gives it the room and
 * shows the picture; the pens draw when it returns -1. view_gpu.c.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef OMVIEW_SCENE_H
#define OMVIEW_SCENE_H

#include "gs_toolpath.h"
#include "gs_view.h"

#include <stdint.h>

/* The vertices omv_scene needs: the toolpath's when path is sliced, else
 * the part's; 0 for nothing. */
int omv_scene_need(const gs_mesh *part, const gt_path *path);

/* The vertices and the stream for a w by h view: path's layers 1 to upto
 * (0: all) when path has any, else part (grid: the bed's grid with it).
 * picture, bpr and depth are the surface's Amiga addresses. Returns the
 * stream's words, or -1 (nothing to draw, or too little room). */
int omv_scene(const gs_mesh *part, const gt_path *path, const gs_cam *cam, int upto, int grid, int w, int h,
              uint32_t *verts, int vert_cap, uint32_t *stream, int stream_cap, uint32_t picture, int bpr,
              uint32_t depth);

#endif
