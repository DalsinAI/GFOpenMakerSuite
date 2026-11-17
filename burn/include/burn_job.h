/* OpenMakeBurn's job: an SVG to Grbl laser G-code, through openmakevec
 * and openmakegcode. Line mode for this cut: each path is followed, in
 * layer order (the colour as it first appears). MIT, Copyright (c) 2026
 * AmiCradle Team. */
#ifndef BURN_JOB_H
#define BURN_JOB_H

#include "gs_gcode.h"
#include "gs_vec.h"
#include "om_machine.h"

typedef struct {
    int size;
    float power;  /* 0 to 100, every layer; 0: 50 */
    float speed;  /* mm/s for a cut; 0: 20 */
    float travel; /* mm/s for a travel; 0: the machine's most X, or 150 */
    int passes;   /* 0 or 1: once */
    int constant; /* 0: M4, not 0: M3 */
    float s100;   /* S at 100%; 0: 1000 */
} burn_opts;

void burn_defaults(burn_opts *o);

/* Replaces p with the line paths of d, each cut at o's power, o->passes
 * times, in layer order. 0, or -1 out of memory. */
int burn_path(gt_path *p, const gv_doc *d, const burn_opts *o);

/* Load svg, build the path, check it against m, write gcode. 0, or -1
 * with why in err. st (may be NULL) is filled as the writer left it. */
int burn_write(const char *svg, const char *gcode, const om_machine *m, const burn_opts *o, gc_stats *st, char *err,
               int errlen);

#endif
