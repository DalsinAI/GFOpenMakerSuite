/* A bench's parts: up to 24 named meshes, each with a colour, shown or
 * hidden, and one picked, as the bench's parts list (cad/bench). Several
 * apps can hold one: the editor's list, a slicer's plate. Opaque, so its
 * fields can grow without moving anyone's code.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef GS_PARTS_H
#define GS_PARTS_H

#include "gs.h"

#define GS_PARTS_MAX 24    /* the bench's limit */
#define GS_PARTS_NAME 48   /* a name's characters, at most */
#define GS_PARTS_COLOURS 6 /* the bench's metals */

typedef struct gs_parts gs_parts;

/* An empty list, or NULL with no memory. Free takes the meshes too. */
gs_parts *gs_parts_new(void);
void gs_parts_free(gs_parts *ps);
int gs_parts_count(const gs_parts *ps);

/* Takes m's triangles (m is left empty) as a new part, shown and picked.
 * name NULL or blank is "Part n". Colours go round the six in turn. Its
 * index, or -1 (24 already, no triangles, no memory). */
int gs_parts_add(gs_parts *ps, gs_mesh *m, const char *name);

/* Part i gone; the last part is picked, or none (-1). 0, or -1. */
int gs_parts_remove(gs_parts *ps, int i);

/* Part i's mesh to draw or change in place, or NULL. */
gs_mesh *gs_parts_mesh(gs_parts *ps, int i);

/* The name, the bench's way: trimmed, at most 48 characters; a blank one
 * is refused (-1). */
const char *gs_parts_name(const gs_parts *ps, int i);
int gs_parts_rename(gs_parts *ps, int i, const char *name);

/* The colour, 0 to 5, and its 0xRRGGBB. */
int gs_parts_colour(const gs_parts *ps, int i);
int gs_parts_set_colour(gs_parts *ps, int i, int colour);
unsigned long gs_parts_rgb(int colour);

/* Shown or hidden: 1 or 0, -1 for no such part. */
int gs_parts_shown(const gs_parts *ps, int i);
int gs_parts_show(gs_parts *ps, int i, int on);

/* The picked part, or -1. Picking -1 picks none. */
int gs_parts_picked(const gs_parts *ps);
int gs_parts_pick(gs_parts *ps, int i);

/* Every shown part's triangles in out (replaced), for the view and the
 * slicer, in list order. 0, or -1. */
int gs_parts_scene(const gs_parts *ps, gs_mesh *out);

/* A whole copy, for undo, or NULL. */
gs_parts *gs_parts_copy(const gs_parts *ps);

#endif
