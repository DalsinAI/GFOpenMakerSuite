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

int gs_gpu_vertices(const gs_mesh *m, const gs_cam *cam, int w, int h, uint32_t *verts, int cap)
{
    return GS_GpuVertices(m, cam, w, h, verts, cap);
}

int gs_gpu_vertices_plate(const gs_mesh *m, const gs_cam *cam, int w, int h, uint32_t *verts, int cap)
{
    return GS_GpuVerticesPlate(m, cam, w, h, verts, cap);
}

int gs_gpu_walls_plate(const gl_walls *wl, const gs_mesh *m, const gs_cam *cam, int w, int h, uint32_t *verts, int cap)
{
    return GS_GpuWallsPlate(wl, m, cam, w, h, verts, cap);
}

int gs_gpu_toolpath(const gt_path *p, const gs_cam *cam, int upto, int part, int w, int h, uint32_t *verts, int cap)
{
    return GS_GpuToolpath(p, cam, upto, part, w, h, verts, cap);
}

int gs_gpu_stream(uint32_t *stream, int cap, uint32_t vert_addr, int nverts, uint32_t picture, int picture_bpr, uint32_t depth, int w, int h, const gs_mesh *m, const gs_cam *cam, int grid)
{
    return GS_GpuStream(stream, cap, vert_addr, nverts, picture, picture_bpr, depth, w, h, m, cam, grid);
}

int gs_gpu_stream_plate(uint32_t *stream, int cap, uint32_t vert_addr, int nverts, uint32_t picture, int picture_bpr, uint32_t depth, int w, int h, const gs_mesh *m, const gs_cam *cam, int grid, int plate)
{
    return GS_GpuStreamPlate(stream, cap, vert_addr, nverts, picture, picture_bpr, depth, w, h, m, cam, grid, plate);
}

void gs_view_map_at(const gs_frame *f, const gs_cam *cam, const gs_v *p, float *xyz)
{
    GS_ViewMapAt(f, cam, p, xyz);
}

int gs_view_colour_of(const gs_cam *cam, const gs_v *pts, int *rgb)
{
    return GS_ViewColourOf(cam, pts, rgb);
}

int gs_view_lines_into(const char *gcode, int len, gs_line *lines, int cap, float *box)
{
    return GS_ViewLinesInto(gcode, len, lines, cap, box);
}

int gs_view_paint(const char *gcode, int len, unsigned char *rgb, int w, int h)
{
    return GS_ViewPaint(gcode, len, rgb, w, h);
}

int gs_view_bed_tint(const gs_mesh *m, const gs_cam *cam, const gs_tint *tints, int ntints, unsigned char *rgb, int w, int h)
{
    return GS_ViewBedTint(m, cam, tints, ntints, rgb, w, h);
}

int gs_gpu_vertices_plate_tint(const gs_mesh *m, const gs_cam *cam, int w, int h, const gs_tint *tints, int ntints, uint32_t *verts, int cap)
{
    return GS_GpuVerticesPlateTint(m, cam, w, h, tints, ntints, verts, cap);
}
