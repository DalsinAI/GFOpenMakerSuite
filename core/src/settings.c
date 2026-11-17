/* A print's settings (gs_settings.h). MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gs_settings.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const patterns[] = {"lines", "grid", NULL};
static const char *const arounds[] = {"none", "skirt", "brim", NULL};

typedef struct {
    gs_setting d;
    size_t at;                  /* where in gs_settings */
} entry;

#define F(key, label, unit, field, lo, hi, step) {{key, label, unit, GS_FLOAT, lo, hi, step, NULL}, offsetof(gs_settings, field)}
#define I(key, label, field, lo, hi) {{key, label, "", GS_INT, lo, hi, 1.f, NULL}, offsetof(gs_settings, field)}
#define C(key, label, field, words) {{key, label, "", GS_CHOICE, 0.f, (float)(sizeof words / sizeof words[0] - 2), 1.f, words}, offsetof(gs_settings, field)}

static const entry table[] = {
    F("layer-height", "Layer height", "mm", layer_h, 0.08f, 0.8f, 0.02f),
    F("line-width", "Line width", "mm", line_w, 0.2f, 1.2f, 0.05f),
    I("walls", "Walls", walls, 1.f, 8.f),
    F("infill", "Infill", "%", infill, 0.f, 100.f, 5.f),
    C("pattern", "Pattern", pattern, patterns),
    I("top-layers", "Top layers", top, 0.f, 8.f),
    I("bottom-layers", "Bottom layers", bottom, 0.f, 8.f),
    F("speed", "Print speed", "mm/s", speed, 5.f, 500.f, 5.f),
    F("travel-speed", "Travel speed", "mm/s", travel, 10.f, 800.f, 10.f),
    F("first-layer-speed", "First layer", "mm/s", first, 5.f, 200.f, 5.f),
    F("hotend", "Hotend", "C", hotend, 0.f, 450.f, 5.f),
    F("bed", "Bed", "C", bed, 0.f, 150.f, 5.f),
    F("filament", "Filament", "mm", filament, 1.f, 3.f, 0.05f),
    C("around", "Around", around, arounds),
    I("loops", "Loops", loops, 0.f, 20.f),
    F("distance", "Distance", "mm", distance, 0.f, 20.f, 0.5f),
    F("retract", "Retract", "mm", retract, 0.f, 10.f, 0.5f),
    F("retract-speed", "Retract speed", "mm/s", retract_speed, 5.f, 100.f, 5.f),
};
#define N ((int)(sizeof table / sizeof table[0]))

void gs_settings_default(gs_settings *s)
{
    memset(s, 0, sizeof *s);
    s->layer_h = 0.2f;
    s->line_w = 0.4f;
    s->walls = 2;
    s->infill = 20.f;
    s->pattern = GS_LINES;
    s->top = 4;
    s->bottom = 3;
    s->speed = 50.f;
    s->travel = 150.f;
    s->first = 20.f;
    s->hotend = 200.f;
    s->bed = 60.f;
    s->filament = 1.75f;
    s->around = GS_SKIRT;
    s->loops = 2;
    s->distance = 3.f;
    s->retract = 1.f;
    s->retract_speed = 25.f;
}

int gs_settings_count(void) { return N; }

const gs_setting *gs_setting_at(int i) { return i >= 0 && i < N ? &table[i].d : NULL; }

int gs_setting_find(const char *key)
{
    int i;
    for (i = 0; i < N; i++)
        if (!strcmp(table[i].d.key, key)) return i;
    return -1;
}

float gs_setting_get(const gs_settings *s, int i)
{
    const char *at = (const char *)s + table[i].at;
    return table[i].d.type == GS_FLOAT ? *(const float *)at : (float)*(const int *)at;
}

void gs_setting_set(gs_settings *s, int i, float v)
{
    char *at = (char *)s + table[i].at;
    if (v < table[i].d.min) v = table[i].d.min;
    if (v > table[i].d.max) v = table[i].d.max;
    if (table[i].d.type == GS_FLOAT) *(float *)at = v;
    else *(int *)at = (int)(v + 0.5f);
}

const char *gs_setting_choice(int i, int v)
{
    int k;
    if (i < 0 || i >= N || !table[i].d.choices) return NULL;
    for (k = 0; table[i].d.choices[k]; k++)
        if (k == v) return table[i].d.choices[k];
    return NULL;
}

static int fail(char *err, int errlen, int line, const char *what, const char *word)
{
    if (err && errlen > 0) snprintf(err, (size_t)errlen, "line %d: %s: %s", line, what, word);
    return -1;
}

/* One "key = value" line, without its comment, trimmed. */
static int one(gs_settings *s, char *ln, int line, char *err, int errlen)
{
    char *eq = strchr(ln, '='), *key = ln, *val, *end;
    int i, k;
    if (!eq) return fail(err, errlen, line, "not key = value", ln);
    *eq = 0;
    val = eq + 1;
    while (*key == ' ' || *key == '\t') key++;
    for (end = eq; end > key && (end[-1] == ' ' || end[-1] == '\t'); end--) end[-1] = 0;
    while (*val == ' ' || *val == '\t') val++;
    if ((i = gs_setting_find(key)) < 0) return fail(err, errlen, line, "no such setting", key);
    if (table[i].d.type == GS_CHOICE) {
        for (k = 0; table[i].d.choices[k]; k++)
            if (!strcmp(table[i].d.choices[k], val)) break;
        if (!table[i].d.choices[k]) return fail(err, errlen, line, "no such choice", val);
        gs_setting_set(s, i, (float)k);
    } else {
        double d = strtod(val, &end);
        if (end == val || *end) return fail(err, errlen, line, "not a number", val);
        gs_setting_set(s, i, (float)d);
    }
    return 0;
}

int gs_settings_parse(gs_settings *s, const char *text, char *err, int errlen)
{
    char ln[160];
    int line = 0;
    while (*text) {
        const char *nl = strchr(text, '\n');
        size_t n = nl ? (size_t)(nl - text) : strlen(text);
        char *hash, *e;
        line++;
        if (n >= sizeof ln) n = sizeof ln - 1;
        memcpy(ln, text, n);
        ln[n] = 0;
        text = nl ? nl + 1 : text + strlen(text);
        if ((hash = strchr(ln, '#')) != NULL) *hash = 0;
        for (e = ln + strlen(ln); e > ln && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\r'); e--) e[-1] = 0;
        if (ln[strspn(ln, " \t")] && one(s, ln, line, err, errlen)) return -1;
    }
    return 0;
}

int gs_settings_format(const gs_settings *s, char *buf, int len)
{
    int i, at = 0, n;
    n = snprintf(buf, (size_t)len, "# GFOpenMake print settings\n");
    for (i = 0; n >= 0 && n < len - at && i <= N; i++) {
        at += n;
        if (i == N) return at;
        if (table[i].d.type == GS_CHOICE)
            n = snprintf(buf + at, (size_t)(len - at), "%s = %s\n", table[i].d.key,
                         gs_setting_choice(i, (int)gs_setting_get(s, i)));
        else n = snprintf(buf + at, (size_t)(len - at), "%s = %g\n", table[i].d.key, (double)gs_setting_get(s, i));
    }
    return -1;
}

int gs_settings_load(gs_settings *s, const char *path, char *err, int errlen)
{
    char text[4096];
    size_t n;
    FILE *f = fopen(path, "rb");
    if (!f) {
        if (err && errlen > 0) snprintf(err, (size_t)errlen, "%s: cannot open", path);
        return -1;
    }
    n = fread(text, 1, sizeof text - 1, f);
    fclose(f);
    text[n] = 0;
    return gs_settings_parse(s, text, err, errlen);
}

int gs_settings_save(const gs_settings *s, const char *path, char *err, int errlen)
{
    char text[2048];
    int n = gs_settings_format(s, text, (int)sizeof text);
    FILE *f = n < 0 ? NULL : fopen(path, "wb");
    int ok = f && fwrite(text, 1, (size_t)n, f) == (size_t)n;
    if (f && fclose(f)) ok = 0;
    if (!ok && err && errlen > 0) snprintf(err, (size_t)errlen, "%s: cannot write", path);
    return ok ? 0 : -1;
}

unsigned gs_settings_greyed(const om_machine *m)
{
    if (m->family != OM_PRINTER) return (1u << N) - 1u;
    return om_has(m, OM_HEATED_BED) && m->bed > 0.f ? 0u : GS_BIT("bed");
}

float gs_setting_most(const om_machine *m, int i)
{
    const char *k = table[i].d.key;
    float most = 0.f;
    if (!strcmp(k, "hotend")) most = m->hotend;
    else if (!strcmp(k, "bed")) most = m->bed;
    else if (!strcmp(k, "speed") || !strcmp(k, "travel-speed") || !strcmp(k, "first-layer-speed"))
        most = m->speed[OM_X] < m->speed[OM_Y] || m->speed[OM_Y] <= 0.f ? m->speed[OM_X] : m->speed[OM_Y];
    else if (!strcmp(k, "retract-speed")) most = m->speed[OM_E];
    return most > 0.f && most < table[i].d.max ? most : table[i].d.max;
}

unsigned gs_settings_fit(gs_settings *s, const om_machine *m)
{
    unsigned grey = gs_settings_greyed(m), held = 0u;
    int i;
    for (i = 0; i < N; i++) {
        float v = gs_setting_get(s, i), to = grey & (1u << i) ? 0.f : gs_setting_most(m, i);
        if (v > to) { /* past its range when greyed: 0 is off */
            char *at = (char *)s + table[i].at;
            if (table[i].d.type == GS_FLOAT) *(float *)at = to;
            else *(int *)at = (int)to;
            held |= 1u << i;
        }
    }
    return held;
}

void gs_settings_opts(const gs_settings *s, gp_opts *po, gc_opts *go)
{
    gc_defaults(go);
    po->layer_h = s->layer_h;
    po->line_w = s->line_w;
    po->walls = s->walls;
    po->skins = s->top > s->bottom ? s->top : s->bottom;
    po->infill = s->infill;
    po->brim = s->around == GS_BRIM ? s->loops : 0;
    go->hotend = s->hotend;
    go->bed = s->bed;
    go->filament = s->filament;
    go->layer_h = s->layer_h;
    go->speed[GT_TRAVEL] = s->travel;
    go->speed[GT_OUTER] = s->speed * 0.5f;
    go->speed[GT_INNER] = go->speed[GT_SKIN] = go->speed[GT_INFILL] = s->speed;
    go->speed[GT_BRIM] = s->first;
    go->retract = s->retract;
    go->retract_speed = s->retract_speed;
}
