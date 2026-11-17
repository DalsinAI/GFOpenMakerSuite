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

int gm_facts(const gs_mesh *m, gs_facts *f)
{
    return GM_Facts(m, f);
}

int gm_extrude(gs_mesh *m, const gs_mesh *part, int kind, int op, float x, float y, float w, float d, float h)
{
    return GM_Extrude(m, part, kind, op, x, y, w, d, h);
}

gs_parts * gs_parts_new(void)
{
    return GS_PartsNew();
}

void gs_parts_free(gs_parts *ps)
{
    GS_PartsFree(ps);
}

int gs_parts_count(const gs_parts *ps)
{
    return GS_PartsCount(ps);
}

int gs_parts_add(gs_parts *ps, gs_mesh *m, const char *name)
{
    return GS_PartsAdd(ps, m, name);
}

int gs_parts_remove(gs_parts *ps, int i)
{
    return GS_PartsRemove(ps, i);
}

gs_mesh * gs_parts_mesh(gs_parts *ps, int i)
{
    return GS_PartsMesh(ps, i);
}

const char * gs_parts_name(const gs_parts *ps, int i)
{
    return GS_PartsName(ps, i);
}

int gs_parts_rename(gs_parts *ps, int i, const char *name)
{
    return GS_PartsRename(ps, i, name);
}

int gs_parts_colour(const gs_parts *ps, int i)
{
    return GS_PartsColour(ps, i);
}

int gs_parts_set_colour(gs_parts *ps, int i, int colour)
{
    return GS_PartsSetColour(ps, i, colour);
}

unsigned long gs_parts_rgb(int colour)
{
    return GS_PartsRgb(colour);
}

int gs_parts_shown(const gs_parts *ps, int i)
{
    return GS_PartsShown(ps, i);
}

int gs_parts_show(gs_parts *ps, int i, int on)
{
    return GS_PartsShow(ps, i, on);
}

int gs_parts_picked(const gs_parts *ps)
{
    return GS_PartsPicked(ps);
}

int gs_parts_pick(gs_parts *ps, int i)
{
    return GS_PartsPick(ps, i);
}

int gs_parts_scene(const gs_parts *ps, gs_mesh *out)
{
    return GS_PartsScene(ps, out);
}

gs_parts * gs_parts_copy(const gs_parts *ps)
{
    return GS_PartsCopy(ps);
}
