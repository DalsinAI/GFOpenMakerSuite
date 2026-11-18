/* GFOpenCNC's ARexx command table. The window Wait()s on GFOPENCNC.1
 * and dispatches through these verbs; host tests use omr_do_line.
 * The shared commands only. A preview verb is not in this cut.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "cnc_rexx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static oc_rexx *app(void *g) { return (oc_rexx *)g; }

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

static int stem_put(omr_cmd *cmd, const char *tail, const char *value)
{
    const char *s = omr_arg(cmd, "STEM");
    if (!s || !s[0]) return 0;
    return omr_set_stem(cmd, s, tail, value);
}

static void result_var(omr_cmd *cmd, const char *value)
{
    const char *var = omr_arg(cmd, "VAR");
    omr_set_result(cmd, value);
    if (var && var[0]) omr_set_var(cmd, var, value);
}

static float hold(float v, float lo, float hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

/* The window's spinner ranges. mode is not a number. */
static const char *const keys[] = {"tool", "depth", "stepdown", "stepover", "feed", "plunge",
                                   "safe", "rpm",   "mode",     "x",        "y",   NULL};
static const float lo[] = {0.f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.f, 0.f, -10000.f, -10000.f};
static const float hi[] = {40.f, 80.f, 40.f, 40.f, 200.f, 80.f, 80.f, 60000.f, 0.f, 10000.f, 10000.f};

static int key_of(const char *s)
{
    int i;
    if (!s) return -1;
    for (i = 0; keys[i]; i++)
        if (eqi(s, keys[i])) return i;
    return -1;
}

static float *field(cnc_opts *o, int k)
{
    switch (k) {
    case 0: return &o->tool;
    case 1: return &o->depth;
    case 2: return &o->stepdown;
    case 3: return &o->stepover;
    case 4: return &o->feed;
    case 5: return &o->plunge;
    case 6: return &o->safe;
    case 7: return &o->rpm;
    case 9: return &o->x;
    case 10: return &o->y;
    default: return NULL;
    }
}

static void key_text(const cnc_opts *o, int k, char *buf, int n)
{
    const float *f;
    if (k == 8) {
        snprintf(buf, (size_t)n, "%s", cnc_mode_name(o->mode));
        return;
    }
    f = field((cnc_opts *)o, k);
    snprintf(buf, (size_t)n, "%.4g", f ? (double)*f : 0.0);
}

static int set_key(cnc_opts *o, int k, const char *value, char *err, int errlen)
{
    char *end = NULL;
    float v;
    int mode;
    if (k == 8) {
        char folded[16];
        int i;
        if (!value) {
            snprintf(err, (size_t)errlen, "mode is profile, pocket or drill");
            return -1;
        }
        for (i = 0; value[i] && i < (int)sizeof folded - 1; i++) {
            char c = value[i];
            if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
            folded[i] = c;
        }
        folded[i] = 0;
        mode = cnc_mode_parse(folded);
        if (mode < 0) {
            snprintf(err, (size_t)errlen, "mode is profile, pocket or drill");
            return -1;
        }
        o->mode = mode;
        return 0;
    }
    if (!value || !value[0]) {
        snprintf(err, (size_t)errlen, "a number is required");
        return -1;
    }
    v = (float)strtod(value, &end);
    if (!end || end == value) {
        snprintf(err, (size_t)errlen, "a number is required");
        return -1;
    }
    while (*end == ' ' || *end == '\t') end++;
    if (*end) {
        snprintf(err, (size_t)errlen, "a number is required");
        return -1;
    }
    *field(o, k) = hold(v, lo[k], hi[k]);
    return 0;
}

static int cmd_version(void *c, void *g0)
{
    oc_rexx *g = app(g0);
    omr_set_result(c, g->version && g->version[0] ? g->version : "GFOpenCNC 0.1.0");
    return OMR_OK;
}

static int cmd_quit(void *c, void *g0)
{
    oc_rexx *g = app(g0);
    if (g->dirty && !omr_switch(c, "FORCE")) {
        omr_set_result(c, "the document has not been saved");
        return OMR_ERROR;
    }
    g->quit = 1;
    return OMR_OK;
}

static int cmd_open(void *c, void *g0)
{
    oc_rexx *g = app(g0);
    const char *file = omr_arg(c, "FILE");
    char err[160];
    err[0] = 0;
    if (!file || !file[0]) {
        omr_set_result(c, "FILE is required");
        return OMR_ERROR;
    }
    if (strlen(file) >= sizeof g->file) {
        omr_set_result(c, "the file name is too long");
        return OMR_ERROR;
    }
    if (!g->doc || !g->have_doc) {
        omr_set_result(c, "no document");
        return OMR_ERROR;
    }
    gv_doc_free(g->doc);
    gv_doc_init(g->doc);
    *g->have_doc = 0;
    if (g->have_path) *g->have_path = 0;
    if (gv_load(g->doc, file, err, (int)sizeof err)) {
        omr_set_result(c, err[0] ? err : "the drawing did not load");
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
    oc_rexx *g = app(g0);
    const char *file = omr_arg(c, "FILE");
    char err[160];
    gc_stats st;
    err[0] = 0;
    if (!file || !file[0]) file = g->save_file;
    if (!file || !file[0]) {
        omr_set_result(c, "no file to write");
        return OMR_ERROR;
    }
    if (!g->file[0]) {
        omr_set_result(c, "Open a drawing first");
        return OMR_ERROR;
    }
    if (!g->have_machine || !g->opts) {
        omr_set_result(c, "no mill");
        return OMR_ERROR;
    }
    memset(&st, 0, sizeof st);
    if (cnc_write(g->file, file, &g->machine, g->opts, &st, err, (int)sizeof err)) {
        omr_set_result(c, err[0] ? err : "Could not write there");
        return OMR_ERROR;
    }
    if (strlen(file) < sizeof g->save_file) snprintf(g->save_file, sizeof g->save_file, "%s", file);
    g->dirty = 0;
    omr_set_result(c, file);
    return OMR_OK;
}

static int cmd_activate(void *c, void *g0)
{
    oc_rexx *g = app(g0);
    (void)c;
    if (g->activate && g->win) g->activate(g->win);
    return OMR_OK;
}

static int cmd_hide(void *c, void *g0)
{
    oc_rexx *g = app(g0);
    (void)c;
    if (g->hide && g->win) g->hide(g->win);
    return OMR_OK;
}

static int cmd_get(void *c, void *g0)
{
    oc_rexx *g = app(g0);
    const char *key = omr_arg(c, "KEY");
    char buf[64], nbuf[16];
    int i, k;
    if (!g->opts) {
        omr_set_result(c, "no document");
        return OMR_ERROR;
    }
    if (!key || !key[0]) {
        int nkeys = 0;
        while (keys[nkeys]) nkeys++;
        snprintf(nbuf, sizeof nbuf, "%d", nkeys);
        stem_put(c, "0", nbuf);
        for (i = 0; i < nkeys; i++) {
            char tail[24];
            snprintf(nbuf, sizeof nbuf, "%d", i + 1);
            stem_put(c, nbuf, keys[i]);
            key_text(g->opts, i, buf, (int)sizeof buf);
            snprintf(tail, sizeof tail, "%d.VALUE", i + 1);
            stem_put(c, tail, buf);
        }
        snprintf(nbuf, sizeof nbuf, "%d", nkeys);
        omr_set_result(c, nbuf);
        return OMR_OK;
    }
    k = key_of(key);
    if (k < 0) {
        omr_set_result(c, "unknown key");
        return OMR_ERROR;
    }
    key_text(g->opts, k, buf, (int)sizeof buf);
    result_var(c, buf);
    return OMR_OK;
}

static int cmd_set(void *c, void *g0)
{
    oc_rexx *g = app(g0);
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
    if (set_key(g->opts, k, value, err, (int)sizeof err)) {
        omr_set_result(c, err[0] ? err : "that value was refused");
        return OMR_ERROR;
    }
    g->dirty = 1;
    key_text(g->opts, k, buf, (int)sizeof buf);
    omr_set_result(c, buf);
    return OMR_OK;
}

static int cmd_status(void *c, void *g0)
{
    oc_rexx *g = app(g0);
    char buf[320];
    const char *port = g->port_name && g->port_name[0] ? g->port_name : OC_REXX_PORT;
    const char *mode = g->opts ? cnc_mode_name(g->opts->mode) : "profile";
    snprintf(buf, sizeof buf, "%.40s %.200s %d %s", port, g->file[0] ? g->file : "-", g->dirty, mode);
    omr_set_result(c, buf);
    stem_put(c, "PORT", port);
    stem_put(c, "FILE", g->file);
    stem_put(c, "DIRTY", g->dirty ? "1" : "0");
    stem_put(c, "MODE", mode);
    return OMR_OK;
}

static const omr_verb verbs[] = {
    {sizeof(omr_verb), "VERSION", "", "The program name and version", cmd_version},
    {sizeof(omr_verb), "QUIT", "FORCE/S", "Close the window; FORCE discards a dirty document", cmd_quit},
    {sizeof(omr_verb), "OPEN", "FILE/A", "Open an SVG or a DXF", cmd_open},
    {sizeof(omr_verb), "SAVE", "FILE/K", "Write mill G-code for the mill picked", cmd_save},
    {sizeof(omr_verb), "ACTIVATE", "", "Bring the window to the front", cmd_activate},
    {sizeof(omr_verb), "HIDE", "", "Send the window to the back", cmd_hide},
    {sizeof(omr_verb), "GET", "KEY/K STEM/K VAR/K", "Read a cut setting", cmd_get},
    {sizeof(omr_verb), "SET", "KEY/A VALUE/A", "Set a cut setting, held to the window's range", cmd_set},
    {sizeof(omr_verb), "STATUS", "STEM/K VAR/K", "The port, the file, whether it is dirty, and the mode", cmd_status},
};

void oc_rexx_init(oc_rexx *g)
{
    if (!g) return;
    memset(g, 0, sizeof *g);
    g->size = (int)sizeof *g;
}

const omr_verb *oc_rexx_verbs(void) { return verbs; }

int oc_rexx_nverbs(void) { return (int)(sizeof verbs / sizeof verbs[0]); }
