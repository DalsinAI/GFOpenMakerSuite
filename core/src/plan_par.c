/* gp_part_par: a part's layers on several tasks (gs_par.h), joined in
 * order. Each layer is worked out from the part alone (gs_plan.h), so task
 * t makes layers t, t + tasks, t + 2 tasks ... into paths of their own and
 * nothing is shared while they run. MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gs_par.h"
#include "gs_plan.h"

#include <stdlib.h>

typedef struct {
    const gs_mesh *m;
    const gp_opts *o;
    const gp_more *x;
    gt_path *layer;             /* one path a layer */
    int *bad;                   /* one a task */
    int layers, tasks;
} work;

static void make(void *arg, int t)
{
    work *w = arg;
    int n;
    for (n = t; n < w->layers; n += w->tasks)
        if (!w->bad[t] && gp_layer_more(&w->layer[n], w->m, w->o, w->x, n + 1)) w->bad[t] = 1;
}

int gp_part_par(gt_path *p, const gs_mesh *m, const gp_opts *o, int tasks)
{
    return gp_part_par_more(p, m, o, NULL, tasks);
}

int gp_part_par_more(gt_path *p, const gs_mesh *m, const gp_opts *o, const gp_more *x, int tasks)
{
    work w;
    int n, bad = 0;
    w.layers = gp_layers(m, o);
    if (tasks > w.layers) tasks = w.layers;
    if (tasks <= 1) return gp_part_more(p, m, o, x);
    w.m = m;
    w.o = o;
    w.x = x;
    w.tasks = tasks;
    w.layer = calloc((size_t)w.layers, sizeof *w.layer);
    w.bad = calloc((size_t)tasks, sizeof *w.bad);
    if (!w.layer || !w.bad) {
        free(w.layer);
        free(w.bad);
        return gp_part_more(p, m, o, x);
    }
    for (n = 0; n < w.layers; n++) gt_init(&w.layer[n]);
    gs_par_run(tasks, make, &w);
    gt_free(p);
    for (n = 0; n < tasks; n++) bad |= w.bad[n];
    for (n = 0; n < w.layers; n++) {
        if (!bad && gt_append(p, &w.layer[n])) bad = 1;
        gt_free(&w.layer[n]);
    }
    free(w.layer);
    free(w.bad);
    return bad ? -1 : 0;
}
