/* A print's settings: defaults, the text profile read and written, each
 * setting by its key, held to what the machine can do, and handed to the
 * planner and the G-code. MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gs_settings.h"

#include <stdio.h>
#include <string.h>

static int fails;

static void expect(int cond, const char *msg)
{
    if (!cond) {
        fprintf(stderr, "fail: %s\n", msg);
        fails++;
    }
}

int main(void)
{
    gs_settings s, t;
    om_machine m;
    gp_opts po;
    gc_opts go;
    char err[128], text[2048];
    int i, n, bed;
    unsigned grey, held;

    gs_settings_default(&s);
    expect(s.walls == 2 && s.infill == 20.f && s.pattern == GS_LINES && s.top == 4 && s.bottom == 3 &&
               s.around == GS_SKIRT && s.loops == 2 && s.filament == 1.75f && s.hotend == 200.f && s.bed == 60.f,
           "the defaults");

    /* Every setting has a key, a label for a window, and a range its
     * default is in; a key finds it. */
    n = gs_settings_count();
    expect(n >= 18 && n <= 32, "18 to 32 settings, a bit each");
    for (i = 0; i < n; i++) {
        const gs_setting *d = gs_setting_at(i);
        float v = gs_setting_get(&s, i);
        expect(d->key && d->label && d->min <= v && v <= d->max && gs_setting_find(d->key) == i,
               "a key, a label, its default in range, and found by its key");
    }
    expect(gs_setting_find("no-such") == -1 && gs_setting_at(n) == NULL, "and nothing else");
    bed = gs_setting_find("bed");
    gs_setting_set(&s, gs_setting_find("walls"), 99.f);
    gs_setting_set(&s, gs_setting_find("infill"), 37.5f);
    expect(s.walls == 8 && s.infill == 37.5f, "set by number: held to its range");
    gs_setting_set(&s, gs_setting_find("pattern"), (float)GS_GRID);
    expect(s.pattern == GS_GRID && !strcmp(gs_setting_choice(gs_setting_find("pattern"), GS_GRID), "grid"),
           "a choice, by number and by word");

    /* The text profile: written, read back the same. */
    expect(gs_settings_format(&s, text, (int)sizeof text) > 0 && strstr(text, "pattern = grid\n") &&
               strstr(text, "infill = 37.5\n") && strstr(text, "around = skirt\n"),
           "written as key = value lines, choices as words");
    gs_settings_default(&t);
    expect(gs_settings_parse(&t, text, err, (int)sizeof err) == 0 && !memcmp(&s, &t, sizeof s), "and read back the same");
    expect(gs_settings_format(&s, text, 40) == -1, "too little room: -1");

    /* Read over what is there: comments, spaces, only some keys. */
    gs_settings_default(&t);
    expect(gs_settings_parse(&t, "# fine\n\n  walls=3\ninfill = 15 # sparse\naround = brim\r\n", err, (int)sizeof err) == 0 &&
               t.walls == 3 && t.infill == 15.f && t.around == GS_BRIM && t.top == 4,
           "comments, blank lines, CR LF, and the rest kept");
    expect(gs_settings_parse(&t, "walls = 2\nflavour = mint\n", err, (int)sizeof err) == -1 && strstr(err, "line 2") &&
               strstr(err, "flavour"),
           "an unknown key: which line, and what");
    expect(gs_settings_parse(&t, "pattern = honeycomb\n", err, (int)sizeof err) == -1 && strstr(err, "honeycomb"),
           "an unknown choice");
    expect(gs_settings_parse(&t, "infill = lots\n", err, (int)sizeof err) == -1 && strstr(err, "line 1"), "not a number");
    expect(gs_settings_parse(&t, "infill = 250\n", err, (int)sizeof err) == 0 && t.infill == 100.f, "held to its range");

    /* Saved and loaded as a file. */
    expect(gs_settings_save(&s, "build/test-settings.txt", err, (int)sizeof err) == 0, "saved");
    gs_settings_default(&t);
    expect(gs_settings_load(&t, "build/test-settings.txt", err, (int)sizeof err) == 0 && !memcmp(&s, &t, sizeof s),
           "and loaded the same");
    expect(gs_settings_load(&t, "build/no-such-settings.txt", err, (int)sizeof err) == -1 && strstr(err, "cannot open"),
           "a missing file");

    /* The machine: what it cannot do is greyed, what is too much held. */
    om_clear(&m);
    expect(om_load(&m, "machines/printer/marlin/driver.om", err, (int)sizeof err) == 0 &&
               om_load(&m, "machines/printer/marlin/profiles/generic-220.om", err, (int)sizeof err) == 0,
           "the generic Marlin printer");
    gs_settings_default(&s);
    expect(gs_settings_greyed(&m) == 0u && gs_settings_fit(&s, &m) == 0u, "it does all of the defaults");
    s.hotend = 300.f;
    s.speed = 400.f;
    s.retract_speed = 60.f;
    held = gs_settings_fit(&s, &m);
    expect(s.hotend == 260.f && s.speed == 300.f && s.retract_speed == 25.f, "held to its hotend, X and E");
    expect(held == (GS_BIT("hotend") | GS_BIT("speed") | GS_BIT("retract-speed")), "and which were held");
    expect(gs_setting_most(&m, gs_setting_find("hotend")) == 260.f &&
               gs_setting_most(&m, gs_setting_find("walls")) == gs_setting_at(gs_setting_find("walls"))->max,
           "the most a window offers: the machine's, or the setting's own");
    m.features &= ~OM_HEATED_BED;
    grey = gs_settings_greyed(&m);
    expect(grey == (1u << bed) && gs_settings_fit(&s, &m) == (1u << bed) && s.bed == 0.f, "no heated bed: greyed, and 0");
    m.family = OM_LASER;
    expect(gs_settings_greyed(&m) == (1u << n) - 1u, "a laser: none of a print's settings");

    /* To the planner and the G-code. */
    gs_settings_default(&s);
    s.top = 5;
    s.around = GS_BRIM;
    s.loops = 4;
    gs_settings_opts(&s, &po, &go);
    expect(po.layer_h == s.layer_h && po.line_w == s.line_w && po.walls == 2 && po.infill == 20.f && po.skins == 5 &&
               po.brim == 4,
           "the planner's: the solid layers the more of top and bottom, a brim its loops");
    expect(go.hotend == 200.f && go.bed == 60.f && go.filament == 1.75f && go.layer_h == s.layer_h && go.flow == 1.f &&
               go.speed[GT_INFILL] == s.speed && go.speed[GT_OUTER] == s.speed * 0.5f &&
               go.speed[GT_TRAVEL] == s.travel && go.speed[GT_BRIM] == s.first && go.retract == s.retract,
           "the G-code's: outer walls at half the speed, the brim at the first layer's");
    s.around = GS_SKIRT;
    gs_settings_opts(&s, &po, &go);
    expect(po.brim == 0, "a skirt is no brim");
    {
        gp_more x;
        s.pattern = GS_GRID;
        s.top = 6;
        s.bottom = 2;
        gs_settings_more(&s, &x);
        expect(x.size == (int)sizeof x && x.pattern == GP_GRID && x.top == 6 && x.bottom == 2 && x.skirt == s.loops &&
                   x.gap == 3.f,
               "the planner's more: a grid, top and bottom apart, a skirt 3 mm out");
        s.around = GS_BRIM;
        gs_settings_more(&s, &x);
        expect(x.skirt == 0 && x.gap == 3.f, "a brim is no skirt, at its distance");
        s.around = GS_NONE;
        gs_settings_more(&s, &x);
        expect(x.skirt == 0 && x.gap == 0.f, "neither");
    }
    {
        gc_more y;
        gs_settings_gc_more(&s, &go, &y);
        expect(y.size == (int)sizeof y && y.o == &go && y.first == s.first, "the G-code's more: the first layer's speed");
    }

    if (fails) {
        printf("%d failed\n", fails);
        return 1;
    }
    printf("settings ok\n");
    return 0;
}
