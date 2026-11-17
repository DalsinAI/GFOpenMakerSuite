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

static float power_of(float p)
{
    if (p <= 0.f) p = 50.f;
    if (p > 100.f) p = 100.f;
    return p;
}

static float speed_of(float v)
{
    return v > 0.f ? v : 20.f;
}

static int passes_of(int n)
{
    return n > 1 ? n : 1;
}

static int is_digit(char c)
{
    return c >= '0' && c <= '9';
}

static int hex_d(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

/* #rgb, #rrggbb, or the same without #. How many characters used, or 0. */
static int parse_colour(const char *s, unsigned *c)
{
    int i, n, h[6], hash = 0;
    if (s[0] == '#') {
        s++;
        hash = 1;
    }
    for (n = 0; n < 6 && hex_d(s[n]) >= 0; n++) h[n] = hex_d(s[n]);
    if (n == 3) {
        *c = (unsigned)((h[0] << 20) | (h[0] << 16) | (h[1] << 12) | (h[1] << 8) | (h[2] << 4) | h[2]);
        return hash ? 4 : 3;
    }
    if (n == 6) {
        *c = 0;
        for (i = 0; i < 6; i++) *c = (*c << 4) | (unsigned)h[i];
        return hash ? 7 : 6;
    }
    return 0;
}

static int parse_key(const char *s, const char *key, const char **rest)
{
    size_t n = strlen(key);
    if (strncmp(s, key, n) || s[n] != '=') return 0;
    *rest = s + n + 1;
    return 1;
}

int burn_parse_layer(const char *s, burn_layer *L, char *err, int errlen)
{
    const char *p, *rest;
    char *end;
    int n;
    if (!s || !s[0]) {
        snprintf(err, (size_t)errlen, "layer=: a colour or a 1-based index");
        return -1;
    }
    memset(L, 0, sizeof *L);
    L->size = (int)sizeof *L;
    L->colour = GV_NONE;
    L->on = 1;
    n = parse_colour(s, &L->colour);
    if (n && (s[n] == ',' || s[n] == 0)) {
        p = s[n] ? s + n + 1 : s + n;
    } else if (is_digit(s[0])) {
        long idx = strtol(s, &end, 10);
        if (end == s || (*end && *end != ',') || idx < 1 || idx > 100000) {
            snprintf(err, (size_t)errlen, "layer=%s: the index is not a positive number", s);
            return -1;
        }
        L->index = (int)idx;
        L->colour = GV_NONE;
        p = (*end == ',') ? end + 1 : end;
    } else {
        snprintf(err, (size_t)errlen, "layer=%s: a #rrggbb colour or a 1-based index", s);
        return -1;
    }
    while (p && *p) {
        const char *next = strchr(p, ',');
        char buf[64];
        size_t len = next ? (size_t)(next - p) : strlen(p);
        if (len >= sizeof buf) {
            snprintf(err, (size_t)errlen, "layer=: a setting is too long");
            return -1;
        }
        memcpy(buf, p, len);
        buf[len] = 0;
        if (parse_key(buf, "power", &rest) || parse_key(buf, "POWER", &rest)) {
            L->power = strtof(rest, &end);
            if (end == rest || *end) {
                snprintf(err, (size_t)errlen, "layer=: power is a number");
                return -1;
            }
            L->set |= BURN_SET_POWER;
        } else if (parse_key(buf, "speed", &rest) || parse_key(buf, "SPEED", &rest)) {
            L->speed = strtof(rest, &end);
            if (end == rest || *end) {
                snprintf(err, (size_t)errlen, "layer=: speed is a number");
                return -1;
            }
            L->set |= BURN_SET_SPEED;
        } else if (parse_key(buf, "passes", &rest) || parse_key(buf, "PASSES", &rest)) {
            long v = strtol(rest, &end, 10);
            if (end == rest || *end || v < 0 || v > 10000) {
                snprintf(err, (size_t)errlen, "layer=: passes is a whole number");
                return -1;
            }
            L->passes = (int)v;
            L->set |= BURN_SET_PASSES;
        } else if (parse_key(buf, "on", &rest) || parse_key(buf, "ON", &rest)) {
            long v = strtol(rest, &end, 10);
            if (end == rest || *end || (v != 0 && v != 1)) {
                snprintf(err, (size_t)errlen, "layer=: on is 0 or 1");
                return -1;
            }
            L->on = (int)v;
            L->set |= BURN_SET_ON;
        } else {
            snprintf(err, (size_t)errlen, "%s: no such setting", buf);
            return -1;
        }
        p = next ? next + 1 : NULL;
    }
    return 0;
}

void burn_layer_of(const burn_opts *o, const gv_doc *d, int L, burn_layer *out)
{
    int i;
    memset(out, 0, sizeof *out);
    out->size = (int)sizeof *out;
    out->power = power_of(o->power);
    out->speed = speed_of(o->speed);
    out->passes = passes_of(o->passes);
    out->on = 1;
    if (!d || L < 0 || L >= d->nlayer) return;
    out->colour = d->layer[L].colour;
    out->index = L + 1;
    for (i = 0; i < o->nlayer; i++) {
        const burn_layer *s = &o->layer[i];
        int match = 0;
        if (s->index > 0 && s->index == L + 1) match = 1;
        else if (s->index <= 0 && s->colour != GV_NONE && s->colour == d->layer[L].colour) match = 1;
        if (!match) continue;
        if (s->set & BURN_SET_POWER) out->power = power_of(s->power);
        if (s->set & BURN_SET_SPEED) out->speed = speed_of(s->speed);
        if (s->set & BURN_SET_PASSES) out->passes = passes_of(s->passes);
        if (s->set & BURN_SET_ON) out->on = s->on ? 1 : 0;
    }
}

/* Follow one path, starting with a travel to its first point. */
static int add_path(gt_path *p, float *speed, int *n, int cap, const gv_path *v, float power, float vcut)
{
    int i, start = p->n;
    if (v->n < 2) return 0;
    if (gt_add(p, GT_TRAVEL, v->xy[0], v->xy[1], 0.f, 0.f)) return -1;
    for (i = 1; i < v->n; i++)
        if (gt_add(p, GT_CUT, v->xy[i * 2], v->xy[i * 2 + 1], 0.f, power)) return -1;
    if (v->closed) {
        if (gt_add(p, GT_CUT, v->xy[0], v->xy[1], 0.f, power)) return -1;
    }
    if (speed) {
        for (i = start; i < p->n && *n < cap; i++) {
            speed[*n] = (p->m[i].kind == GT_CUT) ? vcut : 0.f;
            (*n)++;
        }
        if (*n != p->n) return -1;
    }
    return 0;
}

int burn_path(gt_path *p, float **speed, const gv_doc *d, const burn_opts *o)
{
    float *spd = NULL;
    int pass, L, i, nspd = 0, cap = 0;
    gt_free(p);
    gt_init(p);
    if (speed) *speed = NULL;
    if (speed) {
        cap = 64;
        spd = (float *)malloc((size_t)cap * sizeof *spd);
        if (!spd) return -1;
    }
    for (L = 0; L < d->nlayer; L++) {
        burn_layer ly;
        burn_layer_of(o, d, L, &ly);
        if (!ly.on || ly.colour == GV_NONE) continue;
        for (pass = 0; pass < ly.passes; pass++) {
            for (i = 0; i < d->n; i++) {
                if (gv_path_colour(&d->p[i]) != ly.colour) continue;
                if (speed && p->n + 8 > cap) {
                    int ncap = cap * 2 + 32;
                    float *nspd_mem = (float *)realloc(spd, (size_t)ncap * sizeof *spd);
                    if (!nspd_mem) {
                        free(spd);
                        gt_free(p);
                        return -1;
                    }
                    spd = nspd_mem;
                    cap = ncap;
                }
                if (add_path(p, spd, &nspd, cap, &d->p[i], ly.power, ly.speed)) {
                    free(spd);
                    gt_free(p);
                    return -1;
                }
            }
        }
    }
    if (speed) {
        *speed = spd;
        if (p->n == 0) {
            free(spd);
            *speed = NULL;
        }
    }
    return 0;
}

static long to_file(const char *s, long n, void *ud)
{
    return (long)fwrite(s, 1, (size_t)n, (FILE *)ud);
}

static int check_indexes(const gv_doc *d, const burn_opts *o, char *err, int errlen)
{
    int i;
    for (i = 0; i < o->nlayer; i++) {
        if (o->layer[i].index > 0 && o->layer[i].index > d->nlayer) {
            snprintf(err, (size_t)errlen, "layer %d: no such layer", o->layer[i].index);
            return -1;
        }
    }
    return 0;
}

int burn_write(const char *svg, const char *gcode, const om_machine *m, const burn_opts *o, gc_stats *st, char *err,
               int errlen)
{
    gv_doc d;
    gt_path p;
    gc_laser L;
    gc_laser_more more;
    om_job job;
    gc_stats mine;
    float *speeds = NULL;
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
    if (check_indexes(&d, o, err, errlen)) {
        gv_doc_free(&d);
        return -1;
    }
    if (burn_path(&p, &speeds, &d, o)) {
        snprintf(err, (size_t)errlen, "out of memory");
        gv_doc_free(&d);
        gt_free(&p);
        return -1;
    }
    if (p.n < 2) {
        snprintf(err, (size_t)errlen, "no paths to burn");
        gv_doc_free(&d);
        gt_free(&p);
        free(speeds);
        return -1;
    }
    gc_laser_defaults(&L);
    L.constant = o->constant;
    L.s100 = o->s100;
    L.speed = o->speed;
    L.travel = o->travel;
    gc_laser_more_defaults(&more, &L);
    more.speed = speeds;
    gc_laser_job_more(&p, m, &more, &job);
    if (om_check(m, &job, why, (int)sizeof why) & ~OM_TOO_FAST) {
        snprintf(err, (size_t)errlen, "%.60s cannot burn this: %s", m->name, why);
        gv_doc_free(&d);
        gt_free(&p);
        free(speeds);
        return -1;
    }
    f = fopen(gcode, "w");
    if (!f) {
        snprintf(err, (size_t)errlen, "%.120s: cannot write", gcode);
        gv_doc_free(&d);
        gt_free(&p);
        free(speeds);
        return -1;
    }
    rc = gc_laser_write_more(to_file, f, &p, m, &more, st ? st : &mine, err, errlen);
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
    free(speeds);
    return rc;
}
