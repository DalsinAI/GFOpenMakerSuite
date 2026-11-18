/* OpenMakeBurn's ARexx command table. The window Wait()s on OPENMAKEBURN.1
 * and dispatches through these verbs; host tests use omr_do_line.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "burn_rexx.h"

#include "burn_material.h"
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
                                       "s100",   "mode",     "interval", "cuts_last", "start", "end", NULL};

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

static int cmd_set(void *c, void *g0)
{
    gb_rexx *g = app(g0);
    const char *key = omr_arg(c, "KEY"), *value = omr_arg(c, "VALUE");
    char err[160], buf[64];
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
    if (set_key(g->opts, k, value, err, (int)sizeof err)) {
        omr_set_result(c, err);
        return OMR_ERROR;
    }
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

static int cmd_layer(void *c, void *g0)
{
    gb_rexx *g = app(g0);
    const char *colour_s = omr_arg(c, "COLOUR"), *mode_s = omr_arg(c, "MODE");
    char err[160];
    burn_layer *L;
    int idx = 0, has_idx, on = 0, passes = 0, has;
    unsigned colour = GV_NONE;
    float power = 0, speed = 0, interval = 0;
    err[0] = 0;
    if (!has_doc(g)) {
        omr_set_result(c, "Open a drawing first");
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
    L = override_of(g, has_idx > 0 ? idx : 0, colour, err, (int)sizeof err);
    if (!L) {
        omr_set_result(c, err);
        return OMR_ERROR;
    }
    has = arg_f(c, "POWER", &power);
    if (has < 0) {
        omr_set_result(c, "not a number");
        return OMR_ERROR;
    }
    if (has > 0) {
        L->power = hold(power, 0.f, 100.f);
        L->set |= BURN_SET_POWER;
    }
    has = arg_f(c, "SPEED", &speed);
    if (has < 0) {
        omr_set_result(c, "not a number");
        return OMR_ERROR;
    }
    if (has > 0) {
        L->speed = hold(speed, 0.1f, 500.f);
        L->set |= BURN_SET_SPEED;
    }
    has = arg_i(c, "PASSES", &passes);
    if (has < 0) {
        omr_set_result(c, "not a number");
        return OMR_ERROR;
    }
    if (has > 0) {
        L->passes = passes < 1 ? 1 : passes;
        L->set |= BURN_SET_PASSES;
    }
    has = arg_i(c, "ON", &on);
    if (has < 0) {
        omr_set_result(c, "not a number");
        return OMR_ERROR;
    }
    if (has > 0) {
        L->on = on ? 1 : 0;
        L->set |= BURN_SET_ON;
    }
    if (mode_s) {
        int mode = burn_mode_parse(mode_s);
        if (mode < 0) {
            omr_set_result(c, "MODE is line, fill or offset");
            return OMR_ERROR;
        }
        L->mode = mode;
        L->set |= BURN_SET_MODE;
    }
    has = arg_f(c, "INTERVAL", &interval);
    if (has < 0) {
        omr_set_result(c, "not a number");
        return OMR_ERROR;
    }
    if (has > 0) {
        L->interval = hold(interval, 0.01f, 50.f);
        L->set |= BURN_SET_INTERVAL;
    }
    g->opts->layer = g->store;
    dirty(g);
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
    dirty(g);
    omr_set_result(c, mat.name[0] ? mat.name : file);
    return OMR_OK;
}

static int cmd_laser(void *c, void *g0)
{
    gb_rexx *g = app(g0);
    char err[160], buf[96];
    int has, constant = 0, passes = 0;
    float power = 0, speed = 0, travel = 0, s100 = 0;
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
    if (has > 0) g->opts->power = hold(power, 0.f, 100.f);
    has = arg_f(c, "SPEED", &speed);
    if (has < 0) {
        omr_set_result(c, "not a number");
        return OMR_ERROR;
    }
    if (has > 0) g->opts->speed = hold(speed, 0.1f, 500.f);
    has = arg_f(c, "TRAVEL", &travel);
    if (has < 0) {
        omr_set_result(c, "not a number");
        return OMR_ERROR;
    }
    if (has > 0) g->opts->travel = hold(travel, 0.f, 1000.f);
    has = arg_i(c, "PASSES", &passes);
    if (has < 0) {
        omr_set_result(c, "not a number");
        return OMR_ERROR;
    }
    if (has > 0) g->opts->passes = passes < 1 ? 1 : passes;
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
    if (has > 0) g->opts->s100 = hold(s100, 1.f, 10000.f);
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
    dirty(g);
    omr_set_result(c, burn_mode_name(mode));
    return OMR_OK;
}

static int build_path(gb_rexx *g, char *err, int errlen)
{
    float *speeds = NULL;
    int rc;
    if (!has_doc(g) || !g->path || !g->have_path) {
        snprintf(err, (size_t)errlen, "Open a drawing first");
        return -1;
    }
    gt_free(g->path);
    gt_init(g->path);
    *g->have_path = 0;
    rc = (g->have_image && *g->have_image) ? burn_image_path(g->path, &speeds, g->pic, g->opts)
                                           : burn_path(g->path, &speeds, g->doc, g->opts);
    free(speeds);
    if (rc) {
        snprintf(err, (size_t)errlen, "out of memory");
        return -1;
    }
    if (g->path->n < 2) {
        snprintf(err, (size_t)errlen, "no paths to burn");
        gt_free(g->path);
        gt_init(g->path);
        return -1;
    }
    *g->have_path = 1;
    return 0;
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
    char err[160];
    err[0] = 0;
    if (build_path(g, err, (int)sizeof err)) {
        omr_set_result(c, err);
        return OMR_ERROR;
    }
    if (file && file[0]) {
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

static const omr_verb verbs[] = {
    {sizeof(omr_verb), "VERSION", "", "The program name and version", cmd_version},
    {sizeof(omr_verb), "QUIT", "FORCE/S", "Close the window; FORCE discards a dirty document", cmd_quit},
    {sizeof(omr_verb), "OPEN", "FILE/A", "Open an SVG, a DXF or a Netpbm picture", cmd_open},
    {sizeof(omr_verb), "SAVE", "FILE/K", "Write Grbl laser G-code", cmd_save},
    {sizeof(omr_verb), "ACTIVATE", "", "Bring the window to the front", cmd_activate},
    {sizeof(omr_verb), "HIDE", "", "Send the window to the back", cmd_hide},
    {sizeof(omr_verb), "GET", "KEY/K STEM/K VAR/K", "Read a job setting", cmd_get},
    {sizeof(omr_verb), "SET", "KEY/A VALUE/A", "Set a job setting", cmd_set},
    {sizeof(omr_verb), "STATUS", "STEM/K VAR/K", "The port, the file, whether it is dirty, kind and mode", cmd_status},
    {sizeof(omr_verb), "LAYER", "INDEX/N/K COLOUR/K POWER/K SPEED/K PASSES/N/K ON/N/K MODE/K INTERVAL/K",
     "Set one layer's overrides", cmd_layer},
    {sizeof(omr_verb), "MATERIAL", "FILE/A", "Load a materials profile", cmd_material},
    {sizeof(omr_verb), "LASER", "POWER/K SPEED/K TRAVEL/K PASSES/N/K CONSTANT/N/K S100/K",
     "The job's default power, speed and passes", cmd_laser},
    {sizeof(omr_verb), "MODE", "NAME/A", "The job's default mode: line, fill or offset", cmd_mode},
    {sizeof(omr_verb), "PREVIEW", "FILE/K", "Build the job's preview; with FILE, write a PPM", cmd_preview},
    {sizeof(omr_verb), "FRAME", "FILE/A", "Write a travel around the job with the laser off", cmd_frame},
};

void gb_rexx_init(gb_rexx *g)
{
    if (!g) return;
    memset(g, 0, sizeof *g);
    g->size = (int)sizeof *g;
}

const omr_verb *gb_rexx_verbs(void) { return verbs; }

int gb_rexx_nverbs(void) { return (int)(sizeof verbs / sizeof verbs[0]); }
