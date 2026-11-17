/* The print settings panel (omview_settings.h).
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "omview_settings.h"

#include "omview_ui.h"

#include <stdio.h>
#include <string.h>

/* Each tab's title and its settings, by key. */
static const char *const tabs[][7] = {
    {"Layers", "layer-height", "line-width", "walls", "top-layers", "bottom-layers", NULL},
    {"Infill", "infill", "pattern", "around", "loops", "distance", NULL},
    {"Speed", "speed", "travel-speed", "first-layer-speed", "retract", "retract-speed", NULL},
    {"Heat", "hotend", "bed", "filament", NULL, NULL, NULL},
};
#define TABS ((int)(sizeof tabs / sizeof tabs[0]))

static const char css[] = "#set-tabs{gap:4px}"
                          "#set-tabs row{padding:0;gap:4px}"
                          "#set-tabs label{width:84px;color:#616161}"
                          "#set-tabs spinner,#set-tabs select{flex:1}"
                          "#set-tabs span{width:34px;color:#616161}" /* "mm/s" in Topaz 8 */;

int omv_settings_tabs(void) { return TABS; }

int omv_settings_tab_of(int i)
{
    const gs_setting *g = gs_setting_at(i);
    int t, k;
    for (t = 0; g && t < TABS; t++)
        for (k = 1; tabs[t][k]; k++)
            if (!strcmp(tabs[t][k], g->key)) return t;
    return -1;
}

void omv_settings_tab(olui *ui, int tab)
{
    olui_tabs_select(ui, olui_find(ui, "set-tabs"), tab);
}

/* The places a setting's numbers are written with: its step's. */
static int places(const gs_setting *g)
{
    float s = g->step;
    int d = 0;
    while (d < 4 && s - (float)(int)(s + 1e-4f) > 1e-4f) s *= 10.f, d++;
    return d;
}

static olui_el *field(olui *ui, const gs_setting *g)
{
    char id[48];
    snprintf(id, sizeof id, "set-%s", g->key);
    return olui_find(ui, id);
}

int omv_settings_add(olui *ui, const char *parent_id, char *err, int errlen)
{
    static char m[6144];
    int t, k, at, i;
    at = snprintf(m, sizeof m, "<tabs id=\"set-tabs\">");
    for (t = 0; t < TABS; t++) {
        at += snprintf(m + at, sizeof m - (size_t)at, "<tab title=\"%s\">", tabs[t][0]);
        for (k = 1; tabs[t][k]; k++) {
            const gs_setting *g = gs_setting_at(gs_setting_find(tabs[t][k]));
            int d = places(g);
            if (g->type == GS_CHOICE)
                at += snprintf(m + at, sizeof m - (size_t)at,
                               "<row><label for=\"set-%s\">%s</label><select id=\"set-%s\"></select><span></span></row>",
                               g->key, g->label, g->key);
            else
                at += snprintf(m + at, sizeof m - (size_t)at,
                               "<row><label for=\"set-%s\">%s</label><spinner id=\"set-%s\" min=\"%.*f\" max=\"%.*f\" "
                               "step=\"%.*f\" value=\"%.*f\"><span>%s</span></row>",
                               g->key, g->label, g->key, d, (double)g->min, d, (double)g->max, d, (double)g->step, d,
                               (double)g->min, g->unit);
        }
        at += snprintf(m + at, sizeof m - (size_t)at, "</tab>");
    }
    at += snprintf(m + at, sizeof m - (size_t)at, "</tabs>");
    if (at >= (int)sizeof m - 1) {
        snprintf(err, (size_t)errlen, "the settings panel is too long");
        return -1;
    }
    if (!olui_add_css(ui, css) || !olui_add(ui, olui_find(ui, parent_id), m, err, (size_t)errlen)) return -1;
    for (i = 0; i < gs_settings_count(); i++) {
        const gs_setting *g = gs_setting_at(i);
        for (k = 0; g->type == GS_CHOICE && g->choices[k]; k++)
            olui_select_add(ui, field(ui, g), g->choices[k], g->choices[k]);
    }
    return 0;
}

void omv_settings_show(olui *ui, const gs_settings *s)
{
    int i, d;
    for (i = 0; i < gs_settings_count(); i++) {
        const gs_setting *g = gs_setting_at(i);
        float v = gs_setting_get(s, i), scale = 1.f;
        if (g->type == GS_CHOICE) {
            olui_select_set(ui, field(ui, g), (int)v);
            continue;
        }
        for (d = places(g); d > 0; d--) scale *= 10.f;
        v *= scale;
        olui_set_number(ui, field(ui, g), (int)(v < 0.f ? v - 0.5f : v + 0.5f));
    }
}

void omv_settings_read(olui *ui, gs_settings *s)
{
    int i;
    char id[48];
    for (i = 0; i < gs_settings_count(); i++) {
        const gs_setting *g = gs_setting_at(i);
        if (g->type == GS_CHOICE) gs_setting_set(s, i, (float)olui_select_index(field(ui, g)));
        else {
            snprintf(id, sizeof id, "set-%s", g->key);
            gs_setting_set(s, i, omv_number(ui, id));
        }
    }
}

unsigned omv_settings_fit(olui *ui, const om_machine *m, gs_settings *s, char *why, int whylen)
{
    unsigned grey = gs_settings_greyed(m), held;
    int i, at = 0, any = 0;
    omv_settings_read(ui, s);
    held = gs_settings_fit(s, m);
    omv_settings_show(ui, s);
    for (i = 0; i < gs_settings_count(); i++) olui_set_disabled(ui, field(ui, gs_setting_at(i)), (grey >> i) & 1u);
    why[0] = 0;
    if (m->family != OM_PRINTER) {
        if (held) snprintf(why, (size_t)whylen, "%.40s is not a printer: nothing to print.", m->name);
        return held;
    }
    for (i = 0; i < gs_settings_count() && at < whylen; i++) {
        if (!((held >> i) & 1u) || ((grey >> i) & 1u)) continue;
        at += snprintf(why + at, (size_t)(whylen - at), "%s%s %g", any ? ", " : "", gs_setting_at(i)->label,
                       (double)gs_setting_get(s, i));
        any = 1;
    }
    if (any && at < whylen) { /* "Held to <name>: " in front */
        char list[128];
        snprintf(list, sizeof list, "%s", why);
        at = snprintf(why, (size_t)whylen, "Held to %.30s: %s.", m->name, list);
    }
    if ((held & grey) && at < whylen)
        snprintf(why + at, (size_t)(whylen - at), "%sBed off: %.30s has no heated bed.", any ? " " : "", m->name);
    return held;
}
