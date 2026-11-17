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

static int LIB_GM_Sphere(REG(a0, gs_mesh *m), REG(d0, ULONG x), REG(d1, ULONG y), REG(d2, ULONG z), REG(d3, ULONG r), REG(d4, int seg), REG(d5, int rings), REG(a6, struct Library *base))
{
    (void)base;
    return gm_sphere(m, om_lib_float(x), om_lib_float(y), om_lib_float(z), om_lib_float(r), seg, rings);
}

static int LIB_GM_Cone(REG(a0, gs_mesh *m), REG(d0, ULONG x), REG(d1, ULONG y), REG(d2, ULONG z), REG(d3, ULONG r), REG(d4, ULONG h), REG(d5, int seg), REG(a6, struct Library *base))
{
    (void)base;
    return gm_cone(m, om_lib_float(x), om_lib_float(y), om_lib_float(z), om_lib_float(r), om_lib_float(h), seg);
}

static int LIB_GM_Torus(REG(a0, gs_mesh *m), REG(d0, ULONG x), REG(d1, ULONG y), REG(d2, ULONG z), REG(d3, ULONG ring), REG(d4, ULONG tube), REG(d5, int seg), REG(d6, int tseg), REG(a6, struct Library *base))
{
    (void)base;
    return gm_torus(m, om_lib_float(x), om_lib_float(y), om_lib_float(z), om_lib_float(ring), om_lib_float(tube), seg, tseg);
}

static int LIB_GM_Wedge(REG(a0, gs_mesh *m), REG(d0, ULONG x), REG(d1, ULONG y), REG(d2, ULONG z), REG(d3, ULONG sx), REG(d4, ULONG sy), REG(d5, ULONG sz), REG(a6, struct Library *base))
{
    (void)base;
    return gm_wedge(m, om_lib_float(x), om_lib_float(y), om_lib_float(z), om_lib_float(sx), om_lib_float(sy), om_lib_float(sz));
}

static int LIB_GM_Slot(REG(a0, gs_mesh *m), REG(d0, ULONG x), REG(d1, ULONG y), REG(d2, ULONG z), REG(d3, ULONG sx), REG(d4, ULONG sy), REG(d5, ULONG sz), REG(d6, int seg), REG(a6, struct Library *base))
{
    (void)base;
    return gm_slot(m, om_lib_float(x), om_lib_float(y), om_lib_float(z), om_lib_float(sx), om_lib_float(sy), om_lib_float(sz), seg);
}

static int LIB_GM_BoltCircle(REG(a0, gs_mesh *m), REG(d0, ULONG x), REG(d1, ULONG y), REG(d2, ULONG z), REG(d3, ULONG h), REG(d4, ULONG d), REG(d5, ULONG pcd), REG(d6, int n), REG(a6, struct Library *base))
{
    (void)base;
    return gm_bolt_circle(m, om_lib_float(x), om_lib_float(y), om_lib_float(z), om_lib_float(h), om_lib_float(d), om_lib_float(pcd), n);
}

static int LIB_GM_HoleRow(REG(a0, gs_mesh *m), REG(d0, ULONG x), REG(d1, ULONG y), REG(d2, ULONG z), REG(d3, ULONG h), REG(d4, ULONG d), REG(d5, ULONG pitch), REG(d6, int n), REG(a6, struct Library *base))
{
    (void)base;
    return gm_hole_row(m, om_lib_float(x), om_lib_float(y), om_lib_float(z), om_lib_float(h), om_lib_float(d), om_lib_float(pitch), n);
}

static int LIB_GM_Counterbore(REG(a0, gs_mesh *m), REG(d0, ULONG x), REG(d1, ULONG y), REG(d2, ULONG z), REG(d3, ULONG h), REG(d4, ULONG d), REG(d5, ULONG cb), REG(d6, ULONG deep), REG(a6, struct Library *base))
{
    (void)base;
    return gm_counterbore(m, om_lib_float(x), om_lib_float(y), om_lib_float(z), om_lib_float(h), om_lib_float(d), om_lib_float(cb), om_lib_float(deep));
}

static int LIB_GM_Facts(REG(a0, const gs_mesh *m), REG(a1, gs_facts *f), REG(a6, struct Library *base))
{
    (void)base;
    return gm_facts(m, f);
}

static int LIB_GM_Extrude(REG(a0, gs_mesh *m), REG(a1, const gs_mesh *part), REG(d0, int kind), REG(d1, int op), REG(d2, ULONG x), REG(d3, ULONG y), REG(d4, ULONG w), REG(d5, ULONG d), REG(d6, ULONG h), REG(a6, struct Library *base))
{
    (void)base;
    return gm_extrude(m, part, kind, op, om_lib_float(x), om_lib_float(y), om_lib_float(w), om_lib_float(d), om_lib_float(h));
}

#define OPENMAKEMESH_LAST_BIAS 144
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
    (APTR)LIB_GM_StlWrite, \
    (APTR)LIB_GM_Sphere, \
    (APTR)LIB_GM_Cone, \
    (APTR)LIB_GM_Torus, \
    (APTR)LIB_GM_Wedge, \
    (APTR)LIB_GM_Slot, \
    (APTR)LIB_GM_BoltCircle, \
    (APTR)LIB_GM_HoleRow, \
    (APTR)LIB_GM_Counterbore, \
    (APTR)LIB_GM_Facts, \
    (APTR)LIB_GM_Extrude
