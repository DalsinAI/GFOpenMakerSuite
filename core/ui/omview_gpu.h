/* The view on OpenGPU, for every app's window on the Amiga: gfopengpu.library
 * opened and asked whether it draws 3D triangles with depth, a retained
 * surface and a depth buffer the view's size, room for the vertices and
 * the stream (gs_gpu.h writes both), and the stream submitted and the
 * picture presented in the window. When any of it is missing the app draws
 * with the pens (omview_ui.h's omv_pens). view_gpu_amiga.c.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef OMVIEW_GPU_H
#define OMVIEW_GPU_H

#include <exec/types.h>
#include <graphics/rastport.h>
#include <stdint.h>

#include <gfopengpu/retained.h>

typedef struct {
    int on;                     /* the library draws the view */
    int on_chip;                /* a graphics chip does, not the CPU back end */
    int grid;                   /* it draws lines: the bed's grid */
    struct OGPURetained *rs;    /* the picture, the view's size */
    uint32_t *depth, *verts, *stream;
    ULONG depth_bytes, vert_bytes, stream_bytes;
    int w, h, vert_cap, stream_cap;
} omv_gpu;

/* Opens gfopengpu.library (OpenGPUBase) and, when it can draw the view,
 * room for verts vertices and words of stream: g->on. 0 either way; the
 * pens draw when g->on is 0. */
int omv_gpu_open(omv_gpu *g, int verts, int words);

/* Everything omv_gpu_open and omv_gpu_size took, and the library. */
void omv_gpu_close(omv_gpu *g);

/* A w by h picture and depth buffer: 1, or 0 (draw with the pens). */
int omv_gpu_size(omv_gpu *g, int w, int h);

/* Room for need vertices in g->verts (what was there is not kept): 1, or 0
 * and the old room. */
int omv_gpu_room(omv_gpu *g, int need);

/* The stream's words (n) submitted, waited for, and the picture shown at
 * x, y in rp: 1, or 0 (draw with the pens). */
int omv_gpu_show(omv_gpu *g, int n, struct RastPort *rp, int x, int y);

#endif
