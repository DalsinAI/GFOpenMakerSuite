/* Written by core/library/gen-library.py from openmaketask_lib.sfd: do not edit.
 * The library's side of each call: its registers to the C function. */
#include "omlib.h"
#include <gs_par.h>

static int LIB_GS_ParRun(REG(d0, int n), REG(a0, gs_par_fn fn), REG(a1, void *arg), REG(a6, struct Library *base))
{
    (void)base;
    return gs_par_run(n, fn, arg);
}

static int LIB_GS_ParCores(REG(a6, struct Library *base))
{
    (void)base;
    return gs_par_cores();
}

#define OPENMAKETASK_LAST_BIAS 36
#define OPENMAKETASK_VECTORS \
    (APTR)LIB_GS_ParRun, \
    (APTR)LIB_GS_ParCores
