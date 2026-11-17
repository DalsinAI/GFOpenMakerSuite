/* Written by core/library/gen-library.py from openmakegcode_lib.sfd: do not edit. */
#ifndef INLINE_OPENMAKEGCODE_H
#define INLINE_OPENMAKEGCODE_H

#include "omlib.h"
#include <gs_gcode.h>

extern struct Library *OpenMakeGcodeBase;

static __inline__ void GC_Defaults(gc_opts *o)
{
#ifdef OM_LIB_HOST
    ((void (*)(gc_opts *, struct Library *))om_host_vector(OpenMakeGcodeBase, 30))(o, OpenMakeGcodeBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeGcodeBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(o);
    __asm__ volatile("jsr a6@(-30:W)" : "=r"(_d0), "+r"(_a0) : "r"(_a6) : "d1", "a1", "fp0", "fp1", "cc", "memory");
#endif
}

static __inline__ void GC_Estimate(const gt_path *p, const om_machine *m, const gc_opts *o, gc_stats *st)
{
#ifdef OM_LIB_HOST
    ((void (*)(const gt_path *, const om_machine *, const gc_opts *, gc_stats *, struct Library *))om_host_vector(OpenMakeGcodeBase, 36))(p, m, o, st, OpenMakeGcodeBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeGcodeBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(p);
    register ULONG _a1 __asm("a1") = (ULONG)(m);
    register ULONG _a2 __asm("a2") = (ULONG)(o);
    register ULONG _a3 __asm("a3") = (ULONG)(st);
    __asm__ volatile("jsr a6@(-36:W)" : "=r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6), "r"(_a2), "r"(_a3) : "d1", "fp0", "fp1", "cc", "memory");
#endif
}

static __inline__ void GC_Job(const gt_path *p, const om_machine *m, const gc_opts *o, om_job *job)
{
#ifdef OM_LIB_HOST
    ((void (*)(const gt_path *, const om_machine *, const gc_opts *, om_job *, struct Library *))om_host_vector(OpenMakeGcodeBase, 42))(p, m, o, job, OpenMakeGcodeBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeGcodeBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(p);
    register ULONG _a1 __asm("a1") = (ULONG)(m);
    register ULONG _a2 __asm("a2") = (ULONG)(o);
    register ULONG _a3 __asm("a3") = (ULONG)(job);
    __asm__ volatile("jsr a6@(-42:W)" : "=r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6), "r"(_a2), "r"(_a3) : "d1", "fp0", "fp1", "cc", "memory");
#endif
}

static __inline__ int GC_WriteTo(gc_put put, void *ud, const gt_path *p, const om_machine *m, const gc_opts *o, gc_stats *st, char *err, int errlen)
{
#ifdef OM_LIB_HOST
    return ((int (*)(gc_put, void *, const gt_path *, const om_machine *, const gc_opts *, gc_stats *, char *, int, struct Library *))om_host_vector(OpenMakeGcodeBase, 48))(put, ud, p, m, o, st, err, errlen, OpenMakeGcodeBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeGcodeBase;
    register ULONG _a0 __asm("a0") = (ULONG)(put);
    register ULONG _a1 __asm("a1") = (ULONG)(ud);
    register ULONG _a2 __asm("a2") = (ULONG)(p);
    register ULONG _a3 __asm("a3") = (ULONG)(m);
    register ULONG _d0 __asm("d0") = (ULONG)(o);
    register ULONG _d1 __asm("d1") = (ULONG)(st);
    register ULONG _d2 __asm("d2") = (ULONG)(err);
    register ULONG _d3 __asm("d3") = (ULONG)(errlen);
    __asm__ volatile("jsr a6@(-48:W)" : "+r"(_d0), "+r"(_d1), "+r"(_a0), "+r"(_a1) : "r"(_a6), "r"(_a2), "r"(_a3), "r"(_d2), "r"(_d3) : "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GC_EstimateHow(const gt_path *p, const om_machine *m, const gc_opts *o, gc_stats *st)
{
#ifdef OM_LIB_HOST
    return ((int (*)(const gt_path *, const om_machine *, const gc_opts *, gc_stats *, struct Library *))om_host_vector(OpenMakeGcodeBase, 54))(p, m, o, st, OpenMakeGcodeBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeGcodeBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(p);
    register ULONG _a1 __asm("a1") = (ULONG)(m);
    register ULONG _a2 __asm("a2") = (ULONG)(o);
    register ULONG _a3 __asm("a3") = (ULONG)(st);
    __asm__ volatile("jsr a6@(-54:W)" : "=r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6), "r"(_a2), "r"(_a3) : "d1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

#endif
