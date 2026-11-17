/* G-code for a printer (gs_gcode.h). E counts from 0 again on each layer
 * (M82, absolute), so a float holds it to a ten-thousandth of a mm.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gs_gcode.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
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

/* The time, with acceleration: moves wait in a look-ahead of PLAN_N, as a
 * printer's planner holds them. Each may start no faster than the corner
 * into it allows (the jerk) and than it can still stop from by the end of
 * what is waiting; the oldest half is timed (speed up, run, slow down)
 * whenever the look-ahead is full, and the rest at the end. */
#define PLAN_N 64

typedef struct {
    float len, v, a;            /* mm, mm/s, mm/s2 */
    float ux, uy, uz;           /* along it, a unit; 0 0 0: a pull back, stopped either side */
    float most_in;              /* the fastest it may start: the corner into it */
} seg;

typedef struct {
    seg s[PLAN_N];
    int n;
    float in;                   /* the speed the first waiting move starts at */
    float seconds;
} planner;

/* A move of len from in to out (both at most v) with a: speeding up,
 * running at v, slowing down; or never reaching v. */
static float trapezoid(float len, float v, float a, float in, float out)
{
    float up = (v * v - in * in) / (2.f * a), down = (v * v - out * out) / (2.f * a), peak;
    if (up + down <= len) return (v - in) / a + (v - out) / a + (len - up - down) / v;
    peak = sqrtf((2.f * a * len + in * in + out * out) * 0.5f);
    return (peak - in) / a + (peak - out) / a;
}

/* Times the first k waiting moves; the last waiting one ends stopped. */
static void plan_flush(planner *pl, int k)
{
    float end[PLAN_N], v = 0.f, in = pl->in;
    int i;
    for (i = pl->n - 1; i >= 0; i--) { /* backwards: each must still be able to stop */
        const seg *g = &pl->s[i];
        end[i] = v;
        v = sqrtf(v * v + 2.f * g->a * g->len);
        if (v > g->most_in) v = g->most_in;
    }
    for (i = 0; i < k; i++) { /* forwards: each can only speed up so much */
        const seg *g = &pl->s[i];
        float out;
        if (in > g->most_in) in = g->most_in;
        out = sqrtf(in * in + 2.f * g->a * g->len);
        if (out > end[i]) out = end[i];
        if (out > g->v) out = g->v;
        pl->seconds += trapezoid(g->len, g->v, g->a, in, out);
        in = out;
    }
    memmove(pl->s, pl->s + k, (size_t)(pl->n - k) * sizeof *pl->s);
    pl->n -= k;
    pl->in = in;
}

static float smallest(float a, float b)
{
    return a > 0.f && (b <= 0.f || a < b) ? a : b;
}

/* A move for the planner: the corner into it from the last one waiting. */
static void plan_add(planner *pl, const om_machine *m, float len, float v, float dx, float dy, float dz, int pull)
{
    seg g;
    const seg *last = pl->n ? &pl->s[pl->n - 1] : NULL;
    float j = smallest(m->jerk[OM_X], m->jerk[OM_Y]);
    if (len <= 0.f || v <= 0.f) return;
    memset(&g, 0, sizeof g);
    g.len = len;
    g.v = v;
    if (pull) g.a = m->accel[OM_E];
    else {
        g.a = smallest(m->accel[OM_X], m->accel[OM_Y]);
        if (dz != 0.f) g.a = dx == 0.f && dy == 0.f ? m->accel[OM_Z] : smallest(g.a, m->accel[OM_Z]);
        g.ux = dx / len;
        g.uy = dy / len;
        g.uz = dz / len;
    }
    if (g.a <= 0.f) g.a = 1e9f; /* no acceleration given for it: at once */
    if (last && !pull && (last->ux != 0.f || last->uy != 0.f || last->uz != 0.f)) {
        float c = last->ux * g.ux + last->uy * g.uy + last->uz * g.uz, most = last->v < v ? last->v : v;
        float half = sqrtf((1.f - (c > 1.f ? 1.f : c)) * 0.5f); /* sin of half the turn */
        if (half < 1e-4f) g.most_in = most;
        else if (j > 0.f) g.most_in = j / (2.f * half) < most ? j / (2.f * half) : most;
    }
    if (pl->n == PLAN_N) plan_flush(pl, PLAN_N / 2);
    pl->s[pl->n++] = g;
}

typedef struct {
    gc_put put;                 /* NULL: only counting */
    void *ud;
    const om_machine *m;
    const gc_opts *o;
    gc_stats *st;
    float e;                    /* E now, since the layer's G92 */
    int bad;
    planner *pl;                /* NULL: no acceleration, distance over speed */
} writer;

static void say(writer *w, const char *line)
{
    long n = (long)strlen(line);
    if (w->put && !w->bad && w->put(line, n, w->ud) != n) w->bad = 1;
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
    if (w->pl) plan_add(w->pl, w->m, w->o->retract, v, 0.f, 0.f, 0.f, 1);
    else w->st->seconds += w->o->retract / v;
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
    if (w->pl) plan_add(w->pl, w->m, len, v, dx, dy, dz, 0);
    else w->st->seconds += t;
    if (travel && w->o->retract > 0.f && len >= w->o->retract_min) retract(w, 1);
}

/* The machine's code, ending its last line once. */
static void code_line(writer *w, const char *code)
{
    size_t n = strlen(code);
    say(w, code);
    if (n && code[n - 1] != '\n') say(w, "\n");
}

static void run(writer *w, const gt_path *p)
{
    int i, layer = 0;
    char buf[48];
    planner *pl = NULL;
    memset(w->st, 0, sizeof *w->st);
    /* The profile's acceleration counted when it gives X and Y theirs. */
    if (w->m && w->m->accel[OM_X] > 0.f && w->m->accel[OM_Y] > 0.f) {
        pl = (planner *)calloc(1, sizeof *pl);
        w->pl = pl;
    }
    for (i = 0; i < p->n; i++) {
        if (layer < p->nlayers && p->layer[layer] == i) {
            layer++;
            snprintf(buf, sizeof buf, "; LAYER %d\nG92 E0\n", layer);
            say(w, buf);
            w->e = 0.f;
        }
        move(w, i ? &p->m[i - 1] : NULL, &p->m[i]);
    }
    if (pl) {
        plan_flush(pl, pl->n);
        w->st->seconds = pl->seconds;
        free(pl);
        w->pl = NULL;
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

static long to_file(const char *s, long n, void *ud)
{
    return (long)fwrite(s, 1, (size_t)n, (FILE *)ud);
}

int gc_write(FILE *out, const gt_path *p, const om_machine *m, const gc_opts *o, gc_stats *st, char *err, int errlen)
{
    return gc_write_to(to_file, out, p, m, o, st, err, errlen);
}

int gc_write_to(gc_put put, void *ud, const gt_path *p, const om_machine *m, const gc_opts *o, gc_stats *st, char *err,
                int errlen)
{
    char code[OM_CODE + 256];
    gc_stats mine;
    writer w;
    if (m->family != OM_PRINTER) {
        snprintf(err, (size_t)errlen, "%s is not a printer: G-code here is for a printer", m->name);
        return -1;
    }
    memset(&w, 0, sizeof w);
    w.put = put;
    w.ud = ud;
    w.m = m;
    w.o = o;
    w.st = st ? st : &mine;
    if (om_code(m->start, o->hotend, o->bed, code, (int)sizeof code)) {
        snprintf(err, (size_t)errlen, "the machine's start code is too long");
        return -1;
    }
    say(&w, "; GFOpenMake\n");
    code_line(&w, code);
    run(&w, p);
    if (om_code(m->end, o->hotend, o->bed, code, (int)sizeof code)) {
        snprintf(err, (size_t)errlen, "the machine's end code is too long");
        return -1;
    }
    code_line(&w, code);
    if (w.bad) {
        snprintf(err, (size_t)errlen, "the G-code could not be written");
        return -1;
    }
    return 0;
}
