/* Written by core/library/gen-library.py from openmakegcode_lib.sfd: do not edit.
 * The C API through openmakegcode.library: a program links this instead of
 * the sources, and opens OpenMakeGcodeBase itself. */
#include <proto/openmakegcode.h>

void gc_defaults(gc_opts *o)
{
    GC_Defaults(o);
}

void gc_estimate(const gt_path *p, const om_machine *m, const gc_opts *o, gc_stats *st)
{
    GC_Estimate(p, m, o, st);
}

void gc_job(const gt_path *p, const om_machine *m, const gc_opts *o, om_job *job)
{
    GC_Job(p, m, o, job);
}

int gc_write_to(gc_put put, void *ud, const gt_path *p, const om_machine *m, const gc_opts *o, gc_stats *st, char *err, int errlen)
{
    return GC_WriteTo(put, ud, p, m, o, st, err, errlen);
}

int gc_estimate_how(const gt_path *p, const om_machine *m, const gc_opts *o, gc_stats *st)
{
    return GC_EstimateHow(p, m, o, st);
}

void gc_more_defaults(gc_more *x, const gc_opts *o)
{
    GC_MoreDefaults(x, o);
}

int gc_estimate_more(const gt_path *p, const om_machine *m, const gc_more *x, gc_stats *st)
{
    return GC_EstimateMore(p, m, x, st);
}

int gc_write_more(gc_put put, void *ud, const gt_path *p, const om_machine *m, const gc_more *x, gc_stats *st, char *err, int errlen)
{
    return GC_WriteMore(put, ud, p, m, x, st, err, errlen);
}

void gc_laser_defaults(gc_laser *l)
{
    GC_LaserDefaults(l);
}

void gc_laser_job(const gt_path *p, const om_machine *m, const gc_laser *l, om_job *job)
{
    GC_LaserJob(p, m, l, job);
}

int gc_laser_estimate_how(const gt_path *p, const om_machine *m, const gc_laser *l, gc_stats *st)
{
    return GC_LaserEstimateHow(p, m, l, st);
}

int gc_laser_write_to(gc_put put, void *ud, const gt_path *p, const om_machine *m, const gc_laser *l, gc_stats *st, char *err, int errlen)
{
    return GC_LaserWriteTo(put, ud, p, m, l, st, err, errlen);
}

void gc_laser_more_defaults(gc_laser_more *x, const gc_laser *l)
{
    GC_LaserMoreDefaults(x, l);
}

int gc_laser_estimate_more(const gt_path *p, const om_machine *m, const gc_laser_more *x, gc_stats *st)
{
    return GC_LaserEstimateMore(p, m, x, st);
}

int gc_laser_write_more(gc_put put, void *ud, const gt_path *p, const om_machine *m, const gc_laser_more *x, gc_stats *st, char *err, int errlen)
{
    return GC_LaserWriteMore(put, ud, p, m, x, st, err, errlen);
}
