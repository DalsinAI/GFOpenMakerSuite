/* Adjust a mesh the way a small STL editor does: move, turn, scale,
 * drop it on the bed, centre it, lay a face flat. Undo keeps a copy from
 * before each change. This is not a translation of any editor's source.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef GS_EDIT_H
#define GS_EDIT_H

#include "gs_view.h"

/* Replace dst with a copy of src. 0, or -1. */
int ge_copy(gs_mesh *dst, const gs_mesh *src);

void ge_move(gs_mesh *m, float dx, float dy, float dz);

/* About the centre of the box. Radians, X then Y then Z. */
void ge_rotate(gs_mesh *m, float rx, float ry, float rz);

/* About the centre of the box. 1 leaves it. */
void ge_scale(gs_mesh *m, float sx, float sy, float sz);

/* The lowest point onto z = 0. */
void ge_drop(gs_mesh *m);

/* The middle of the box onto the middle of the plate. */
void ge_centre(gs_mesh *m);

/* The biggest triangle, or -1. */
int ge_largest_face(const gs_mesh *m);

/* That face's outside points down, and the face sits on z = 0. 0, or -1. */
int ge_lay_face(gs_mesh *m, int face);

/* The front triangle under a pixel of the plate view, or -1. */
int ge_pick(const gs_mesh *m, const gs_cam *cam, int w, int h, int x, int y);

/* Reads an STL for the editor: welded, empty triangles dropped, the
 * outside turned out, and sitting on z = 0. It does not close cracks.
 * Replaces m. 0, or -1 with why in err and m empty. */
int ge_open(gs_mesh *m, const char *path, char *err, int errlen);

/* The solid GFOpenMake starts with: the STL named by path, or with no
 * path the 20 mm cube with a hole, 8 mm off the plate. A file is read as
 * ge_open reads it. Reads only; the file is written only by Save.
 * 0, or -1 with why in err. */
int ge_start(gs_mesh *m, const char *path, char *err, int errlen);

/* The editor's two parts: the stock, and an added part to combine with it.
 * on_added: the added part is the one selected. */
typedef struct {
    gs_mesh stock, added;
    int has_added, on_added;
} ge_scene;

void ge_scene_free(ge_scene *s);

/* Up to 32 steps. Each step keeps both parts. */
typedef struct ge_hist ge_hist;

ge_hist *ge_hist_new(void);
void ge_hist_free(ge_hist *h);
int ge_hist_push(ge_hist *h, const gs_mesh *m);
int ge_hist_pop(ge_hist *h, gs_mesh *m);
int ge_hist_depth(const ge_hist *h);
/* The whole scene, so undo after Join puts both parts back. 0, or -1. */
int ge_hist_push_scene(ge_hist *h, const ge_scene *s);
int ge_hist_pop_scene(ge_hist *h, ge_scene *s);

/* 0 join, 1 cut (the selected part out of the other), 2 intersect. The
 * result is the stock and the added part is gone. The scene before is one
 * step in h. 0; -1 with no second part; -2 when they do not meet or memory
 * ran out, and then the scene and h are as they were. */
int ge_bool(ge_scene *s, int op, ge_hist *h);

/* The shapes Parts adds, as the bench's Make has them. */
enum { GE_BOX, GE_CYLINDER, GE_SPHERE, GE_CONE, GE_TORUS, GE_WEDGE, GE_SLOT, GE_SHAPES };

/* "Box" ... "Slot", or NULL. */
const char *ge_shape_name(int kind);

/* A part of that kind to add to the stock, placed and sized from it: the
 * box beside its corner (to join), the cylinder and the slot down through
 * its middle (to cut), the rest on the plate at its middle. Replaces m.
 * 0, or -1. */
int ge_shape(gs_mesh *m, int kind, const gs_mesh *stock);

/* Size and place, as the bench's: where is the middle of the part's box
 * in X and Y and its bottom in Z; size its box. Both 0 with no part. */
void ge_where(const gs_mesh *m, float pos[3], float size[3]);
/* Moved so where is (x, y, z). */
void ge_place(gs_mesh *m, float x, float y, float z);
/* Scaled evenly so its box is mm along axis (0 X, 1 Y, 2 Z), where kept.
 * 0, or -1 (no part, no axis, no size, or one too thin to scale). */
int ge_fit(gs_mesh *m, int axis, float mm);

/* The bench's hole tools, about the middle of the part and through it:
 * n drills d wide round a pitch circle pcd wide (2 to 12), or along X
 * pitch apart (2 to 8), or one drill with a counterbore cb wide and deep
 * in its top. */
enum { GE_BOLT_CIRCLE, GE_HOLE_ROW, GE_COUNTERBORE };
typedef struct {
    float d;
    int n;
    float pcd, pitch, cb, deep;
} ge_holes;

/* The drill to cut (core's gm_bolt_circle, gm_hole_row, gm_counterbore),
 * replacing tool. 0, or -1 with why says what is wrong, as the bench. */
int ge_hole_tool(gs_mesh *tool, int kind, const ge_holes *h, const gs_mesh *part, char *why, int whylen);
/* Those holes cut from part, replacing it. 0, or -1 with why, part kept. */
int ge_drill(gs_mesh *part, int kind, const ge_holes *h, char *why, int whylen);

#endif
