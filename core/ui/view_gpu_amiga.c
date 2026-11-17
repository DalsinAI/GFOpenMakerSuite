/* The view on OpenGPU, on the Amiga: see omview_gpu.h. Moved here from
 * GFOpenMake's make.c so every app's window shares it.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "omview_gpu.h"

#include <exec/libraries.h>
#include <exec/memory.h>
#include <proto/exec.h>

#include <gfopengpu/gfopengpu.h>
#include <gfopengpu/retained.h>
#include <gfopengpu/stream3d.h>
#include <proto/gfopengpu.h>

#include <string.h>

struct Library *OpenGPUBase;

static int takes(unsigned long op, unsigned long fmt)
{
    return OGPU_ANSWER(OGPU_Query(op, fmt)) != OGPU_NONE;
}

static void picture_free(omv_gpu *g)
{
    if (g->rs && OpenGPUBase) OGPU_RetainedDestroy(g->rs);
    g->rs = NULL;
    if (g->depth) FreeMem(g->depth, g->depth_bytes);
    g->depth = NULL;
    g->depth_bytes = 0;
    g->w = g->h = 0;
}

int omv_gpu_open(omv_gpu *g, int verts, int words)
{
    memset(g, 0, sizeof *g);
    OpenGPUBase = OpenLibrary((CONST_STRPTR)OPENGPU_NAME, OPENGPU_VERSION);
    if (!OpenGPUBase || OpenGPUBase->lib_Revision < OGPU_RETAINED_REVISION ||
        !takes(OGPU_OP_TRIANGLES, OGPU_FMT_ARGB32) || !takes(OGPU_OP_RENDER3D, OGPU_FMT_ARGB32) ||
        !(takes(OGPU_OP_DEPTH, OGPU_FMT_ARGB32) || takes(OGPU_OP_DEPTH, OGPU_FMT_Z32)) ||
        !(takes(OGPU_OP_CLEAR3D, OGPU_FMT_ARGB32) || takes(OGPU_OP_CLEAR3D, 0)))
        return 0;
    g->on_chip = OGPU_ONGPU(OGPU_Query(OGPU_OP_TRIANGLES, OGPU_FMT_ARGB32));
    g->grid = takes(OGPU_OP_LINE, OGPU_FMT_ARGB32);
    if (verts < 3) verts = 3;
    g->vert_bytes = (ULONG)verts * 6UL * 4UL;
    g->stream_bytes = (ULONG)words * 4UL;
    g->verts = (uint32_t *)AllocMem(g->vert_bytes, MEMF_PUBLIC | MEMF_CLEAR);
    g->stream = (uint32_t *)AllocMem(g->stream_bytes, MEMF_PUBLIC | MEMF_CLEAR);
    if (!g->verts || !g->stream) {
        omv_gpu_close(g);
        return 0;
    }
    g->vert_cap = verts;
    g->stream_cap = words;
    g->on = 1;
    return 0;
}

void omv_gpu_close(omv_gpu *g)
{
    picture_free(g);
    if (g->verts) FreeMem(g->verts, g->vert_bytes);
    if (g->stream) FreeMem(g->stream, g->stream_bytes);
    g->verts = g->stream = NULL;
    g->vert_bytes = g->stream_bytes = 0;
    g->vert_cap = g->stream_cap = 0;
    g->on = 0;
    if (OpenGPUBase) CloseLibrary(OpenGPUBase);
    OpenGPUBase = NULL;
}

int omv_gpu_size(omv_gpu *g, int w, int h)
{
    ULONG bytes;
    if (!g->on || w < 4 || h < 4 || w > 2048 || h > 2048) return 0;
    if (g->rs && g->depth && g->w == w && g->h == h && g->rs->rs_Pixels) return 1;
    picture_free(g);
    g->rs = OGPU_RetainedCreate((ULONG)w, (ULONG)h, OGPU_FMT_ARGB32, 0);
    bytes = (ULONG)w * (ULONG)h * 4UL;
    if (g->rs && g->rs->rs_Pixels) g->depth = (uint32_t *)AllocMem(bytes, MEMF_PUBLIC | MEMF_CLEAR);
    if (!g->rs || !g->rs->rs_Pixels || !g->depth || g->rs->rs_BytesPerRow < (ULONG)w * 4UL) {
        if (g->depth) g->depth_bytes = bytes;
        picture_free(g);
        return 0;
    }
    g->depth_bytes = bytes;
    g->w = w;
    g->h = h;
    return 1;
}

int omv_gpu_room(omv_gpu *g, int need)
{
    uint32_t *p;
    ULONG bytes;
    if (!g->verts) return 0;
    if (need <= g->vert_cap) return 1;
    bytes = (ULONG)need * 6UL * 4UL;
    p = (uint32_t *)AllocMem(bytes, MEMF_PUBLIC | MEMF_CLEAR);
    if (!p) return 0;
    FreeMem(g->verts, g->vert_bytes);
    g->verts = p;
    g->vert_bytes = bytes;
    g->vert_cap = need;
    return 1;
}

int omv_gpu_show(omv_gpu *g, int n, struct RastPort *rp, int x, int y)
{
    ULONG fence = 0;
    if (!g->on || !g->rs || n < 1) return 0;
    if (OGPU_Submit(g->stream, (ULONG)n, &fence) != OGPU_OK) return 0;
    if (OGPU_Wait(fence) != OGPU_OK) return 0;
    OGPU_RetainedMark(g->rs, 0, 0, g->w, g->h);
    return OGPU_RetainedPresent(g->rs, rp, x, y, g->w, g->h) >= 0;
}
