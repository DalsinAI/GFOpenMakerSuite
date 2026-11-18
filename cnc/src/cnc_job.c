/* OpenMakeCNC's job (cnc/include/cnc_job.h). MIT, Copyright (c) 2026 AmiCradle Team. */
#include "cnc_job.h"

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void cnc_defaults(cnc_opts *o)
{
    memset(o, 0, sizeof *o);
    o->size = (int)sizeof *o;
}

int cnc_mode_parse(const char *s)
{
    if (!s) return -1;
    if (!strcmp(s, "profile")) return CNC_PROFILE;
    if (!strcmp(s, "pocket")) return CNC_POCKET;
    if (!strcmp(s, "drill")) return CNC_DRILL;
    return -1;
}

const char *cnc_mode_name(int mode)
{
    if (mode == CNC_POCKET) return "pocket";
    if (mode == CNC_DRILL) return "drill";
    return "profile";
}

static void say(char *err, int errlen, const char *why)
{
    if (err && errlen > 0) snprintf(err, (size_t)errlen, "%s", why);
}

static float pick(float v, float d)
{
    return v > 0.f ? v : d;
}

/* size 0 is a caller from this build that did not set it. A shorter
 * size keeps the fields it has; start and end stay unset. */
static void take_opts(cnc_opts *u, const cnc_opts *o)
{
    int n;
    cnc_defaults(u);
    if (!o) return;
    n = o->size;
    if (n < (int)offsetof(cnc_opts, tool)) n = (int)sizeof *u;
    if (n > (int)sizeof *u) n = (int)sizeof *u;
    memcpy(u, o, (size_t)n);
    u->size = (int)sizeof *u;
}

static int opts_have(const cnc_opts *o, size_t off, size_t sz)
{
    return o && o->size >= (int)(off + sz);
}

/* Closed shapes, moved by x, y. Open paths are left out: a mill tool
 * here follows a closed edge, clears an inside, or plunges a centre. */
static int shapes(const gv_doc *d, float dx, float dy, gl_loops *out)
{
    int i, n = 0;
    memset(out, 0, sizeof *out);
    if (!d) return 0;
    for (i = 0; i < d->n; i++)
        if (d->p[i].closed && d->p[i].n >= 3) n++;
    if (!n) return 0;
    out->l = (gl_loop *)calloc((size_t)n, sizeof *out->l);
    if (!out->l) return -1;
    for (i = 0; i < d->n; i++) {
        const gv_path *v = &d->p[i];
        int np, k;
        float *xy;
        if (!v->closed || v->n < 3) continue;
        np = v->n;
        if (np >= 2 && v->xy[0] == v->xy[(np - 1) * 2] && v->xy[1] == v->xy[(np - 1) * 2 + 1]) np--;
        if (np < 3) continue;
        xy = (float *)malloc((size_t)np * 2 * sizeof *xy);
        if (!xy) {
            gl_loops_free(out);
            return -1;
        }
        for (k = 0; k < np; k++) {
            xy[2 * k] = v->xy[2 * k] + dx;
            xy[2 * k + 1] = v->xy[2 * k + 1] + dy;
        }
        out->l[out->n].xy = xy;
        out->l[out->n].n = np;
        out->l[out->n].closed = 1;
        out->n++;
    }
    return 0;
}

static void reverse_loop(gl_loop *l)
{
    int i, j;
    for (i = 0, j = l->n - 1; i < j; i++, j--) {
        float x = l->xy[i * 2], y = l->xy[i * 2 + 1];
        l->xy[i * 2] = l->xy[j * 2];
        l->xy[i * 2 + 1] = l->xy[j * 2 + 1];
        l->xy[j * 2] = x;
        l->xy[j * 2 + 1] = y;
    }
}

static int loop_contains(const gl_loop *l, float x, float y)
{
    int c = 0, i, j;
    for (i = 0, j = l->n - 1; i < l->n; j = i++) {
        float xi = l->xy[i * 2], yi = l->xy[i * 2 + 1];
        float xj = l->xy[j * 2], yj = l->xy[j * 2 + 1];
        if ((yi > y) != (yj > y)) {
            float xh = xi + (y - yi) / (yj - yi) * (xj - xi);
            if (x < xh) c++;
        }
    }
    return c & 1;
}

/* An outermost shape winds anticlockwise, a hole the other way, so an
 * inset shrinks the part and grows the hole. */
static void face(gl_loops *ls)
{
    int i, j, n;
    for (i = 0; i < ls->n; i++) {
        float x = 0.f, y = 0.f, a;
        int k;
        if (!ls->l[i].closed || ls->l[i].n < 3) continue;
        for (k = 0; k < ls->l[i].n; k++) {
            x += ls->l[i].xy[2 * k];
            y += ls->l[i].xy[2 * k + 1];
        }
        x /= (float)ls->l[i].n;
        y /= (float)ls->l[i].n;
        if (!loop_contains(&ls->l[i], x, y)) continue;
        n = 0;
        for (j = 0; j < ls->n; j++)
            if (j != i && ls->l[j].closed && loop_contains(&ls->l[j], x, y)) n++;
        a = gl_loop_area(&ls->l[i]);
        if ((n % 2 == 0 && a < 0.f) || (n % 2 == 1 && a > 0.f)) reverse_loop(&ls->l[i]);
    }
}

static int passes(float depth, float step, float *z, int cap)
{
    int n = 0;
    float d = 0.f;
    if (!(depth > 0.f)) depth = 1.f;
    if (!(step > 0.f) || step > depth) step = depth;
    while (d + 0.001f < depth && n < cap) {
        d += step;
        if (d > depth) d = depth;
        z[n++] = -d;
    }
    return n;
}

static int cut_loop(gt_path *p, const gl_loop *l, float z, float safe)
{
    int i;
    if (!l->closed || l->n < 3) return 0;
    if (gt_add(p, GT_TRAVEL, l->xy[0], l->xy[1], safe, 0.f)) return -1;
    if (gt_add(p, GT_CUT, l->xy[0], l->xy[1], z, 1.f)) return -1;
    for (i = 1; i < l->n; i++)
        if (gt_add(p, GT_CUT, l->xy[2 * i], l->xy[2 * i + 1], z, 1.f)) return -1;
    if (gt_add(p, GT_CUT, l->xy[0], l->xy[1], z, 1.f)) return -1;
    return gt_add(p, GT_TRAVEL, l->xy[0], l->xy[1], safe, 0.f);
}

static int drill_at(gt_path *p, float x, float y, float z, float safe)
{
    if (gt_add(p, GT_TRAVEL, x, y, safe, 0.f)) return -1;
    if (gt_add(p, GT_CUT, x, y, z, 1.f)) return -1;
    return gt_add(p, GT_TRAVEL, x, y, safe, 0.f);
}

static void centre(const gl_loop *l, float *x, float *y)
{
    int i;
    double sx = 0.0, sy = 0.0;
    for (i = 0; i < l->n; i++) {
        sx += l->xy[2 * i];
        sy += l->xy[2 * i + 1];
    }
    *x = (float)(sx / l->n);
    *y = (float)(sy / l->n);
}

static int cut_all(gt_path *p, const gl_loops *ls, const float *z, int nz, float safe)
{
    int a, b;
    for (a = 0; a < nz; a++)
        for (b = 0; b < ls->n; b++)
            if (cut_loop(p, &ls->l[b], z[a], safe)) return -1;
    return 0;
}

int cnc_path(gt_path *p, const gv_doc *d, const cnc_opts *o, char *err, int errlen)
{
    cnc_opts u;
    gl_loops raw, solid, off;
    float z[64], radius, over, safe;
    int nz, i, rc = 0;
    cnc_defaults(&u);
    take_opts(&u, o);
    radius = u.tool > 0.f ? u.tool * 0.5f : 0.f;
    over = u.stepover > 0.f ? u.stepover : (u.tool > 0.f ? u.tool * 0.4f : 0.f);
    safe = pick(u.safe, 5.f);
    nz = passes(u.depth, u.stepdown, z, 64);
    gt_free(p);
    gt_init(p);
    memset(&raw, 0, sizeof raw);
    memset(&solid, 0, sizeof solid);
    memset(&off, 0, sizeof off);
    if (shapes(d, u.x, u.y, &raw)) {
        say(err, errlen, "Out of memory.");
        return -1;
    }
    if (!raw.n) {
        say(err, errlen, "Nothing closed to cut.");
        gl_loops_free(&raw);
        return -1;
    }
    face(&raw);
    if (gl_union(&raw, &solid)) {
        say(err, errlen, "Out of memory.");
        gl_loops_free(&raw);
        return -1;
    }
    gl_loops_free(&raw);
    if (u.mode == CNC_DRILL) {
        for (i = 0; i < nz && !rc; i++) {
            int b;
            for (b = 0; b < solid.n && !rc; b++) {
                float x, y;
                if (gl_loop_area(&solid.l[b]) <= 0.f) continue; /* a hole is not a drill */
                centre(&solid.l[b], &x, &y);
                rc = drill_at(p, x, y, z[i], safe);
            }
        }
    } else if (u.mode == CNC_POCKET) {
        gl_loops ring[48];
        int nring = 0;
        memset(ring, 0, sizeof ring);
        if (!(u.tool > 0.f) || !(over > 0.f)) {
            say(err, errlen, "A pocket needs a tool diameter.");
            rc = -1;
        } else if (gl_inset(&solid, radius, &ring[0])) {
            say(err, errlen, "Out of memory.");
            rc = -1;
        } else if (!ring[0].n) {
            say(err, errlen, "The tool does not fit in that shape.");
            rc = -1;
        } else {
            nring = 1;
            while (nring < 48 && !rc) {
                if (gl_inset(&ring[nring - 1], over, &ring[nring])) {
                    say(err, errlen, "Out of memory.");
                    rc = -1;
                    break;
                }
                if (!ring[nring].n) {
                    gl_loops_free(&ring[nring]);
                    break;
                }
                nring++;
            }
            for (i = 0; i < nring && !rc; i++) rc = cut_all(p, &ring[i], z, nz, safe);
            if (nring == 48 && ring[47].n) {
                say(err, errlen, "That pocket needs more than 48 steps.");
                rc = -1;
            }
        }
        for (i = 0; i < 48; i++) gl_loops_free(&ring[i]);
    } else if (radius > 0.f) {
        if (gl_inset(&solid, -radius, &off)) {
            say(err, errlen, "Out of memory.");
            rc = -1;
        } else
            rc = cut_all(p, &off, z, nz, safe);
    } else
        rc = cut_all(p, &solid, z, nz, safe);
    if (rc && !(err && err[0])) say(err, errlen, "Out of memory.");
    if (!rc && !p->n) say(err, errlen, "Nothing closed to cut.");
    gl_loops_free(&solid);
    gl_loops_free(&off);
    return rc || !p->n ? -1 : 0;
}

static int emit(FILE *out, const char *s)
{
    return fputs(s, out) < 0 ? -1 : 0;
}

/* A line of the user's, with one newline. Empty is left out. */
static int emit_line(FILE *out, const char *s)
{
    size_t n;
    if (!s || !s[0]) return 0;
    n = strlen(s);
    if (fputs(s, out) < 0) return -1;
    if (s[n - 1] != '\n' && fputc('\n', out) == EOF) return -1;
    return 0;
}

static int write_moves(FILE *out, const gt_path *p, float feed, float plunge, gc_stats *st)
{
    int i, f = 0, moves = 0;
    float seconds = 0.f;
    char buf[96];
    for (i = 0; i < p->n; i++) {
        const gt_move *a = i ? &p->m[i - 1] : NULL;
        const gt_move *b = &p->m[i];
        float dx = a ? b->x - a->x : 0.f, dy = a ? b->y - a->y : 0.f, dz = a ? b->z - a->z : 0.f;
        float len = sqrtf(dx * dx + dy * dy + dz * dz);
        int down = b->kind != GT_TRAVEL && a && fabsf(dz) > 1e-4f && fabsf(dz) >= fabsf(dx) && fabsf(dz) >= fabsf(dy);
        int use = down ? (int)(plunge * 60.f + 0.5f) : (int)(feed * 60.f + 0.5f);
        if (b->kind == GT_TRAVEL)
            snprintf(buf, sizeof buf, "G0 X%.3f Y%.3f Z%.3f\n", (double)b->x, (double)b->y, (double)b->z);
        else if (use != f) {
            snprintf(buf, sizeof buf, "G1 X%.3f Y%.3f Z%.3f F%d\n", (double)b->x, (double)b->y, (double)b->z, use);
            f = use;
        } else
            snprintf(buf, sizeof buf, "G1 X%.3f Y%.3f Z%.3f\n", (double)b->x, (double)b->y, (double)b->z);
        if (emit(out, buf)) return -1;
        moves++;
        if (len > 0.f) seconds += len / (b->kind == GT_TRAVEL ? (feed > 0.f ? feed * 4.f : 32.f) : down ? plunge : feed);
    }
    if (st) {
        st->moves = moves;
        st->seconds = seconds;
        st->filament = 0.f;
    }
    return 0;
}

int cnc_write(const char *drawing, const char *gcode, const om_machine *m, const cnc_opts *o, gc_stats *st, char *err,
              int errlen)
{
    gv_doc d;
    gt_path p;
    cnc_opts u;
    FILE *f;
    char code[OM_CODE + 64], why[160], line[96];
    float min[3], max[3];
    om_job job;
    int rc;
    if (!drawing || !gcode || !m) {
        say(err, errlen, "No drawing, no file, or no machine.");
        return -1;
    }
    if (m->family != OM_CNC) {
        say(err, errlen, "That machine is not a mill.");
        return -1;
    }
    cnc_defaults(&u);
    take_opts(&u, o);
    gv_doc_init(&d);
    gt_init(&p);
    why[0] = 0;
    if (gv_load(&d, drawing, why, (int)sizeof why)) {
        say(err, errlen, why[0] ? why : "Could not read that drawing.");
        gv_doc_free(&d);
        return -1;
    }
    if (cnc_path(&p, &d, &u, why, (int)sizeof why)) {
        say(err, errlen, why[0] ? why : "Could not plan that cut.");
        gv_doc_free(&d);
        gt_free(&p);
        return -1;
    }
    gv_doc_free(&d);
    memset(&job, 0, sizeof job);
    if (gt_bounds(&p, min, max) == 0) {
        job.min[0] = min[0];
        job.min[1] = min[1];
        job.min[2] = 0.f;
        job.max[0] = max[0];
        job.max[1] = max[1];
        job.max[2] = max[2] > 0.f ? max[2] : 0.f;
    }
    if (-min[2] > m->area[2] && m->area[2] > 0.f) {
        snprintf(why, sizeof why, "The cut is %.1f mm deep; the machine has %.1f.", (double)(-min[2]), (double)m->area[2]);
        say(err, errlen, why);
        gt_free(&p);
        return -1;
    }
    job.rpm = pick(u.rpm, 10000.f);
    job.speed[OM_X] = job.speed[OM_Y] = pick(u.feed, 8.f);
    job.speed[OM_Z] = pick(u.plunge, 3.f);
    job.needs = OM_SPINDLE;
    if (om_check(m, &job, why, (int)sizeof why)) {
        say(err, errlen, why[0] ? why : "The machine cannot run this.");
        gt_free(&p);
        return -1;
    }
    f = fopen(gcode, "w");
    if (!f) {
        say(err, errlen, "Could not write there.");
        gt_free(&p);
        return -1;
    }
    rc = 0;
    if (om_code(m->start, 0.f, 0.f, code, (int)sizeof code)) {
        say(err, errlen, "The machine's start code is too long.");
        rc = -1;
    }
    if (!rc && (emit(f, "; OpenMakeCNC\n") || emit(f, code))) rc = -1;
    if (!rc && opts_have(&u, offsetof(cnc_opts, start), sizeof u.start) && emit_line(f, u.start)) rc = -1;
    if (!rc) {
        snprintf(line, sizeof line, "M3 S%.0f\n", (double)job.rpm);
        if (emit(f, line)) rc = -1;
    }
    if (!rc && write_moves(f, &p, pick(u.feed, 8.f), pick(u.plunge, 3.f), st)) rc = -1;
    if (!rc && emit(f, "M5\n")) rc = -1;
    if (!rc && opts_have(&u, offsetof(cnc_opts, end), sizeof u.end) && emit_line(f, u.end)) rc = -1;
    if (!rc && om_code(m->end, 0.f, 0.f, code, (int)sizeof code)) {
        say(err, errlen, "The machine's end code is too long.");
        rc = -1;
    }
    if (!rc && emit(f, code)) rc = -1;
    if (rc && !(err && err[0])) say(err, errlen, "Could not write there.");
    fclose(f);
    if (rc) remove(gcode);
    gt_free(&p);
    return rc;
}
