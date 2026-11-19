/* GFOpenCNC's ARexx command table. The window Wait()s on GFOPENCNC.1
 * and dispatches through these verbs; host tests use omr_do_line.
 * The shared commands, and PREVIEW: the cut, the window's line, and a PPM when FILE is given.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "cnc_rexx.h"

#include "gs_toolpath.h"
#include "gs_view.h"
#include "omview_machines.h"

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
                                   "safe", "rpm",   "mode",     "x",        "y",   "start", "end", NULL};
static const float lo[] = {0.f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.f, 0.f, -10000.f, -10000.f, 0.f, 0.f};
static const float hi[] = {40.f, 80.f, 40.f, 40.f, 200.f, 80.f, 80.f, 60000.f, 0.f, 10000.f, 10000.f, 0.f, 0.f};

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
    if (k == 11) {
        snprintf(buf, (size_t)n, "%s", o->start ? o->start : "");
        return;
    }
    if (k == 12) {
        snprintf(buf, (size_t)n, "%s", o->end ? o->end : "");
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
    omr_set_result(c, g->version && g->version[0] ? g->version : "GFOpenCNC 0.1.0-rc3");
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
    {
        char nbuf[32];
        snprintf(nbuf, sizeof nbuf, "%d", st.moves);
        stem_put(c, "MOVES", nbuf);
        snprintf(nbuf, sizeof nbuf, "%.3f", (double)st.seconds);
        stem_put(c, "SECONDS", nbuf);
    }
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
    oc_rexx *g = app(g0);
    const char *key = omr_arg(c, "KEY");
    char buf[240], nbuf[16];
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
    if (k == 11 || k == 12) {
        char *dst = (k == 11) ? g->start : g->end;
        int n = (k == 11) ? (int)sizeof g->start : (int)sizeof g->end;
        own_copy(dst, n, value);
        if (k == 11) g->opts->start = dst[0] ? dst : NULL;
        else g->opts->end = dst[0] ? dst : NULL;
        g->dirty = 1;
        omr_set_result(c, dst);
        return OMR_OK;
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

static const char *drawing_kind(const char *path)
{
    const char *dot;
    if (!path || !path[0]) return "File";
    dot = strrchr(path, '.');
    if (dot && (!strcmp(dot, ".svg") || !strcmp(dot, ".SVG"))) return "SVG";
    if (dot && (!strcmp(dot, ".dxf") || !strcmp(dot, ".DXF"))) return "DXF";
    return "File";
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
    stem_put(c, "KIND", drawing_kind(g->file));
    stem_put(c, "MILL", (g->have_machine && g->machine.name[0]) ? g->machine.name : "");
    {
        char pbuf[32];
        if (g->opts) {
            key_text(g->opts, key_of("x"), pbuf, (int)sizeof pbuf);
            stem_put(c, "X", pbuf);
            key_text(g->opts, key_of("y"), pbuf, (int)sizeof pbuf);
            stem_put(c, "Y", pbuf);
        } else {
            stem_put(c, "X", "");
            stem_put(c, "Y", "");
        }
    }
    return OMR_OK;
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

/* The window's Preview: plan the cut, and with FILE write that picture. */
static int cmd_preview(void *c, void *g0)
{
    oc_rexx *g = app(g0);
    const char *file = omr_arg(c, "FILE");
    char err[160];
    err[0] = 0;
    if (!g->doc || !g->have_doc || !*g->have_doc || !g->path || !g->have_path || !g->opts) {
        omr_set_result(c, "Open a drawing first");
        return OMR_ERROR;
    }
    if (cnc_path(g->path, g->doc, g->opts, err, (int)sizeof err) || g->path->n < 2) {
        *g->have_path = 0;
        omr_set_result(c, err[0] ? err : "no paths to cut");
        return OMR_ERROR;
    }
    *g->have_path = 1;
    if (!file || !file[0]) {
        char line[96];
        const char *mill = (g->have_machine && g->machine.name[0]) ? g->machine.name : "";
        snprintf(line, sizeof line, "%s, %s. %d moves, 0 s.", mill[0] ? mill : "No mill", cnc_mode_name(g->opts->mode),
                 g->path->n);
        omr_set_result(c, line);
        return OMR_OK;
    }
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

static int cmd_mill(void *c, void *g0)
{
    oc_rexx *g = app(g0);
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
    if (g->machine.family != OM_CNC) {
        omr_set_result(c, "not a mill");
        om_clear(&g->machine);
        return OMR_ERROR;
    }
    g->have_machine = 1;
    g->dirty = 1;
    omr_set_result(c, g->machine.name[0] ? g->machine.name : (profile && profile[0] ? profile : driver));
    return OMR_OK;
}

/* The picker's list. DIR is the machines drawer the window scans.
 * It does not load a mill and it does not mark the cut dirty. */
static int cmd_mills(void *c, void *g0)
{
    oc_rexx *g = app(g0);
    const char *dir = omr_arg(c, "DIR");
    omv_machines lib;
    char nbuf[16];
    int i;
    (void)g;
#if defined(AMIGA) || defined(__amigaos__)
    if (!dir || !dir[0]) dir = "PROGDIR:machines";
#else
    if (!dir || !dir[0]) dir = "machines";
#endif
    memset(&lib, 0, sizeof lib);
    if (omv_machines_scan(&lib, dir, "cnc") < 0) {
        omv_machines_free(&lib);
        omr_set_result(c, "out of memory");
        return OMR_ERROR;
    }
    snprintf(nbuf, sizeof nbuf, "%d", lib.n);
    stem_put(c, "0", nbuf);
    for (i = 0; i < lib.n; i++) {
        char tail[24];
        snprintf(nbuf, sizeof nbuf, "%d", i + 1);
        stem_put(c, nbuf, lib.at[i].name);
        snprintf(tail, sizeof tail, "%d.DRIVER", i + 1);
        stem_put(c, tail, lib.at[i].driver);
        snprintf(tail, sizeof tail, "%d.PROFILE", i + 1);
        stem_put(c, tail, lib.at[i].profile);
    }
    snprintf(nbuf, sizeof nbuf, "%d", lib.n);
    omr_set_result(c, nbuf);
    omv_machines_free(&lib);
    return OMR_OK;
}

static const char *const look_names[] = {"top", "front", "side", "frame"};
static const char *const look_said[] = {"Top.", "Front.", "Side.", "Framed."};

/* Top, front, side, or the view at open. The picture changes. The cut does not. */
static int cmd_look(void *c, void *g0)
{
    oc_rexx *g = app(g0);
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
    {sizeof(omr_verb), "OPEN", "FILE/A", "Open an SVG or a DXF", cmd_open},
    {sizeof(omr_verb), "SAVE", "FILE/K STEM/K VAR/K", "Write mill G-code for the mill picked", cmd_save},
    {sizeof(omr_verb), "ACTIVATE", "", "Bring the window to the front", cmd_activate},
    {sizeof(omr_verb), "HIDE", "", "Send the window to the back", cmd_hide},
    {sizeof(omr_verb), "GET", "KEY/K STEM/K VAR/K", "Read a cut setting", cmd_get},
    {sizeof(omr_verb), "SET", "KEY/A VALUE/A", "Set a cut setting, held to the window's range", cmd_set},
    {sizeof(omr_verb), "STATUS", "STEM/K VAR/K", "The port, the file, whether it is dirty, and the mode", cmd_status},
    {sizeof(omr_verb), "PREVIEW", "FILE/K", "Plan the cut; with FILE, write a PPM of it", cmd_preview},
    {sizeof(omr_verb), "MILL", "DRIVER/K PROFILE/K", "Pick the mill from a driver and profile", cmd_mill},
    {sizeof(omr_verb), "MILLS", "DIR/K STEM/K", "the mills the picker lists", cmd_mills},
    {sizeof(omr_verb), "LOOK", "NAME/A", "top, front, side or frame", cmd_look},
};

void oc_rexx_init(oc_rexx *g)
{
    if (!g) return;
    memset(g, 0, sizeof *g);
    g->size = (int)sizeof *g;
    g->look = -1;
}

const omr_verb *oc_rexx_verbs(void) { return verbs; }

int oc_rexx_nverbs(void) { return (int)(sizeof verbs / sizeof verbs[0]); }
