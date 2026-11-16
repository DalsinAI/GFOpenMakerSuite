/* Written by core/library/gen-library.py from openmakepath_lib.sfd: do not edit.
 * The library's side of each call: its registers to the C function. */
#include "omlib.h"
#include <gs_layer.h>

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

#define OPENMAKEPATH_LAST_BIAS 84
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
    (APTR)LIB_GL_LoopLength
