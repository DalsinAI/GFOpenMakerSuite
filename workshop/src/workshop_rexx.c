/* GFOpenMakerWorkshop's ARexx command table. The window Wait()s on
 * OPENMAKEWORKSHOP.1 and dispatches through these verbs; host tests
 * use omr_do_line.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "workshop_rexx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static gw_rexx *app(void *g) { return (gw_rexx *)g; }

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

static int arg_i(void *cmd, const char *key, int *out)
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

static int stem_put(void *cmd, const char *tail, const char *value)
{
    const char *s = omr_arg(cmd, "STEM");
    if (!s || !s[0]) return 0;
    return omr_set_stem(cmd, s, tail, value ? value : "");
}

static void var_put(void *cmd, const char *value)
{
    const char *v = omr_arg(cmd, "VAR");
    if (v && v[0]) omr_set_var(cmd, v, value ? value : "");
}

static int have(gw_rexx *g) { return g && g->ws && g->ws->have; }

static int need(gw_rexx *g, void *cmd)
{
    if (!have(g)) {
        omr_set_result(cmd, "no project");
        return -1;
    }
    return 0;
}

static int blocked(gw_rexx *g, void *cmd)
{
    if (have(g) && g->ws->project.dirty) {
        omr_set_result(cmd, "the project has not been saved");
        return -1;
    }
    return 0;
}

static const char *port_of(gw_rexx *g)
{
    if (g && g->port_name && g->port_name[0]) return g->port_name;
    return "OPENMAKEWORKSHOP.1";
}

static int cmd_version(void *c, void *g0)
{
    gw_rexx *g = app(g0);
    omr_set_result(c, g->version && g->version[0] ? g->version : "GFOpenMakerWorkshop 0.1.0");
    return OMR_OK;
}

static int cmd_quit(void *c, void *g0)
{
    gw_rexx *g = app(g0);
    if (have(g) && g->ws->project.dirty && !omr_switch(c, "FORCE")) {
        omr_set_result(c, "the project has not been saved");
        return OMR_ERROR;
    }
    g->quit = 1;
    return OMR_OK;
}

static int cmd_open(void *c, void *g0)
{
    gw_rexx *g = app(g0);
    const char *file = omr_arg(c, "FILE");
    char err[160];
    err[0] = 0;
    if (!file || !file[0]) {
        omr_set_result(c, "FILE is required");
        return OMR_ERROR;
    }
    if (!g->ws) {
        omr_set_result(c, "no project");
        return OMR_ERROR;
    }
    if (blocked(g, c)) return OMR_ERROR;
    if (gpr_open(&g->ws->project, file, err, (int)sizeof err)) {
        omr_set_result(c, err[0] ? err : "the project did not open");
        return OMR_ERROR;
    }
    g->ws->have = 1;
    g->ws->picked = -1;
    omr_set_result(c, g->ws->project.path);
    return OMR_OK;
}

static int cmd_save(void *c, void *g0)
{
    gw_rexx *g = app(g0);
    const char *file = omr_arg(c, "FILE");
    char err[160], om[GPR_PATH_MAX + 16];
    err[0] = 0;
    if (need(g, c)) return OMR_ERROR;
    if (file && file[0]) {
        snprintf(om, sizeof om, "%s/project.om", g->ws->project.path);
        if (strcmp(file, g->ws->project.path) != 0 && strcmp(file, om) != 0) {
            omr_set_result(c, "SAVE writes this project");
            return OMR_ERROR;
        }
    }
    if (gpr_save(&g->ws->project, err, (int)sizeof err)) {
        omr_set_result(c, err[0] ? err : "the project did not save");
        return OMR_ERROR;
    }
    omr_set_result(c, g->ws->project.path);
    return OMR_OK;
}

static int cmd_activate(void *c, void *g0)
{
    gw_rexx *g = app(g0);
    (void)c;
    if (g->activate && g->win) g->activate(g->win);
    return OMR_OK;
}

static int cmd_hide(void *c, void *g0)
{
    gw_rexx *g = app(g0);
    (void)c;
    if (g->hide && g->win) g->hide(g->win);
    return OMR_OK;
}

static const char *const keys[] = {"name", "notes", NULL};

static int key_of(const char *s)
{
    int i;
    if (!s) return -1;
    for (i = 0; keys[i]; i++)
        if (eqi(s, keys[i])) return i;
    return -1;
}

static void key_text(const gpr_project *p, int k, char *buf, int n)
{
    const char *s = "";
    if (!buf || n < 1) return;
    if (p) s = (k == 0) ? p->name : p->notes;
    snprintf(buf, (size_t)n, "%s", s ? s : "");
}

static int cmd_get(void *c, void *g0)
{
    gw_rexx *g = app(g0);
    const char *key = omr_arg(c, "KEY");
    char buf[GPR_NOTES_MAX + 8], nbuf[16];
    int i, k;
    if (need(g, c)) return OMR_ERROR;
    if (!key) {
        for (i = 0; keys[i]; i++) {
        }
        snprintf(nbuf, sizeof nbuf, "%d", i);
        stem_put(c, "0", nbuf);
        for (i = 0; keys[i]; i++) {
            char tail[24];
            snprintf(nbuf, sizeof nbuf, "%d", i + 1);
            stem_put(c, nbuf, keys[i]);
            key_text(&g->ws->project, i, buf, (int)sizeof buf);
            snprintf(tail, sizeof tail, "%d.VALUE", i + 1);
            stem_put(c, tail, buf);
        }
        omr_set_result(c, nbuf);
        return OMR_OK;
    }
    k = key_of(key);
    if (k < 0) {
        omr_set_result(c, "unknown key");
        return OMR_ERROR;
    }
    key_text(&g->ws->project, k, buf, (int)sizeof buf);
    var_put(c, buf);
    omr_set_result(c, buf);
    return OMR_OK;
}

static int cmd_set(void *c, void *g0)
{
    gw_rexx *g = app(g0);
    const char *key = omr_arg(c, "KEY"), *value = omr_arg(c, "VALUE");
    char buf[GPR_NOTES_MAX + 8];
    int k;
    if (need(g, c)) return OMR_ERROR;
    k = key_of(key);
    if (k < 0) {
        omr_set_result(c, "unknown key");
        return OMR_ERROR;
    }
    if (!value) value = "";
    if (k == 0) {
        if (!value[0] || gpr_set_name(&g->ws->project, value)) {
            omr_set_result(c, "the name was not taken");
            return OMR_ERROR;
        }
    } else if (gpr_set_notes(&g->ws->project, value)) {
        omr_set_result(c, "the notes were not taken");
        return OMR_ERROR;
    }
    key_text(&g->ws->project, k, buf, (int)sizeof buf);
    omr_set_result(c, buf);
    return OMR_OK;
}

static int cmd_status(void *c, void *g0)
{
    gw_rexx *g = app(g0);
    char buf[GPR_PATH_MAX + 80], nbuf[16];
    const char *file = "";
    int dirty = 0, n = 0;
    if (have(g)) {
        file = g->ws->project.path;
        dirty = g->ws->project.dirty ? 1 : 0;
        n = g->ws->project.n;
    }
    snprintf(buf, sizeof buf, "%s %s %d", port_of(g), file, dirty);
    snprintf(nbuf, sizeof nbuf, "%d", n);
    stem_put(c, "PORT", port_of(g));
    stem_put(c, "FILE", file);
    stem_put(c, "DIRTY", dirty ? "1" : "0");
    stem_put(c, "ITEMS", nbuf);
    stem_put(c, "NAME", have(g) ? g->ws->project.name : "");
    stem_put(c, "NOTES", have(g) ? g->ws->project.notes : "");
    {
        const gpr_item *it = NULL;
        int at = -1;
        if (have(g) && g->ws->picked >= 0 && g->ws->picked < g->ws->project.n) {
            at = g->ws->picked;
            it = &g->ws->project.items[at];
        }
        snprintf(nbuf, sizeof nbuf, "%d", at >= 0 ? at + 1 : 0);
        stem_put(c, "PICKED", nbuf);
        stem_put(c, "ITEM", it ? it->name : "");
        stem_put(c, "KIND", it ? it->kind : "");
        stem_put(c, "PATH", it ? it->file : "");
        if (it && it->from > 0) snprintf(nbuf, sizeof nbuf, "%d", it->from);
        else nbuf[0] = 0;
        stem_put(c, "FROM", nbuf);
    }
    var_put(c, buf);
    omr_set_result(c, buf);
    return OMR_OK;
}

static int cmd_new(void *c, void *g0)
{
    gw_rexx *g = app(g0);
    const char *path = omr_arg(c, "PATH"), *name = omr_arg(c, "NAME");
    char err[160];
    err[0] = 0;
    if (!g->ws) {
        omr_set_result(c, "no project");
        return OMR_ERROR;
    }
    if (!path || !path[0]) {
        omr_set_result(c, "PATH is required");
        return OMR_ERROR;
    }
    if (blocked(g, c)) return OMR_ERROR;
    if (gpr_create(&g->ws->project, path, name, err, (int)sizeof err)) {
        omr_set_result(c, err[0] ? err : "the project was not created");
        return OMR_ERROR;
    }
    g->ws->have = 1;
    g->ws->picked = -1;
    omr_set_result(c, g->ws->project.path);
    return OMR_OK;
}

static int cmd_add(void *c, void *g0)
{
    gw_rexx *g = app(g0);
    gpr_add_opts o;
    const char *kind = omr_arg(c, "KIND"), *file = omr_arg(c, "FILE"), *name = omr_arg(c, "NAME");
    char err[160], buf[GPR_NAME_MAX + 16];
    int from = 0, has, idx;
    err[0] = 0;
    if (need(g, c)) return OMR_ERROR;
    has = arg_i(c, "FROM", &from);
    if (has < 0) {
        omr_set_result(c, "FROM is not a number");
        return OMR_ERROR;
    }
    gpr_add_defaults(&o);
    o.kind = kind;
    o.file = file;
    o.name = name;
    if (has > 0) o.from = from;
    idx = gpr_add(&g->ws->project, &o, err, (int)sizeof err);
    if (idx < 0) {
        omr_set_result(c, err[0] ? err : "the item was not added");
        return OMR_ERROR;
    }
    snprintf(buf, sizeof buf, "%d %s", idx + 1, g->ws->project.items[idx].name);
    omr_set_result(c, buf);
    return OMR_OK;
}

static int cmd_remove(void *c, void *g0)
{
    gw_rexx *g = app(g0);
    const char *item = omr_arg(c, "ITEM");
    int idx;
    if (need(g, c)) return OMR_ERROR;
    idx = gpr_find(&g->ws->project, item);
    if (idx < 0 || gpr_remove(&g->ws->project, idx)) {
        omr_set_result(c, "no such item");
        return OMR_ERROR;
    }
    if (g->ws->picked == idx) g->ws->picked = -1;
    else if (g->ws->picked > idx) g->ws->picked--;
    omr_set_result(c, item ? item : "");
    return OMR_OK;
}

static int cmd_rename(void *c, void *g0)
{
    gw_rexx *g = app(g0);
    const char *item = omr_arg(c, "ITEM"), *name = omr_arg(c, "NAME");
    int idx;
    if (need(g, c)) return OMR_ERROR;
    idx = gpr_find(&g->ws->project, item);
    if (idx < 0 || !name || gpr_rename(&g->ws->project, idx, name)) {
        omr_set_result(c, "no such item");
        return OMR_ERROR;
    }
    omr_set_result(c, g->ws->project.items[idx].name);
    return OMR_OK;
}

/* The list: show that row. It does not open the item. */
static int cmd_pick(void *c, void *g0)
{
    gw_rexx *g = app(g0);
    const char *item = omr_arg(c, "ITEM");
    int idx;
    if (need(g, c)) return OMR_ERROR;
    idx = gpr_find(&g->ws->project, item);
    if (idx < 0) {
        omr_set_result(c, "no such item");
        return OMR_ERROR;
    }
    g->ws->picked = idx;
    g->ws->status[0] = 0;
    g->home = 1;
    omr_set_result(c, g->ws->project.items[idx].name);
    return OMR_OK;
}

static int kind_match(const char *item_kind, const char *want)
{
    char a[GPR_KIND_MAX + 1], b[GPR_KIND_MAX + 1];
    int i;
    if (!want || !want[0]) return 1;
    for (i = 0; item_kind && item_kind[i] && i < GPR_KIND_MAX; i++) {
        int ch = (unsigned char)item_kind[i];
        if (ch >= 'A' && ch <= 'Z') ch = ch - 'A' + 'a';
        a[i] = (char)ch;
    }
    a[i] = 0;
    for (i = 0; want[i] && i < GPR_KIND_MAX; i++) {
        int ch = (unsigned char)want[i];
        if (ch >= 'A' && ch <= 'Z') ch = ch - 'A' + 'a';
        b[i] = (char)ch;
    }
    b[i] = 0;
    return strcmp(a, b) == 0;
}

static int cmd_items(void *c, void *g0)
{
    gw_rexx *g = app(g0);
    const char *kind = omr_arg(c, "KIND");
    char nbuf[16], from[16], tail[32];
    int i, n = 0;
    if (need(g, c)) return OMR_ERROR;
    for (i = 0; i < g->ws->project.n; i++)
        if (kind_match(g->ws->project.items[i].kind, kind)) n++;
    snprintf(nbuf, sizeof nbuf, "%d", n);
    stem_put(c, "0", nbuf);
    n = 0;
    for (i = 0; i < g->ws->project.n; i++) {
        const gpr_item *it = &g->ws->project.items[i];
        if (!kind_match(it->kind, kind)) continue;
        n++;
        snprintf(nbuf, sizeof nbuf, "%d", n);
        stem_put(c, nbuf, it->name);
        snprintf(tail, sizeof tail, "%d.KIND", n);
        stem_put(c, tail, it->kind);
        snprintf(tail, sizeof tail, "%d.FILE", n);
        stem_put(c, tail, it->file);
        if (it->from > 0) snprintf(from, sizeof from, "%d", it->from);
        else from[0] = 0;
        snprintf(tail, sizeof tail, "%d.FROM", n);
        stem_put(c, tail, from);
    }
    snprintf(nbuf, sizeof nbuf, "%d", n);
    var_put(c, nbuf);
    omr_set_result(c, nbuf);
    return OMR_OK;
}

static const char *send_port(const char *app)
{
    if (!app || !app[0]) return NULL;
    if (eqi(app, "CAD") || eqi(app, "GFOPENMAKE")) return "GFOPENMAKE";
    if (eqi(app, "SLICER") || eqi(app, "PRINT") || eqi(app, "OPENMAKESLICER")) return "OPENMAKESLICER";
    if (eqi(app, "BURN") || eqi(app, "LASER") || eqi(app, "OPENMAKEBURN")) return "OPENMAKEBURN";
    if (eqi(app, "CNC") || eqi(app, "GFOPENCNC")) return "GFOPENCNC";
    if (eqi(app, "KICAD") || eqi(app, "ELECTRONICS")) return NULL;
    return app;
}

static void note(char *err, int errlen, const char *s)
{
    if (err && errlen > 0) snprintf(err, (size_t)errlen, "%s", s ? s : "");
}

static int deliver(const char *port, const char *line, char *reply, int replylen, char *err, int errlen)
{
    char found[40], result[512];
    omr_client *cl;
    omr_io io;
    if (!port) {
        note(err, errlen, "no port for that app");
        return OMR_ERROR;
    }
    if (omr_find(port, found, (int)sizeof found) != 0) {
        note(err, errlen, "the port is not up");
        return OMR_ERROR;
    }
    cl = omr_connect(found, NULL);
    if (!cl) {
        note(err, errlen, "the port is not up");
        return OMR_ERROR;
    }
    memset(&io, 0, sizeof io);
    io.size = (int)sizeof io;
    io.result = result;
    io.resultlen = (int)sizeof result;
    result[0] = 0;
    if (omr_send(cl, line, &io) != 0) {
        omr_disconnect(cl);
        note(err, errlen, result[0] ? result : "the other app did not answer");
        return OMR_FAIL;
    }
    omr_disconnect(cl);
    if (io.rc != OMR_OK) note(err, errlen, result[0] ? result : "the other app refused");
    else if (reply && replylen > 0) snprintf(reply, (size_t)replylen, "%s", result);
    return io.rc;
}

static int send_line(void *c, const char *port, const char *line)
{
    char reply[512], err[160];
    int rc;
    reply[0] = 0;
    err[0] = 0;
    rc = deliver(port, line, reply, (int)sizeof reply, err, (int)sizeof err);
    omr_set_result(c, rc == OMR_OK ? reply : err);
    return rc;
}

int gw_open_item(workshop *w, int index, char *used, int usedlen, char *err, int errlen)
{
    const gpr_item *it;
    const char *aport, *prog, *name;
    char file[GPR_PATH_MAX + 1], line[GPR_PATH_MAX + 32], found[40];
    if (err && errlen > 0) err[0] = 0;
    if (used && usedlen > 0) used[0] = 0;
    if (!w || !w->have || index < 0 || index >= w->project.n) {
        note(err, errlen, "no item is picked");
        return OMR_ERROR;
    }
    it = &w->project.items[index];
    prog = gpr_kind_app(it->kind);
    aport = gpr_kind_port(it->kind);
    if (!prog) {
        note(err, errlen, "no app for this kind");
        return OMR_ERROR;
    }
    if (gpr_resolve(&w->project, index, file, (int)sizeof file)) {
        note(err, errlen, "the file path is too long");
        return OMR_ERROR;
    }
    if (strchr(file, '"')) {
        note(err, errlen, "the file name has a quote");
        return OMR_ERROR;
    }
    w->picked = index;
    if (aport && omr_find(aport, found, (int)sizeof found) == 0) {
        int rc;
        snprintf(line, sizeof line, "OPEN FILE \"%s\"", file);
        rc = deliver(found, line, NULL, 0, err, errlen);
        if (rc == OMR_OK) note(used, usedlen, found);
        return rc;
    }
    if (workshop_launch(w, err, errlen)) {
        if (err && errlen > 0 && !err[0]) note(err, errlen, "the app did not start");
        return OMR_ERROR;
    }
    name = (aport && aport[0]) ? aport : prog;
    note(used, usedlen, name);
    return OMR_OK;
}

static int cmd_openitem(void *c, void *g0)
{
    gw_rexx *g = app(g0);
    const char *item = omr_arg(c, "ITEM");
    char used[64], err[160];
    int idx, rc;
    if (need(g, c)) return OMR_ERROR;
    idx = gpr_find(&g->ws->project, item);
    if (idx < 0) {
        omr_set_result(c, "no such item");
        return OMR_ERROR;
    }
    rc = gw_open_item(g->ws, idx, used, (int)sizeof used, err, (int)sizeof err);
    omr_set_result(c, rc == OMR_OK ? used : err);
    return rc;
}

static int cmd_send(void *c, void *g0)
{
    const char *appname = omr_arg(c, "APP"), *cmd = omr_arg(c, "CMD");
    (void)g0;
    if (!cmd || !cmd[0]) {
        omr_set_result(c, "CMD is required");
        return OMR_ERROR;
    }
    return send_line(c, send_port(appname), cmd);
}

static int cmd_recent(void *c, void *g0)
{
    gw_rexx *g = app(g0);
    char nbuf[16];
    const char *first = "";
    int i, n = 0;
    if (g->ws) {
        n = g->ws->recent.n;
        if (n > 0) first = g->ws->recent.path[0];
    }
    snprintf(nbuf, sizeof nbuf, "%d", n);
    stem_put(c, "0", nbuf);
    for (i = 0; g->ws && i < n; i++) {
        snprintf(nbuf, sizeof nbuf, "%d", i + 1);
        stem_put(c, nbuf, g->ws->recent.path[i]);
    }
    var_put(c, first);
    omr_set_result(c, first);
    return OMR_OK;
}

static const omr_verb verbs[] = {
    {sizeof(omr_verb), "VERSION", "", "The program name and version", cmd_version},
    {sizeof(omr_verb), "QUIT", "FORCE/S", "Close the window; FORCE discards a dirty project", cmd_quit},
    {sizeof(omr_verb), "OPEN", "FILE/A", "Open a project drawer or its project.om", cmd_open},
    {sizeof(omr_verb), "SAVE", "FILE/K", "Write this project's project.om", cmd_save},
    {sizeof(omr_verb), "ACTIVATE", "", "Bring the window to the front", cmd_activate},
    {sizeof(omr_verb), "HIDE", "", "Send the window to the back", cmd_hide},
    {sizeof(omr_verb), "GET", "KEY/K STEM/K VAR/K", "Read name or notes", cmd_get},
    {sizeof(omr_verb), "SET", "KEY/A VALUE/A", "Set name or notes", cmd_set},
    {sizeof(omr_verb), "STATUS", "STEM/K VAR/K", "The port, the project, whether it is dirty, how many items", cmd_status},
    {sizeof(omr_verb), "NEW", "PATH/A NAME/K", "Create a project drawer and open it", cmd_new},
    {sizeof(omr_verb), "ADD", "KIND/A FILE/A NAME/K FROM/N/K", "Add an item", cmd_add},
    {sizeof(omr_verb), "REMOVE", "ITEM/A", "Drop an item from the manifest", cmd_remove},
    {sizeof(omr_verb), "RENAME", "ITEM/A NAME/A", "Rename an item in the manifest", cmd_rename},
    {sizeof(omr_verb), "ITEMS", "KIND/K STEM/K VAR/K", "List the items", cmd_items},
    {sizeof(omr_verb), "PICK", "ITEM/A", "show that item, as a click on the list", cmd_pick},
    {sizeof(omr_verb), "OPENITEM", "ITEM/A", "Open an item in its app", cmd_openitem},
    {sizeof(omr_verb), "SEND", "APP/A CMD/F/A", "Pass a command to another app's port", cmd_send},
    {sizeof(omr_verb), "RECENT", "STEM/K VAR/K", "The recent projects", cmd_recent},
};

void gw_rexx_init(gw_rexx *g)
{
    if (!g) return;
    memset(g, 0, sizeof *g);
    g->size = (int)sizeof *g;
}

const omr_verb *gw_rexx_verbs(void) { return verbs; }

int gw_rexx_nverbs(void) { return (int)(sizeof verbs / sizeof verbs[0]); }
