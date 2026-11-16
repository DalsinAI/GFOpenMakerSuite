/* A toolpath: the moves a machine makes, layer by layer, whatever the
 * machine. The slicer fills one for a printer; the laser and CNC apps can
 * fill one for theirs. A writer (G-code for a printer, a laser or a mill)
 * turns the moves into commands; the preview draws them.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef GS_TOOLPATH_H
#define GS_TOOLPATH_H

#include "gs_layer.h"

/* What a move is for. A travel lays nothing down and cuts nothing. */
enum {
    GT_TRAVEL = 0,
    GT_OUTER,      /* the outside wall, the one that shows */
    GT_INNER,      /* the walls inside it */
    GT_SKIN,       /* solid top and bottom */
    GT_INFILL,     /* sparse inside */
    GT_BRIM,       /* round the part on the first layer */
    GT_CUT,        /* a laser's or a mill's line */
    GT_KINDS
};

/* A straight move from the one before it to (x, y, z). w is the line's
 * width in mm for a printer (0 for a travel); a laser or a mill may use it
 * for power or depth. */
typedef struct {
    float x, y, z, w;
    unsigned char kind;
} gt_move;

typedef struct {
    gt_move *m;
    int n, cap;
    int *layer;    /* nlayers indexes into m: where each layer starts */
    int nlayers, lcap;
} gt_path;

void gt_init(gt_path *p);
void gt_free(gt_path *p);

/* Starts a new layer at the next move. 0, or -1 out of memory. */
int gt_layer(gt_path *p);
/* One move. 0, or -1 out of memory. */
int gt_add(gt_path *p, int kind, float x, float y, float z, float w);
/* A closed loop at z: a travel to its first point, then round it and back
 * to the start. An open piece is traced end to end. 0, or -1. */
int gt_loop(gt_path *p, int kind, const gl_loop *l, float z, float w);
/* src's moves and layers after dst's. 0, or -1 (dst as it was). */
int gt_append(gt_path *dst, const gt_path *src);

/* mm moved by moves of one kind, or by all of them (kind -1). The first
 * move only places the tool, so it has no length. */
float gt_length(const gt_path *p, int kind);
/* The box round every move but travels. 0, or -1 when there are none. */
int gt_bounds(const gt_path *p, float min[3], float max[3]);

#endif
