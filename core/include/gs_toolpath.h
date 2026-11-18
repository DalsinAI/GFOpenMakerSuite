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
/* The box round every move but travels, three floats each (x, y, z). 0,
 * or -1 when there are none. */
int gt_bounds(const gt_path *p, float *min, float *max);

/* Lines across the region in (its loops' insides, even-odd), one line
 * width w apart on a grid fixed to the bed, so a layer's lines sit on the
 * one's below. Along x, or along y when cross. Each run is laid back and
 * forth, with a travel between.
 *   solid: the layers that decide what is skin, nsolid of them (those
 *   above and below). Where all of them have material the line is sparse
 *   (GT_INFILL, every every-th line only); elsewhere it is GT_SKIN. A NULL
 *   is a layer past the part's end: everything is skin. nsolid 0: no skin.
 * 0, or -1 out of memory. */
int gt_fill(gt_path *p, const gl_loops *region, const gl_loops *const *solid, int nsolid, float z, float w,
            int every, int cross);

/* gt_fill as a grid: the skin as gt_fill lays it, and the sparse lines
 * both ways, every 2 every-th line each way. 0, or -1. */
int gt_fill_grid(gt_path *p, const gl_loops *region, const gl_loops *const *solid, int nsolid, float z, float w,
                 int every, int cross);

/* The colour each kind is drawn in: 0xRRGGBB. A travel is not drawn. */
unsigned long gt_colour(int kind);
/* Its colour at height z, top the highest drawn: darker the lower. */
unsigned long gt_shade(int kind, float z, float top);
/* Where layers 1 to upto end (0: all): the moves drawn are those before. */
int gt_end(const gt_path *p, int upto);
/* The frame a picture of p is drawn in: the plate's (part 0), or fitted
 * to what the moves lay down (part 1). cam is not NULL. 0, or -1. */
int gt_frame(const gt_path *p, const gs_cam *cam, int part, int w, int h, gs_frame *f);

/* The toolpath over the plate, framed as gs_view_bed frames it (cam NULL:
 * the home view), into rgb, w*h*3. Layers 1 to upto (0: all), each line
 * in its kind's colour, darker the lower it is, nearer lines in front.
 * The pen drawer's picture of a slice. 0, or -1. */
int gt_paint(const gt_path *p, const gs_cam *cam, int upto, unsigned char *rgb, int w, int h);
/* The same, framed to the part (what the moves lay down) over a plain
 * background instead of the plate, for a close look. */
int gt_paint_part(const gt_path *p, const gs_cam *cam, int upto, unsigned char *rgb, int w, int h);
/* The moves drawn onto a picture that already holds the solid, in that
 * solid's plate frame. The lines are laid on; the picture is not cleared.
 * 0, or -1. */
int gt_paint_on(const gt_path *p, const gs_mesh *solid, const gs_cam *cam, int upto, unsigned char *rgb, int w, int h);

#endif
