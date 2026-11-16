/* Written by core/library/gen-library.py from openmakepath_lib.sfd: do not edit.
 * The C API through openmakepath.library: a program links this instead of
 * the sources, and opens OpenMakePathBase itself. */
#include <proto/openmakepath.h>

float gl_height(float h)
{
    return GL_Height(h);
}

int gl_layers(const gs_mesh *m, float h)
{
    return GL_Layers(m, h);
}

float gl_layer_z(const gs_mesh *m, float h, int layer)
{
    return GL_LayerZ(m, h, layer);
}

int gl_cut(const gs_mesh *m, float h, int layer, gl_walls *w)
{
    return GL_Cut(m, h, layer, w);
}

void gl_walls_free(gl_walls *w)
{
    GL_WallsFree(w);
}

void gl_status(char *buf, int len, const gs_mesh *m, float h, int layer, const gl_walls *w)
{
    GL_Status(buf, len, m, h, layer, w);
}

int gl_weld(const gl_walls *w, float tol, gl_loops *out)
{
    return GL_Weld(w, tol, out);
}

void gl_loops_free(gl_loops *out)
{
    GL_LoopsFree(out);
}

float gl_loop_area(const gl_loop *l)
{
    return GL_LoopArea(l);
}

float gl_loop_length(const gl_loop *l)
{
    return GL_LoopLength(l);
}

int gl_inset(const gl_loops *in, float d, gl_loops *out)
{
    return GL_Inset(in, d, out);
}

void gt_init(gt_path *p)
{
    GT_Init(p);
}

void gt_free(gt_path *p)
{
    GT_Free(p);
}

int gt_layer(gt_path *p)
{
    return GT_Layer(p);
}

int gt_add(gt_path *p, int kind, float x, float y, float z, float w)
{
    return GT_Add(p, kind, x, y, z, w);
}

int gt_loop(gt_path *p, int kind, const gl_loop *l, float z, float w)
{
    return GT_Loop(p, kind, l, z, w);
}

int gt_append(gt_path *dst, const gt_path *src)
{
    return GT_Append(dst, src);
}

float gt_length(const gt_path *p, int kind)
{
    return GT_Length(p, kind);
}

int gt_bounds(const gt_path *p, float *min, float *max)
{
    return GT_Bounds(p, min, max);
}

int gt_fill(gt_path *p, const gl_loops *region, const gl_loops *const *solid, int nsolid, float z, float w, int every, int cross)
{
    return GT_Fill(p, region, solid, nsolid, z, w, every, cross);
}
