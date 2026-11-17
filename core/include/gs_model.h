/* Solids the way a small CAD does them: a box, a cylinder, a sphere, a
 * cone, a torus, a wedge, a slot, and the three
 * ways of joining two solids. The triangles are a gs_mesh, so the slicer's
 * view and the chip's triangle op can draw them without a second scene.
 *
 * The operations are JSCAD's (cube, cylinder, union, subtract, intersect).
 * This file is not a translation of that source.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef GS_MODEL_H
#define GS_MODEL_H

#include "gs.h"

/* Replaces m. 0, or -1. The box starts at (x, y, z) and measures sx, sy, sz. */
int gm_cube(gs_mesh *m, float x, float y, float z, float sx, float sy, float sz);

/* Axis along Z, centred on (x, y), from z up by h. seg is the number of
 * sides, at least 3. */
int gm_cylinder(gs_mesh *m, float x, float y, float z, float r, float h, int seg);

/* The bench's other solids. Each replaces m: 0, or -1 (no size, or out
 * of memory). Closed, the outside out. */

/* Centred on (x, y, z): seg round, rings from pole to pole (at least 3
 * and 2). */
int gm_sphere(gs_mesh *m, float x, float y, float z, float r, int seg, int rings);

/* Axis along Z: the base centred on (x, y) at z, the point h above. */
int gm_cone(gs_mesh *m, float x, float y, float z, float r, float h, int seg);

/* Round Z through (x, y, z): the ring's radius to the tube's middle, the
 * tube's radius (less than the ring's); seg round the ring, tseg round
 * the tube. */
int gm_torus(gs_mesh *m, float x, float y, float z, float ring, float tube, int seg, int tseg);

/* In the box from (x, y, z) measuring sx, sy, sz: full height at x,
 * sloping down to the base at x + sx. */
int gm_wedge(gs_mesh *m, float x, float y, float z, float sx, float sy, float sz);

/* A slot (a stadium: round ends) filling the box from (x, y, z) measuring
 * sx, sy, sz, along its longer side; as long as wide, a round. seg is
 * the sides of a whole round end (4 to 128). */
int gm_slot(gs_mesh *m, float x, float y, float z, float sx, float sy, float sz, int seg);

/* The bench's hole tools, as drills to cut from a part with gm_subtract:
 * each drill d wide, upright from z for h. A bolt circle is n drills
 * evenly round a pitch circle pcd wide about x, y, the first on +X; a hole
 * row n drills along X, pitch apart, about x, y. A counterbore is one
 * drill with a bore cb wide and deep at its top. Replace m; 0, or -1 when
 * there is no drill, length or hole, the holes would meet, or the bore is
 * no wider than the drill or as deep as the drill is long. */
int gm_bolt_circle(gs_mesh *m, float x, float y, float z, float h, float d, float pcd, int n);
int gm_hole_row(gs_mesh *m, float x, float y, float z, float h, float d, float pitch, int n);
int gm_counterbore(gs_mesh *m, float x, float y, float z, float h, float d, float cb, float deep);

/* What Measure shows of a part: its volume (mm3, inside out negative),
 * its surface (mm2), the size of its box (mm) and its triangles. Fields
 * are only ever added at the end (openmakemesh.library hands it out). */
typedef struct {
    float volume, area;
    float sx, sy, sz;
    int tris;
} gs_facts;

/* m's facts into f; an empty part's are all nought. 0, or -1. */
int gm_facts(const gs_mesh *m, gs_facts *f);

/* The bench's Extrude: a footprint about (x, y) stood up h. kind:
 * GM_RECT w by d, GM_ROUND w across (d unused), GM_SLOT w by d with round
 * ends. op: GM_ON_BED from z 0; GM_ON_TOP on part's top, 0.4 into it so
 * a union joins; GM_FROM_TOP down h from part's top and 0.4 over it, to
 * subtract, or right through (0.4 past both faces) when h is the part's
 * height or more. Replaces m; 0, or -1 (no footprint or height, no part
 * for a top, or no such kind or op). */
#define GM_RECT 0
#define GM_ROUND 1
#define GM_SLOT 2
#define GM_ON_BED 0
#define GM_ON_TOP 1
#define GM_FROM_TOP 2
int gm_extrude(gs_mesh *m, const gs_mesh *part, int kind, int op, float x, float y, float w, float d, float h);

/* dst = a and b joined. a and b are not changed. 0, or -1. */
int gm_union(gs_mesh *dst, const gs_mesh *a, const gs_mesh *b);

/* dst = a with b cut out. a and b are not changed. 0, or -1. */
int gm_subtract(gs_mesh *dst, const gs_mesh *a, const gs_mesh *b);

/* dst = only the overlap of a and b. 0, or -1. An empty result has no triangles. */
int gm_intersect(gs_mesh *dst, const gs_mesh *a, const gs_mesh *b);

/* Edges not matched by one going the other way: 0 for a closed solid. */
int gm_open_edges(const gs_mesh *m);

/* Binary STL, little-endian, which is what a desktop slicer writes. */
int gm_stl_write(const gs_mesh *m, const char *path);

#endif
