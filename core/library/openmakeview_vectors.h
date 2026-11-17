/* Written by core/library/gen-library.py from openmakeview_lib.sfd: do not edit.
 * The library's side of each call: its registers to the C function. */
#include "omlib.h"
#include <gs_view.h>

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

#define OPENMAKEVIEW_LAST_BIAS 90
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
    (APTR)LIB_GS_ViewOnGpu
