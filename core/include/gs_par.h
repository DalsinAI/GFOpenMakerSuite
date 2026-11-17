/* Work spread over the machine's cores: n pieces, each run once, on as
 * many tasks as there are (threads on the host, exec tasks on GFOS), or all
 * on the caller where there is only one or tasks cannot be made. Which one
 * is linked decides: par_posix.c, par_one.c, (par_exec.c for the Amiga).
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef GS_PAR_H
#define GS_PAR_H

/* One piece of the work: i is 0 to n - 1. Pieces run at the same time, so
 * each touches only what is its own. */
typedef void (*gs_par_fn)(void *arg, int i);

/* fn(arg, i) for every i, each once, returning when all have: on how many
 * tasks they ran (1: all on the caller, in order). */
int gs_par_run(int n, gs_par_fn fn, void *arg);

/* The cores there are, 1 at least: how many tasks are worth making. */
int gs_par_cores(void);

#endif
