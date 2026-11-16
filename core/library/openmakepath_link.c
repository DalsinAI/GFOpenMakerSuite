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
