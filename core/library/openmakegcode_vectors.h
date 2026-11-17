/* Written by core/library/gen-library.py from openmakegcode_lib.sfd: do not edit.
 * The library's side of each call: its registers to the C function. */
#include "omlib.h"
#include <gs_gcode.h>

static void LIB_GC_Defaults(REG(a0, gc_opts *o), REG(a6, struct Library *base))
{
    (void)base;
    gc_defaults(o);
}

static void LIB_GC_Estimate(REG(a0, const gt_path *p), REG(a1, const om_machine *m), REG(a2, const gc_opts *o), REG(a3, gc_stats *st), REG(a6, struct Library *base))
{
    (void)base;
    gc_estimate(p, m, o, st);
}

static void LIB_GC_Job(REG(a0, const gt_path *p), REG(a1, const om_machine *m), REG(a2, const gc_opts *o), REG(a3, om_job *job), REG(a6, struct Library *base))
{
    (void)base;
    gc_job(p, m, o, job);
}

static int LIB_GC_WriteTo(REG(a0, gc_put put), REG(a1, void *ud), REG(a2, const gt_path *p), REG(a3, const om_machine *m), REG(d0, const gc_opts *o), REG(d1, gc_stats *st), REG(d2, char *err), REG(d3, int errlen), REG(a6, struct Library *base))
{
    (void)base;
    return gc_write_to(put, ud, p, m, o, st, err, errlen);
}

static int LIB_GC_EstimateHow(REG(a0, const gt_path *p), REG(a1, const om_machine *m), REG(a2, const gc_opts *o), REG(a3, gc_stats *st), REG(a6, struct Library *base))
{
    (void)base;
    return gc_estimate_how(p, m, o, st);
}

static void LIB_GC_MoreDefaults(REG(a0, gc_more *x), REG(a1, const gc_opts *o), REG(a6, struct Library *base))
{
    (void)base;
    gc_more_defaults(x, o);
}

static int LIB_GC_EstimateMore(REG(a0, const gt_path *p), REG(a1, const om_machine *m), REG(a2, const gc_more *x), REG(a3, gc_stats *st), REG(a6, struct Library *base))
{
    (void)base;
    return gc_estimate_more(p, m, x, st);
}

static int LIB_GC_WriteMore(REG(a0, gc_put put), REG(a1, void *ud), REG(a2, const gt_path *p), REG(a3, const om_machine *m), REG(d0, const gc_more *x), REG(d1, gc_stats *st), REG(d2, char *err), REG(d3, int errlen), REG(a6, struct Library *base))
{
    (void)base;
    return gc_write_more(put, ud, p, m, x, st, err, errlen);
}

#define OPENMAKEGCODE_LAST_BIAS 72
#define OPENMAKEGCODE_VECTORS \
    (APTR)LIB_GC_Defaults, \
    (APTR)LIB_GC_Estimate, \
    (APTR)LIB_GC_Job, \
    (APTR)LIB_GC_WriteTo, \
    (APTR)LIB_GC_EstimateHow, \
    (APTR)LIB_GC_MoreDefaults, \
    (APTR)LIB_GC_EstimateMore, \
    (APTR)LIB_GC_WriteMore
