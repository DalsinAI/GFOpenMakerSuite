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

/* The part's layers at o's height. */
int gp_layers(const gs_mesh *m, const gp_opts *o);

/* Layer n (1 is the first) as a new layer after p's, at n heights above
 * the bed. 0, or -1 out of memory. */
int gp_layer(gt_path *p, const gs_mesh *m, const gp_opts *o, int n);

#endif
