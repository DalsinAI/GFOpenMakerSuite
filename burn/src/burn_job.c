/* An SVG to a laser toolpath and its Grbl G-code (burn_job.h).
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "burn_job.h"
#include "burn_image.h"

#include "gs_layer.h"
#include "gs_toolpath.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

void burn_defaults(burn_opts *o)
{
    memset(o, 0, sizeof *o);
    o->size = (int)sizeof *o;
    o->power = 50.f;
    o->speed = 20.f;
    o->passes = 1;
    o->s100 = 1000.f;
    o->interval = 0.2f;
    o->cuts_last = 1;
}

int burn_mode_parse(const char *s)
{
    if (!s || !s[0]) return -1;
    if (!strcmp(s, "line") || !strcmp(s, "LINE")) return BURN_LINE;
    if (!strcmp(s, "fill") || !strcmp(s, "FILL")) return BURN_FILL;
    if (!strcmp(s, "offset") || !strcmp(s, "OFFSET")) return BURN_OFFSET;
    return -1;
}

const char *burn_mode_name(int mode)
{
    if (mode == BURN_FILL) return "fill";
    if (mode == BURN_OFFSET) return "offset";
    return "line";
}

static float interval_of(float v)
{
    if (v <= 0.f) v = 0.2f;
    if (v < 0.01f) v = 0.01f;
    return v;
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
        } else if (parse_key(buf, "mode", &rest) || parse_key(buf, "MODE", &rest)) {
            int m = burn_mode_parse(rest);
            if (m < 0) {
                snprintf(err, (size_t)errlen, "layer=: mode=%s: line, fill or offset", rest);
                return -1;
            }
            L->mode = m;
            L->set |= BURN_SET_MODE;
        } else if (parse_key(buf, "interval", &rest) || parse_key(buf, "INTERVAL", &rest)) {
            L->interval = strtof(rest, &end);
            if (end == rest || *end) {
                snprintf(err, (size_t)errlen, "layer=: interval is a number, in mm");
                return -1;
            }
            L->set |= BURN_SET_INTERVAL;
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
    out->mode = o->mode;
    out->interval = interval_of(o->interval);
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
        if (s->set & BURN_SET_MODE) out->mode = s->mode;
        if (s->set & BURN_SET_INTERVAL) out->interval = interval_of(s->interval);
    }
}

static int grow_speed(float **spd, int *cap, int need)
{
    int ncap;
    float *n;
    if (need <= *cap) return 0;
    ncap = *cap ? *cap : 64;
    while (ncap < need) ncap = ncap * 2 + 32;
    n = (float *)realloc(*spd, (size_t)ncap * sizeof *n);
    if (!n) return -1;
    *spd = n;
    *cap = ncap;
    return 0;
}

static int tag_speeds(float *spd, int *nspd, const gt_path *p, int start, float vcut)
{
    int i;
    for (i = start; i < p->n; i++) spd[(*nspd)++] = (p->m[i].kind == GT_CUT) ? vcut : 0.f;
    return *nspd == p->n ? 0 : -1;
}

/* Follow one path, starting with a travel to its first point. */
static int add_path(gt_path *p, float **spd, int *nspd, int *cap, const gv_path *v, float power, float vcut)
{
    int start = p->n, i;
    if (v->n < 2) return 0;
    if (gt_add(p, GT_TRAVEL, v->xy[0], v->xy[1], 0.f, 0.f)) return -1;
    for (i = 1; i < v->n; i++)
        if (gt_add(p, GT_CUT, v->xy[i * 2], v->xy[i * 2 + 1], 0.f, power)) return -1;
    if (v->closed) {
        if (gt_add(p, GT_CUT, v->xy[0], v->xy[1], 0.f, power)) return -1;
    }
    if (spd && *spd) {
        if (grow_speed(spd, cap, p->n)) return -1;
        if (tag_speeds(*spd, nspd, p, start, vcut)) return -1;
    }
    return 0;
}

/* Closed paths of one colour as loops for gt_fill. xy is copied. */
static int loops_of(const gv_doc *d, unsigned colour, gl_loops *out)
{
    int i, n = 0;
    memset(out, 0, sizeof *out);
    for (i = 0; i < d->n; i++)
        if (gv_path_colour(&d->p[i]) == colour && d->p[i].closed && d->p[i].n >= 3) n++;
    if (!n) return 0;
    out->l = (gl_loop *)calloc((size_t)n, sizeof *out->l);
    if (!out->l) return -1;
    for (i = 0; i < d->n; i++) {
        const gv_path *v = &d->p[i];
        int np, k;
        float *xy;
        if (gv_path_colour(v) != colour || !v->closed || v->n < 3) continue;
        np = v->n;
        if (np >= 2 && v->xy[0] == v->xy[(np - 1) * 2] && v->xy[1] == v->xy[(np - 1) * 2 + 1]) np--;
        if (np < 3) continue;
        xy = (float *)malloc((size_t)np * 2 * sizeof *xy);
        if (!xy) {
            gl_loops_free(out);
            return -1;
        }
        for (k = 0; k < np * 2; k++) xy[k] = v->xy[k];
        out->l[out->n].xy = xy;
        out->l[out->n].n = np;
        out->l[out->n].closed = 1;
        out->n++;
    }
    return 0;
}

static void remap_cuts(gt_path *p, int from, float power)
{
    int i;
    for (i = from; i < p->n; i++) {
        if (p->m[i].kind == GT_TRAVEL) p->m[i].w = 0.f;
        else {
            p->m[i].kind = GT_CUT;
            p->m[i].w = power;
        }
    }
}

/* Scan-line fill of the layer's closed paths, through gt_fill (every line,
 * no skin, along x). Open paths of the same colour are still followed. */
static int add_fill(gt_path *p, float **spd, int *nspd, int *cap, const gv_doc *d, const burn_layer *ly)
{
    gl_loops region;
    gt_path hatch;
    int start = p->n, i;
    if (loops_of(d, ly->colour, &region)) return -1;
    if (region.n) {
        gt_init(&hatch);
        if (gt_fill(&hatch, &region, NULL, 0, 0.f, ly->interval, 1, 0)) {
            gt_free(&hatch);
            gl_loops_free(&region);
            return -1;
        }
        remap_cuts(&hatch, 0, ly->power);
        if (gt_append(p, &hatch)) {
            gt_free(&hatch);
            gl_loops_free(&region);
            return -1;
        }
        gt_free(&hatch);
        if (spd && *spd) {
            if (grow_speed(spd, cap, p->n) || tag_speeds(*spd, nspd, p, start, ly->speed)) {
                gl_loops_free(&region);
                return -1;
            }
        }
    }
    gl_loops_free(&region);
    for (i = 0; i < d->n; i++) {
        if (gv_path_colour(&d->p[i]) != ly->colour || d->p[i].closed) continue;
        if (add_path(p, spd, nspd, cap, &d->p[i], ly->power, ly->speed)) return -1;
    }
    return 0;
}


/* Even-odd as the SVG reader: a loop whose inside sits in an odd number of
 * others is a hole and must wind clockwise, so gl_union and gl_inset keep
 * the hole and grow it. An outside winds anticlockwise. */
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

static int inside_point(const gl_loop *l, float *px, float *py)
{
    int i;
    for (i = 0; i < l->n; i++) {
        float x0 = l->xy[i * 2], y0 = l->xy[i * 2 + 1];
        float x1 = l->xy[((i + 1) % l->n) * 2], y1 = l->xy[((i + 1) % l->n) * 2 + 1];
        float dx = x1 - x0, dy = y1 - y0, s = (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy);
        float mx, my, nx, ny;
        if (s < 1e-8f) continue;
        mx = (x0 + x1) * 0.5f;
        my = (y0 + y1) * 0.5f;
        nx = -dy * 0.001f / s;
        ny = dx * 0.001f / s;
        if (loop_contains(l, mx + nx, my + ny)) {
            *px = mx + nx;
            *py = my + ny;
            return 0;
        }
        if (loop_contains(l, mx - nx, my - ny)) {
            *px = mx - nx;
            *py = my - ny;
            return 0;
        }
    }
    return -1;
}

static void face_even_odd(gl_loops *ls)
{
    int i, j, n;
    float x, y, a;
    for (i = 0; i < ls->n; i++) {
        if (!ls->l[i].closed || ls->l[i].n < 3) continue;
        if (inside_point(&ls->l[i], &x, &y)) continue;
        n = 0;
        for (j = 0; j < ls->n; j++) {
            if (j == i || !ls->l[j].closed) continue;
            if (loop_contains(&ls->l[j], x, y)) n++;
        }
        a = gl_loop_area(&ls->l[i]);
        if ((n % 2 == 0 && a < 0.f) || (n % 2 == 1 && a > 0.f)) reverse_loop(&ls->l[i]);
    }
}

/* Offset fill: the outline, then gl_inset by the interval, until nothing
 * is left. Open paths of the same colour are still followed. */
static int add_offset(gt_path *p, float **spd, int *nspd, int *cap, const gv_doc *d, const burn_layer *ly)
{
    gl_loops region, cur, next;
    int ring, i, start;
    if (loops_of(d, ly->colour, &region)) return -1;
    face_even_odd(&region);
    memset(&cur, 0, sizeof cur);
    memset(&next, 0, sizeof next);
    if (region.n) {
        if (gl_union(&region, &cur)) {
            gl_loops_free(&region);
            return -1;
        }
        gl_loops_free(&region);
        for (ring = 0; cur.n > 0 && ring < 10000; ring++) {
            start = p->n;
            for (i = 0; i < cur.n; i++) {
                if (!cur.l[i].closed || cur.l[i].n < 3) continue;
                if (gt_loop(p, GT_CUT, &cur.l[i], 0.f, ly->power)) {
                    gl_loops_free(&cur);
                    return -1;
                }
            }
            if (spd && *spd) {
                if (grow_speed(spd, cap, p->n) || tag_speeds(*spd, nspd, p, start, ly->speed)) {
                    gl_loops_free(&cur);
                    return -1;
                }
            }
            if (gl_inset(&cur, ly->interval, &next)) {
                gl_loops_free(&cur);
                return -1;
            }
            gl_loops_free(&cur);
            cur = next;
            memset(&next, 0, sizeof next);
        }
        gl_loops_free(&cur);
    } else
        gl_loops_free(&region);
    for (i = 0; i < d->n; i++) {
        if (gv_path_colour(&d->p[i]) != ly->colour || d->p[i].closed) continue;
        if (add_path(p, spd, nspd, cap, &d->p[i], ly->power, ly->speed)) return -1;
    }
    return 0;
}

float burn_heaviness(const burn_layer *ly)
{
    float s;
    if (!ly) return 0.f;
    s = ly->speed;
    if (s < 0.001f) s = 0.001f;
    return ly->power * (float)(ly->passes > 0 ? ly->passes : 1) / s;
}

static int opts_have(const burn_opts *o, size_t off, size_t sz)
{
    return o && o->size >= (int)(off + sz);
}

int burn_seq(const burn_opts *o, const gv_doc *d, int *seq, int cap)
{
    int n, i, j, used[256], cuts_last = 1, nseq = 0;
    const int *user = NULL;
    burn_layer ly[256];
    if (!d || !seq) return -1;
    n = d->nlayer;
    if (n > cap) return -1;
    if (n > 256) return -1;
    if (opts_have(o, offsetof(burn_opts, cuts_last), sizeof o->cuts_last))
        cuts_last = o->cuts_last ? 1 : 0;
    if (opts_have(o, offsetof(burn_opts, nseq), sizeof o->nseq) &&
        opts_have(o, offsetof(burn_opts, seq), sizeof o->seq) && o->seq && o->nseq > 0) {
        user = o->seq;
        nseq = o->nseq;
    }
    for (i = 0; i < n; i++) used[i] = 0;
    if (user) {
        int k = 0;
        for (i = 0; i < nseq; i++) {
            int L = user[i];
            if (L < 0 || L >= n || used[L]) continue;
            seq[k++] = L;
            used[L] = 1;
        }
        for (i = 0; i < n; i++)
            if (!used[i]) seq[k++] = i;
    } else {
        for (i = 0; i < n; i++) seq[i] = i;
    }
    if (!cuts_last) return 0;
    for (i = 0; i < n; i++) burn_layer_of(o, d, seq[i], &ly[i]);
    /* Stable: fills before lines, then lighter before heavier. */
    for (i = 1; i < n; i++) {
        int L = seq[i];
        burn_layer keep = ly[i];
        j = i;
        while (j > 0) {
            int af, bf;
            float ah, bh;
            af = (ly[j - 1].mode == BURN_FILL || ly[j - 1].mode == BURN_OFFSET);
            bf = (keep.mode == BURN_FILL || keep.mode == BURN_OFFSET);
            if (af != bf) {
                if (bf) {
                    /* keep is a fill, previous is a line: move keep up */
                } else
                    break;
            } else {
                ah = burn_heaviness(&ly[j - 1]);
                bh = burn_heaviness(&keep);
                if (ah <= bh) break;
            }
            seq[j] = seq[j - 1];
            ly[j] = ly[j - 1];
            j--;
        }
        seq[j] = L;
        ly[j] = keep;
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
        *speed = spd;
    }
    {
        int run[256], r;
        if (d->nlayer > 256) {
            free(spd);
            if (speed) *speed = NULL;
            return -1;
        }
        if (burn_seq(o, d, run, 256)) {
            free(spd);
            if (speed) *speed = NULL;
            return -1;
        }
        for (r = 0; r < d->nlayer; r++) {
            burn_layer ly;
            L = run[r];
            burn_layer_of(o, d, L, &ly);
            if (!ly.on || ly.colour == GV_NONE) continue;
            for (pass = 0; pass < ly.passes; pass++) {
                if (ly.mode == BURN_FILL || ly.mode == BURN_OFFSET) {
                    int rc = (ly.mode == BURN_FILL)
                                 ? add_fill(p, speed ? &spd : NULL, &nspd, &cap, d, &ly)
                                 : add_offset(p, speed ? &spd : NULL, &nspd, &cap, d, &ly);
                    if (rc) {
                        free(spd);
                        if (speed) *speed = NULL;
                        gt_free(p);
                        return -1;
                    }
                    continue;
                }
                for (i = 0; i < d->n; i++) {
                    if (gv_path_colour(&d->p[i]) != ly.colour) continue;
                    if (add_path(p, speed ? &spd : NULL, &nspd, &cap, &d->p[i], ly.power, ly.speed)) {
                        free(spd);
                        if (speed) *speed = NULL;
                        gt_free(p);
                        return -1;
                    }
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
    int i, nseq;
    if (!o) return 0;
    for (i = 0; i < o->nlayer; i++) {
        if (o->layer[i].index > 0 && o->layer[i].index > d->nlayer) {
            snprintf(err, (size_t)errlen, "layer %d: no such layer", o->layer[i].index);
            return -1;
        }
    }
    if (opts_have(o, offsetof(burn_opts, nseq), sizeof o->nseq) &&
        opts_have(o, offsetof(burn_opts, seq), sizeof o->seq) && o->seq && o->nseq > 0) {
        nseq = o->nseq;
        for (i = 0; i < nseq; i++) {
            int L = o->seq[i];
            if (L < 0 || L >= d->nlayer) {
                snprintf(err, (size_t)errlen, "order: no such layer %d", L + 1);
                return -1;
            }
        }
    }
    return 0;
}

int burn_write(const char *drawing, const char *gcode, const om_machine *m, const burn_opts *o, gc_stats *st, char *err,
               int errlen)
{
    gv_doc d;
    gt_path p;
    float *speeds = NULL;
    int rc;
    if (!m || m->family != OM_LASER) {
        snprintf(err, (size_t)errlen, "%s is not a laser", (m && m->name[0]) ? m->name : "the machine");
        return -1;
    }
    gv_doc_init(&d);
    gt_init(&p);
    if (burn_is_image(drawing)) {
        burn_image im;
        burn_image_init(&im);
        if (burn_image_load(&im, drawing, err, errlen)) {
            burn_image_free(&im);
            return -1;
        }
        if (burn_image_path(&p, &speeds, &im, o)) {
            snprintf(err, (size_t)errlen, "out of memory");
            burn_image_free(&im);
            gt_free(&p);
            return -1;
        }
        if (p.n < 2) {
            snprintf(err, (size_t)errlen, "no paths to burn");
            burn_image_free(&im);
            gt_free(&p);
            free(speeds);
            return -1;
        }
        rc = burn_write_path(&p, speeds, gcode, m, o, st, err, errlen);
        burn_image_free(&im);
        gt_free(&p);
        free(speeds);
        return rc;
    }
    if (gv_load(&d, drawing, err, errlen)) {
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
    rc = burn_write_path(&p, speeds, gcode, m, o, st, err, errlen);
    gv_doc_free(&d);
    gt_free(&p);
    free(speeds);
    return rc;
}

int burn_frame_path(gt_path *out, const gt_path *job)
{
    float mn[3], mx[3];
    gt_free(out);
    gt_init(out);
    if (gt_bounds(job, mn, mx)) return -1;
    if (gt_add(out, GT_TRAVEL, mn[0], mn[1], 0.f, 0.f)) return -1;
    if (gt_add(out, GT_TRAVEL, mx[0], mn[1], 0.f, 0.f)) return -1;
    if (gt_add(out, GT_TRAVEL, mx[0], mx[1], 0.f, 0.f)) return -1;
    if (gt_add(out, GT_TRAVEL, mn[0], mx[1], 0.f, 0.f)) return -1;
    if (gt_add(out, GT_TRAVEL, mn[0], mn[1], 0.f, 0.f)) return -1;
    return 0;
}

int burn_write_path(const gt_path *p, const float *speed, const char *gcode, const om_machine *m, const burn_opts *o,
                    gc_stats *st, char *err, int errlen)
{
    gc_laser L;
    gc_laser_more more;
    om_job job;
    gc_stats mine;
    char why[128];
    FILE *f;
    int rc;
    if (!m || m->family != OM_LASER) {
        snprintf(err, (size_t)errlen, "%s is not a laser", (m && m->name[0]) ? m->name : "the machine");
        return -1;
    }
    if (!p || p->n < 2) {
        snprintf(err, (size_t)errlen, "no paths to burn");
        return -1;
    }
    gc_laser_defaults(&L);
    if (o) {
        L.constant = o->constant;
        L.s100 = o->s100;
        L.speed = o->speed;
        L.travel = o->travel;
    }
    gc_laser_more_defaults(&more, &L);
    more.speed = speed;
    gc_laser_job_more(p, m, &more, &job);
    if (om_check(m, &job, why, (int)sizeof why) & ~OM_TOO_FAST) {
        snprintf(err, (size_t)errlen, "%.60s cannot burn this: %s", m->name, why);
        return -1;
    }
    f = fopen(gcode, "w");
    if (!f) {
        snprintf(err, (size_t)errlen, "%.120s: cannot write", gcode);
        return -1;
    }
    rc = gc_laser_write_more(to_file, f, p, m, &more, st ? st : &mine, err, errlen);
    if (fclose(f) && !rc) {
        snprintf(err, (size_t)errlen, "%.120s: cannot write", gcode);
        rc = -1;
    }
    if (rc) {
        remove(gcode);
        rc = -1;
    }
    return rc;
}
