/* Written by core/library/gen-library.py from openmakemesh_lib.sfd: do not edit.
 * The C API through openmakemesh.library: a program links this instead of
 * the sources, and opens OpenMakeMeshBase itself. */
#include <proto/openmakemesh.h>

int gs_stl_load(gs_mesh *m, const char *path, char *err, int errlen)
{
    return GS_StlLoad(m, path, err, errlen);
}

void gs_mesh_free(gs_mesh *m)
{
    GS_MeshFree(m);
}

void gs_mesh_repair(gs_mesh *m)
{
    GS_MeshRepair(m);
}

int gm_cube(gs_mesh *m, float x, float y, float z, float sx, float sy, float sz)
{
    return GM_Cube(m, x, y, z, sx, sy, sz);
}

int gm_cylinder(gs_mesh *m, float x, float y, float z, float r, float h, int seg)
{
    return GM_Cylinder(m, x, y, z, r, h, seg);
}

int gm_union(gs_mesh *dst, const gs_mesh *a, const gs_mesh *b)
{
    return GM_Union(dst, a, b);
}

int gm_subtract(gs_mesh *dst, const gs_mesh *a, const gs_mesh *b)
{
    return GM_Subtract(dst, a, b);
}

int gm_intersect(gs_mesh *dst, const gs_mesh *a, const gs_mesh *b)
{
    return GM_Intersect(dst, a, b);
}

int gm_open_edges(const gs_mesh *m)
{
    return GM_OpenEdges(m);
}

int gm_stl_write(const gs_mesh *m, const char *path)
{
    return GM_StlWrite(m, path);
}

int gm_sphere(gs_mesh *m, float x, float y, float z, float r, int seg, int rings)
{
    return GM_Sphere(m, x, y, z, r, seg, rings);
}

int gm_cone(gs_mesh *m, float x, float y, float z, float r, float h, int seg)
{
    return GM_Cone(m, x, y, z, r, h, seg);
}

int gm_torus(gs_mesh *m, float x, float y, float z, float ring, float tube, int seg, int tseg)
{
    return GM_Torus(m, x, y, z, ring, tube, seg, tseg);
}

int gm_wedge(gs_mesh *m, float x, float y, float z, float sx, float sy, float sz)
{
    return GM_Wedge(m, x, y, z, sx, sy, sz);
}

int gm_slot(gs_mesh *m, float x, float y, float z, float sx, float sy, float sz, int seg)
{
    return GM_Slot(m, x, y, z, sx, sy, sz, seg);
}

int gm_bolt_circle(gs_mesh *m, float x, float y, float z, float h, float d, float pcd, int n)
{
    return GM_BoltCircle(m, x, y, z, h, d, pcd, n);
}

int gm_hole_row(gs_mesh *m, float x, float y, float z, float h, float d, float pitch, int n)
{
    return GM_HoleRow(m, x, y, z, h, d, pitch, n);
}

int gm_counterbore(gs_mesh *m, float x, float y, float z, float h, float d, float cb, float deep)
{
    return GM_Counterbore(m, x, y, z, h, d, cb, deep);
}
