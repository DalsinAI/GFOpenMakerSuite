/* Written by core/library/gen-library.py from openmakemesh_lib.sfd: do not edit.
 * The library's side of each call: its registers to the C function. */
#include "omlib.h"
#include <gs_model.h>

static int LIB_GS_StlLoad(REG(a0, gs_mesh *m), REG(a1, const char *path), REG(a2, char *err), REG(d0, int errlen), REG(a6, struct Library *base))
{
    (void)base;
    return gs_stl_load(m, path, err, errlen);
}

static void LIB_GS_MeshFree(REG(a0, gs_mesh *m), REG(a6, struct Library *base))
{
    (void)base;
    gs_mesh_free(m);
}

static void LIB_GS_MeshRepair(REG(a0, gs_mesh *m), REG(a6, struct Library *base))
{
    (void)base;
    gs_mesh_repair(m);
}

static int LIB_GM_Cube(REG(a0, gs_mesh *m), REG(d0, ULONG x), REG(d1, ULONG y), REG(d2, ULONG z), REG(d3, ULONG sx), REG(d4, ULONG sy), REG(d5, ULONG sz), REG(a6, struct Library *base))
{
    (void)base;
    return gm_cube(m, om_lib_float(x), om_lib_float(y), om_lib_float(z), om_lib_float(sx), om_lib_float(sy), om_lib_float(sz));
}

static int LIB_GM_Cylinder(REG(a0, gs_mesh *m), REG(d0, ULONG x), REG(d1, ULONG y), REG(d2, ULONG z), REG(d3, ULONG r), REG(d4, ULONG h), REG(d5, int seg), REG(a6, struct Library *base))
{
    (void)base;
    return gm_cylinder(m, om_lib_float(x), om_lib_float(y), om_lib_float(z), om_lib_float(r), om_lib_float(h), seg);
}

static int LIB_GM_Union(REG(a0, gs_mesh *dst), REG(a1, const gs_mesh *a), REG(a2, const gs_mesh *b), REG(a6, struct Library *base))
{
    (void)base;
    return gm_union(dst, a, b);
}

static int LIB_GM_Subtract(REG(a0, gs_mesh *dst), REG(a1, const gs_mesh *a), REG(a2, const gs_mesh *b), REG(a6, struct Library *base))
{
    (void)base;
    return gm_subtract(dst, a, b);
}

static int LIB_GM_Intersect(REG(a0, gs_mesh *dst), REG(a1, const gs_mesh *a), REG(a2, const gs_mesh *b), REG(a6, struct Library *base))
{
    (void)base;
    return gm_intersect(dst, a, b);
}

static int LIB_GM_OpenEdges(REG(a0, const gs_mesh *m), REG(a6, struct Library *base))
{
    (void)base;
    return gm_open_edges(m);
}

static int LIB_GM_StlWrite(REG(a0, const gs_mesh *m), REG(a1, const char *path), REG(a6, struct Library *base))
{
    (void)base;
    return gm_stl_write(m, path);
}

#define OPENMAKEMESH_LAST_BIAS 84
#define OPENMAKEMESH_VECTORS \
    (APTR)LIB_GS_StlLoad, \
    (APTR)LIB_GS_MeshFree, \
    (APTR)LIB_GS_MeshRepair, \
    (APTR)LIB_GM_Cube, \
    (APTR)LIB_GM_Cylinder, \
    (APTR)LIB_GM_Union, \
    (APTR)LIB_GM_Subtract, \
    (APTR)LIB_GM_Intersect, \
    (APTR)LIB_GM_OpenEdges, \
    (APTR)LIB_GM_StlWrite
