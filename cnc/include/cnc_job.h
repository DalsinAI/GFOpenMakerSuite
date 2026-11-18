/* OpenMakeCNC's job: a drawing to Grbl mill G-code, through openmakevec
 * and the shared loops. Profile follows each closed shape, the tool
 * outside it by its radius, so the drawn size is the finished size.
 * Pocket clears the inside, the tool kept in, one step-over at a time.
 * Drill plunges at the centre of each closed shape. A pass is one depth;
 * stepdown splits the depth. MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef CNC_JOB_H
#define CNC_JOB_H

#include "gs_gcode.h"
#include "gs_vec.h"
#include "om_machine.h"

enum { CNC_PROFILE = 0, CNC_POCKET = 1, CNC_DRILL = 2 };

typedef struct {
    int size;
    float tool;     /* mm across; 0: follow the line (profile) */
    float depth;    /* mm into the work; 0: 1 */
    float stepdown; /* mm per pass; 0: the whole depth */
    float stepover; /* mm between pocket loops; 0: 40% of the tool */
    float feed;     /* mm/s along a cut; 0: 8 */
    float plunge;   /* mm/s down; 0: 3 */
    float safe;     /* mm above the surface for a travel; 0: 5 */
    float rpm;      /* spindle; 0: 10000 */
    int mode;       /* CNC_PROFILE, CNC_POCKET or CNC_DRILL */
    float x, y;     /* mm to move the drawing; 0: leave it */
} cnc_opts;

void cnc_defaults(cnc_opts *o);

/* profile, pocket, drill, or -1. */
int cnc_mode_parse(const char *s);
const char *cnc_mode_name(int mode);

/* The moves, travels at the safe height. 0, or -1 with why in err. */
int cnc_path(gt_path *p, const gv_doc *d, const cnc_opts *o, char *err, int errlen);

/* drawing (SVG or DXF) to gcode for the mill m. 0, or -1 with why in err. */
int cnc_write(const char *drawing, const char *gcode, const om_machine *m, const cnc_opts *o, gc_stats *st, char *err,
              int errlen);

#endif
