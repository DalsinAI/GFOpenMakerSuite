/* Written by core/library/gen-library.py from openmaketask_lib.sfd: do not edit.
 * The C API through openmaketask.library: a program links this instead of
 * the sources, and opens OpenMakeTaskBase itself. */
#include <proto/openmaketask.h>

int gs_par_run(int n, gs_par_fn fn, void *arg)
{
    return GS_ParRun(n, fn, arg);
}

int gs_par_cores(void)
{
    return GS_ParCores();
}
