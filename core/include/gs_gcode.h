/* G-code for a printer from a toolpath (gs_toolpath.h): the machine's own
 * start and end code, a speed for each kind of move held to what the
 * machine can do, filament worked out from each line's width and height,
 * retraction over long travels, and how long and how much it will take.
 * The Amiga build is to be openmakegcode.library; this is its C API.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef GS_GCODE_H
#define GS_GCODE_H

#include "gs_toolpath.h"
#include "om_machine.h"

typedef struct {
    float hotend, bed;          /* degrees, for the start code's {hotend} and {bed} */
    float filament;             /* mm across */
    float layer_h;              /* mm: each line's height */
    float flow;                 /* 1 is as worked out */
    float speed[GT_KINDS];      /* mm/s for each kind; GT_TRAVEL's for travels */
    float retract;              /* mm of filament pulled back over a travel; 0: none */
    float retract_speed;        /* mm/s */
    float retract_min;          /* travels shorter than this, mm, do not retract */
} gc_opts;

typedef struct {
    float seconds;              /* moving, at the set speeds: no acceleration counted */
    float filament;             /* mm of filament */
    int moves;                  /* G0 and G1 lines */
} gc_stats;

void gc_defaults(gc_opts *o);

/* How long and how much, as gc_write would write it for m (m may be NULL:
 * the speeds are not held to a machine's). */
void gc_estimate(const gt_path *p, const om_machine *m, const gc_opts *o, gc_stats *st);

/* The job p is for om_check: where every move goes (travels too), the
 * fastest of each axis as gc_write would ask it of m (held to m's most,
 * so never too fast for m itself; m may be NULL), o's temperatures, one
 * extruder, and a heated bed when o heats one. */
void gc_job(const gt_path *p, const om_machine *m, const gc_opts *o, om_job *job);

/* p's G-code for the printer m to out, st (may be NULL) filled in. 0, or
 * -1 with why in err: m is not a printer, its code does not fit, or out
 * could not be written. */
int gc_write(FILE *out, const gt_path *p, const om_machine *m, const gc_opts *o, gc_stats *st, char *err, int errlen);

#endif
