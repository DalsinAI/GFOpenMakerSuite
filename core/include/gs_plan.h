/* A printer's layer from a part: brim, walls, then skin and infill, as a
 * toolpath. Each layer is worked out from the part alone (it cuts the
 * layers round it again for its skin), so any layer can be made on its
 * own, in any order, on any task. MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef GS_PLAN_H
#define GS_PLAN_H

#include "gs_toolpath.h"

typedef struct {
    float layer_h;      /* mm, held to 0.08 to 0.8 */
    float line_w;       /* mm */
    int walls;          /* round each loop, 1 to 8 */
    int skins;          /* solid layers at the top and the bottom */
    float infill;       /* % of the inside, 0 to 100 */
    int brim;           /* loops round the part on the first layer */
} gp_opts;

void gp_defaults(gp_opts *o);

enum { GP_LINES, GP_GRID };     /* infill patterns */

/* More for a layer than gp_opts says, taken by the _more calls. A
 * program sets size to sizeof (gp_more) as it was built: fields past its
 * size are as gp_more_defaults gives them, so this may grow at its end. */
typedef struct {
    int size;
    int pattern;        /* GP_LINES: one way, across each layer the next's; GP_GRID: both ways on each */
    int top, bottom;    /* solid layers at the top and the bottom, 0 to 8; -1: gp_opts' skins */
    int skirt;          /* loops round the part on the first layer, apart from it */
    float gap;          /* mm between the part and its brim or skirt */
    int support;        /* 0: none. Not 0: one wall under an overhang steeper than
                         * 45 degrees, from the bed up to it. Drawn as a brim line,
                         * so a new kind does not grow gc_opts. A sliver thinner
                         * than the line is left out. */
} gp_more;

/* As gp_opts alone: lines, its skins top and bottom, no skirt, no gap. */
void gp_more_defaults(gp_more *x);

/* The part's layers at o's height. */
int gp_layers(const gs_mesh *m, const gp_opts *o);

/* Layer n (1 is the first) as a new layer after p's, at n heights above
 * the bed. 0, or -1 out of memory. */
int gp_layer(gt_path *p, const gs_mesh *m, const gp_opts *o, int n);

/* Moves m to sit on the bed, its lowest point at z 0, centred on a bed
 * bed_x by bed_y mm. */
void gp_place(gs_mesh *m, float bed_x, float bed_y);

/* Every layer of m, in order, replacing p. 0, or -1 out of memory. */
int gp_part(gt_path *p, const gs_mesh *m, const gp_opts *o);

/* gp_part on tasks tasks at once (gs_par.h; plan_par.c): the same
 * toolpath, move for move. tasks 1 or less, or no memory for the layers
 * apart: gp_part itself. 0, or -1 out of memory. */
int gp_part_par(gt_path *p, const gs_mesh *m, const gp_opts *o, int tasks);

/* gp_layer, gp_part and gp_part_par with the more x (NULL: none). */
int gp_layer_more(gt_path *p, const gs_mesh *m, const gp_opts *o, const gp_more *x, int n);
int gp_part_more(gt_path *p, const gs_mesh *m, const gp_opts *o, const gp_more *x);
int gp_part_par_more(gt_path *p, const gs_mesh *m, const gp_opts *o, const gp_more *x, int tasks);

#endif
