/* The wrench's layers made on several tasks at once, joined in order: the
 * same toolpath, move for move, as one task makes. Built twice: with
 * par_posix.c (threads) and with par_one.c (all on the caller).
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gs_model.h"
#include "gs_par.h"
#include "gs_plan.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

static int fails;

static void expect(int cond, const char *msg)
{
    if (!cond) {
        fprintf(stderr, "fail: %s\n", msg);
        fails++;
    }
}

static int same_path(const gt_path *a, const gt_path *b)
{
    int i;
    if (a->n != b->n || a->nlayers != b->nlayers) return 0;
    for (i = 0; i < a->n; i++)
        if (a->m[i].x != b->m[i].x || a->m[i].y != b->m[i].y || a->m[i].z != b->m[i].z || a->m[i].w != b->m[i].w ||
            a->m[i].kind != b->m[i].kind)
            return 0;
    return !a->nlayers || !memcmp(a->layer, b->layer, (size_t)a->nlayers * sizeof *a->layer);
}

static int ran[64];

static void mark(void *arg, int i)
{
    (void)arg;
    ran[i]++;
}

static double now(void) { return (double)clock() / CLOCKS_PER_SEC; }

int main(void)
{
    static const int tasks[] = {1, 2, 3, 4, 8, 500};
    gs_mesh wrench;
    gp_opts o;
    gt_path one, many;
    char err[128];
    int i, k, on;
    double t;

    on = gs_par_run(64, mark, NULL);
    for (i = 0, k = 1; i < 64; i++)
        if (ran[i] != 1) k = 0;
    expect(k && on >= 1 && on <= 64, "gs_par_run runs each once");
    expect(gs_par_cores() >= 1, "at least the caller's core");
    printf("gs_par_run: 64 on %d task(s); %d core(s)\n", on, gs_par_cores());

    memset(&wrench, 0, sizeof wrench);
    expect(gs_stl_load(&wrench, "core/tests/data/nasa-wrench.stl", err, (int)sizeof err) == 0, "the wrench");
    gp_place(&wrench, 220.f, 220.f);
    gp_defaults(&o);
    gt_init(&one);
    gt_init(&many);
    t = now();
    expect(gp_part(&one, &wrench, &o) == 0 && one.nlayers == 130, "on one task");
    printf("one task: %d moves, %.2f s of CPU\n", one.n, now() - t);
    for (k = 0; k < (int)(sizeof tasks / sizeof tasks[0]); k++) {
        char what[64];
        gt_add(&many, GT_TRAVEL, 1, 2, 3, 0); /* replaced, as gp_part replaces */
        expect(gp_part_par(&many, &wrench, &o, tasks[k]) == 0, "made");
        snprintf(what, sizeof what, "%d tasks: the same toolpath", tasks[k]);
        expect(same_path(&one, &many), what);
    }
    {
        /* With the more (a grid, a skirt, top and bottom apart): the same. */
        gp_more x;
        gp_more_defaults(&x);
        x.pattern = GP_GRID;
        x.skirt = 2;
        x.gap = 3.f;
        x.top = 5;
        x.bottom = 2;
        expect(gp_part_more(&one, &wrench, &o, &x) == 0 && one.nlayers == 130, "the more, on one task");
        for (k = 0; k < (int)(sizeof tasks / sizeof tasks[0]); k++) {
            char what[64];
            expect(gp_part_par_more(&many, &wrench, &o, &x, tasks[k]) == 0, "made with the more");
            snprintf(what, sizeof what, "%d tasks with the more: the same toolpath", tasks[k]);
            expect(same_path(&one, &many), what);
        }
    }
    gt_free(&one);
    gt_free(&many);
    gs_mesh_free(&wrench);
    if (fails) {
        printf("%d failed\n", fails);
        return 1;
    }
    printf("plan par ok\n");
    return 0;
}
