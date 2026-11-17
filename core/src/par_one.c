/* gs_par.h with no tasks: every piece on the caller, in order. The fallback
 * wherever tasks cannot be made. MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gs_par.h"

int gs_par_run(int n, gs_par_fn fn, void *arg)
{
    int i;
    for (i = 0; i < n; i++) fn(arg, i);
    return 1;
}

int gs_par_cores(void) { return 1; }
