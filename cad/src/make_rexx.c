/* GFOpenMake's ARexx command table (docs/arexx.md). It talks to the
 * parts list and the editor (gs_edit.h); the window only opens the port
 * and calls omr_dispatch.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "make_rexx.h"

#include "gs_gcode.h"
#include "gs_model.h"
#include "gs_plan.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int eqi(const char *a, const char *b)
{
    if (!a || !b) return 0;
    while (*a && *b) {
        int ca = (unsigned char)*a, cb = (unsigned char)*b;
        if (ca >= 'a' && ca <= 'z') ca -= 32;
        if (cb >= 'a' && cb <= 'z') cb -= 32;
        if (ca != cb) return 0;
        a++;
        b++;
    }
    return *a == 0 && *b == 0;
}

static gm_rexx *app(void *p) { return p; }

static gs_parts *list(gm_rexx *g) { return g && g->parts ? *g->parts : NULL; }

static gs_mesh *picked(gm_rexx *g)
{
    gs_parts *ps = list(g);
    if (!ps) return NULL;
    return gs_parts_mesh(ps, gs_parts_picked(ps));
}

static void remember(gm_rexx *g)
{
    gs_parts *ps = list(g);
    if (g->steps && ps && gs_parts_count(ps)) ge_steps_push(g->steps, ps);
}

static void forget(gm_rexx *g)
{
    gs_parts *was;
    if (!g->steps || !g->parts) return;
    was = ge_steps_forget(g->steps, *g->parts);
    if (was) *g->parts = was;
}

static void changed(gm_rexx *g)
{
    g->dirty = 1;
    if (g->after) g->after(g->ui);
}

static int arg_f(omr_cmd *cmd, const char *key, float *out)
{
    const char *s = omr_arg(cmd, key);
    char *end;
    if (!s) return 0;
    *out = (float)strtod(s, &end);
    if (end == s || *end) return -1;
    return 1;
}

static int arg_i(omr_cmd *cmd, const char *key, int *out)
{
    const char *s = omr_arg(cmd, key);
    char *end;
    long v;
    if (!s) return 0;
    v = strtol(s, &end, 10);
    if (end == s || *end) return -1;
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

static void first_word(const char *text, char *out, int len)
{
    int n = 0;
    const char *s = text ? text : "";
    while (*s == ' ' || *s == '\t') s++;
    while (s[n] && s[n] != ' ' && s[n] != '\t' && n < len - 1) {
        out[n] = s[n];
        n++;
    }
    out[n] = 0;
}

static long to_file(const char *t, long n, void *ud)
{
    return (long)fwrite(t, 1, (size_t)n, (FILE *)ud);
}

static int cmd_quit(void *c, void *g0)
{
    gm_rexx *g = app(g0);
    if (g->dirty && !omr_switch(c, "FORCE")) {
        omr_set_result(c, "the document is not saved");
        return OMR_ERROR;
    }
    g->quit = 1;
    return OMR_OK;
}

static int cmd_open(void *c, void *g0)
{
    gm_rexx *g = app(g0);
    gs_parts *ps = list(g);
    gs_mesh next;
    const char *file = omr_arg(c, "FILE");
    char err[160];
    int i;
    if (!ps || !file) {
        omr_set_result(c, "no document");
        return OMR_ERROR;
    }
    if (gs_parts_count(ps) >= GS_PARTS_MAX) {
        omr_set_result(c, "Twenty-four parts is the limit.");
        return OMR_ERROR;
    }
    memset(&next, 0, sizeof next);
    if (ge_open(&next, file, err, (int)sizeof err)) {
        omr_set_result(c, err[0] ? err : "Could not open that STL");
        return OMR_ERROR;
    }
    remember(g);
    i = gs_parts_add(ps, &next, file);
    if (i < 0) {
        gs_mesh_free(&next);
        forget(g);
        omr_set_result(c, "Could not add that part");
        return OMR_ERROR;
    }
    snprintf(g->file, sizeof g->file, "%s", file);
    g->face = -1;
    changed(g);
    omr_set_result(c, file);
    return OMR_OK;
}

/* File's Shelf: an example from examples/cad/ on the bed. */
static int cmd_shelf(void *c, void *g0)
{
    gm_rexx *g = app(g0);
    gs_parts *ps = list(g);
    gs_mesh next;
    const char *name = omr_arg(c, "NAME");
    char err[160], line[96];
    int i, n, has, index = 0;
    if (!ps) {
        omr_set_result(c, "no document");
        return OMR_ERROR;
    }
    n = ge_shelf_count();
    if (n < 1) {
        omr_set_result(c, "the shelf is empty");
        return OMR_ERROR;
    }
    if (name && name[0]) {
        index = -1;
        for (i = 0; i < n; i++)
            if (ge_shelf_name(i) && eqi(ge_shelf_name(i), name)) index = i;
        if (index < 0) {
            omr_set_result(c, "no such example");
            return OMR_ERROR;
        }
    } else {
        has = arg_i(c, "INDEX", &index);
        if (has < 0) {
            omr_set_result(c, "not a number");
            return OMR_ERROR;
        }
        if (has > 0) index -= 1;
        if (index < 0 || index >= n) {
            omr_set_result(c, "no such example");
            return OMR_ERROR;
        }
    }
    if (gs_parts_count(ps) >= GS_PARTS_MAX) {
        omr_set_result(c, "Twenty-four parts is the limit.");
        return OMR_ERROR;
    }
    err[0] = 0;
    memset(&next, 0, sizeof next);
    if (ge_shelf_load(&next, index, err, (int)sizeof err)) {
        omr_set_result(c, err[0] ? err : "Could not open that example");
        return OMR_ERROR;
    }
    remember(g);
    i = gs_parts_add(ps, &next, ge_shelf_name(index));
    if (i < 0) {
        gs_mesh_free(&next);
        forget(g);
        omr_set_result(c, "Could not add that part");
        return OMR_ERROR;
    }
    g->face = -1;
    changed(g);
    snprintf(line, sizeof line, "%s is on the bed.", ge_shelf_name(index));
    omr_set_result(c, line);
    return OMR_OK;
}

static int extrude_kind(const char *s)
{
    if (!s) return -1;
    if (eqi(s, "block") || eqi(s, "rect")) return GM_RECT;
    if (eqi(s, "round") || eqi(s, "cylinder")) return GM_ROUND;
    if (eqi(s, "slot")) return GM_SLOT;
    return -1;
}

static int extrude_op(const char *s)
{
    if (!s || !s[0]) return GM_ON_BED;
    if (eqi(s, "add") || eqi(s, "bed")) return GM_ON_BED;
    if (eqi(s, "on") || eqi(s, "top")) return GM_ON_TOP;
    if (eqi(s, "cut")) return GM_FROM_TOP;
    return -1;
}

/* Extrude's Add, On top or Cut, from the width, depth and height fields. */
static int cmd_extrude(void *c, void *g0)
{
    gm_rexx *g = app(g0);
    gs_parts *ps = list(g);
    gs_mesh out, *part;
    char line[160];
    const char *kind_s = omr_arg(c, "KIND");
    int kind, op, has, i;
    float w = 20.f, d = 10.f, h = 8.f;
    static const char *const names[] = {"Block", "Round", "Slot"};
    if (!ps) {
        omr_set_result(c, "no document");
        return OMR_ERROR;
    }
    kind = extrude_kind(kind_s);
    if (kind < 0) {
        omr_set_result(c, "KIND is block, round or slot");
        return OMR_ERROR;
    }
    op = extrude_op(omr_arg(c, "OP"));
    if (op < 0) {
        omr_set_result(c, "OP is add, on or cut");
        return OMR_ERROR;
    }
    has = arg_f(c, "W", &w);
    if (has < 0 || arg_f(c, "D", &d) < 0 || arg_f(c, "H", &h) < 0) {
        omr_set_result(c, "not a number");
        return OMR_ERROR;
    }
    if (op == GM_ON_BED && gs_parts_count(ps) >= GS_PARTS_MAX) {
        omr_set_result(c, "Twenty-four parts is the limit.");
        return OMR_ERROR;
    }
    part = op == GM_ON_BED ? NULL : picked(g);
    memset(&out, 0, sizeof out);
    line[0] = 0;
    if (ge_extrude(part, kind, op, w, d, h, &out, line, (int)sizeof line)) {
        gs_mesh_free(&out);
        omr_set_result(c, line[0] ? line : "that extrude did not build");
        return OMR_ERROR;
    }
    remember(g);
    if (op == GM_ON_BED) {
        i = gs_parts_add(ps, &out, names[kind]);
        if (i < 0) {
            gs_mesh_free(&out);
            forget(g);
            omr_set_result(c, "Could not add that part");
            return OMR_ERROR;
        }
    } else {
        gs_mesh *m = picked(g);
        gs_mesh_free(m);
        *m = out;
    }
    g->face = -1;
    changed(g);
    omr_set_result(c, line);
    return OMR_OK;
}

static int bed_begin(void *c, gm_rexx *g, gs_mesh **m)
{
    *m = picked(g);
    if (!*m || !(*m)->n) {
        omr_set_result(c, "no part");
        return -1;
    }
    remember(g);
    return 0;
}

/* Drop, Centre and Lay, as the window's bed buttons. */
static int cmd_drop(void *c, void *g0)
{
    gm_rexx *g = app(g0);
    gs_mesh *m;
    if (bed_begin(c, g, &m)) return OMR_ERROR;
    ge_drop(m);
    g->face = -1;
    changed(g);
    omr_set_result(c, "On the bed.");
    return OMR_OK;
}

static int cmd_centre(void *c, void *g0)
{
    gm_rexx *g = app(g0);
    gs_mesh *m;
    if (bed_begin(c, g, &m)) return OMR_ERROR;
    ge_centre(m);
    g->face = -1;
    changed(g);
    omr_set_result(c, "Centred on the bed.");
    return OMR_OK;
}

static int cmd_lay(void *c, void *g0)
{
    gm_rexx *g = app(g0);
    gs_mesh *m;
    gs_v was_min, was_max;
    int face, has;
    float d;
    has = arg_i(c, "FACE", &face);
    if (has < 0) {
        omr_set_result(c, "not a number");
        return OMR_ERROR;
    }
    m = picked(g);
    if (!m || !m->n) {
        omr_set_result(c, "no part");
        return OMR_ERROR;
    }
    if (has <= 0) face = g->face >= 0 ? g->face : ge_largest_face(m);
    if (face < 0) {
        omr_set_result(c, "No face to lay");
        return OMR_ERROR;
    }
    was_min = m->min;
    was_max = m->max;
    remember(g);
    if (ge_lay_face(m, face)) {
        forget(g);
        omr_set_result(c, "That face will not lie flat");
        return OMR_ERROR;
    }
    d = m->min.x - was_min.x;
    if (d < 0.f) d = -d;
    if (d < 0.001f) {
        d = m->min.y - was_min.y;
        if (d < 0.f) d = -d;
    }
    if (d < 0.001f) {
        d = m->min.z - was_min.z;
        if (d < 0.f) d = -d;
    }
    if (d < 0.001f) {
        d = m->max.x - was_max.x;
        if (d < 0.f) d = -d;
    }
    if (d < 0.001f) {
        d = m->max.y - was_max.y;
        if (d < 0.f) d = -d;
    }
    if (d < 0.001f) {
        d = m->max.z - was_max.z;
        if (d < 0.f) d = -d;
    }
    if (d < 0.001f) {
        forget(g);
        omr_set_result(c, "That face is already flat on the bed");
        return OMR_OK;
    }
    g->face = face;
    changed(g);
    omr_set_result(c, "That face is on the bed.");
    return OMR_OK;
}

static const char *const metal_names[] = {"brass", "sand", "copper", "steel", "teal", "sage"};

static int metal_of(const char *s)
{
    int i;
    if (!s) return -1;
    for (i = 0; i < 6; i++)
        if (eqi(s, metal_names[i])) return i;
    return -1;
}

/* The name field, Hide and the metal swatches. */
static int cmd_rename(void *c, void *g0)
{
    gm_rexx *g = app(g0);
    gs_parts *ps = list(g);
    const char *name = omr_arg(c, "NAME");
    int at;
    if (!ps || (at = gs_parts_picked(ps)) < 0) {
        omr_set_result(c, "no part");
        return OMR_ERROR;
    }
    if (!name || !name[0]) {
        omr_set_result(c, "NAME is required");
        return OMR_ERROR;
    }
    if (!strcmp(name, gs_parts_name(ps, at))) {
        omr_set_result(c, name);
        return OMR_OK;
    }
    remember(g);
    if (gs_parts_rename(ps, at, name) < 0) {
        forget(g);
        omr_set_result(c, "That name can't be used");
        return OMR_ERROR;
    }
    changed(g);
    omr_set_result(c, gs_parts_name(ps, at));
    return OMR_OK;
}

static int cmd_show(void *c, void *g0)
{
    gm_rexx *g = app(g0);
    gs_parts *ps = list(g);
    int at, on, has;
    if (!ps || (at = gs_parts_picked(ps)) < 0) {
        omr_set_result(c, "no part");
        return OMR_ERROR;
    }
    has = arg_i(c, "ON", &on);
    if (has < 0) {
        omr_set_result(c, "use 0 or 1");
        return OMR_ERROR;
    }
    if (has <= 0) on = !gs_parts_shown(ps, at);
    else on = on ? 1 : 0;
    remember(g);
    if (gs_parts_show(ps, at, on) < 0) {
        forget(g);
        omr_set_result(c, "no part");
        return OMR_ERROR;
    }
    changed(g);
    omr_set_result(c, on ? "Showing." : "Hidden.");
    return OMR_OK;
}

static int cmd_colour(void *c, void *g0)
{
    gm_rexx *g = app(g0);
    gs_parts *ps = list(g);
    const char *name = omr_arg(c, "NAME");
    int at, colour, has;
    if (!ps || (at = gs_parts_picked(ps)) < 0) {
        omr_set_result(c, "no part");
        return OMR_ERROR;
    }
    if (name && name[0]) colour = metal_of(name);
    else {
        has = arg_i(c, "INDEX", &colour);
        if (has < 0) {
            omr_set_result(c, "not a number");
            return OMR_ERROR;
        }
        if (has <= 0) {
            omr_set_result(c, "NAME or INDEX is required");
            return OMR_ERROR;
        }
        colour -= 1;
    }
    if (colour < 0 || colour > 5) {
        omr_set_result(c, "NAME is brass, sand, copper, steel, teal or sage");
        return OMR_ERROR;
    }
    remember(g);
    if (gs_parts_set_colour(ps, at, colour) < 0) {
        forget(g);
        omr_set_result(c, "That metal can't be used");
        return OMR_ERROR;
    }
    changed(g);
    omr_set_result(c, metal_names[colour]);
    return OMR_OK;
}

static const char *const look_names[] = {"top", "front", "side", "frame"};
static const char *const look_said[] = {"Top.", "Front.", "Side.", "Framed."};
static const char *const shade_names[] = {"metal", "clay", "wire"};
static const char *const shade_said[] = {"Metal.", "Clay.", "Wire."};

static int word_of(const char *s, const char *const *names, int n)
{
    int i;
    if (!s) return -1;
    for (i = 0; i < n; i++)
        if (eqi(s, names[i])) return i;
    return -1;
}

/* Top, front, side, or frame. The picture changes. The part does not. */
static int cmd_look(void *c, void *g0)
{
    gm_rexx *g = app(g0);
    int which = word_of(omr_arg(c, "NAME"), look_names, 4);
    if (which < 0) {
        omr_set_result(c, "NAME is top, front, side or frame");
        return OMR_ERROR;
    }
    g->look = which;
    omr_set_result(c, look_said[which]);
    return OMR_OK;
}

static int cmd_shade(void *c, void *g0)
{
    gm_rexx *g = app(g0);
    int which = word_of(omr_arg(c, "NAME"), shade_names, 3);
    if (which < 0) {
        omr_set_result(c, "NAME is metal, clay or wire");
        return OMR_ERROR;
    }
    g->shade = which;
    omr_set_result(c, shade_said[which]);
    return OMR_OK;
}

static int cmd_snap(void *c, void *g0)
{
    gm_rexx *g = app(g0);
    int on, has;
    has = arg_i(c, "ON", &on);
    if (has < 0) {
        omr_set_result(c, "use 0 or 1");
        return OMR_ERROR;
    }
    if (has <= 0) on = !g->snap;
    else on = on ? 1 : 0;
    g->snap = on;
    omr_set_result(c, on ? "Snap is 1 mm, 15 degrees, and steps of 0.1 scale." : "Snap is off.");
    return OMR_OK;
}

/* Section hides the picture above Z. It does not cut the part. */
static int cmd_section(void *c, void *g0)
{
    gm_rexx *g = app(g0);
    int on, has_on;
    float z;
    int has_z;
    char buf[96];
    has_on = arg_i(c, "ON", &on);
    if (has_on < 0) {
        omr_set_result(c, "use 0 or 1");
        return OMR_ERROR;
    }
    has_z = arg_f(c, "Z", &z);
    if (has_z < 0) {
        omr_set_result(c, "not a number");
        return OMR_ERROR;
    }
    if (has_z > 0) {
        if (z < 0.f) z = 0.f;
        if (z > 220.f) z = 220.f;
        g->section_z = (float)(long)(z * 10.f + 0.5f) / 10.f;
    }
    if (has_on > 0) g->section = on ? 1 : 0;
    else if (has_z > 0) g->section = 1;
    else g->section = !g->section;
    if (!g->section) {
        omr_set_result(c, "Section is off.");
        return OMR_OK;
    }
    snprintf(buf, sizeof buf, "Section at %.1f mm. Everything above the plane is hidden.", (double)g->section_z);
    omr_set_result(c, buf);
    return OMR_OK;
}

static const char *const tool_names[] = {"orbit", "move", "rot", "scale", "measure", "pull", "draw"};
static const char *const tool_said[] = {
    "Orbit. Drag turns the view. A click picks a face.",
    "Move. Drag slides. Shift-drag lifts.",
    "Rotate. Drag turns the part.",
    "Scale. Drag up grows it.",
    "Measure. Click two points on the part or bed.",
    "Pull. Drag the face along its normal. Escape leaves.",
    "Draw. Drag the footprint on the bed. Escape leaves.",
};

/* The tool buttons. Pull needs a face. None of them changes the part. */
static int cmd_tool(void *c, void *g0)
{
    gm_rexx *g = app(g0);
    const char *name = omr_arg(c, "NAME");
    gs_mesh *m;
    int which;
    if (name && eqi(name, "rotate")) name = "rot";
    if (name && eqi(name, "sketch")) name = "draw";
    which = word_of(name, tool_names, 7);
    if (which < 0) {
        omr_set_result(c, "NAME is orbit, move, rot, scale, measure, pull or draw");
        return OMR_ERROR;
    }
    if (which == 5) {
        m = picked(g);
        if (g->face < 0 || !m || !m->n) {
            omr_set_result(c, "Click a face on the part, then Drag.");
            return OMR_ERROR;
        }
    }
    g->tool = which;
    omr_set_result(c, tool_said[which]);
    return OMR_OK;
}

static int write_stl(gm_rexx *g, omr_cmd *c, const char *file)
{
    gs_mesh *m = picked(g);
    if (!file || !file[0]) {
        omr_set_result(c, "no file");
        return OMR_ERROR;
    }
    if (!m || !m->n) {
        omr_set_result(c, "no part");
        return OMR_ERROR;
    }
    if (gm_stl_write(m, file)) {
        omr_set_result(c, "Could not save that STL");
        return OMR_ERROR;
    }
    snprintf(g->file, sizeof g->file, "%s", file);
    g->dirty = 0;
    omr_set_result(c, file);
    return OMR_OK;
}

static int cmd_save(void *c, void *g0)
{
    gm_rexx *g = app(g0);
    const char *file = omr_arg(c, "FILE");
    if (!file) file = g->file;
    return write_stl(g, c, file);
}

static int cmd_export(void *c, void *g0)
{
    gm_rexx *g = app(g0);
    return write_stl(g, c, omr_arg(c, "FILE"));
}

static int cmd_activate(void *c, void *g0)
{
    gm_rexx *g = app(g0);
    (void)c;
    if (g->activate && g->win) g->activate(g->win);
    return OMR_OK;
}

static int cmd_hide(void *c, void *g0)
{
    gm_rexx *g = app(g0);
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
    gm_rexx *g = app(g0);
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
    gm_rexx *g = app(g0);
    const char *key = omr_arg(c, "KEY"), *val = omr_arg(c, "VALUE");
    char err[80];
    int i, got;
    float f = 0;
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
        got = 0;
        f = (float)strtod(val, NULL);
        /* a second pass with end-check */
        {
            char *end = NULL;
            f = (float)strtod(val, &end);
            if (end == val || (end && *end)) {
                omr_set_result(c, "not a number");
                return OMR_ERROR;
            }
            (void)got;
        }
        gs_setting_set(&g->set, i, f);
    }
    fmt_setting(&g->set, i, err, (int)sizeof err);
    omr_set_result(c, err);
    return OMR_OK;
}

static int cmd_status(void *c, void *g0)
{
    gm_rexx *g = app(g0);
    gs_parts *ps = list(g);
    char buf[320], nbuf[16];
    int n = ps ? gs_parts_count(ps) : 0, at = ps ? gs_parts_picked(ps) : -1;
    const char *port = g->port_name ? g->port_name : "";
    snprintf(buf, sizeof buf, "%.40s %.240s %d", port[0] ? port : GM_REXX_PORT, g->file[0] ? g->file : "-", g->dirty);
    omr_set_result(c, buf);
    stem_put(c, "ST", "PORT", port[0] ? port : GM_REXX_PORT);
    stem_put(c, "ST", "FILE", g->file);
    stem_put(c, "ST", "DIRTY", g->dirty ? "1" : "0");
    snprintf(nbuf, sizeof nbuf, "%d", n);
    stem_put(c, "ST", "PARTS", nbuf);
    snprintf(nbuf, sizeof nbuf, "%d", at >= 0 ? at + 1 : 0);
    stem_put(c, "ST", "PICKED", nbuf);
    snprintf(nbuf, sizeof nbuf, "%d", g->face);
    stem_put(c, "ST", "FACE", nbuf);
    return OMR_OK;
}

static int shape_kind(const char *name)
{
    int i;
    for (i = 0; i < GE_SHAPES; i++)
        if (ge_shape_name(i) && eqi(ge_shape_name(i), name)) return i;
    return -1;
}

static int cmd_add(void *c, void *g0)
{
    gm_rexx *g = app(g0);
    gs_parts *ps = list(g);
    gs_mesh part;
    const char *shape = omr_arg(c, "SHAPE");
    char buf[80];
    int kind, i, has;
    float x = 0, y = 0, z = 0, w = 0, d = 0, h = 0;
    if (!ps || !shape) {
        omr_set_result(c, "no shape");
        return OMR_ERROR;
    }
    kind = shape_kind(shape);
    if (kind < 0) {
        omr_set_result(c, "unknown shape");
        return OMR_ERROR;
    }
    if (gs_parts_count(ps) >= GS_PARTS_MAX) {
        omr_set_result(c, "Twenty-four parts is the limit.");
        return OMR_ERROR;
    }
    memset(&part, 0, sizeof part);
    if (ge_shape(&part, kind, picked(g))) {
        gs_mesh_free(&part);
        snprintf(buf, sizeof buf, "The %s did not build", ge_shape_name(kind));
        omr_set_result(c, buf);
        return OMR_ERROR;
    }
    has = arg_f(c, "X", &x);
    if (has < 0 || arg_f(c, "Y", &y) < 0 || arg_f(c, "Z", &z) < 0 || arg_f(c, "W", &w) < 0 ||
        arg_f(c, "D", &d) < 0 || arg_f(c, "H", &h) < 0) {
        gs_mesh_free(&part);
        omr_set_result(c, "not a number");
        return OMR_ERROR;
    }
    if (arg_f(c, "X", &x) > 0 || arg_f(c, "Y", &y) > 0 || arg_f(c, "Z", &z) > 0) {
        float pos[3], size[3];
        ge_where(&part, pos, size);
        if (arg_f(c, "X", &x) <= 0) x = pos[0];
        if (arg_f(c, "Y", &y) <= 0) y = pos[1];
        if (arg_f(c, "Z", &z) <= 0) z = pos[2];
        ge_place(&part, x, y, z);
    }
    if (arg_f(c, "W", &w) > 0 && ge_fit(&part, 0, w)) {
        gs_mesh_free(&part);
        omr_set_result(c, "could not size the part");
        return OMR_ERROR;
    }
    if (arg_f(c, "D", &d) > 0 && ge_fit(&part, 1, d)) {
        gs_mesh_free(&part);
        omr_set_result(c, "could not size the part");
        return OMR_ERROR;
    }
    if (arg_f(c, "H", &h) > 0 && ge_fit(&part, 2, h)) {
        gs_mesh_free(&part);
        omr_set_result(c, "could not size the part");
        return OMR_ERROR;
    }
    remember(g);
    i = gs_parts_add(ps, &part, ge_shape_name(kind));
    if (i < 0) {
        gs_mesh_free(&part);
        forget(g);
        omr_set_result(c, "Could not add that part");
        return OMR_ERROR;
    }
    g->face = -1;
    changed(g);
    snprintf(buf, sizeof buf, "%d %s", i + 1, gs_parts_name(ps, i));
    omr_set_result(c, buf);
    return OMR_OK;
}

static int cmd_select(void *c, void *g0)
{
    gm_rexx *g = app(g0);
    gs_parts *ps = list(g);
    const char *name = omr_arg(c, "NAME");
    int part = 0, i, has;
    if (!ps) {
        omr_set_result(c, "no document");
        return OMR_ERROR;
    }
    has = arg_i(c, "PART", &part);
    if (has < 0) {
        omr_set_result(c, "not a number");
        return OMR_ERROR;
    }
    if (has > 0) {
        if (part < 1 || part > gs_parts_count(ps)) {
            omr_set_result(c, "no such part");
            return OMR_ERROR;
        }
        gs_parts_pick(ps, part - 1);
        g->face = -1;
        if (g->after) g->after(g->ui);
        return OMR_OK;
    }
    if (!name) {
        omr_set_result(c, "PART or NAME is required");
        return OMR_ERROR;
    }
    for (i = 0; i < gs_parts_count(ps); i++)
        if (eqi(gs_parts_name(ps, i), name)) {
            gs_parts_pick(ps, i);
            g->face = -1;
            if (g->after) g->after(g->ui);
            return OMR_OK;
        }
    omr_set_result(c, "no such part");
    return OMR_ERROR;
}

static int cmd_delete(void *c, void *g0)
{
    gm_rexx *g = app(g0);
    gs_parts *ps = list(g);
    int at;
    (void)c;
    if (!ps) {
        omr_set_result(c, "no document");
        return OMR_ERROR;
    }
    at = gs_parts_picked(ps);
    if (at < 0 || gs_parts_count(ps) < 2) {
        omr_set_result(c, "the last part cannot be deleted");
        return OMR_ERROR;
    }
    remember(g);
    gs_parts_remove(ps, at);
    g->face = -1;
    changed(g);
    return OMR_OK;
}

static int cmd_undo(void *c, void *g0)
{
    gm_rexx *g = app(g0);
    gs_parts *was;
    char v[16];
    int redo;
    first_word(omr_text(c), v, (int)sizeof v);
    redo = eqi(v, "REDO");
    if (!g->steps || !g->parts) {
        omr_set_result(c, redo ? "Nothing to redo" : "Nothing to undo");
        return OMR_ERROR;
    }
    was = redo ? ge_steps_redo(g->steps, *g->parts) : ge_steps_undo(g->steps, *g->parts);
    if (!was) {
        omr_set_result(c, redo ? "Nothing to redo" : "Nothing to undo");
        return OMR_ERROR;
    }
    *g->parts = was;
    g->face = -1;
    g->marks = 0;
    changed(g);
    return OMR_OK;
}

static int cmd_bool(void *c, void *g0)
{
    gm_rexx *g = app(g0);
    gs_parts *ps = list(g), *was;
    char line[96], v[16];
    int op = 0;
    if (!ps) {
        omr_set_result(c, "no document");
        return OMR_ERROR;
    }
    first_word(omr_text(c), v, (int)sizeof v);
    if (eqi(v, "CUT")) op = 1;
    else if (eqi(v, "INTERSECT")) op = 2;
    was = gs_parts_copy(ps);
    if (ge_combine(ps, op, line, (int)sizeof line)) {
        if (was) gs_parts_free(was);
        omr_set_result(c, line[0] ? line : "the parts could not be combined");
        return OMR_ERROR;
    }
    if (g->steps && was) ge_steps_push(g->steps, was);
    if (was) gs_parts_free(was);
    g->face = -1;
    changed(g);
    omr_set_result(c, line);
    return OMR_OK;
}

static int cmd_move(void *c, void *g0)
{
    gm_rexx *g = app(g0);
    gs_mesh *m = picked(g);
    float x = 0, y = 0, z = 0;
    if (!m) {
        omr_set_result(c, "no part");
        return OMR_ERROR;
    }
    if (arg_f(c, "X", &x) < 0 || arg_f(c, "Y", &y) < 0 || arg_f(c, "Z", &z) < 0) {
        omr_set_result(c, "not a number");
        return OMR_ERROR;
    }
    if (arg_f(c, "X", &x) <= 0 && arg_f(c, "Y", &y) <= 0 && arg_f(c, "Z", &z) <= 0) {
        omr_set_result(c, "X, Y or Z is required");
        return OMR_ERROR;
    }
    if (arg_f(c, "X", &x) <= 0) x = 0;
    if (arg_f(c, "Y", &y) <= 0) y = 0;
    if (arg_f(c, "Z", &z) <= 0) z = 0;
    remember(g);
    ge_move(m, x, y, z);
    changed(g);
    return OMR_OK;
}

static int cmd_place(void *c, void *g0)
{
    gm_rexx *g = app(g0);
    gs_mesh *m = picked(g);
    float pos[3], size[3], x = 0, y = 0, z = 0;
    int hx, hy, hz;
    if (!m) {
        omr_set_result(c, "no part");
        return OMR_ERROR;
    }
    hx = arg_f(c, "X", &x);
    hy = arg_f(c, "Y", &y);
    hz = arg_f(c, "Z", &z);
    if (hx < 0 || hy < 0 || hz < 0) {
        omr_set_result(c, "not a number");
        return OMR_ERROR;
    }
    if (hx <= 0 && hy <= 0 && hz <= 0) {
        omr_set_result(c, "X, Y or Z is required");
        return OMR_ERROR;
    }
    ge_where(m, pos, size);
    if (hx <= 0) x = pos[0];
    if (hy <= 0) y = pos[1];
    if (hz <= 0) z = pos[2];
    remember(g);
    ge_place(m, x, y, z);
    changed(g);
    return OMR_OK;
}

static int axis_of(const char *s)
{
    if (!s) return -1;
    if (eqi(s, "x") || eqi(s, "0")) return 0;
    if (eqi(s, "y") || eqi(s, "1")) return 1;
    if (eqi(s, "z") || eqi(s, "2")) return 2;
    return -1;
}

static int cmd_size(void *c, void *g0)
{
    gm_rexx *g = app(g0);
    gs_mesh *m = picked(g);
    const char *ax = omr_arg(c, "AXIS");
    float mm = 0, x = 0, y = 0, z = 0;
    int a, hx, hy, hz, hm;
    if (!m) {
        omr_set_result(c, "no part");
        return OMR_ERROR;
    }
    hx = arg_f(c, "X", &x);
    hy = arg_f(c, "Y", &y);
    hz = arg_f(c, "Z", &z);
    hm = arg_f(c, "MM", &mm);
    if (hx < 0 || hy < 0 || hz < 0 || hm < 0) {
        omr_set_result(c, "not a number");
        return OMR_ERROR;
    }
    remember(g);
    if (ax || hm > 0) {
        a = axis_of(ax);
        if (a < 0 || hm <= 0 || ge_fit(m, a, mm)) {
            forget(g);
            omr_set_result(c, "could not size the part");
            return OMR_ERROR;
        }
    } else {
        if (hx <= 0 && hy <= 0 && hz <= 0) {
            forget(g);
            omr_set_result(c, "a size is required");
            return OMR_ERROR;
        }
        if (hx > 0 && ge_fit(m, 0, x)) {
            forget(g);
            omr_set_result(c, "could not size the part");
            return OMR_ERROR;
        }
        if (hy > 0 && ge_fit(m, 1, y)) {
            forget(g);
            omr_set_result(c, "could not size the part");
            return OMR_ERROR;
        }
        if (hz > 0 && ge_fit(m, 2, z)) {
            forget(g);
            omr_set_result(c, "could not size the part");
            return OMR_ERROR;
        }
    }
    changed(g);
    return OMR_OK;
}

static int cmd_hole(void *c, void *g0)
{
    gm_rexx *g = app(g0);
    gs_mesh *m = picked(g), copy;
    const char *kind_s = omr_arg(c, "KIND");
    ge_holes h;
    char why[96];
    int kind;
    memset(&h, 0, sizeof h);
    if (!m) {
        omr_set_result(c, "no part");
        return OMR_ERROR;
    }
    if (!kind_s) {
        omr_set_result(c, "KIND is required");
        return OMR_ERROR;
    }
    if (eqi(kind_s, "circle")) kind = GE_BOLT_CIRCLE;
    else if (eqi(kind_s, "row")) kind = GE_HOLE_ROW;
    else if (eqi(kind_s, "counterbore")) kind = GE_COUNTERBORE;
    else if (eqi(kind_s, "drill")) kind = GE_DRILL;
    else if (eqi(kind_s, "countersink")) kind = GE_COUNTERSINK;
    else {
        omr_set_result(c, "KIND is circle, row, counterbore, drill or countersink");
        return OMR_ERROR;
    }
    if (arg_f(c, "D", &h.d) < 0 || arg_i(c, "N", &h.n) < 0 || arg_f(c, "PCD", &h.pcd) < 0 ||
        arg_f(c, "PITCH", &h.pitch) < 0 || arg_f(c, "CB", &h.cb) < 0 || arg_f(c, "DEEP", &h.deep) < 0) {
        omr_set_result(c, "not a number");
        return OMR_ERROR;
    }
    h.mouth = h.cb;
    memset(&copy, 0, sizeof copy);
    if (ge_copy(&copy, m) || ge_drill(&copy, kind, &h, why, (int)sizeof why)) {
        gs_mesh_free(&copy);
        omr_set_result(c, why[0] ? why : "the holes could not be cut");
        return OMR_ERROR;
    }
    remember(g);
    gs_mesh_free(m);
    *picked(g) = copy;
    g->face = -1;
    changed(g);
    omr_set_result(c, why);
    return OMR_OK;
}

/* The bench's washer, a new part on the bed past the parts shown. */
static int cmd_washer(void *c, void *g0)
{
    gm_rexx *g = app(g0);
    char line[120];
    if (!list(g)) {
        omr_set_result(c, "no part");
        return OMR_ERROR;
    }
    remember(g);
    if (ge_washer(list(g), line, (int)sizeof line) < 0) {
        forget(g);
        omr_set_result(c, line[0] ? line : "the washer did not build");
        return OMR_ERROR;
    }
    g->face = -1;
    changed(g);
    omr_set_result(c, line);
    return OMR_OK;
}

static int axis_of(const char *s);

static int arrange_begin(void *c, gm_rexx *g)
{
    if (!list(g) || gs_parts_count(list(g)) < 1) {
        omr_set_result(c, "no part");
        return -1;
    }
    remember(g);
    return 0;
}

static int arrange_finish(void *c, gm_rexx *g, int rc, char *line)
{
    if (rc < 0) {
        forget(g);
        omr_set_result(c, line && line[0] ? line : "that did not arrange");
        return OMR_ERROR;
    }
    g->face = -1;
    changed(g);
    omr_set_result(c, line);
    return OMR_OK;
}

/* The bench's Copy: one copy, 6 mm on in X, picked. */
static int cmd_copy(void *c, void *g0)
{
    gm_rexx *g = app(g0);
    char line[160];
    if (arrange_begin(c, g)) return OMR_ERROR;
    return arrange_finish(c, g, ge_duplicate(list(g), line, (int)sizeof line), line);
}

/* Line of copies along an axis. COUNT and GAP match the Arrange fields. */
static int cmd_line(void *c, void *g0)
{
    gm_rexx *g = app(g0);
    char line[160];
    int axis, count = 3, has;
    float gap = 6.f;
    const char *ax = omr_arg(c, "AXIS");
    axis = ax && ax[0] ? axis_of(ax) : 0;
    if (axis < 0) {
        omr_set_result(c, "AXIS is x, y or z");
        return OMR_ERROR;
    }
    has = arg_i(c, "COUNT", &count);
    if (has < 0) {
        omr_set_result(c, "not a number");
        return OMR_ERROR;
    }
    has = arg_f(c, "GAP", &gap);
    if (has < 0) {
        omr_set_result(c, "not a number");
        return OMR_ERROR;
    }
    if (arrange_begin(c, g)) return OMR_ERROR;
    return arrange_finish(c, g, ge_line(list(g), axis, count, gap, line, (int)sizeof line), line);
}

static int cmd_ring(void *c, void *g0)
{
    gm_rexx *g = app(g0);
    char line[160];
    int count = 3, has;
    has = arg_i(c, "COUNT", &count);
    if (has < 0) {
        omr_set_result(c, "not a number");
        return OMR_ERROR;
    }
    if (arrange_begin(c, g)) return OMR_ERROR;
    return arrange_finish(c, g, ge_ring(list(g), count, line, (int)sizeof line), line);
}

static int cmd_mirror(void *c, void *g0)
{
    gm_rexx *g = app(g0);
    char line[160];
    if (arrange_begin(c, g)) return OMR_ERROR;
    return arrange_finish(c, g, ge_mirror(list(g), line, (int)sizeof line), line);
}

static int cmd_turn(void *c, void *g0)
{
    gm_rexx *g = app(g0);
    char line[160];
    int axis, has;
    float deg = 90.f;
    const char *ax = omr_arg(c, "AXIS");
    axis = ax && ax[0] ? axis_of(ax) : 2;
    if (axis < 0) {
        omr_set_result(c, "AXIS is x, y or z");
        return OMR_ERROR;
    }
    has = arg_f(c, "DEG", &deg);
    if (has < 0) {
        omr_set_result(c, "not a number");
        return OMR_ERROR;
    }
    if (arrange_begin(c, g)) return OMR_ERROR;
    return arrange_finish(c, g, ge_turn(list(g), axis, deg, line, (int)sizeof line), line);
}

static int cmd_stack(void *c, void *g0)
{
    gm_rexx *g = app(g0);
    char line[160];
    if (arrange_begin(c, g)) return OMR_ERROR;
    return arrange_finish(c, g, ge_stack(list(g), line, (int)sizeof line), line);
}

static int cmd_solo(void *c, void *g0)
{
    gm_rexx *g = app(g0);
    char line[160];
    if (arrange_begin(c, g)) return OMR_ERROR;
    return arrange_finish(c, g, ge_solo(list(g), line, (int)sizeof line), line);
}

/* The bench's split: the picked part cut at Z, or through its middle. */
static int cmd_split(void *c, void *g0)
{
    gm_rexx *g = app(g0);
    char line[120];
    float z = 0.f;
    int have;
    if (!list(g)) {
        omr_set_result(c, "no part");
        return OMR_ERROR;
    }
    have = arg_f(c, "Z", &z);
    if (have < 0) {
        omr_set_result(c, "not a number");
        return OMR_ERROR;
    }
    remember(g);
    if (ge_split(list(g), z, have > 0, line, (int)sizeof line) < 0) {
        forget(g);
        omr_set_result(c, line[0] ? line : "the split did not cut");
        return OMR_ERROR;
    }
    g->face = -1;
    changed(g);
    omr_set_result(c, line);
    return OMR_OK;
}

static int cmd_face(void *c, void *g0, int chamfer)
{
    gm_rexx *g = app(g0);
    gs_mesh *m = picked(g);
    char line[96];
    float mm = 0;
    int face = g->face, has;
    if (!m) {
        omr_set_result(c, "no part");
        return OMR_ERROR;
    }
    has = arg_i(c, "FACE", &face);
    if (has < 0) {
        omr_set_result(c, "not a number");
        return OMR_ERROR;
    }
    if (arg_f(c, "MM", &mm) <= 0) {
        omr_set_result(c, "MM is required");
        return OMR_ERROR;
    }
    remember(g);
    if ((chamfer ? ge_chamfer(m, face, mm, line, (int)sizeof line) : ge_pull(m, face, mm, line, (int)sizeof line)) < 0) {
        forget(g);
        omr_set_result(c, line);
        return OMR_ERROR;
    }
    changed(g);
    omr_set_result(c, line);
    return OMR_OK;
}

static int cmd_pull(void *c, void *g0) { return cmd_face(c, g0, 0); }

static int cmd_chamfer(void *c, void *g0) { return cmd_face(c, g0, 1); }

static int cmd_printer(void *c, void *g0)
{
    gm_rexx *g = app(g0);
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
    changed(g);
    omr_set_result(c, g->machine.name[0] ? g->machine.name : (profile && profile[0] ? profile : driver));
    return OMR_OK;
}

static int cmd_gcode(void *c, void *g0)
{
    gm_rexx *g = app(g0);
    gs_parts *ps = list(g);
    gs_mesh shown;
    gt_path path;
    gp_opts o;
    gc_opts go;
    gp_more x;
    gc_more gx;
    om_job job;
    const char *file = omr_arg(c, "FILE");
    char err[160];
    FILE *f;
    int rc = OMR_ERROR;
    if (!file) {
        omr_set_result(c, "FILE is required");
        return OMR_ERROR;
    }
    if (!ps) {
        omr_set_result(c, "no document");
        return OMR_ERROR;
    }
    memset(&shown, 0, sizeof shown);
    memset(&path, 0, sizeof path);
    if (gs_parts_scene(ps, &shown) || !shown.n) {
        omr_set_result(c, "nothing to slice");
        goto done;
    }
    gs_settings_opts(&g->set, &o, &go);
    gs_settings_more(&g->set, &x);
    gs_settings_gc_more(&g->set, &go, &gx);
    if (gp_part_more(&path, &shown, &o, &x) || !path.n) {
        omr_set_result(c, "That did not slice: out of memory, or nothing to slice");
        goto done;
    }
    if (!g->have_machine) {
        omr_set_result(c, "no printer");
        goto done;
    }
    gc_job(&path, &g->machine, &go, &job);
    if (om_check(&g->machine, &job, err, (int)sizeof err) & ~OM_TOO_FAST) {
        omr_set_result(c, err);
        goto done;
    }
    f = fopen(file, "w");
    if (!f || gc_write_more(to_file, f, &path, &g->machine, &gx, NULL, err, (int)sizeof err)) {
        if (f) fclose(f);
        omr_set_result(c, err[0] ? err : "Could not write there");
        goto done;
    }
    fclose(f);
    omr_set_result(c, file);
    rc = OMR_OK;
done:
    gs_mesh_free(&shown);
    gt_free(&path);
    return rc;
}

static int cmd_measure(void *c, void *g0)
{
    gm_rexx *g = app(g0);
    gs_v a, b;
    char buf[160], nbuf[32];
    float x1 = 0, y1 = 0, z1 = 0, x2 = 0, y2 = 0, z2 = 0;
    int h1, h2, h3, h4, h5, h6;
    memset(&a, 0, sizeof a);
    memset(&b, 0, sizeof b);
    h1 = arg_f(c, "X1", &x1);
    h2 = arg_f(c, "Y1", &y1);
    h3 = arg_f(c, "Z1", &z1);
    h4 = arg_f(c, "X2", &x2);
    h5 = arg_f(c, "Y2", &y2);
    h6 = arg_f(c, "Z2", &z2);
    if (h1 < 0 || h2 < 0 || h3 < 0 || h4 < 0 || h5 < 0 || h6 < 0) {
        omr_set_result(c, "not a number");
        return OMR_ERROR;
    }
    if (h1 > 0 || h2 > 0 || h3 > 0 || h4 > 0 || h5 > 0 || h6 > 0) {
        a.x = h1 > 0 ? x1 : 0;
        a.y = h2 > 0 ? y1 : 0;
        a.z = h3 > 0 ? z1 : 0;
        b.x = h4 > 0 ? x2 : 0;
        b.y = h5 > 0 ? y2 : 0;
        b.z = h6 > 0 ? z2 : 0;
    } else if (g->marks >= 2) {
        a = g->mark[0];
        b = g->mark[1];
    } else {
        omr_set_result(c, "two points are needed");
        return OMR_ERROR;
    }
    ge_measure_line(buf, (int)sizeof buf, a, b);
    omr_set_result(c, buf);
    {
        float dx = b.x - a.x, dy = b.y - a.y, dz = b.z - a.z;
        float dist = dx * dx + dy * dy + dz * dz;
        /* sqrt without pulling a second math path: ge_measure_line already wrote RESULT */
        snprintf(nbuf, sizeof nbuf, "%.3f", (double)(dist > 0 ? 0 : 0)); /* filled below */
        (void)nbuf;
        {
            float d = (float)strtod(buf, NULL); /* the line starts with the distance */
            snprintf(nbuf, sizeof nbuf, "%.3f", (double)d);
            stem_put(c, "ST", "DISTANCE", nbuf);
            snprintf(nbuf, sizeof nbuf, "%.3f", (double)(dx < 0 ? -dx : dx));
            stem_put(c, "ST", "X", nbuf);
            snprintf(nbuf, sizeof nbuf, "%.3f", (double)(dy < 0 ? -dy : dy));
            stem_put(c, "ST", "Y", nbuf);
            snprintf(nbuf, sizeof nbuf, "%.3f", (double)(dz < 0 ? -dz : dz));
            stem_put(c, "ST", "Z", nbuf);
        }
    }
    return OMR_OK;
}

static int cmd_facts(void *c, void *g0)
{
    gm_rexx *g = app(g0);
    gs_parts *ps = list(g);
    gs_mesh *m;
    gs_facts f;
    char buf[160], nbuf[32];
    int part = 0, has, at;
    if (!ps) {
        omr_set_result(c, "no document");
        return OMR_ERROR;
    }
    has = arg_i(c, "PART", &part);
    if (has < 0) {
        omr_set_result(c, "not a number");
        return OMR_ERROR;
    }
    at = has > 0 ? part - 1 : gs_parts_picked(ps);
    m = gs_parts_mesh(ps, at);
    if (!m) {
        omr_set_result(c, "no such part");
        return OMR_ERROR;
    }
    ge_facts_line(buf, (int)sizeof buf, m);
    omr_set_result(c, buf);
    if (gm_facts(m, &f) == 0) {
        snprintf(nbuf, sizeof nbuf, "%.4g", (double)f.volume);
        stem_put(c, "ST", "VOLUME", nbuf);
        snprintf(nbuf, sizeof nbuf, "%.4g", (double)f.area);
        stem_put(c, "ST", "SURFACE", nbuf);
        snprintf(nbuf, sizeof nbuf, "%.3f", (double)f.sx);
        stem_put(c, "ST", "X", nbuf);
        snprintf(nbuf, sizeof nbuf, "%.3f", (double)f.sy);
        stem_put(c, "ST", "Y", nbuf);
        snprintf(nbuf, sizeof nbuf, "%.3f", (double)f.sz);
        stem_put(c, "ST", "Z", nbuf);
        snprintf(nbuf, sizeof nbuf, "%d", f.tris);
        stem_put(c, "ST", "TRIANGLES", nbuf);
    }
    return OMR_OK;
}

static int cmd_version(void *c, void *g0)
{
    gm_rexx *g = app(g0);
    omr_set_result(c, g->version ? g->version : "GFOpenMake");
    return OMR_OK;
}

static const omr_verb verbs[] = {
    {sizeof(omr_verb), "VERSION", "", "the program name and version", cmd_version},
    {sizeof(omr_verb), "QUIT", "FORCE/S", "close the window", cmd_quit},
    {sizeof(omr_verb), "OPEN", "FILE/A", "open an STL as a new part", cmd_open},
    {sizeof(omr_verb), "SHELF", "NAME/K INDEX/N/K", "put a shelf example on the bed", cmd_shelf},
    {sizeof(omr_verb), "EXTRUDE", "KIND/A OP/K W/K D/K H/K", "add, join or cut a block, round or slot", cmd_extrude},
    {sizeof(omr_verb), "DROP", "", "drop the picked part onto the bed", cmd_drop},
    {sizeof(omr_verb), "CENTRE", "", "centre the picked part on the bed", cmd_centre},
    {sizeof(omr_verb), "LAY", "FACE/N/K", "lay a face of the picked part on the bed", cmd_lay},
    {sizeof(omr_verb), "RENAME", "NAME/A", "rename the picked part", cmd_rename},
    {sizeof(omr_verb), "SHOW", "ON/N/K", "show or hide the picked part", cmd_show},
    {sizeof(omr_verb), "COLOUR", "NAME/K INDEX/N/K", "the picked part's metal", cmd_colour},
    {sizeof(omr_verb), "LOOK", "NAME/A", "top, front, side or frame", cmd_look},
    {sizeof(omr_verb), "SHADE", "NAME/A", "metal, clay or wire", cmd_shade},
    {sizeof(omr_verb), "SNAP", "ON/N/K", "snap moves to 1 mm, 15 degrees and 0.1 scale", cmd_snap},
    {sizeof(omr_verb), "SECTION", "ON/N/K Z/K", "hide the picture above a height", cmd_section},
    {sizeof(omr_verb), "TOOL", "NAME/A", "orbit, move, rot, scale, measure, pull or draw", cmd_tool},
    {sizeof(omr_verb), "SAVE", "FILE/K", "write the picked part as an STL", cmd_save},
    {sizeof(omr_verb), "ACTIVATE", "", "bring the window to the front", cmd_activate},
    {sizeof(omr_verb), "HIDE", "", "send the window to the back", cmd_hide},
    {sizeof(omr_verb), "GET", "KEY/K STEM/K VAR/K", "read a print setting", cmd_get},
    {sizeof(omr_verb), "SET", "KEY/A VALUE/A", "set a print setting", cmd_set},
    {sizeof(omr_verb), "STATUS", "STEM/K VAR/K", "the port, the file and the parts", cmd_status},
    {sizeof(omr_verb), "ADD", "SHAPE/A X/K Y/K Z/K W/K D/K H/K", "add a part", cmd_add},
    {sizeof(omr_verb), "SELECT", "PART/N NAME/K", "pick a part", cmd_select},
    {sizeof(omr_verb), "DELETE", "", "delete the picked part", cmd_delete},
    {sizeof(omr_verb), "UNDO", "", "one step back", cmd_undo},
    {sizeof(omr_verb), "REDO", "", "one step forward", cmd_undo},
    {sizeof(omr_verb), "JOIN", "", "join the picked part to the stock", cmd_bool},
    {sizeof(omr_verb), "CUT", "", "cut the picked part from the stock", cmd_bool},
    {sizeof(omr_verb), "INTERSECT", "", "intersect the picked part with the stock", cmd_bool},
    {sizeof(omr_verb), "MOVE", "X/K Y/K Z/K", "move the picked part", cmd_move},
    {sizeof(omr_verb), "PLACE", "X/K Y/K Z/K", "place the picked part", cmd_place},
    {sizeof(omr_verb), "SIZE", "X/K Y/K Z/K AXIS/K MM/K", "size the picked part", cmd_size},
    {sizeof(omr_verb), "HOLE", "KIND/A D/K N/K PCD/K PITCH/K CB/K DEEP/K", "cut holes in the picked part", cmd_hole},
    {sizeof(omr_verb), "WASHER", "", "a 32 mm washer on the bed", cmd_washer},
    {sizeof(omr_verb), "COPY", "", "one copy of the picked part, 6 mm on in X", cmd_copy},
    {sizeof(omr_verb), "LINE", "AXIS/K COUNT/N/K GAP/K", "copies of the picked part in a line", cmd_line},
    {sizeof(omr_verb), "RING", "COUNT/N/K", "copies of the picked part round the bed", cmd_ring},
    {sizeof(omr_verb), "MIRROR", "", "mirror the picked part through X", cmd_mirror},
    {sizeof(omr_verb), "TURN", "AXIS/K DEG/K", "turn the picked part about its middle", cmd_turn},
    {sizeof(omr_verb), "STACK", "", "sit the picked part on the one before it", cmd_stack},
    {sizeof(omr_verb), "SOLO", "", "show only the picked part, or all of them again", cmd_solo},
    {sizeof(omr_verb), "SPLIT", "Z/K", "cut the picked part into two at a height", cmd_split},
    {sizeof(omr_verb), "PULL", "MM/A FACE/N/K", "pull a face by a distance", cmd_pull},
    {sizeof(omr_verb), "CHAMFER", "MM/A FACE/N/K", "chamfer a face's edges", cmd_chamfer},
    {sizeof(omr_verb), "EXPORT", "FILE/A", "write the picked part as an STL", cmd_export},
    {sizeof(omr_verb), "PRINTER", "DRIVER/K PROFILE/K", "pick the printer from a driver and profile", cmd_printer},
    {sizeof(omr_verb), "GCODE", "FILE/A", "slice the shown parts and write G-code", cmd_gcode},
    {sizeof(omr_verb), "MEASURE", "X1/K Y1/K Z1/K X2/K Y2/K Z2/K STEM/K VAR/K", "the distance between two points",
     cmd_measure},
    {sizeof(omr_verb), "FACTS", "PART/N/K STEM/K VAR/K", "a part's volume, surface, size and triangles", cmd_facts},
};

void gm_rexx_init(gm_rexx *g)
{
    if (!g) return;
    memset(g, 0, sizeof *g);
    g->size = (int)sizeof *g;
    g->face = -1;
    g->look = -1;
    g->tool = -1;
    g->version = "GFOpenMake 0.1.0";
    gs_settings_default(&g->set);
}

const omr_verb *gm_rexx_verbs(void) { return verbs; }

int gm_rexx_nverbs(void) { return (int)(sizeof verbs / sizeof verbs[0]); }
