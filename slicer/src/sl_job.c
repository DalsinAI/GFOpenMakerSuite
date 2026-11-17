/* OpenMakeSlicer's job (sl_job.h).
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "sl_job.h"

#include "gs_layer.h"
#include "gs_par.h"

#include <stdio.h>
#include <string.h>

void sl_init(sl_job *j)
{
    memset(j, 0, sizeof *j);
    gt_init(&j->path);
}

static void drop_slice(sl_job *j)
{
    gt_free(&j->path);
    gt_init(&j->path);
    memset(&j->st, 0, sizeof j->st);
    j->how = GC_AT_SPEEDS;
    j->sliced = 0;
    j->tasks = 0;
}

void sl_free(sl_job *j)
{
    drop_slice(j);
    gs_mesh_free(&j->part);
    memset(j, 0, sizeof *j);
    gt_init(&j->path);
}

int sl_load(sl_job *j, const char *stl, float bed_x, float bed_y, char *err, int errlen)
{
    gs_mesh m;
    memset(&m, 0, sizeof m);
    if (gs_stl_load(&m, stl, err, errlen)) return -1;
    gs_mesh_repair(&m);
    gp_place(&m, bed_x, bed_y);
    drop_slice(j);
    gs_mesh_free(&j->part);
    j->part = m;
    j->have_part = 1;
    return 0;
}

int sl_slice(sl_job *j, const gp_opts *po, const gc_opts *go, const om_machine *m, int tasks, char *err, int errlen)
{
    gp_opts o = *po;
    gc_opts g = *go;
    int layers;
    drop_slice(j);
    if (!j->have_part) {
        snprintf(err, (size_t)errlen, "Open an STL first");
        return -1;
    }
    o.layer_h = gl_height(o.layer_h);
    g.layer_h = o.layer_h;
    layers = gp_layers(&j->part, &o);
    if (tasks > layers) tasks = layers; /* as gp_part_par does */
    if (tasks < 1) tasks = 1;
    if (gp_part_par(&j->path, &j->part, &o, tasks) || !j->path.n) {
        drop_slice(j);
        snprintf(err, (size_t)errlen, "That did not slice: out of memory, or nothing to slice");
        return -1;
    }
    j->how = gc_estimate_how(&j->path, m, &g, &j->st);
    j->sliced = 1;
    j->tasks = tasks;
    return 0;
}

int sl_write(sl_job *j, const char *out, const om_machine *m, const gc_opts *go, char *err, int errlen)
{
    om_job job;
    char why[128];
    FILE *f;
    int rc;
    if (!j->sliced) {
        snprintf(err, (size_t)errlen, "Slice first");
        return -1;
    }
    gc_job(&j->path, m, go, &job);
    if (om_check(m, &job, why, (int)sizeof why) & ~OM_TOO_FAST) {
        snprintf(err, (size_t)errlen, "%.60s cannot print this: %s", m->name, why);
        return -1;
    }
    f = fopen(out, "w");
    if (!f) {
        snprintf(err, (size_t)errlen, "%.120s: cannot write", out);
        return -1;
    }
    rc = gc_write(f, &j->path, m, go, &j->st, err, errlen);
    if (fclose(f) && !rc) {
        snprintf(err, (size_t)errlen, "%.120s: cannot write", out);
        rc = -1;
    }
    return rc ? -1 : 0;
}
