/* Adjust a mesh the way a small STL editor does: move, turn, scale,
 * drop it on the bed, centre it, lay a face flat. Undo keeps a copy from
 * before each change. This is not a translation of any editor's source.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef GS_EDIT_H
#define GS_EDIT_H

#include "gs_view.h"
#include "gs_parts.h"

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
/* The same, with the point on it under the pixel in at. */
int ge_pick_at(const gs_mesh *m, const gs_cam *cam, int w, int h, int x, int y, gs_v *at);
/* Where p shows in that view. 0, or -1. */
/* Off the part: the point on the bed (z 0, inside the plate) under the
 * pointer, as the bench measures from. 0, or -1 off the plate. */
int ge_pick_bed(const gs_mesh *m, const gs_cam *cam, int w, int h, int x, int y, gs_v *at);
int ge_project(const gs_mesh *m, const gs_cam *cam, int w, int h, gs_v p, float *x, float *y);
/* Measure's lines: the distance between two points and along each axis;
 * a part's volume, its PLA (1.24 g/cm3, as the bench), size and triangles. */
void ge_measure_line(char *buf, int len, gs_v a, gs_v b);
void ge_facts_line(char *buf, int len, const gs_mesh *m);

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

/* Undo and redo over a parts list (gs_parts.h), as the bench's: up to 32
 * steps back. Push a copy before each change; that empties redo. Undo
 * takes now (kept for redo, not copied) and hands back the list before;
 * redo hands back the one undone and keeps now for undo. NULL, and now
 * still the caller's, when there is nothing to go to. */
typedef struct ge_steps ge_steps;
ge_steps *ge_steps_new(void);
void ge_steps_free(ge_steps *s);
int ge_steps_push(ge_steps *s, const gs_parts *now);
gs_parts *ge_steps_undo(ge_steps *s, gs_parts *now);
gs_parts *ge_steps_redo(ge_steps *s, gs_parts *now);
/* Takes back the step just pushed when the change did not happen: as
 * undo, but now is freed, not kept for redo. */
gs_parts *ge_steps_forget(ge_steps *s, gs_parts *now);
int ge_steps_back(const ge_steps *s);
int ge_steps_ahead(const ge_steps *s);

/* The bench's Join, Cut and Intersect (0, 1, 2) on a parts list: the
 * picked part is the tool, the one before it (or after, when first) the
 * stock. Both go; the result is added last with the stock's name (an
 * intersect "stock and tool") and colour, and picked. What was done in
 * line. 0; -1 without two parts; -2 when they do not meet or memory ran
 * out, the list as it was. */
int ge_combine(gs_parts *ps, int op, char *line, int len);

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
enum { GE_BOLT_CIRCLE, GE_HOLE_ROW, GE_COUNTERBORE, GE_DRILL, GE_COUNTERSINK };
typedef struct {
    float d;
    int n;
    float pcd, pitch, cb, deep;
    /* GE_DRILL and GE_COUNTERSINK: aimed (1) at ax, ay, where the face was
     * clicked, or else the part's middle; and the countersink's mouth. */
    int aimed;
    float ax, ay, mouth;
} ge_holes;

/* The drill to cut (core's gm_bolt_circle, gm_hole_row, gm_counterbore),
 * replacing tool. 0, or -1 with why says what is wrong, as the bench. */
int ge_hole_tool(gs_mesh *tool, int kind, const ge_holes *h, const gs_mesh *part, char *why, int whylen);
/* Those holes cut from part, replacing it. 0, or -1 with why, part kept. */
int ge_drill(gs_mesh *part, int kind, const ge_holes *h, char *why, int whylen);
/* The bench's Washer: 32 mm across, a 14 mm hole, 8 mm thick, on the bed
 * past the parts shown, a new part picked. 0, line saying so; or -1. */
int ge_washer(gs_parts *ps, char *line, int len);

/* The bench's Split. The picked part is cut by a horizontal plane at z
 * (have_z 0: through the middle) into two closed halves, the lower one
 * keeping the part's place in the list and the upper one added and
 * picked. Both keep its colour. Over 2,500 triangles it is refused, as
 * the bench's cut. 0, line saying so; or -1, the list as it was. */
int ge_split(gs_parts *ps, float z, int have_z, char *line, int len);

/* The part at or below z, for a section view. Triangles that cross the
 * plane are cut. The cut is open: no lid. dst is replaced. 0, or -1.
 * dst and src must not be the same mesh. */
int ge_clip_below(gs_mesh *dst, const gs_mesh *src, float z);

/* The bench's Pull by a distance: face and every face in its plane (its
 * normal within 7 degrees, its points within 0.4 mm of the plane) moved
 * mm along its normal (less than 0: pushed in), with the points of other
 * faces on them, so the part stays closed. How many points of the face
 * moved, line saying so; or -1 with line saying why, the part kept. */
int ge_pull(gs_mesh *m, int face, float mm, char *line, int len);

/* Drag to pull, as the bench's Pull mode. The screen's pixels per mm
 * along face's outward normal, in sx and sy, as the view frames scene
 * (the parts shown) at w by h; m is the face's part. 0, or -1 with no
 * face or the normal seen too nearly end on (within about 12 degrees of
 * the line of sight: turn the view). */
int ge_face_axis(const gs_mesh *scene, const gs_mesh *m, int face, const gs_cam *cam, int w, int h, float *sx,
                 float *sy);
/* A drag of dx, dy pixels as mm along that axis, to the nearest 0.1. */
float ge_drag_mm(float sx, float sy, float dx, float dy);

/* The bench's Chamfer: face's plane (as Pull's) moved in by mm (0.2 to
 * 20; its size if less than 0), its edge points in along the walls by as
 * much, with the points of other faces on them. 0, line saying so; or -1
 * with line saying why (no face, no edge, or more than the part or the
 * face has), the part kept. */
int ge_chamfer(gs_mesh *m, int face, float mm, char *line, int len);

/* Arrange, as the bench's, each on ps's picked part with line saying what
 * it did; or -1 with line saying why, ps kept. Copies keep the part's name
 * and metal, the list's 24 at most. Duplicate: one copy 6 mm on past it in
 * X, picked. Line: count (2 to 8) in all along axis (0 X, 1 Y, 2 Z), gap
 * (0 to 80) mm apart, the last picked. Ring: count (3 to 12) in all round
 * the bed's middle, each turned with its place; a part on the middle has
 * its copies round it, clear of it and of each other. Mirror: through X
 * about its middle. Turn: deg about axis through its middle. Stack: on the part before it in the list (the
 * second, for the first), over its middle. Solo: the others hidden, or
 * with them hidden already, every part shown. */
int ge_duplicate(gs_parts *ps, char *line, int len);
int ge_line(gs_parts *ps, int axis, int count, float gap, char *line, int len);
int ge_ring(gs_parts *ps, int count, char *line, int len);
int ge_mirror(gs_parts *ps, char *line, int len);
int ge_turn(gs_parts *ps, int axis, float deg, char *line, int len);
int ge_stack(gs_parts *ps, char *line, int len);
int ge_solo(gs_parts *ps, char *line, int len);

/* The bench's Extrude: kind GM_RECT, GM_ROUND or GM_SLOT, w by d (a round
 * w across), h tall. GM_ON_BED: out is the solid on the bed mid-plate, to
 * place and join as an added part. GM_ON_TOP and GM_FROM_TOP: out is the
 * part with it joined on its top, or cut down from its top (through when
 * as deep), about the middle of the part's box. part is not changed. 0,
 * with what was done in line, or -1 with why. */
int ge_extrude(const gs_mesh *part, int kind, int op, float w, float d, float h, gs_mesh *out, char *line, int len);

/* The same, stood at x, y instead of the plate's middle or the part's.
 * A dragged footprint uses this. */
int ge_extrude_at(const gs_mesh *part, int kind, int op, float x, float y, float w, float d, float h, gs_mesh *out,
                  char *line, int len);

/* Two points on the bed, as the bench's drag, to a tenth of a millimetre.
 * A block or a slot is the box between them. A round is centred on the
 * first point; w is the diameter. Under 1 mm (a round: under 1 mm of
 * radius) is refused. 0, with the centre and the size, or -1 with why. */
int ge_footprint(int kind, float x0, float y0, float x1, float y1, float *cx, float *cy, float *w, float *d, char *line,
                 int len);

#endif
