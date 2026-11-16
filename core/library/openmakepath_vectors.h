/* Written by core/library/gen-library.py from openmakepath_lib.sfd: do not edit.
 * The library's side of each call: its registers to the C function. */
#include "omlib.h"
#include <gs_layer.h>
#include <gs_toolpath.h>
#include <gs_plan.h>

static ULONG LIB_GL_Height(REG(d0, ULONG h), REG(a6, struct Library *base))
{
    (void)base;
    return om_lib_bits(gl_height(om_lib_float(h)));
}

static int LIB_GL_Layers(REG(a0, const gs_mesh *m), REG(d0, ULONG h), REG(a6, struct Library *base))
{
    (void)base;
    return gl_layers(m, om_lib_float(h));
}

static ULONG LIB_GL_LayerZ(REG(a0, const gs_mesh *m), REG(d0, ULONG h), REG(d1, int layer), REG(a6, struct Library *base))
{
    (void)base;
    return om_lib_bits(gl_layer_z(m, om_lib_float(h), layer));
}

static int LIB_GL_Cut(REG(a0, const gs_mesh *m), REG(d0, ULONG h), REG(d1, int layer), REG(a1, gl_walls *w), REG(a6, struct Library *base))
{
    (void)base;
    return gl_cut(m, om_lib_float(h), layer, w);
}

static void LIB_GL_WallsFree(REG(a0, gl_walls *w), REG(a6, struct Library *base))
{
    (void)base;
    gl_walls_free(w);
}

static void LIB_GL_Status(REG(a0, char *buf), REG(d0, int len), REG(a1, const gs_mesh *m), REG(d1, ULONG h), REG(d2, int layer), REG(a2, const gl_walls *w), REG(a6, struct Library *base))
{
    (void)base;
    gl_status(buf, len, m, om_lib_float(h), layer, w);
}

static int LIB_GL_Weld(REG(a0, const gl_walls *w), REG(d0, ULONG tol), REG(a1, gl_loops *out), REG(a6, struct Library *base))
{
    (void)base;
    return gl_weld(w, om_lib_float(tol), out);
}

static void LIB_GL_LoopsFree(REG(a0, gl_loops *out), REG(a6, struct Library *base))
{
    (void)base;
    gl_loops_free(out);
}

static ULONG LIB_GL_LoopArea(REG(a0, const gl_loop *l), REG(a6, struct Library *base))
{
    (void)base;
    return om_lib_bits(gl_loop_area(l));
}

static ULONG LIB_GL_LoopLength(REG(a0, const gl_loop *l), REG(a6, struct Library *base))
{
    (void)base;
    return om_lib_bits(gl_loop_length(l));
}

static int LIB_GL_Inset(REG(a0, const gl_loops *in), REG(d0, ULONG d), REG(a1, gl_loops *out), REG(a6, struct Library *base))
{
    (void)base;
    return gl_inset(in, om_lib_float(d), out);
}

static void LIB_GT_Init(REG(a0, gt_path *p), REG(a6, struct Library *base))
{
    (void)base;
    gt_init(p);
}

static void LIB_GT_Free(REG(a0, gt_path *p), REG(a6, struct Library *base))
{
    (void)base;
    gt_free(p);
}

static int LIB_GT_Layer(REG(a0, gt_path *p), REG(a6, struct Library *base))
{
    (void)base;
    return gt_layer(p);
}

static int LIB_GT_Add(REG(a0, gt_path *p), REG(d0, int kind), REG(d1, ULONG x), REG(d2, ULONG y), REG(d3, ULONG z), REG(d4, ULONG w), REG(a6, struct Library *base))
{
    (void)base;
    return gt_add(p, kind, om_lib_float(x), om_lib_float(y), om_lib_float(z), om_lib_float(w));
}

static int LIB_GT_Loop(REG(a0, gt_path *p), REG(d0, int kind), REG(a1, const gl_loop *l), REG(d1, ULONG z), REG(d2, ULONG w), REG(a6, struct Library *base))
{
    (void)base;
    return gt_loop(p, kind, l, om_lib_float(z), om_lib_float(w));
}

static int LIB_GT_Append(REG(a0, gt_path *dst), REG(a1, const gt_path *src), REG(a6, struct Library *base))
{
    (void)base;
    return gt_append(dst, src);
}

static ULONG LIB_GT_Length(REG(a0, const gt_path *p), REG(d0, int kind), REG(a6, struct Library *base))
{
    (void)base;
    return om_lib_bits(gt_length(p, kind));
}

static int LIB_GT_Bounds(REG(a0, const gt_path *p), REG(a1, float *min), REG(a2, float *max), REG(a6, struct Library *base))
{
    (void)base;
    return gt_bounds(p, min, max);
}

static int LIB_GT_Fill(REG(a0, gt_path *p), REG(a1, const gl_loops *region), REG(a2, const gl_loops *const *solid), REG(d0, int nsolid), REG(d1, ULONG z), REG(d2, ULONG w), REG(d3, int every), REG(d4, int cross), REG(a6, struct Library *base))
{
    (void)base;
    return gt_fill(p, region, solid, nsolid, om_lib_float(z), om_lib_float(w), every, cross);
}

static void LIB_GP_Defaults(REG(a0, gp_opts *o), REG(a6, struct Library *base))
{
    (void)base;
    gp_defaults(o);
}

static int LIB_GP_Layers(REG(a0, const gs_mesh *m), REG(a1, const gp_opts *o), REG(a6, struct Library *base))
{
    (void)base;
    return gp_layers(m, o);
}

static int LIB_GP_Layer(REG(a0, gt_path *p), REG(a1, const gs_mesh *m), REG(a2, const gp_opts *o), REG(d0, int n), REG(a6, struct Library *base))
{
    (void)base;
    return gp_layer(p, m, o, n);
}

static void LIB_GP_Place(REG(a0, gs_mesh *m), REG(d0, ULONG bed_x), REG(d1, ULONG bed_y), REG(a6, struct Library *base))
{
    (void)base;
    gp_place(m, om_lib_float(bed_x), om_lib_float(bed_y));
}

static int LIB_GP_Part(REG(a0, gt_path *p), REG(a1, const gs_mesh *m), REG(a2, const gp_opts *o), REG(a6, struct Library *base))
{
    (void)base;
    return gp_part(p, m, o);
}

#define OPENMAKEPATH_LAST_BIAS 174
#define OPENMAKEPATH_VECTORS \
    (APTR)LIB_GL_Height, \
    (APTR)LIB_GL_Layers, \
    (APTR)LIB_GL_LayerZ, \
    (APTR)LIB_GL_Cut, \
    (APTR)LIB_GL_WallsFree, \
    (APTR)LIB_GL_Status, \
    (APTR)LIB_GL_Weld, \
    (APTR)LIB_GL_LoopsFree, \
    (APTR)LIB_GL_LoopArea, \
    (APTR)LIB_GL_LoopLength, \
    (APTR)LIB_GL_Inset, \
    (APTR)LIB_GT_Init, \
    (APTR)LIB_GT_Free, \
    (APTR)LIB_GT_Layer, \
    (APTR)LIB_GT_Add, \
    (APTR)LIB_GT_Loop, \
    (APTR)LIB_GT_Append, \
    (APTR)LIB_GT_Length, \
    (APTR)LIB_GT_Bounds, \
    (APTR)LIB_GT_Fill, \
    (APTR)LIB_GP_Defaults, \
    (APTR)LIB_GP_Layers, \
    (APTR)LIB_GP_Layer, \
    (APTR)LIB_GP_Place, \
    (APTR)LIB_GP_Part
