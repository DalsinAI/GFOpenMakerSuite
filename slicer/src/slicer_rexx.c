/* GFOpenSlicer's ARexx command table. The window Wait()s on OPENMAKESLICER.1
 * and dispatches through these verbs; host tests use omr_do_line.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "slicer_rexx.h"

#include "gs_plan.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static os_rexx *app(void *g) { return (os_rexx *)g; }

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

static int stem_put(omr_cmd *cmd, const char *stem, const char *tail, const char *value)
{
    const char *s = omr_arg(cmd, "STEM");
    if (!s) s = stem;
    if (!s) return 0;
    return omr_set_stem(cmd, s, tail, value);
}

static void dirty(os_rexx *g)
{
    if (g) g->dirty = 1;
}

static void bed_of(os_rexx *g, float *x, float *y)
{
    *x = (g->have_machine && g->machine.area[0] > 0.f) ? g->machine.area[0] : 220.f;
    *y = (g->have_machine && g->machine.area[1] > 0.f) ? g->machine.area[1] : 220.f;
}

static void fmt_stats(const sl_job *j, char *buf, int len)
{
    int layers = (j && j->sliced) ? j->path.nlayers : 0;
    float time = (j && j->sliced) ? j->st.seconds : 0.f;
    float fil = (j && j->sliced) ? j->st.filament : 0.f;
    snprintf(buf, (size_t)len, "%d layers, %.1f s, %.1f mm", layers, (double)time, (double)fil);
}

static int put_stats(omr_cmd *c, const sl_job *j)
{
    char buf[160], nbuf[32];
    fmt_stats(j, buf, (int)sizeof buf);
    omr_set_result(c, buf);
    snprintf(nbuf, sizeof nbuf, "%d", (j && j->sliced) ? j->path.nlayers : 0);
    stem_put(c, NULL, "LAYERS", nbuf);
    snprintf(nbuf, sizeof nbuf, "%.3f", (j && j->sliced) ? (double)j->st.seconds : 0.0);
    stem_put(c, NULL, "TIME", nbuf);
    snprintf(nbuf, sizeof nbuf, "%.3f", (j && j->sliced) ? (double)j->st.filament : 0.0);
    stem_put(c, NULL, "FILAMENT", nbuf);
    return OMR_OK;
}

static int cmd_version(void *c, void *g0)
{
    os_rexx *g = app(g0);
    omr_set_result(c, g->version && g->version[0] ? g->version : "GFOpenSlicer 0.1.0");
    return OMR_OK;
}

static int cmd_quit(void *c, void *g0)
{
    os_rexx *g = app(g0);
    if (g->dirty && !omr_switch(c, "FORCE")) {
        omr_set_result(c, "the document has not been saved");
        return OMR_ERROR;
    }
    g->quit = 1;
    return OMR_OK;
}

static int cmd_open(void *c, void *g0)
{
    os_rexx *g = app(g0);
    const char *file = omr_arg(c, "FILE");
    char err[160];
    float bx, by;
    err[0] = 0;
    if (!file) {
        omr_set_result(c, "FILE is required");
        return OMR_ERROR;
    }
    if (!g->job) {
        omr_set_result(c, "no document");
        return OMR_ERROR;
    }
    bed_of(g, &bx, &by);
    if (sl_load(g->job, file, bx, by, err, (int)sizeof err)) {
        omr_set_result(c, err[0] ? err : "the part did not load");
        return OMR_ERROR;
    }
    snprintf(g->file, sizeof g->file, "%s", file);
    g->dirty = 0;
    omr_set_result(c, g->file);
    return OMR_OK;
}

static int cmd_save(void *c, void *g0)
{
    os_rexx *g = app(g0);
    const char *file = omr_arg(c, "FILE");
    char err[160];
    err[0] = 0;
    if (!file || !file[0]) file = g->save_file;
    if (!file || !file[0]) {
        omr_set_result(c, "no file to write");
        return OMR_ERROR;
    }
    if (!g->job || !g->job->sliced) {
        omr_set_result(c, "Slice first");
        return OMR_ERROR;
    }
    if (!g->have_machine || g->machine.family != OM_PRINTER) {
        omr_set_result(c, "no printer");
        return OMR_ERROR;
    }
    if (sl_write(g->job, file, &g->machine, err, (int)sizeof err)) {
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
    os_rexx *g = app(g0);
    (void)c;
    if (g->activate && g->win) g->activate(g->win);
    return OMR_OK;
}

static int cmd_hide(void *c, void *g0)
{
    os_rexx *g = app(g0);
    (void)c;
    if (g->hide && g->win) g->hide(g->win);
    return OMR_OK;
}

static int set_choice(gs_settings *s, int i, const char *val, char *err, int errlen)
{
    const gs_setting *d = gs_setting_at(i);
    int k;
    char *end;
    long n;
    if (!d || d->type != GS_CHOICE) return -1;
    n = strtol(val, &end, 10);
    if (end != val && !*end) {
        gs_setting_set(s, i, (float)n);
        return 0;
    }
    for (k = 0; d->choices && d->choices[k]; k++)
        if (eqi(d->choices[k], val)) {
            gs_setting_set(s, i, (float)k);
            return 0;
        }
    snprintf(err, (size_t)errlen, "%s: not a known value", d->key);
    return -1;
}

static void fmt_setting(const gs_settings *s, int i, char *buf, int len)
{
    const gs_setting *d = gs_setting_at(i);
    const char *word;
    float v = gs_setting_get(s, i);
    if (!d) {
        buf[0] = 0;
        return;
    }
    if (d->type == GS_CHOICE) {
        word = gs_setting_choice(i, (int)v);
        snprintf(buf, (size_t)len, "%s", word ? word : "");
        return;
    }
    if (d->type == GS_INT)
        snprintf(buf, (size_t)len, "%d", (int)v);
    else
        snprintf(buf, (size_t)len, "%.4g", (double)v);
}

static int cmd_get(void *c, void *g0)
{
    os_rexx *g = app(g0);
    const char *key = omr_arg(c, "KEY");
    const char *stem = omr_arg(c, "STEM");
    const char *var = omr_arg(c, "VAR");
    char buf[64], nbuf[16];
    int i, n;
    if (key) {
        i = gs_setting_find(key);
        if (i < 0) {
            omr_set_result(c, "no such setting");
            return OMR_ERROR;
        }
        fmt_setting(&g->set, i, buf, (int)sizeof buf);
        omr_set_result(c, buf);
        if (var) omr_set_var(c, var, buf);
        return OMR_OK;
    }
    n = gs_settings_count();
    snprintf(nbuf, sizeof nbuf, "%d", n);
    if (stem) omr_set_stem(c, stem, "0", nbuf);
    for (i = 0; i < n; i++) {
        const gs_setting *d = gs_setting_at(i);
        char tail[24];
        fmt_setting(&g->set, i, buf, (int)sizeof buf);
        if (stem) {
            snprintf(tail, sizeof tail, "%d", i + 1);
            omr_set_stem(c, stem, tail, d->key);
            snprintf(tail, sizeof tail, "%d.VALUE", i + 1);
            omr_set_stem(c, stem, tail, buf);
        }
    }
    omr_set_result(c, nbuf);
    return OMR_OK;
}

static int cmd_set(void *c, void *g0)
{
    os_rexx *g = app(g0);
    const char *key = omr_arg(c, "KEY"), *val = omr_arg(c, "VALUE");
    char err[80], *end = NULL;
    int i;
    float f;
    if (!key || !val) {
        omr_set_result(c, "KEY and VALUE are required");
        return OMR_ERROR;
    }
    i = gs_setting_find(key);
    if (i < 0) {
        omr_set_result(c, "no such setting");
        return OMR_ERROR;
    }
    if (gs_setting_at(i)->type == GS_CHOICE) {
        if (set_choice(&g->set, i, val, err, (int)sizeof err)) {
            omr_set_result(c, err);
            return OMR_ERROR;
        }
    } else {
        f = (float)strtod(val, &end);
        if (end == val || (end && *end)) {
            omr_set_result(c, "not a number");
            return OMR_ERROR;
        }
        gs_setting_set(&g->set, i, f);
    }
    dirty(g);
    fmt_setting(&g->set, i, err, (int)sizeof err);
    omr_set_result(c, err);
    return OMR_OK;
}

static int cmd_status(void *c, void *g0)
{
    os_rexx *g = app(g0);
    sl_job *j = g->job;
    char buf[320], nbuf[32];
    const char *port = g->port_name ? g->port_name : "";
    snprintf(buf, sizeof buf, "%.40s %.240s %d", port[0] ? port : OS_REXX_PORT, g->file[0] ? g->file : "-", g->dirty);
    omr_set_result(c, buf);
    stem_put(c, NULL, "PORT", port[0] ? port : OS_REXX_PORT);
    stem_put(c, NULL, "FILE", g->file);
    stem_put(c, NULL, "DIRTY", g->dirty ? "1" : "0");
    stem_put(c, NULL, "LOADED", (j && j->have_part) ? "1" : "0");
    stem_put(c, NULL, "SLICED", (j && j->sliced) ? "1" : "0");
    snprintf(nbuf, sizeof nbuf, "%d", (j && j->sliced) ? j->path.nlayers : 0);
    stem_put(c, NULL, "LAYERS", nbuf);
    snprintf(nbuf, sizeof nbuf, "%.3f", (j && j->sliced) ? (double)j->st.seconds : 0.0);
    stem_put(c, NULL, "TIME", nbuf);
    snprintf(nbuf, sizeof nbuf, "%.3f", (j && j->sliced) ? (double)j->st.filament : 0.0);
    stem_put(c, NULL, "FILAMENT", nbuf);
    return OMR_OK;
}

static int cmd_profile(void *c, void *g0)
{
    os_rexx *g = app(g0);
    const char *file = omr_arg(c, "FILE");
    char err[160];
    err[0] = 0;
    if (!file) {
        omr_set_result(c, "FILE is required");
        return OMR_ERROR;
    }
    if (gs_settings_load(&g->set, file, err, (int)sizeof err)) {
        omr_set_result(c, err[0] ? err : "the profile did not load");
        return OMR_ERROR;
    }
    dirty(g);
    omr_set_result(c, file);
    return OMR_OK;
}

static int cmd_printer(void *c, void *g0)
{
    os_rexx *g = app(g0);
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
    if (g->machine.family != OM_PRINTER) {
        omr_set_result(c, "not a printer");
        om_clear(&g->machine);
        return OMR_ERROR;
    }
    g->have_machine = 1;
    dirty(g);
    omr_set_result(c, g->machine.name[0] ? g->machine.name : (profile && profile[0] ? profile : driver));
    return OMR_OK;
}

static int cmd_slice(void *c, void *g0)
{
    os_rexx *g = app(g0);
    char err[160];
    int tasks;
    err[0] = 0;
    if (!g->job || !g->job->have_part) {
        omr_set_result(c, "Open a part first");
        return OMR_ERROR;
    }
    if (g->have_machine) gp_place(&g->job->part, g->machine.area[0], g->machine.area[1]);
    tasks = g->tasks > 0 ? g->tasks : 1;
    if (sl_slice(g->job, &g->set, g->have_machine ? &g->machine : NULL, tasks, err, (int)sizeof err)) {
        omr_set_result(c, err[0] ? err : "That did not slice");
        return OMR_ERROR;
    }
    return put_stats(c, g->job);
}

static int cmd_stats(void *c, void *g0)
{
    os_rexx *g = app(g0);
    if (!g->job || !g->job->sliced) {
        omr_set_result(c, "not sliced yet");
        return OMR_ERROR;
    }
    return put_stats(c, g->job);
}

static const omr_verb verbs[] = {
    {sizeof(omr_verb), "VERSION", "", "The program name and version", cmd_version},
    {sizeof(omr_verb), "QUIT", "FORCE/S", "Close the window; FORCE discards a dirty document", cmd_quit},
    {sizeof(omr_verb), "OPEN", "FILE/A", "Open an STL", cmd_open},
    {sizeof(omr_verb), "SAVE", "FILE/K", "Write G-code for the last slice", cmd_save},
    {sizeof(omr_verb), "ACTIVATE", "", "Bring the window to the front", cmd_activate},
    {sizeof(omr_verb), "HIDE", "", "Send the window to the back", cmd_hide},
    {sizeof(omr_verb), "GET", "KEY/K STEM/K VAR/K", "Read a print setting", cmd_get},
    {sizeof(omr_verb), "SET", "KEY/A VALUE/A", "Set a print setting", cmd_set},
    {sizeof(omr_verb), "STATUS", "STEM/K VAR/K", "The port, the file, whether it is loaded and sliced", cmd_status},
    {sizeof(omr_verb), "LOAD", "FILE/A", "Open an STL, as OPEN", cmd_open},
    {sizeof(omr_verb), "PROFILE", "FILE/A", "Load a print-settings profile", cmd_profile},
    {sizeof(omr_verb), "PRINTER", "DRIVER/K PROFILE/K", "Pick the machine from a driver and profile", cmd_printer},
    {sizeof(omr_verb), "SLICE", "STEM/K VAR/K", "Slice the loaded part", cmd_slice},
    {sizeof(omr_verb), "STATS", "STEM/K VAR/K", "The last slice's figures", cmd_stats},
};

void os_rexx_init(os_rexx *g)
{
    if (!g) return;
    memset(g, 0, sizeof *g);
    g->size = (int)sizeof *g;
    gs_settings_default(&g->set);
    g->tasks = 1;
}

const omr_verb *os_rexx_verbs(void) { return verbs; }

int os_rexx_nverbs(void) { return (int)(sizeof verbs / sizeof verbs[0]); }
