/* gs_par.h on POSIX threads: a thread a piece but the first, which runs on
 * the caller. A thread that cannot be made: its piece runs on the caller
 * too. MIT, Copyright (c) 2026 AmiCradle Team. */
#define _DEFAULT_SOURCE
#include "gs_par.h"

#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct {
    gs_par_fn fn;
    void *arg;
    int i;
    pthread_t t;
    int started;
} piece;

static void *run_piece(void *v)
{
    piece *p = v;
    p->fn(p->arg, p->i);
    return NULL;
}

int gs_par_run(int n, gs_par_fn fn, void *arg)
{
    piece *ps;
    int i, tasks = 1;
    if (n <= 0) return 1;
    ps = calloc((size_t)n, sizeof *ps);
    if (!ps) {
        for (i = 0; i < n; i++) fn(arg, i);
        return 1;
    }
    for (i = 1; i < n; i++) {
        ps[i].fn = fn;
        ps[i].arg = arg;
        ps[i].i = i;
        ps[i].started = pthread_create(&ps[i].t, NULL, run_piece, &ps[i]) == 0;
        tasks += ps[i].started;
    }
    fn(arg, 0);
    for (i = 1; i < n; i++) {
        if (ps[i].started)
            pthread_join(ps[i].t, NULL);
        else
            fn(arg, i);
    }
    free(ps);
    return tasks;
}

int gs_par_cores(void)
{
    long n = sysconf(_SC_NPROCESSORS_ONLN);
    return n > 1 ? (int)n : 1;
}
