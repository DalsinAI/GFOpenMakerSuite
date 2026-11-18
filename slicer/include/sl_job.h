/* OpenMakeSlicer's job, as the window and openmake-slice both run it: a
 * part loaded and set on the bed, sliced by the toolpath engine
 * (gp_part_par, on several tasks), its time and filament, and its G-code
 * written for a machine once the machine says it can do it.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef SL_JOB_H
#define SL_JOB_H

#include "gs_gcode.h"
#include "gs_plan.h"
#include "gs_settings.h"

typedef struct {
    gs_mesh part;              /* repaired, centred on the bed, on it */
    gt_path path;              /* the toolpath, once sliced */
    gc_stats st;               /* its estimate */
    int how;                   /* how its time was got: GC_AT_SPEEDS and so on */
    gs_settings set;           /* what it was sliced with, the layer height held */
    int have_part, sliced;
    int tasks;                 /* the tasks the last slice ran on */
    char start[240], end[240]; /* the user's own G-code, around the machine's */
} sl_job;

void sl_init(sl_job *j);
void sl_free(sl_job *j);

/* Lines of the user's own, after the machine's start and before its end.
 * NULL leaves that one as it was. An empty string clears it. */
void sl_own(sl_job *j, const char *start, const char *end);

/* stl, repaired, set centred on a bed_x by bed_y bed; any slice dropped.
 * 0, or -1 with why in err (the part kept as it was). */
int sl_load(sl_job *j, const char *stl, float bed_x, float bed_y, char *err, int errlen);

/* The part sliced with s (its layer height held as gl_height holds it)
 * on up to tasks tasks, and estimated for m (m may be NULL); s kept in
 * j->set. 0, or -1 with why in err (no part, or out of memory: no slice
 * then). */
int sl_slice(sl_job *j, const gs_settings *s, const om_machine *m, int tasks, char *err, int errlen);

/* The slice's G-code, with the settings it was sliced with, for m to the
 * file out, once om_check says m can do it (too fast is fine: speeds are
 * held to m's as they are written). 0, or -1 with why in err ("<m>
 * cannot print this: ..."). */
int sl_write(sl_job *j, const char *out, const om_machine *m, char *err, int errlen);

#endif
