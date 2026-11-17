/* An SVG to a laser toolpath and its Grbl G-code (burn_job.h).
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "burn_job.h"

#include "gs_toolpath.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void burn_defaults(burn_opts *o)
{
    memset(o, 0, sizeof *o);
    o->size = (int)sizeof *o;
    o->power = 50.f;
    o->speed = 20.f;
    o->passes = 1;
    o->s100 = 1000.f;
}

static float power_of(const burn_opts *o)
{
    float p = o->power;
    if (p <= 0.f) p = 50.f;
    if (p > 100.f) p = 100.f;
    return p;
}

static int passes_of(const burn_opts *o)
{
    return o->passes > 1 ? o->passes : 1;
}

/* Follow one path, starting with a travel to its first point. */
static int add_path(gt_path *p, const gv_path *v, float power)
{
    int i;
    if (v->n < 2) return 0;
    if (gt_add(p, GT_TRAVEL, v->xy[0], v->xy[1], 0.f, 0.f)) return -1;
    for (i = 1; i < v->n; i++)
        if (gt_add(p, GT_CUT, v->xy[i * 2], v->xy[i * 2 + 1], 0.f, power)) return -1;
    if (v->closed) {
        if (gt_add(p, GT_CUT, v->xy[0], v->xy[1], 0.f, power)) return -1;
    }
    return 0;
}

int burn_path(gt_path *p, const gv_doc *d, const burn_opts *o)
{
    float power = power_of(o);
    int pass, npass = passes_of(o), L, i;
    gt_free(p);
    gt_init(p);
    for (pass = 0; pass < npass; pass++) {
        for (L = 0; L < d->nlayer; L++) {
            unsigned c = d->layer[L].colour;
            if (c == GV_NONE) continue;
            for (i = 0; i < d->n; i++) {
                if (gv_path_colour(&d->p[i]) != c) continue;
                if (add_path(p, &d->p[i], power)) {
                    gt_free(p);
                    return -1;
                }
            }
        }
    }
    return 0;
}

static long to_file(const char *s, long n, void *ud)
{
    return (long)fwrite(s, 1, (size_t)n, (FILE *)ud);
}

int burn_write(const char *svg, const char *gcode, const om_machine *m, const burn_opts *o, gc_stats *st, char *err,
               int errlen)
{
    gv_doc d;
    gt_path p;
    gc_laser L;
    om_job job;
    gc_stats mine;
    char why[128];
    FILE *f;
    int rc;
    if (!m || m->family != OM_LASER) {
        snprintf(err, (size_t)errlen, "%s is not a laser", (m && m->name[0]) ? m->name : "the machine");
        return -1;
    }
    gv_doc_init(&d);
    gt_init(&p);
    if (gv_load_svg(&d, svg, err, errlen)) {
        gv_doc_free(&d);
        return -1;
    }
    if (burn_path(&p, &d, o)) {
        snprintf(err, (size_t)errlen, "out of memory");
        gv_doc_free(&d);
        gt_free(&p);
        return -1;
    }
    if (p.n < 2) {
        snprintf(err, (size_t)errlen, "no paths to burn");
        gv_doc_free(&d);
        gt_free(&p);
        return -1;
    }
    gc_laser_defaults(&L);
    L.constant = o->constant;
    L.s100 = o->s100;
    L.speed = o->speed;
    L.travel = o->travel;
    gc_laser_job(&p, m, &L, &job);
    if (om_check(m, &job, why, (int)sizeof why) & ~OM_TOO_FAST) {
        snprintf(err, (size_t)errlen, "%.60s cannot burn this: %s", m->name, why);
        gv_doc_free(&d);
        gt_free(&p);
        return -1;
    }
    f = fopen(gcode, "w");
    if (!f) {
        snprintf(err, (size_t)errlen, "%.120s: cannot write", gcode);
        gv_doc_free(&d);
        gt_free(&p);
        return -1;
    }
    rc = gc_laser_write_to(to_file, f, &p, m, &L, st ? st : &mine, err, errlen);
    if (fclose(f) && !rc) {
        snprintf(err, (size_t)errlen, "%.120s: cannot write", gcode);
        rc = -1;
    }
    if (rc) {
        remove(gcode);
        rc = -1;
    }
    gv_doc_free(&d);
    gt_free(&p);
    return rc;
}
