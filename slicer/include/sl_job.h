/* OpenMakeSlicer's job, as the window and openmake-slice both run it: a
 * part loaded and set on the bed, sliced by the toolpath engine
 * (gp_part_par, on several tasks), its time and filament, and its G-code
 * written for a machine once the machine says it can do it.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef SL_JOB_H
#define SL_JOB_H

#include "gs_gcode.h"
#include "gs_plan.h"

typedef struct {
    gs_mesh part;              /* repaired, centred on the bed, on it */
    gt_path path;              /* the toolpath, once sliced */
    gc_stats st;               /* its estimate */
    int how;                   /* how its time was got: GC_AT_SPEEDS and so on */
    int have_part, sliced;
    int tasks;                 /* the tasks the last slice ran on */
} sl_job;

void sl_init(sl_job *j);
void sl_free(sl_job *j);

/* stl, repaired, set centred on a bed_x by bed_y bed; any slice dropped.
 * 0, or -1 with why in err (the part kept as it was). */
int sl_load(sl_job *j, const char *stl, float bed_x, float bed_y, char *err, int errlen);

/* The part sliced at po (its layer height held as gl_height holds it) on
 * up to tasks tasks, and estimated with go for m (m may be NULL). 0, or
 * -1 with why in err (no part, or out of memory: no slice then). */
int sl_slice(sl_job *j, const gp_opts *po, const gc_opts *go, const om_machine *m, int tasks, char *err, int errlen);

/* The slice's G-code for m to the file out, once om_check says m can do
 * it (too fast is fine: speeds are held to m's as they are written).
 * 0, or -1 with why in err ("<m> cannot print this: ..."). */
int sl_write(sl_job *j, const char *out, const om_machine *m, const gc_opts *go, char *err, int errlen);

#endif
