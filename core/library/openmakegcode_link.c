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
