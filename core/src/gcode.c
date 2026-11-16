/* G-code for a printer (gs_gcode.h). E counts from 0 again on each layer
 * (M82, absolute), so a float holds it to a ten-thousandth of a mm.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gs_gcode.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

void gc_defaults(gc_opts *o)
{
    memset(o, 0, sizeof *o);
    o->hotend = 200.f;
    o->bed = 60.f;
    o->filament = 1.75f;
    o->layer_h = 0.2f;
    o->flow = 1.f;
    o->speed[GT_TRAVEL] = 150.f;
    o->speed[GT_OUTER] = 25.f;
    o->speed[GT_INNER] = 45.f;
    o->speed[GT_SKIN] = 40.f;
    o->speed[GT_INFILL] = 60.f;
    o->speed[GT_BRIM] = 30.f;
    o->speed[GT_CUT] = 20.f;
    o->retract = 1.f;
    o->retract_speed = 35.f;
    o->retract_min = 2.f;
}

/* v held to the machine's most on an axis; no machine or no most: v. */
static float held(const om_machine *m, int axis, float v)
{
    return m && m->speed[axis] > 0.f && v > m->speed[axis] ? m->speed[axis] : v;
}

typedef struct {
    FILE *out;                  /* NULL: only counting */
    const om_machine *m;
    const gc_opts *o;
    gc_stats *st;
    float e;                    /* E now, since the layer's G92 */
    int bad;
} writer;

static void say(writer *w, const char *line)
{
    size_t n = strlen(line);
    if (w->out && fwrite(line, 1, n, w->out) != n) w->bad = 1;
}

/* A pull back (sign -1) or its return (+1). */
static void retract(writer *w, int sign)
{
    char buf[64];
    float v = held(w->m, OM_E, w->o->retract_speed);
    w->e += (float)sign * w->o->retract;
    snprintf(buf, sizeof buf, "G1 E%.5f F%d\n", (double)w->e, (int)(v * 60.f + 0.5f));
    say(w, buf);
    w->st->moves++;
    w->st->seconds += w->o->retract / v;
}

/* The move to b from a (NULL: the first, which only places the head). */
static void move(writer *w, const gt_move *a, const gt_move *b)
{
    char buf[128];
    float dx = a ? b->x - a->x : 0.f, dy = a ? b->y - a->y : 0.f, dz = a ? b->z - a->z : 0.f;
    float len = sqrtf(dx * dx + dy * dy + dz * dz), v = held(w->m, OM_X, w->o->speed[b->kind]), t = 0.f;
    float area = 3.14159265f * w->o->filament * w->o->filament * 0.25f;
    int travel = b->kind == GT_TRAVEL;
    if (v <= 0.f) v = 10.f;
    if (len > 0.f) {
        float vz = held(w->m, OM_Z, v);
        t = len / v;
        if (dz != 0.f && fabsf(dz) / vz > t) t = fabsf(dz) / vz;
        v = len / t;
    }
    if (travel && w->o->retract > 0.f && len >= w->o->retract_min) retract(w, -1);
    if (!travel) {
        float de = len * b->w * w->o->layer_h * w->o->flow / area;
        w->e += de;
        w->st->filament += de;
        snprintf(buf, sizeof buf, "G1 X%.3f Y%.3f Z%.3f E%.5f F%d\n", (double)b->x, (double)b->y, (double)b->z,
                 (double)w->e, (int)(v * 60.f + 0.5f));
    } else {
        snprintf(buf, sizeof buf, "G0 X%.3f Y%.3f Z%.3f F%d\n", (double)b->x, (double)b->y, (double)b->z,
                 (int)(v * 60.f + 0.5f));
    }
    say(w, buf);
    w->st->moves++;
    w->st->seconds += t;
    if (travel && w->o->retract > 0.f && len >= w->o->retract_min) retract(w, 1);
}

static void run(writer *w, const gt_path *p)
{
    int i, layer = 0;
    char buf[48];
    memset(w->st, 0, sizeof *w->st);
    for (i = 0; i < p->n; i++) {
        if (layer < p->nlayers && p->layer[layer] == i) {
            layer++;
            snprintf(buf, sizeof buf, "; LAYER %d\nG92 E0\n", layer);
            say(w, buf);
            w->e = 0.f;
        }
        move(w, i ? &p->m[i - 1] : NULL, &p->m[i]);
    }
}

void gc_estimate(const gt_path *p, const om_machine *m, const gc_opts *o, gc_stats *st)
{
    writer w;
    memset(&w, 0, sizeof w);
    w.m = m;
    w.o = o;
    w.st = st;
    run(&w, p);
}

void gc_job(const gt_path *p, const om_machine *m, const gc_opts *o, om_job *job)
{
    int i, k;
    memset(job, 0, sizeof *job);
    for (i = 0; i < p->n; i++) {
        const gt_move *a = &p->m[i];
        const float v[3] = {a->x, a->y, a->z};
        float s = held(m, OM_X, o->speed[a->kind]);
        for (k = 0; k < 3; k++) {
            if (!i || v[k] < job->min[k]) job->min[k] = v[k];
            if (!i || v[k] > job->max[k]) job->max[k] = v[k];
        }
        if (s > job->speed[OM_X]) job->speed[OM_X] = job->speed[OM_Y] = s;
        if (i && a->z != p->m[i - 1].z) job->speed[OM_Z] = held(m, OM_Z, s);
    }
    if (o->retract > 0.f) job->speed[OM_E] = held(m, OM_E, o->retract_speed);
    job->hotend = o->hotend;
    job->bed = o->bed;
    job->extruders = 1;
    job->needs = o->bed > 0.f ? OM_HEATED_BED : 0u;
}

int gc_write(FILE *out, const gt_path *p, const om_machine *m, const gc_opts *o, gc_stats *st, char *err, int errlen)
{
    char code[OM_CODE + 256];
    gc_stats mine;
    writer w;
    if (m->family != OM_PRINTER) {
        snprintf(err, (size_t)errlen, "%s is not a printer: G-code here is for a printer", m->name);
        return -1;
    }
    memset(&w, 0, sizeof w);
    w.out = out;
    w.m = m;
    w.o = o;
    w.st = st ? st : &mine;
    if (om_code(m->start, o->hotend, o->bed, code, (int)sizeof code)) {
        snprintf(err, (size_t)errlen, "the machine's start code is too long");
        return -1;
    }
    say(&w, "; GFOpenMake\n");
    say(&w, code);
    say(&w, "\n");
    run(&w, p);
    if (om_code(m->end, o->hotend, o->bed, code, (int)sizeof code)) {
        snprintf(err, (size_t)errlen, "the machine's end code is too long");
        return -1;
    }
    say(&w, code);
    say(&w, "\n");
    if (w.bad) {
        snprintf(err, (size_t)errlen, "the G-code could not be written");
        return -1;
    }
    return 0;
}
