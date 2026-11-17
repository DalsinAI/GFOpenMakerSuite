/* Written by core/library/gen-library.py from openmakeview_lib.sfd: do not edit.
 * The C API through openmakeview.library: a program links this instead of
 * the sources, and opens OpenMakeViewBase itself. */
#include <proto/openmakeview.h>

void gs_cam_home(gs_cam *c)
{
    GS_CamHome(c);
}

void gs_cam_orbit(gs_cam *c, float dx, float dy)
{
    GS_CamOrbit(c, dx, dy);
}

void gs_cam_pan(gs_cam *c, float dx, float dy)
{
    GS_CamPan(c, dx, dy);
}

void gs_cam_zoom(gs_cam *c, float factor)
{
    GS_CamZoom(c, factor);
}

int gs_view_frame(const gs_mesh *m, const gs_cam *cam, int w, int h, gs_frame *f)
{
    return GS_ViewFrame(m, cam, w, h, f);
}

int gs_view_frame_bed(const gs_mesh *m, const gs_cam *cam, int w, int h, float x0, float y0, float x1, float y1, gs_frame *f)
{
    return GS_ViewFrameBed(m, cam, w, h, x0, y0, x1, y1, f);
}

int gs_view_solid(const gs_mesh *m, const gs_cam *cam, unsigned char *rgb, int w, int h)
{
    return GS_ViewSolid(m, cam, rgb, w, h);
}

int gs_view_bed(const gs_mesh *m, const gs_cam *cam, unsigned char *rgb, int w, int h)
{
    return GS_ViewBed(m, cam, rgb, w, h);
}

int gs_pen_of(const unsigned char *rgb)
{
    return GS_PenOf(rgb);
}

int gs_pen_row(const unsigned char *rgb, int w, gs_run *runs)
{
    return GS_PenRow(rgb, w, runs);
}

int gs_view_on_gpu(void)
{
    return GS_ViewOnGpu();
}

int gl_paint(const gl_walls *w, const gs_mesh *m, const gs_cam *cam, unsigned char *rgb, int width, int height)
{
    return GL_Paint(w, m, cam, rgb, width, height);
}

unsigned long gt_colour(int kind)
{
    return GT_Colour(kind);
}

unsigned long gt_shade(int kind, float z, float top)
{
    return GT_Shade(kind, z, top);
}

int gt_end(const gt_path *p, int upto)
{
    return GT_End(p, upto);
}

int gt_frame(const gt_path *p, const gs_cam *cam, int part, int w, int h, gs_frame *f)
{
    return GT_Frame(p, cam, part, w, h, f);
}

int gt_paint(const gt_path *p, const gs_cam *cam, int upto, unsigned char *rgb, int w, int h)
{
    return GT_Paint(p, cam, upto, rgb, w, h);
}

int gt_paint_part(const gt_path *p, const gs_cam *cam, int upto, unsigned char *rgb, int w, int h)
{
    return GT_PaintPart(p, cam, upto, rgb, w, h);
}
