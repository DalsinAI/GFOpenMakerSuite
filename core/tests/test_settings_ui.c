/* The print settings panel both windows share (omview_settings.h), laid
 * out on the host by the real GFOpenLayout in a side column as narrow as
 * the windows give it, in a window of 640 by 480: each tab's fields in the
 * column and clear of each other, the settings shown and read back, and
 * held to a machine. MIT, Copyright (c) 2026 AmiCradle Team. */
#include "omview_settings.h"

#include <stdio.h>
#include <string.h>

static int fails;

static void expect(int ok, const char *what)
{
    if (!ok) {
        printf("fail: %s\n", what);
        fails++;
    }
}

static int measure(void *userdata, const char *utf8, size_t length, const ol_style *style, ol_text_metrics *metrics)
{
    (void)userdata;
    (void)utf8;
    metrics->width = (ol_unit)(length * (size_t)style->font_size * 60 / 100);
    metrics->ascent = style->font_size * 80 / 100;
    metrics->descent = style->font_size * 22 / 100;
    return 1;
}

static int within(const olui_rect *r, const olui_rect *in)
{
    return r->w > 0 && r->h > 0 && r->x >= in->x && r->y >= in->y && r->x + r->w <= in->x + in->w &&
           r->y + r->h <= in->y + in->h;
}

static int overlap(const olui_rect *a, const olui_rect *b)
{
    return a->x < b->x + b->w && b->x < a->x + a->w && a->y < b->y + b->h && b->y < a->y + a->h;
}

static const char markup[] = "<row id=\"body\"><column id=\"side\"><fieldset><legend>Settings</legend>"
                             "<div id=\"settings\"></div></fieldset></column><div id=\"view\"></div></row>"
                             "<div id=\"status\">-</div>";
static const char css[] = "window{font-size:12px;padding:8px;gap:6px}#body{flex:1;gap:8px}"
                          "#side{width:" OMV_SETTINGS_W_CSS "}fieldset{padding:0 8px 6px}#view{flex:1}";

int main(void)
{
    olui *ui = olui_new();
    om_machine m;
    gs_settings s, t, d;
    olui_rect side, r[32], view;
    char err[160], why[128];
    int i, j, k, n = gs_settings_count(), tabs, sizes;
    unsigned held;
    olui_set_measure(ui, measure, NULL);
    expect(olui_load(ui, markup, css, err, sizeof err), "a window with a side column loads");
    expect(omv_settings_add(ui, "settings", err, (int)sizeof err) == 0, "the panel added to it");
    for (i = 0; i < n; i++) {
        char id[48], what[96];
        const gs_setting *g = gs_setting_at(i);
        olui_el *el;
        snprintf(id, sizeof id, "set-%s", g->key);
        el = olui_find(ui, id);
        snprintf(what, sizeof what, "%s: a %s", id, g->type == GS_CHOICE ? "select" : "spinner");
        expect(el && !strcmp(olui_tag(el), g->type == GS_CHOICE ? "select" : "spinner"), what);
    }
    tabs = omv_settings_tabs();
    expect(tabs >= 3 && tabs <= 5, "three to five tabs");

    /* Each tab at the least and a big window: its fields in the column,
     * clear of each other, and the view keeps the most of the width. */
    for (sizes = 0; sizes < 2; sizes++) {
        int w = sizes ? 1024 : 640, h = sizes ? 768 : 480;
        for (k = 0; k < tabs; k++) {
            int shown = 0;
            omv_settings_tab(ui, k);
            expect(olui_layout(ui, w, h), "laid out");
            expect(olui_box(ui, olui_find(ui, "side"), &side) && side.y + side.h <= h, "the side column");
            for (i = 0; i < n; i++) {
                char id[48], what[96];
                snprintf(id, sizeof id, "set-%s", gs_setting_at(i)->key);
                if (omv_settings_tab_of(i) != k) continue;
                shown++;
                snprintf(what, sizeof what, "%dx%d: %s in the side column", w, h, id);
                expect(olui_box(ui, olui_find(ui, id), &r[i]) && within(&r[i], &side), what);
                for (j = 0; j < i; j++)
                    if (omv_settings_tab_of(j) == k) {
                        snprintf(what, sizeof what, "%s clear of set-%s", id, gs_setting_at(j)->key);
                        expect(!overlap(&r[i], &r[j]), what);
                    }
            }
            expect(shown >= 3 && shown <= 6, "three to six on a tab");
        }
        expect(olui_box(ui, olui_find(ui, "view"), &view) && view.w >= w - OMV_SETTINGS_W - 40, "the view has the rest");
    }

    /* Shown and read back the same: the defaults, and others. */
    gs_settings_default(&d);
    omv_settings_show(ui, &d);
    omv_settings_read(ui, &s);
    expect(!memcmp(&s, &d, sizeof s), "the defaults shown and read back");
    t = d;
    t.pattern = GS_GRID;
    t.around = GS_BRIM;
    t.top = 6;
    t.layer_h = 0.16f;
    t.hotend = 215.f;
    t.filament = 2.85f;
    t.distance = 1.5f;
    omv_settings_show(ui, &t);
    omv_settings_read(ui, &s);
    expect(!memcmp(&s, &t, sizeof s), "a grid, a brim, 6 on top, 0.16 mm, 215 C, 2.85 mm, 1.5 mm out");

    /* Held to the machine, the fields too, and said in a line. */
    om_clear(&m);
    expect(om_load(&m, "machines/printer/marlin/driver.om", err, (int)sizeof err) == 0 &&
               om_load(&m, "machines/printer/marlin/profiles/generic-220.om", err, (int)sizeof err) == 0,
           "the generic Marlin printer");
    t.hotend = 300.f;
    t.speed = 400.f;
    omv_settings_show(ui, &t);
    held = omv_settings_fit(ui, &m, &s, why, (int)sizeof why);
    expect(held == (GS_BIT("hotend") | GS_BIT("speed")) && s.hotend == 260.f && s.speed == 300.f, "held to its most");
    omv_settings_read(ui, &t);
    expect(t.hotend == 260.f && t.speed == 300.f, "the fields show it");
    expect(!strcmp(why, "Held to Generic Marlin 220: Print speed 300, Hotend 260."), "and the line says so");
    printf("%s\n", why);
    expect(omv_settings_fit(ui, &m, &s, why, (int)sizeof why) == 0 && !why[0], "nothing more to hold: nothing said");
    m.features &= ~OM_HEATED_BED;
    held = omv_settings_fit(ui, &m, &s, why, (int)sizeof why);
    expect(held == GS_BIT("bed") && s.bed == 0.f && strstr(why, "Bed off: Generic Marlin 220 has no heated bed"),
           "no heated bed: the bed off, and why");
    printf("%s\n", why);
    expect(strlen(why) <= 78, "it fits the status line");

    olui_free(ui);
    if (fails) {
        printf("%d failed\n", fails);
        return 1;
    }
    printf("settings ui ok\n");
    return 0;
}
