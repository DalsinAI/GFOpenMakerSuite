/* Written by core/library/gen-library.py from openmakeview_lib.sfd: do not edit.
 * The library's side of each call: its registers to the C function. */
#include "omlib.h"
#include <gs_view.h>
#include <gs_layer.h>
#include <gs_toolpath.h>
#include <gs_gpu.h>

static void LIB_GS_CamHome(REG(a0, gs_cam *c), REG(a6, struct Library *base))
{
    (void)base;
    gs_cam_home(c);
}

static void LIB_GS_CamOrbit(REG(a0, gs_cam *c), REG(d0, ULONG dx), REG(d1, ULONG dy), REG(a6, struct Library *base))
{
    (void)base;
    gs_cam_orbit(c, om_lib_float(dx), om_lib_float(dy));
}

static void LIB_GS_CamPan(REG(a0, gs_cam *c), REG(d0, ULONG dx), REG(d1, ULONG dy), REG(a6, struct Library *base))
{
    (void)base;
    gs_cam_pan(c, om_lib_float(dx), om_lib_float(dy));
}

static void LIB_GS_CamZoom(REG(a0, gs_cam *c), REG(d0, ULONG factor), REG(a6, struct Library *base))
{
    (void)base;
    gs_cam_zoom(c, om_lib_float(factor));
}

static int LIB_GS_ViewFrame(REG(a0, const gs_mesh *m), REG(a1, const gs_cam *cam), REG(d0, int w), REG(d1, int h), REG(a2, gs_frame *f), REG(a6, struct Library *base))
{
    (void)base;
    return gs_view_frame(m, cam, w, h, f);
}

static int LIB_GS_ViewFrameBed(REG(a0, const gs_mesh *m), REG(a1, const gs_cam *cam), REG(d0, int w), REG(d1, int h), REG(d2, ULONG x0), REG(d3, ULONG y0), REG(d4, ULONG x1), REG(d5, ULONG y1), REG(a2, gs_frame *f), REG(a6, struct Library *base))
{
    (void)base;
    return gs_view_frame_bed(m, cam, w, h, om_lib_float(x0), om_lib_float(y0), om_lib_float(x1), om_lib_float(y1), f);
}

static int LIB_GS_ViewSolid(REG(a0, const gs_mesh *m), REG(a1, const gs_cam *cam), REG(a2, unsigned char *rgb), REG(d0, int w), REG(d1, int h), REG(a6, struct Library *base))
{
    (void)base;
    return gs_view_solid(m, cam, rgb, w, h);
}

static int LIB_GS_ViewBed(REG(a0, const gs_mesh *m), REG(a1, const gs_cam *cam), REG(a2, unsigned char *rgb), REG(d0, int w), REG(d1, int h), REG(a6, struct Library *base))
{
    (void)base;
    return gs_view_bed(m, cam, rgb, w, h);
}

static int LIB_GS_PenOf(REG(a0, const unsigned char *rgb), REG(a6, struct Library *base))
{
    (void)base;
    return gs_pen_of(rgb);
}

static int LIB_GS_PenRow(REG(a0, const unsigned char *rgb), REG(d0, int w), REG(a1, gs_run *runs), REG(a6, struct Library *base))
{
    (void)base;
    return gs_pen_row(rgb, w, runs);
}

static int LIB_GS_ViewOnGpu(REG(a6, struct Library *base))
{
    (void)base;
    return gs_view_on_gpu();
}

static int LIB_GL_Paint(REG(a0, const gl_walls *w), REG(a1, const gs_mesh *m), REG(a2, const gs_cam *cam), REG(a3, unsigned char *rgb), REG(d0, int width), REG(d1, int height), REG(a6, struct Library *base))
{
    (void)base;
    return gl_paint(w, m, cam, rgb, width, height);
}

static unsigned long LIB_GT_Colour(REG(d0, int kind), REG(a6, struct Library *base))
{
    (void)base;
    return gt_colour(kind);
}

static unsigned long LIB_GT_Shade(REG(d0, int kind), REG(d1, ULONG z), REG(d2, ULONG top), REG(a6, struct Library *base))
{
    (void)base;
    return gt_shade(kind, om_lib_float(z), om_lib_float(top));
}

static int LIB_GT_End(REG(a0, const gt_path *p), REG(d0, int upto), REG(a6, struct Library *base))
{
    (void)base;
    return gt_end(p, upto);
}

static int LIB_GT_Frame(REG(a0, const gt_path *p), REG(a1, const gs_cam *cam), REG(d0, int part), REG(d1, int w), REG(d2, int h), REG(a2, gs_frame *f), REG(a6, struct Library *base))
{
    (void)base;
    return gt_frame(p, cam, part, w, h, f);
}

static int LIB_GT_Paint(REG(a0, const gt_path *p), REG(a1, const gs_cam *cam), REG(d0, int upto), REG(a2, unsigned char *rgb), REG(d1, int w), REG(d2, int h), REG(a6, struct Library *base))
{
    (void)base;
    return gt_paint(p, cam, upto, rgb, w, h);
}

static int LIB_GT_PaintPart(REG(a0, const gt_path *p), REG(a1, const gs_cam *cam), REG(d0, int upto), REG(a2, unsigned char *rgb), REG(d1, int w), REG(d2, int h), REG(a6, struct Library *base))
{
    (void)base;
    return gt_paint_part(p, cam, upto, rgb, w, h);
}

static int LIB_GS_GpuVertices(REG(a0, const gs_mesh *m), REG(a1, const gs_cam *cam), REG(d0, int w), REG(d1, int h), REG(a2, uint32_t *verts), REG(d2, int cap), REG(a6, struct Library *base))
{
    (void)base;
    return gs_gpu_vertices(m, cam, w, h, verts, cap);
}

static int LIB_GS_GpuVerticesPlate(REG(a0, const gs_mesh *m), REG(a1, const gs_cam *cam), REG(d0, int w), REG(d1, int h), REG(a2, uint32_t *verts), REG(d2, int cap), REG(a6, struct Library *base))
{
    (void)base;
    return gs_gpu_vertices_plate(m, cam, w, h, verts, cap);
}

static int LIB_GS_GpuWallsPlate(REG(a0, const gl_walls *wl), REG(a1, const gs_mesh *m), REG(a2, const gs_cam *cam), REG(d0, int w), REG(d1, int h), REG(a3, uint32_t *verts), REG(d2, int cap), REG(a6, struct Library *base))
{
    (void)base;
    return gs_gpu_walls_plate(wl, m, cam, w, h, verts, cap);
}

static int LIB_GS_GpuToolpath(REG(a0, const gt_path *p), REG(a1, const gs_cam *cam), REG(d0, int upto), REG(d1, int part), REG(d2, int w), REG(d3, int h), REG(a2, uint32_t *verts), REG(d4, int cap), REG(a6, struct Library *base))
{
    (void)base;
    return gs_gpu_toolpath(p, cam, upto, part, w, h, verts, cap);
}

static int LIB_GS_GpuStream(REG(a0, uint32_t *stream), REG(d0, int cap), REG(d1, uint32_t vert_addr), REG(d2, int nverts), REG(d3, uint32_t picture), REG(d4, int picture_bpr), REG(d5, uint32_t depth), REG(d6, int w), REG(d7, int h), REG(a1, const gs_mesh *m), REG(a2, const gs_cam *cam), REG(a3, int grid), REG(a6, struct Library *base))
{
    (void)base;
    return gs_gpu_stream(stream, cap, vert_addr, nverts, picture, picture_bpr, depth, w, h, m, cam, grid);
}

static int LIB_GS_GpuStreamPlate(REG(a0, uint32_t *stream), REG(d0, int cap), REG(d1, uint32_t vert_addr), REG(d2, int nverts), REG(d3, uint32_t picture), REG(d4, int picture_bpr), REG(d5, uint32_t depth), REG(d6, int w), REG(d7, int h), REG(a1, const gs_mesh *m), REG(a2, const gs_cam *cam), REG(a3, int grid), REG(a4, int plate), REG(a6, struct Library *base))
{
    (void)base;
    return gs_gpu_stream_plate(stream, cap, vert_addr, nverts, picture, picture_bpr, depth, w, h, m, cam, grid, plate);
}

static void LIB_GS_ViewMapAt(REG(a0, const gs_frame *f), REG(a1, const gs_cam *cam), REG(a2, const gs_v *p), REG(a3, float *xyz), REG(a6, struct Library *base))
{
    (void)base;
    gs_view_map_at(f, cam, p, xyz);
}

static int LIB_GS_ViewColourOf(REG(a0, const gs_cam *cam), REG(a1, const gs_v *pts), REG(a2, int *rgb), REG(a6, struct Library *base))
{
    (void)base;
    return gs_view_colour_of(cam, pts, rgb);
}

static int LIB_GS_ViewLinesInto(REG(a0, const char *gcode), REG(d0, int len), REG(a1, gs_line *lines), REG(d1, int cap), REG(a2, float *box), REG(a6, struct Library *base))
{
    (void)base;
    return gs_view_lines_into(gcode, len, lines, cap, box);
}

static int LIB_GS_ViewPaint(REG(a0, const char *gcode), REG(d0, int len), REG(a1, unsigned char *rgb), REG(d1, int w), REG(d2, int h), REG(a6, struct Library *base))
{
    (void)base;
    return gs_view_paint(gcode, len, rgb, w, h);
}

static int LIB_GS_ViewBedTint(REG(a0, const gs_mesh *m), REG(a1, const gs_cam *cam), REG(a2, const gs_tint *tints), REG(d0, int ntints), REG(a3, unsigned char *rgb), REG(d1, int w), REG(d2, int h), REG(a6, struct Library *base))
{
    (void)base;
    return gs_view_bed_tint(m, cam, tints, ntints, rgb, w, h);
}

static int LIB_GS_GpuVerticesPlateTint(REG(a0, const gs_mesh *m), REG(a1, const gs_cam *cam), REG(d0, int w), REG(d1, int h), REG(a2, const gs_tint *tints), REG(d2, int ntints), REG(a3, uint32_t *verts), REG(d3, int cap), REG(a6, struct Library *base))
{
    (void)base;
    return gs_gpu_vertices_plate_tint(m, cam, w, h, tints, ntints, verts, cap);
}

#define OPENMAKEVIEW_LAST_BIAS 204
#define OPENMAKEVIEW_VECTORS \
    (APTR)LIB_GS_CamHome, \
    (APTR)LIB_GS_CamOrbit, \
    (APTR)LIB_GS_CamPan, \
    (APTR)LIB_GS_CamZoom, \
    (APTR)LIB_GS_ViewFrame, \
    (APTR)LIB_GS_ViewFrameBed, \
    (APTR)LIB_GS_ViewSolid, \
    (APTR)LIB_GS_ViewBed, \
    (APTR)LIB_GS_PenOf, \
    (APTR)LIB_GS_PenRow, \
    (APTR)LIB_GS_ViewOnGpu, \
    (APTR)LIB_GL_Paint, \
    (APTR)LIB_GT_Colour, \
    (APTR)LIB_GT_Shade, \
    (APTR)LIB_GT_End, \
    (APTR)LIB_GT_Frame, \
    (APTR)LIB_GT_Paint, \
    (APTR)LIB_GT_PaintPart, \
    (APTR)LIB_GS_GpuVertices, \
    (APTR)LIB_GS_GpuVerticesPlate, \
    (APTR)LIB_GS_GpuWallsPlate, \
    (APTR)LIB_GS_GpuToolpath, \
    (APTR)LIB_GS_GpuStream, \
    (APTR)LIB_GS_GpuStreamPlate, \
    (APTR)LIB_GS_ViewMapAt, \
    (APTR)LIB_GS_ViewColourOf, \
    (APTR)LIB_GS_ViewLinesInto, \
    (APTR)LIB_GS_ViewPaint, \
    (APTR)LIB_GS_ViewBedTint, \
    (APTR)LIB_GS_GpuVerticesPlateTint
