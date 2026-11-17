/* Written by core/library/gen-library.py from openmaketask_lib.sfd: do not edit. */
#ifndef INLINE_OPENMAKETASK_H
#define INLINE_OPENMAKETASK_H

#include "omlib.h"
#include <gs_par.h>

extern struct Library *OpenMakeTaskBase;

static __inline__ int GS_ParRun(int n, gs_par_fn fn, void *arg)
{
#ifdef OM_LIB_HOST
    return ((int (*)(int, gs_par_fn, void *, struct Library *))om_host_vector(OpenMakeTaskBase, 30))(n, fn, arg, OpenMakeTaskBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeTaskBase;
    register ULONG _d0 __asm("d0") = (ULONG)(n);
    register ULONG _a0 __asm("a0") = (ULONG)(fn);
    register ULONG _a1 __asm("a1") = (ULONG)(arg);
    __asm__ volatile("jsr a6@(-30:W)" : "+r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6) : "d1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GS_ParCores(void)
{
#ifdef OM_LIB_HOST
    return ((int (*)(struct Library *))om_host_vector(OpenMakeTaskBase, 36))(OpenMakeTaskBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeTaskBase;
    register ULONG _d0 __asm("d0");
    __asm__ volatile("jsr a6@(-36:W)" : "=r"(_d0) : "r"(_a6) : "d1", "a0", "a1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

#endif
