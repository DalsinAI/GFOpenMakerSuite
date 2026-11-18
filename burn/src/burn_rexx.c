/* OpenMakeBurn's ARexx command table. The window Wait()s on OPENMAKEBURN.1
 * and dispatches through these verbs; host tests use omr_do_line.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "burn_rexx.h"

#include "burn_material.h"
#include "gs_gcode.h"
#include "gs_toolpath.h"
#include "gs_view.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static gb_rexx *app(void *g) { return (gb_rexx *)g; }

static int eqi(const char *a, const char *b)
{
    if (!a || !b) return 0;
    while (*a && *b) {
        int ca = (unsigned char)*a, cb = (unsigned char)*b;
        if (ca >= 'A' && ca <= 'Z') ca = ca - 'A' + 'a';
        if (cb >= 'A' && cb <= 'Z') cb = cb - 'A' + 'a';
        if (ca != cb) return 0;
        a++;
        b++;
    }
    return *a == 0 && *b == 0;
}

static int arg_f(omr_cmd *cmd, const char *key, float *out)
{
    const char *s = omr_arg(cmd, key);
    char *end = NULL;
    if (!s) return 0;
    *out = (float)strtod(s, &end);
    if (!end || end == s) return -1;
    while (*end == ' ' || *end == '\t') end++;
    return *end ? -1 : 1;
}

static int arg_i(omr_cmd *cmd, const char *key, int *out)
{
    const char *s = omr_arg(cmd, key);
    char *end = NULL;
    long v;
    if (!s) return 0;
    v = strtol(s, &end, 10);
    if (!end || end == s) return -1;
    while (*end == ' ' || *end == '\t') end++;
    if (*end) return -1;
    *out = (int)v;
    return 1;
}

static int stem_put(omr_cmd *cmd, const char *stem, const char *tail, const char *value)
{
    const char *s = omr_arg(cmd, "STEM");
    if (!s) s = stem;
    if (!s) return 0;
    return omr_set_stem(cmd, s, tail, value);
}

static int flag01(const char *s, int *out)
{
    if (!s) return -1;
    if (eqi(s, "1") || eqi(s, "on") || eqi(s, "yes") || eqi(s, "true")) {
        *out = 1;
        return 0;
    }
    if (eqi(s, "0") || eqi(s, "off") || eqi(s, "no") || eqi(s, "false")) {
        *out = 0;
        return 0;
    }
    return -1;
}

static float hold(float v, float lo, float hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static int has_doc(gb_rexx *g) { return g && g->have_doc && *g->have_doc && g->doc; }

static void dirty(gb_rexx *g)
{
    if (g) g->dirty = 1;
}

static void drop_job(gb_rexx *g)
{
    if (g->pic) burn_image_free(g->pic);
    if (g->doc) {
        gv_doc_free(g->doc);
        gv_doc_init(g->doc);
    }
    if (g->path) {
        gt_free(g->path);
        gt_init(g->path);
    }
    if (g->have_doc) *g->have_doc = 0;
    if (g->have_image) *g->have_image = 0;
    if (g->have_path) *g->have_path = 0;
}

static int cmd_version(void *c, void *g0)
{
    gb_rexx *g = app(g0);
    omr_set_result(c, g->version && g->version[0] ? g->version : "OpenMakeBurn 0.1.0");
    return OMR_OK;
}

static int cmd_quit(void *c, void *g0)
{
    gb_rexx *g = app(g0);
    if (g->dirty && !omr_switch(c, "FORCE")) {
        omr_set_result(c, "the document has not been saved");
        return OMR_ERROR;
    }
    g->quit = 1;
    return OMR_OK;
}

static int cmd_open(void *c, void *g0)
{
    gb_rexx *g = app(g0);
    const char *file = omr_arg(c, "FILE");
    char err[160];
    err[0] = 0;
    if (!file) {
        omr_set_result(c, "FILE is required");
        return OMR_ERROR;
    }
    if (!g->doc || !g->have_doc) {
        omr_set_result(c, "no document");
        return OMR_ERROR;
    }
    drop_job(g);
    if (g->opts) {
        burn_defaults(g->opts);
        g->opts->layer = g->store;
        g->opts->nlayer = 0;
        g->have_order = 0;
        g->nseq = 0;
        gb_rexx_keep(g);
    }
    if (burn_is_image(file)) {
        if (!g->pic || burn_image_load(g->pic, file, err, (int)sizeof err) ||
            burn_image_doc(g->doc, g->pic, g->opts)) {
            omr_set_result(c, err[0] ? err : "the picture did not load");
            drop_job(g);
            return OMR_ERROR;
        }
        *g->have_image = 1;
    } else if (gv_load(g->doc, file, err, (int)sizeof err)) {
        omr_set_result(c, err[0] ? err : "the drawing did not load");
        drop_job(g);
        return OMR_ERROR;
    }
    *g->have_doc = 1;
    snprintf(g->file, sizeof g->file, "%s", file);
    g->dirty = 0;
    g->look = 3;
    omr_set_result(c, g->file);
    return OMR_OK;
}

static int cmd_save(void *c, void *g0)
{
    gb_rexx *g = app(g0);
    const char *file = omr_arg(c, "FILE");
    char err[160];
    gc_stats st;
    err[0] = 0;
    if (!file || !file[0]) file = g->save_file;
    if (!file || !file[0]) {
        omr_set_result(c, "no file to write");
        return OMR_ERROR;
    }
    if (!has_doc(g) || !g->file[0]) {
        omr_set_result(c, "Open a drawing first");
        return OMR_ERROR;
    }
    if (!g->have_machine || g->machine.family != OM_LASER) {
        omr_set_result(c, "no laser");
        return OMR_ERROR;
    }
    memset(&st, 0, sizeof st);
    if (burn_write(g->file, file, &g->machine, g->opts, &st, err, (int)sizeof err)) {
        omr_set_result(c, err[0] ? err : "Could not write there");
        return OMR_ERROR;
    }
    snprintf(g->save_file, sizeof g->save_file, "%s", file);
    g->dirty = 0;
    omr_set_result(c, g->save_file);
    {
        char nbuf[32];
        snprintf(nbuf, sizeof nbuf, "%d", st.moves);
        stem_put(c, NULL, "MOVES", nbuf);
        snprintf(nbuf, sizeof nbuf, "%.3f", (double)st.seconds);
        stem_put(c, NULL, "SECONDS", nbuf);
    }
    return OMR_OK;
}

static int cmd_activate(void *c, void *g0)
{
    gb_rexx *g = app(g0);
    (void)c;
    if (g->activate && g->win) g->activate(g->win);
    return OMR_OK;
}

static int cmd_hide(void *c, void *g0)
{
    gb_rexx *g = app(g0);
    (void)c;
    if (g->hide && g->win) g->hide(g->win);
    return OMR_OK;
}

static const char *const job_keys[] = {"power",  "speed",    "travel", "passes",   "constant",
                                       "s100",   "mode",     "interval", "cuts_last", "start", "end",
                                       "invert", "width", "x", "y", "rot", "kerf", "tabs", "tabw",
                                       "order", NULL};

static int key_of(const char *s)
{
    int i;
    if (!s) return -1;
    for (i = 0; job_keys[i]; i++)
        if (eqi(s, job_keys[i])) return i;
    return -1;
}

static void key_value(const burn_opts *o, int k, char *buf, int len)
{
    if (!o || !buf || len < 1) return;
    buf[0] = 0;
    switch (k) {
    case 0:
        snprintf(buf, (size_t)len, "%g", (double)o->power);
        break;
    case 1:
        snprintf(buf, (size_t)len, "%g", (double)o->speed);
        break;
    case 2:
        snprintf(buf, (size_t)len, "%g", (double)o->travel);
        break;
    case 3:
        snprintf(buf, (size_t)len, "%d", o->passes);
        break;
    case 4:
        snprintf(buf, (size_t)len, "%d", o->constant ? 1 : 0);
        break;
    case 5:
        snprintf(buf, (size_t)len, "%g", (double)o->s100);
        break;
    case 6:
        snprintf(buf, (size_t)len, "%s", burn_mode_name(o->mode));
        break;
    case 7:
        snprintf(buf, (size_t)len, "%g", (double)o->interval);
        break;
    case 8:
        snprintf(buf, (size_t)len, "%d", o->cuts_last ? 1 : 0);
        break;
    case 9:
        snprintf(buf, (size_t)len, "%s", o->start ? o->start : "");
        break;
    case 10:
        snprintf(buf, (size_t)len, "%s", o->end ? o->end : "");
        break;
    case 11:
        snprintf(buf, (size_t)len, "%d", o->invert ? 1 : 0);
        break;
    case 12:
        snprintf(buf, (size_t)len, "%g", (double)o->width);
        break;
    case 13:
        snprintf(buf, (size_t)len, "%g", (double)o->x);
        break;
    case 14:
        snprintf(buf, (size_t)len, "%g", (double)o->y);
        break;
    case 15:
        snprintf(buf, (size_t)len, "%g", (double)o->rot);
        break;
    case 16:
        snprintf(buf, (size_t)len, "%g", (double)o->kerf);
        break;
    case 17:
        snprintf(buf, (size_t)len, "%d", o->tabs);
        break;
    case 18:
        snprintf(buf, (size_t)len, "%g", (double)o->tabw);
        break;
    case 19: {
        int i, p = 0;
        buf[0] = 0;
        if (!o->seq || o->nseq <= 0) break;
        for (i = 0; i < o->nseq && p < len - 1; i++) {
            int wr = snprintf(buf + p, (size_t)(len - p), i ? ",%d" : "%d", o->seq[i] + 1);
            if (wr < 0 || wr >= len - p) break;
            p += wr;
        }
        break;
    }
    default:
        break;
    }
}

static int set_key(burn_opts *o, int k, const char *value, char *err, int errlen)
{
    float f;
    int n, mode;
    char *end = NULL;
    if (!o || !value) {
        snprintf(err, (size_t)errlen, "a value is required");
        return -1;
    }
    switch (k) {
    case 0:
    case 1:
    case 2:
    case 5:
    case 7:
        f = (float)strtod(value, &end);
        if (!end || end == value) {
            snprintf(err, (size_t)errlen, "not a number");
            return -1;
        }
        if (k == 0) o->power = hold(f, 0.f, 100.f);
        else if (k == 1) o->speed = hold(f, 0.1f, 500.f);
        else if (k == 2) o->travel = hold(f, 0.f, 1000.f);
        else if (k == 5) o->s100 = hold(f, 1.f, 10000.f);
        else o->interval = hold(f, 0.01f, 50.f);
        return 0;
    case 3:
        n = (int)strtol(value, &end, 10);
        if (!end || end == value) {
            snprintf(err, (size_t)errlen, "not a number");
            return -1;
        }
        o->passes = n < 1 ? 1 : n;
        return 0;
    case 4:
    case 8:
        if (flag01(value, &n)) {
            snprintf(err, (size_t)errlen, "use 0 or 1");
            return -1;
        }
        if (k == 4) o->constant = n;
        else o->cuts_last = n;
        return 0;
    case 6:
        mode = burn_mode_parse(value);
        if (mode < 0) {
            snprintf(err, (size_t)errlen, "mode is line, fill or offset");
            return -1;
        }
        o->mode = mode;
        return 0;
    case 11:
        if (flag01(value, &n)) {
            snprintf(err, (size_t)errlen, "use 0 or 1");
            return -1;
        }
        o->invert = n;
        return 0;
    case 12:
        f = (float)strtod(value, &end);
        if (!end || end == value || f < 0.f) {
            snprintf(err, (size_t)errlen, "width is 0 or more, in mm");
            return -1;
        }
        o->width = hold(f, 0.f, 2000.f);
        return 0;
    case 13:
    case 14:
    case 15:
        f = (float)strtod(value, &end);
        if (!end || end == value) {
            snprintf(err, (size_t)errlen, "not a number");
            return -1;
        }
        if (k == 15) o->rot = hold(f, -3600.f, 3600.f);
        else if (k == 13) o->x = hold(f, -10000.f, 10000.f);
        else o->y = hold(f, -10000.f, 10000.f);
        return 0;
    case 16:
        f = (float)strtod(value, &end);
        if (!end || end == value || f < 0.f) {
            snprintf(err, (size_t)errlen, "kerf is 0 or more, in mm");
            return -1;
        }
        o->kerf = hold(f, 0.f, 20.f);
        return 0;
    case 17:
        n = (int)strtol(value, &end, 10);
        if (!end || end == value || n < 0) {
            snprintf(err, (size_t)errlen, "tabs is a whole number, 0 to 32");
            return -1;
        }
        o->tabs = n > 32 ? 32 : n;
        return 0;
    case 18:
        f = (float)strtod(value, &end);
        if (!end || end == value || f < 0.f) {
            snprintf(err, (size_t)errlen, "tab width is 0 or more, in mm");
            return -1;
        }
        o->tabw = hold(f, 0.f, 20.f);
        return 0;
    default:
        snprintf(err, (size_t)errlen, "unknown key");
        return -1;
    }
}

static void own_copy(char *dst, int n, const char *src)
{
    size_t k;
    if (!dst || n < 1) return;
    if (!src) src = "";
    k = strlen(src);
    if (k >= (size_t)n) k = (size_t)n - 1;
    memcpy(dst, src, k);
    dst[k] = 0;
}

/* The window keeps power, speed, passes, mode, gap, kerf and tabs on the
 * layer rows. A job SET has to write those rows or the next rebuild throws
 * it away. mask bits: 1 power, 2 speed, 4 passes, 8 mode, 16 gap, 32 kerf,
 * 64 tabs, 128 tab width. */
static void layers_job(gb_rexx *g, unsigned mask)
{
    int i, n;
    if (!g || !g->opts || !g->store || !mask) return;
    n = g->opts->nlayer;
    if (n > g->store_cap) n = g->store_cap;
    for (i = 0; i < n; i++) {
        burn_layer *L = &g->store[i];
        if (mask & 1u) {
            L->power = g->opts->power;
            L->set |= BURN_SET_POWER;
        }
        if (mask & 2u) {
            L->speed = g->opts->speed;
            L->set |= BURN_SET_SPEED;
        }
        if (mask & 4u) {
            L->passes = g->opts->passes;
            L->set |= BURN_SET_PASSES;
        }
        if (mask & 8u) {
            L->mode = g->opts->mode;
            L->set |= BURN_SET_MODE;
        }
        if (mask & 16u) {
            L->interval = g->opts->interval;
            L->set |= BURN_SET_INTERVAL;
        }
        if (mask & 32u) {
            L->kerf = g->opts->kerf;
            L->set |= BURN_SET_KERF;
        }
        if (mask & 64u) {
            L->tabs = g->opts->tabs;
            L->set |= BURN_SET_TABS;
        }
        if (mask & 128u) {
            L->tabw = g->opts->tabw;
            L->set |= BURN_SET_TABW;
        }
    }
}

static int cmd_get(void *c, void *g0)
{
    gb_rexx *g = app(g0);
    const char *key = omr_arg(c, "KEY");
    char buf[240], nbuf[16];
    int i, k;
    if (!g->opts) {
        omr_set_result(c, "no document");
        return OMR_ERROR;
    }
    if (!key) {
        for (i = 0; job_keys[i]; i++) {
        }
        snprintf(nbuf, sizeof nbuf, "%d", i);
        stem_put(c, NULL, "0", nbuf);
        for (i = 0; job_keys[i]; i++) {
            snprintf(nbuf, sizeof nbuf, "%d", i + 1);
            stem_put(c, NULL, nbuf, job_keys[i]);
            key_value(g->opts, i, buf, (int)sizeof buf);
            {
                char tail[24];
                snprintf(tail, sizeof tail, "%d.VALUE", i + 1);
                stem_put(c, NULL, tail, buf);
            }
        }
        omr_set_result(c, nbuf);
        return OMR_OK;
    }
    k = key_of(key);
    if (k < 0) {
        omr_set_result(c, "unknown key");
        return OMR_ERROR;
    }
    key_value(g->opts, k, buf, (int)sizeof buf);
    omr_set_result(c, buf);
    return OMR_OK;
}

/* order= on the command: 1-based layers, comma separated. Empty is the
 * file order. A layer left out follows, in file order. */
static int parse_order(gb_rexx *g, const char *value, char *err, int errlen)
{
    const char *s = value ? value : "";
    int seq[64], nseq = 0;
    int nlayer;
    while (*s == ' ' || *s == '\t') s++;
    if (!*s) {
        g->have_order = 0;
        g->nseq = 0;
        g->opts->seq = NULL;
        g->opts->nseq = 0;
        return 0;
    }
    if (!has_doc(g)) {
        snprintf(err, (size_t)errlen, "Open a drawing first");
        return -1;
    }
    nlayer = g->doc->nlayer;
    while (*s) {
        long n = 0;
        if (*s == ',') s++;
        while (*s == ' ' || *s == '\t') s++;
        if (*s < '0' || *s > '9') {
            snprintf(err, (size_t)errlen, "order is 1-based layers, comma separated");
            return -1;
        }
        while (*s >= '0' && *s <= '9') {
            n = n * 10 + (*s++ - '0');
            if (n > 100000) break;
        }
        while (*s == ' ' || *s == '\t') s++;
        if (n < 1 || nseq >= (int)(sizeof seq / sizeof seq[0])) {
            snprintf(err, (size_t)errlen, "order is 1-based layers, comma separated");
            return -1;
        }
        if (n > nlayer) {
            snprintf(err, (size_t)errlen, "order: no such layer %d", (int)n);
            return -1;
        }
        seq[nseq++] = (int)n - 1;
        if (*s && *s != ',') {
            snprintf(err, (size_t)errlen, "order is 1-based layers, comma separated");
            return -1;
        }
    }
    memcpy(g->seq, seq, (size_t)nseq * sizeof seq[0]);
    g->nseq = nseq;
    g->have_order = 1;
    g->opts->seq = g->seq;
    g->opts->nseq = nseq;
    return 0;
}

/* Up and Dn: move one layer in the burn order. INDEX is that layer, 1-based.
 * STEP -1 is Up, 1 is Dn. A step that cannot move leaves the list. */
static int cmd_order(void *c, void *g0)
{
    gb_rexx *g = app(g0);
    int index = 0, step = 0, has_i, has_s, n, i, at = -1, L, tmp, moved = 0;
    int seq[64];
    char buf[240];
    if (!g->opts) {
        omr_set_result(c, "no document");
        return OMR_ERROR;
    }
    has_i = arg_i(c, "INDEX", &index);
    has_s = arg_i(c, "STEP", &step);
    if (has_i < 0 || has_s < 0) {
        omr_set_result(c, "not a number");
        return OMR_ERROR;
    }
    if (has_i <= 0 || has_s <= 0) {
        omr_set_result(c, "INDEX and STEP are required");
        return OMR_ERROR;
    }
    if (step != -1 && step != 1) {
        omr_set_result(c, "STEP is -1 or 1");
        return OMR_ERROR;
    }
    if (!has_doc(g)) {
        omr_set_result(c, "Open a drawing first");
        return OMR_ERROR;
    }
    n = g->doc->nlayer;
    if (index < 1 || index > n) {
        omr_set_result(c, "no such layer");
        return OMR_ERROR;
    }
    if (n > 64) n = 64;
    if (g->have_order && g->nseq > 0) {
        int m = g->nseq > 64 ? 64 : g->nseq;
        for (i = 0; i < m; i++) seq[i] = g->seq[i];
        n = m;
    } else {
        for (i = 0; i < n; i++) seq[i] = i;
    }
    L = index - 1;
    for (i = 0; i < n; i++)
        if (seq[i] == L) at = i;
    if (at >= 0 && step < 0 && at > 0) {
        tmp = seq[at];
        seq[at] = seq[at - 1];
        seq[at - 1] = tmp;
        moved = 1;
    } else if (at >= 0 && step > 0 && at + 1 < n) {
        tmp = seq[at];
        seq[at] = seq[at + 1];
        seq[at + 1] = tmp;
        moved = 1;
    }
    if (!moved) {
        key_value(g->opts, key_of("order"), buf, (int)sizeof buf);
        omr_set_result(c, buf);
        return OMR_OK;
    }
    memcpy(g->seq, seq, (size_t)n * sizeof seq[0]);
    g->nseq = n;
    g->have_order = 1;
    g->opts->seq = g->seq;
    g->opts->nseq = n;
    dirty(g);
    key_value(g->opts, key_of("order"), buf, (int)sizeof buf);
    omr_set_result(c, buf);
    return OMR_OK;
}

static int cmd_set(void *c, void *g0)
{
    gb_rexx *g = app(g0);
    const char *key = omr_arg(c, "KEY"), *value = omr_arg(c, "VALUE");
    char err[160], buf[240];
    int k;
    err[0] = 0;
    if (!g->opts) {
        omr_set_result(c, "no document");
        return OMR_ERROR;
    }
    k = key_of(key);
    if (k < 0) {
        omr_set_result(c, "unknown key");
        return OMR_ERROR;
    }
    if (k == 9 || k == 10) {
        char *dst = (k == 9) ? g->start : g->end;
        int n = (k == 9) ? (int)sizeof g->start : (int)sizeof g->end;
        own_copy(dst, n, value);
        if (k == 9) g->opts->start = dst[0] ? dst : NULL;
        else g->opts->end = dst[0] ? dst : NULL;
        dirty(g);
        omr_set_result(c, dst);
        return OMR_OK;
    }
    if (k == 19) {
        if (parse_order(g, value, err, (int)sizeof err)) {
            omr_set_result(c, err);
            return OMR_ERROR;
        }
        dirty(g);
        key_value(g->opts, k, buf, (int)sizeof buf);
        omr_set_result(c, buf);
        return OMR_OK;
    }
    if (set_key(g->opts, k, value, err, (int)sizeof err)) {
        omr_set_result(c, err);
        return OMR_ERROR;
    }
    if (k == 11) g->invert = g->opts->invert;
    if (k == 12) g->width = g->opts->width;
    if (k == 2) {
        g->have_travel = 1;
        g->travel = g->opts->travel;
    }
    if (k == 5) {
        g->have_s100 = 1;
        g->s100 = g->opts->s100;
    }
    if (k == 0) layers_job(g, 1u);
    if (k == 1) layers_job(g, 2u);
    if (k == 3) layers_job(g, 4u);
    if (k == 6) layers_job(g, 8u);
    if (k == 7) layers_job(g, 16u);
    if (k == 16) layers_job(g, 32u);
    if (k == 17) layers_job(g, 64u);
    if (k == 18) layers_job(g, 128u);
    dirty(g);
    key_value(g->opts, k, buf, (int)sizeof buf);
    omr_set_result(c, buf);
    return OMR_OK;
}

static int cmd_status(void *c, void *g0)
{
    gb_rexx *g = app(g0);
    char buf[320], nbuf[16];
    const char *port = g->port_name ? g->port_name : "";
    const char *kind = g->file[0] ? burn_drawing_kind(g->file) : "";
    int layers = has_doc(g) ? g->doc->nlayer : 0;
    const char *mode = g->opts ? burn_mode_name(g->opts->mode) : "line";
    snprintf(buf, sizeof buf, "%.40s %.240s %d", port[0] ? port : GB_REXX_PORT, g->file[0] ? g->file : "-", g->dirty);
    omr_set_result(c, buf);
    stem_put(c, NULL, "PORT", port[0] ? port : GB_REXX_PORT);
    stem_put(c, NULL, "FILE", g->file);
    stem_put(c, NULL, "DIRTY", g->dirty ? "1" : "0");
    stem_put(c, NULL, "KIND", kind);
    snprintf(nbuf, sizeof nbuf, "%d", layers);
    stem_put(c, NULL, "LAYERS", nbuf);
    stem_put(c, NULL, "MODE", mode);
    stem_put(c, NULL, "MACHINE", (g->have_machine && g->machine.name[0]) ? g->machine.name : "");
    {
        char pbuf[32];
        key_value(g->opts, key_of("x"), pbuf, (int)sizeof pbuf);
        stem_put(c, NULL, "X", pbuf);
        key_value(g->opts, key_of("y"), pbuf, (int)sizeof pbuf);
        stem_put(c, NULL, "Y", pbuf);
        key_value(g->opts, key_of("rot"), pbuf, (int)sizeof pbuf);
        stem_put(c, NULL, "ROT", pbuf);
    }
    stem_put(c, NULL, "MATERIAL", g->material);
    return OMR_OK;
}

static int parse_colour(const char *s, unsigned *c)
{
    unsigned v = 0;
    int n = 0, i;
    const char *p = s;
    if (!s) return -1;
    if (*p == '#') p++;
    while (p[n]) {
        char ch = p[n];
        int d;
        if (ch >= '0' && ch <= '9') d = ch - '0';
        else if (ch >= 'a' && ch <= 'f') d = ch - 'a' + 10;
        else if (ch >= 'A' && ch <= 'F') d = ch - 'A' + 10;
        else break;
        v = (v << 4) | (unsigned)d;
        n++;
    }
    if (n == 3) {
        unsigned r = (v >> 8) & 0xf, g = (v >> 4) & 0xf, b = v & 0xf;
        *c = (r << 20) | (r << 16) | (g << 12) | (g << 8) | (b << 4) | b;
        return 0;
    }
    if (n == 6) {
        *c = v;
        return 0;
    }
    (void)i;
    return -1;
}

/* Find or add an override for document layer idx (1-based) or colour. */
static burn_layer *override_of(gb_rexx *g, int idx, unsigned colour, char *err, int errlen)
{
    burn_opts *o = g->opts;
    burn_layer *L;
    int i;
    if (!o || !g->store || g->store_cap < 1) {
        snprintf(err, (size_t)errlen, "no layer list");
        return NULL;
    }
    o->layer = g->store;
    for (i = 0; i < o->nlayer; i++) {
        L = &g->store[i];
        if (idx > 0 && L->index == idx) return L;
        if (idx <= 0 && colour != GV_NONE && L->index <= 0 && L->colour == colour) return L;
    }
    if (o->nlayer >= g->store_cap) {
        snprintf(err, (size_t)errlen, "too many layer overrides");
        return NULL;
    }
    L = &g->store[o->nlayer++];
    memset(L, 0, sizeof *L);
    L->size = (int)sizeof *L;
    if (idx > 0) {
        L->index = idx;
        L->colour = GV_NONE;
    } else {
        L->index = 0;
        L->colour = colour;
    }
    L->on = 1;
    return L;
}

/* The list's row for document layer at (0-based): the job, then that
 * layer's overrides. Same numbers the window's spinners show. */
static void layer_row(gb_rexx *g, int at, char *buf, int len)
{
    burn_layer ly;
    burn_layer_of(g->opts, g->doc, at, &ly);
    snprintf(buf, (size_t)len, "%.0f %.1f %d %s %d %.2f %.2f %d %.2f", (double)ly.power, (double)ly.speed, ly.passes,
             burn_mode_name(ly.mode), ly.on ? 1 : 0, (double)ly.interval, (double)ly.kerf, ly.tabs, (double)ly.tabw);
}

static int cmd_layer(void *c, void *g0)
{
    gb_rexx *g = app(g0);
    const char *colour_s = omr_arg(c, "COLOUR"), *mode_s = omr_arg(c, "MODE");
    char err[160], buf[96];
    burn_layer *L;
    int idx = 0, has_idx, on = 0, passes = 0, tabs = 0, has, mode = 0, at, setting;
    unsigned colour = GV_NONE;
    float power = 0, speed = 0, interval = 0, kerf = 0, tabw = 0;
    err[0] = 0;
    if (!has_doc(g)) {
        omr_set_result(c, "Open a drawing first");
        return OMR_ERROR;
    }
    if (!g->opts) {
        omr_set_result(c, "no document");
        return OMR_ERROR;
    }
    has_idx = arg_i(c, "INDEX", &idx);
    if (has_idx < 0) {
        omr_set_result(c, "not a number");
        return OMR_ERROR;
    }
    if (colour_s && parse_colour(colour_s, &colour)) {
        omr_set_result(c, "COLOUR is a #rrggbb colour");
        return OMR_ERROR;
    }
    if (has_idx <= 0 && colour == GV_NONE) {
        omr_set_result(c, "INDEX or COLOUR is required");
        return OMR_ERROR;
    }
    if (has_idx > 0 && (idx < 1 || idx > g->doc->nlayer)) {
        omr_set_result(c, "no such layer");
        return OMR_ERROR;
    }
    if (colour != GV_NONE && has_idx <= 0) {
        int i, found = 0;
        for (i = 0; i < g->doc->nlayer; i++)
            if (g->doc->layer[i].colour == colour) found = 1;
        if (!found) {
            omr_set_result(c, "no such layer");
            return OMR_ERROR;
        }
    }
    has = arg_f(c, "POWER", &power);
    if (has < 0) {
        omr_set_result(c, "not a number");
        return OMR_ERROR;
    }
    has = arg_f(c, "SPEED", &speed);
    if (has < 0) {
        omr_set_result(c, "not a number");
        return OMR_ERROR;
    }
    has = arg_i(c, "PASSES", &passes);
    if (has < 0) {
        omr_set_result(c, "not a number");
        return OMR_ERROR;
    }
    has = arg_i(c, "ON", &on);
    if (has < 0) {
        omr_set_result(c, "not a number");
        return OMR_ERROR;
    }
    if (mode_s) {
        mode = burn_mode_parse(mode_s);
        if (mode < 0) {
            omr_set_result(c, "MODE is line, fill or offset");
            return OMR_ERROR;
        }
    }
    has = arg_f(c, "INTERVAL", &interval);
    if (has < 0) {
        omr_set_result(c, "not a number");
        return OMR_ERROR;
    }
    has = arg_f(c, "KERF", &kerf);
    if (has < 0 || (has > 0 && kerf < 0.f)) {
        omr_set_result(c, has < 0 ? "not a number" : "kerf is 0 or more, in mm");
        return OMR_ERROR;
    }
    has = arg_i(c, "TABS", &tabs);
    if (has < 0 || (has > 0 && tabs < 0)) {
        omr_set_result(c, "tabs is a whole number, 0 to 32");
        return OMR_ERROR;
    }
    has = arg_f(c, "TABW", &tabw);
    if (has < 0 || (has > 0 && tabw < 0.f)) {
        omr_set_result(c, has < 0 ? "not a number" : "tab width is 0 or more, in mm");
        return OMR_ERROR;
    }
    setting = arg_f(c, "POWER", &power) > 0 || arg_f(c, "SPEED", &speed) > 0 || arg_i(c, "PASSES", &passes) > 0 ||
              arg_i(c, "ON", &on) > 0 || mode_s || arg_f(c, "INTERVAL", &interval) > 0 || arg_f(c, "KERF", &kerf) > 0 ||
              arg_i(c, "TABS", &tabs) > 0 || arg_f(c, "TABW", &tabw) > 0;
    at = idx - 1;
    if (has_idx <= 0) {
        int i;
        at = -1;
        for (i = 0; i < g->doc->nlayer; i++)
            if (g->doc->layer[i].colour == colour) {
                at = i;
                break;
            }
    }
    if (!setting) {
        if (at < 0) {
            omr_set_result(c, "no such layer");
            return OMR_ERROR;
        }
        layer_row(g, at, buf, (int)sizeof buf);
        omr_set_result(c, buf);
        return OMR_OK;
    }
    L = override_of(g, has_idx > 0 ? idx : 0, colour, err, (int)sizeof err);
    if (!L) {
        omr_set_result(c, err);
        return OMR_ERROR;
    }
    if (arg_f(c, "POWER", &power) > 0) {
        L->power = hold(power, 0.f, 100.f);
        L->set |= BURN_SET_POWER;
    }
    if (arg_f(c, "SPEED", &speed) > 0) {
        L->speed = hold(speed, 0.1f, 500.f);
        L->set |= BURN_SET_SPEED;
    }
    if (arg_i(c, "PASSES", &passes) > 0) {
        L->passes = passes < 1 ? 1 : passes;
        L->set |= BURN_SET_PASSES;
    }
    if (arg_i(c, "ON", &on) > 0) {
        L->on = on ? 1 : 0;
        L->set |= BURN_SET_ON;
    }
    if (mode_s) {
        L->mode = mode;
        L->set |= BURN_SET_MODE;
    }
    if (arg_f(c, "INTERVAL", &interval) > 0) {
        L->interval = hold(interval, 0.01f, 50.f);
        L->set |= BURN_SET_INTERVAL;
    }
    if (arg_f(c, "KERF", &kerf) > 0) {
        L->kerf = hold(kerf, 0.f, 20.f);
        L->set |= BURN_SET_KERF;
    }
    if (arg_i(c, "TABS", &tabs) > 0) {
        L->tabs = tabs > 32 ? 32 : tabs;
        L->set |= BURN_SET_TABS;
    }
    if (arg_f(c, "TABW", &tabw) > 0) {
        L->tabw = hold(tabw, 0.f, 20.f);
        L->set |= BURN_SET_TABW;
    }
    g->opts->layer = g->store;
    dirty(g);
    return OMR_OK;
}

static int cmd_machine(void *c, void *g0)
{
    gb_rexx *g = app(g0);
    const char *driver = omr_arg(c, "DRIVER"), *profile = omr_arg(c, "PROFILE");
    char err[160];
    err[0] = 0;
    if ((!driver || !driver[0]) && (!profile || !profile[0])) {
        omr_set_result(c, "DRIVER or PROFILE is required");
        return OMR_ERROR;
    }
    om_clear(&g->machine);
    g->have_machine = 0;
    if (driver && driver[0] && om_load(&g->machine, driver, err, (int)sizeof err)) {
        omr_set_result(c, err[0] ? err : "the driver did not load");
        return OMR_ERROR;
    }
    if (profile && profile[0] && om_load(&g->machine, profile, err, (int)sizeof err)) {
        omr_set_result(c, err[0] ? err : "the profile did not load");
        return OMR_ERROR;
    }
    if (g->machine.family != OM_LASER) {
        omr_set_result(c, "not a laser");
        om_clear(&g->machine);
        return OMR_ERROR;
    }
    g->have_machine = 1;
    dirty(g);
    omr_set_result(c, g->machine.name[0] ? g->machine.name : (profile && profile[0] ? profile : driver));
    return OMR_OK;
}

static int cmd_material(void *c, void *g0)
{
    gb_rexx *g = app(g0);
    const char *file = omr_arg(c, "FILE");
    burn_material mat;
    char err[160];
    err[0] = 0;
    if (!file) {
        omr_set_result(c, "FILE is required");
        return OMR_ERROR;
    }
    if (!g->opts) {
        omr_set_result(c, "no document");
        return OMR_ERROR;
    }
    memset(&mat, 0, sizeof mat);
    if (burn_material_load(&mat, file, err, (int)sizeof err)) {
        omr_set_result(c, err[0] ? err : "the material did not load");
        return OMR_ERROR;
    }
    burn_material_apply(&mat, g->opts);
    {
        unsigned mask = 0;
        if (mat.set & BURN_SET_POWER) mask |= 1u;
        if (mat.set & BURN_SET_SPEED) mask |= 2u;
        if (mat.set & BURN_SET_PASSES) mask |= 4u;
        if (mat.set & BURN_SET_MODE) mask |= 8u;
        if (mat.set & BURN_SET_INTERVAL) mask |= 16u;
        layers_job(g, mask);
    }
    snprintf(g->material, sizeof g->material, "%s", mat.name);
    dirty(g);
    omr_set_result(c, mat.name[0] ? mat.name : file);
    return OMR_OK;
}

/* The picker's list. DIR defaults to the same drawer the window scans.
 * It does not load a profile and it does not mark the job dirty. */
static int cmd_materials(void *c, void *g0)
{
    gb_rexx *g = app(g0);
    const char *dir = omr_arg(c, "DIR");
    burn_materials lib;
    char nbuf[16];
    int i;
    (void)g;
#if defined(AMIGA) || defined(__amigaos__)
    if (!dir || !dir[0]) dir = "PROGDIR:materials/laser";
#else
    if (!dir || !dir[0]) dir = "materials/laser";
#endif
    burn_materials_init(&lib);
    if (burn_materials_scan(&lib, dir)) {
        burn_materials_free(&lib);
        omr_set_result(c, "out of memory");
        return OMR_ERROR;
    }
    snprintf(nbuf, sizeof nbuf, "%d", lib.n);
    stem_put(c, NULL, "0", nbuf);
    for (i = 0; i < lib.n; i++) {
        char tail[24];
        snprintf(nbuf, sizeof nbuf, "%d", i + 1);
        stem_put(c, NULL, nbuf, lib.at[i].name);
        snprintf(tail, sizeof tail, "%d.PATH", i + 1);
        stem_put(c, NULL, tail, lib.at[i].path);
    }
    snprintf(nbuf, sizeof nbuf, "%d", lib.n);
    omr_set_result(c, nbuf);
    burn_materials_free(&lib);
    return OMR_OK;
}

static int cmd_laser(void *c, void *g0)
{
    gb_rexx *g = app(g0);
    char err[160], buf[96];
    int has, constant = 0, passes = 0;
    float power = 0, speed = 0, travel = 0, s100 = 0;
    unsigned mask = 0;
    err[0] = 0;
    if (!g->opts) {
        omr_set_result(c, "no document");
        return OMR_ERROR;
    }
    has = arg_f(c, "POWER", &power);
    if (has < 0) {
        omr_set_result(c, "not a number");
        return OMR_ERROR;
    }
    if (has > 0) {
        g->opts->power = hold(power, 0.f, 100.f);
        mask |= 1u;
    }
    has = arg_f(c, "SPEED", &speed);
    if (has < 0) {
        omr_set_result(c, "not a number");
        return OMR_ERROR;
    }
    if (has > 0) {
        g->opts->speed = hold(speed, 0.1f, 500.f);
        mask |= 2u;
    }
    has = arg_f(c, "TRAVEL", &travel);
    if (has < 0) {
        omr_set_result(c, "not a number");
        return OMR_ERROR;
    }
    if (has > 0) {
        g->opts->travel = hold(travel, 0.f, 1000.f);
        g->have_travel = 1;
        g->travel = g->opts->travel;
    }
    has = arg_i(c, "PASSES", &passes);
    if (has < 0) {
        omr_set_result(c, "not a number");
        return OMR_ERROR;
    }
    if (has > 0) {
        g->opts->passes = passes < 1 ? 1 : passes;
        mask |= 4u;
    }
    has = arg_i(c, "CONSTANT", &constant);
    if (has < 0) {
        omr_set_result(c, "not a number");
        return OMR_ERROR;
    }
    if (has > 0) g->opts->constant = constant ? 1 : 0;
    has = arg_f(c, "S100", &s100);
    if (has < 0) {
        omr_set_result(c, "not a number");
        return OMR_ERROR;
    }
    if (has > 0) {
        g->opts->s100 = hold(s100, 1.f, 10000.f);
        g->have_s100 = 1;
        g->s100 = g->opts->s100;
    }
    layers_job(g, mask);
    dirty(g);
    snprintf(buf, sizeof buf, "%g %g", (double)g->opts->power, (double)g->opts->speed);
    omr_set_result(c, buf);
    (void)err;
    return OMR_OK;
}

static int cmd_mode(void *c, void *g0)
{
    gb_rexx *g = app(g0);
    const char *name = omr_arg(c, "NAME");
    int mode;
    if (!g->opts) {
        omr_set_result(c, "no document");
        return OMR_ERROR;
    }
    mode = burn_mode_parse(name);
    if (mode < 0) {
        omr_set_result(c, "NAME is line, fill or offset");
        return OMR_ERROR;
    }
    g->opts->mode = mode;
    layers_job(g, 8u);
    dirty(g);
    omr_set_result(c, burn_mode_name(mode));
    return OMR_OK;
}

static int build_path(gb_rexx *g, float **speeds, char *err, int errlen)
{
    float *sp = NULL;
    int rc;
    if (speeds) *speeds = NULL;
    if (!has_doc(g) || !g->path || !g->have_path) {
        snprintf(err, (size_t)errlen, "Open a drawing first");
        return -1;
    }
    gt_free(g->path);
    gt_init(g->path);
    *g->have_path = 0;
    rc = (g->have_image && *g->have_image) ? burn_image_path(g->path, &sp, g->pic, g->opts)
                                           : burn_path(g->path, &sp, g->doc, g->opts);
    if (rc) {
        free(sp);
        snprintf(err, (size_t)errlen, "out of memory");
        return -1;
    }
    if (g->path->n < 2) {
        free(sp);
        snprintf(err, (size_t)errlen, "no paths to burn");
        gt_free(g->path);
        gt_init(g->path);
        return -1;
    }
    *g->have_path = 1;
    if (speeds) *speeds = sp;
    else free(sp);
    return 0;
}

/* Same words as burn_ui_status. */
static void preview_line(char *buf, int len, const char *name, int moves, float seconds)
{
    char time[40];
    if (seconds > 0.f && seconds < 60.f)
        snprintf(time, sizeof time, ", about %.1f s", (double)seconds);
    else if (seconds >= 60.f) {
        int s = (int)(seconds + 0.5f);
        snprintf(time, sizeof time, ", about %d min %d s", s / 60, s % 60);
    } else
        time[0] = 0;
    snprintf(buf, (size_t)len, "%d moves for %s%s", moves, (name && name[0]) ? name : "the laser", time);
}

static int write_ppm(const char *file, const unsigned char *rgb, int w, int h)
{
    FILE *f;
    if (!file || !rgb || w < 1 || h < 1) return -1;
    f = fopen(file, "wb");
    if (!f) return -1;
    fprintf(f, "P6\n%d %d\n255\n", w, h);
    if (fwrite(rgb, 1, (size_t)w * (size_t)h * 3, f) != (size_t)w * (size_t)h * 3) {
        fclose(f);
        return -1;
    }
    fclose(f);
    return 0;
}

static int cmd_preview(void *c, void *g0)
{
    gb_rexx *g = app(g0);
    const char *file = omr_arg(c, "FILE");
    float *speeds = NULL;
    char err[160];
    err[0] = 0;
    if (build_path(g, &speeds, err, (int)sizeof err)) {
        omr_set_result(c, err);
        return OMR_ERROR;
    }
    if (!file || !file[0]) {
        gc_laser L;
        gc_laser_more more;
        gc_stats st;
        char line[160];
        const om_machine *m = (g->have_machine && g->machine.family == OM_LASER) ? &g->machine : NULL;
        gc_laser_defaults(&L);
        if (g->opts) {
            L.constant = g->opts->constant;
            L.s100 = g->opts->s100;
            L.speed = g->opts->speed;
            L.travel = g->opts->travel;
        }
        gc_laser_more_defaults(&more, &L);
        more.speed = speeds;
        memset(&st, 0, sizeof st);
        gc_laser_estimate_more(g->path, m, &more, &st);
        free(speeds);
        preview_line(line, (int)sizeof line, m ? m->name : "the laser", st.moves, st.seconds);
        omr_set_result(c, line);
        return OMR_OK;
    }
    free(speeds);
    {
        enum { W = 320, H = 240 };
        unsigned char *rgb;
        gs_cam cam;
        rgb = (unsigned char *)malloc((size_t)W * (size_t)H * 3);
        if (!rgb) {
            omr_set_result(c, "out of memory");
            return OMR_ERROR;
        }
        gs_cam_home(&cam);
        if (gt_paint_part(g->path, &cam, 0, rgb, W, H) || write_ppm(file, rgb, W, H)) {
            free(rgb);
            omr_set_result(c, "Could not write there");
            return OMR_ERROR;
        }
        free(rgb);
        omr_set_result(c, file);
    }
    return OMR_OK;
}

static int cmd_frame(void *c, void *g0)
{
    gb_rexx *g = app(g0);
    const char *file = omr_arg(c, "FILE");
    char err[160];
    gc_stats st;
    err[0] = 0;
    if (!file) {
        omr_set_result(c, "FILE is required");
        return OMR_ERROR;
    }
    if (!g->have_machine || g->machine.family != OM_LASER) {
        omr_set_result(c, "no laser");
        return OMR_ERROR;
    }
    if (!has_doc(g) || !g->file[0]) {
        omr_set_result(c, "Open a drawing first");
        return OMR_ERROR;
    }
    memset(&st, 0, sizeof st);
    if (burn_write_frame(g->file, file, &g->machine, g->opts, &st, err, (int)sizeof err)) {
        omr_set_result(c, err[0] ? err : "Could not write there");
        return OMR_ERROR;
    }
    omr_set_result(c, file);
    return OMR_OK;
}

static const char *const look_names[] = {"top", "front", "side", "frame"};
static const char *const look_said[] = {"Top.", "Front.", "Side.", "Framed."};

/* Top, front, side, or the view at open. Not the Frame button. The picture changes. */
static int cmd_look(void *c, void *g0)
{
    gb_rexx *g = app(g0);
    const char *name = omr_arg(c, "NAME");
    int which = -1, i;
    if (name) {
        for (i = 0; i < 4; i++)
            if (eqi(name, look_names[i])) which = i;
    }
    if (which < 0) {
        omr_set_result(c, "NAME is top, front, side or frame");
        return OMR_ERROR;
    }
    g->look = which;
    omr_set_result(c, look_said[which]);
    return OMR_OK;
}

static const omr_verb verbs[] = {
    {sizeof(omr_verb), "VERSION", "", "The program name and version", cmd_version},
    {sizeof(omr_verb), "QUIT", "FORCE/S", "Close the window; FORCE discards a dirty document", cmd_quit},
    {sizeof(omr_verb), "OPEN", "FILE/A", "Open an SVG, a DXF or a Netpbm picture", cmd_open},
    {sizeof(omr_verb), "SAVE", "FILE/K STEM/K VAR/K", "Write Grbl laser G-code", cmd_save},
    {sizeof(omr_verb), "ACTIVATE", "", "Bring the window to the front", cmd_activate},
    {sizeof(omr_verb), "HIDE", "", "Send the window to the back", cmd_hide},
    {sizeof(omr_verb), "GET", "KEY/K STEM/K VAR/K", "Read a job setting", cmd_get},
    {sizeof(omr_verb), "SET", "KEY/A VALUE/A", "Set a job setting", cmd_set},
    {sizeof(omr_verb), "STATUS", "STEM/K VAR/K", "The port, the file, whether it is dirty, kind and mode", cmd_status},
    {sizeof(omr_verb), "LAYER",
     "INDEX/N/K COLOUR/K POWER/K SPEED/K PASSES/N/K ON/N/K MODE/K INTERVAL/K KERF/K TABS/N/K TABW/K",
     "one layer's row, as the list", cmd_layer},
    {sizeof(omr_verb), "ORDER", "INDEX/N/A STEP/N/A", "move a layer up or down, as Up and Dn", cmd_order},
    {sizeof(omr_verb), "MATERIAL", "FILE/A", "Load a materials profile", cmd_material},
    {sizeof(omr_verb), "MATERIALS", "DIR/K STEM/K", "the materials the picker lists", cmd_materials},
    {sizeof(omr_verb), "MACHINE", "DRIVER/K PROFILE/K", "Pick the laser from a driver and profile", cmd_machine},
    {sizeof(omr_verb), "LASER", "POWER/K SPEED/K TRAVEL/K PASSES/N/K CONSTANT/N/K S100/K",
     "The job's default power, speed and passes", cmd_laser},
    {sizeof(omr_verb), "MODE", "NAME/A", "The job's default mode: line, fill or offset", cmd_mode},
    {sizeof(omr_verb), "PREVIEW", "FILE/K", "Build the job's preview; with FILE, write a PPM", cmd_preview},
    {sizeof(omr_verb), "FRAME", "FILE/A", "Write a travel around the job with the laser off", cmd_frame},
    {sizeof(omr_verb), "LOOK", "NAME/A", "top, front, side or frame", cmd_look},
};

void gb_rexx_init(gb_rexx *g)
{
    if (!g) return;
    memset(g, 0, sizeof *g);
    g->size = (int)sizeof *g;
    g->look = -1;
}

void gb_rexx_keep(gb_rexx *g)
{
    if (!g || !g->opts) return;
    g->opts->start = g->start[0] ? g->start : NULL;
    g->opts->end = g->end[0] ? g->end : NULL;
    g->opts->invert = g->invert;
    g->opts->width = g->width;
    if (g->have_travel) g->opts->travel = g->travel;
    if (g->have_s100) g->opts->s100 = g->s100;
    if (g->have_order && g->nseq > 0) {
        g->opts->seq = g->seq;
        g->opts->nseq = g->nseq;
    }
}

const omr_verb *gb_rexx_verbs(void) { return verbs; }

int gb_rexx_nverbs(void) { return (int)(sizeof verbs / sizeof verbs[0]); }
