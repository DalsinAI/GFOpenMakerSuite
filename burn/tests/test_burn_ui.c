/* OpenMakeBurn's window (burn/src/markup.h), laid out on the host by
 * the real GFOpenLayout at the size amiga.c opens it: 640 by 480 inside,
 * which with the title bar and sizing border fits a 640 by 512 PAL screen.
 * Its status line and numbers go through core/ui (omview_ui.h); its layer
 * list is burn_ui.h.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "burn_ui.h"
#include "markup.h"
#include "omview_machines.h"
#include "omview_ui.h"

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

static int inside(const olui_rect *r, int w, int h)
{
    return r->w > 0 && r->h > 0 && r->x >= 0 && r->y >= 0 && r->x + r->w <= w && r->y + r->h <= h;
}

static int overlap(const olui_rect *a, const olui_rect *b)
{
    return a->x < b->x + b->w && b->x < a->x + a->w && a->y < b->y + b->h && b->y < a->y + a->h;
}

/* Every control amiga.c reads or answers, and what it is. */
static const char *const controls[][2] = {
    {"path", "input"},   {"open", "button"}, {"preview", "button"}, {"machine", "select"},
    {"constant", "checkbox"}, {"frame", "button"}, {"gcode", "button"},
};
#define N ((int)(sizeof controls / sizeof controls[0]))

int main(void)
{
    olui *ui = olui_new();
    char err[160];
    olui_rect box[N], view, status, side;
    int w = 640, h = 480, i, j;
    omv_machines lasers;
    burn_opts o;
    burn_layer store[8];
    gv_doc d;
    char line[160];

    olui_set_measure(ui, measure, NULL);
    expect(olui_load(ui, burn_markup, burn_css, err, sizeof err), "the window loads");
    expect(olui_layout(ui, w, h), "the window lays out at 640 by 480, as amiga.c opens it");
    for (i = 0; i < N; i++) {
        olui_el *el = olui_find(ui, controls[i][0]);
        char what[80];
        snprintf(what, sizeof what, "%s is a %s in the window", controls[i][0], controls[i][1]);
        expect(el && !strcmp(olui_tag(el), controls[i][1]) && olui_box(ui, el, &box[i]) && inside(&box[i], w, h), what);
        for (j = 0; el && j < i; j++) {
            snprintf(what, sizeof what, "%s is clear of %s", controls[i][0], controls[j][0]);
            expect(!overlap(&box[i], &box[j]), what);
        }
    }
    expect(olui_find(ui, "ver") == NULL, "the name and version are in the title bar, not a row");
    expect(olui_box(ui, olui_find(ui, "view"), &view) && inside(&view, w, h) && view.h >= h / 2,
           "the view has at least half the height");
    expect(olui_box(ui, olui_find(ui, "status"), &status) && inside(&status, w, h) && status.y >= view.y + view.h,
           "the status line is under the view");
    expect(status.y + status.h >= h - 40 && status.w >= 78 * 8, "the status line along the bottom, 78 characters wide");
    expect(olui_box(ui, olui_find(ui, "side"), &side) && side.w >= 220 && side.w <= 250,
           "the side column holds the picker and the layers");

    memset(&lasers, 0, sizeof lasers);
    expect(omv_machines_scan(&lasers, "machines", "laser") > 0, "the laser profiles");
    expect(omv_machines_fill(ui, "machine", &lasers, "No lasers") > 0, "the picker lists them");
    expect(strstr(lasers.at[omv_machines_chosen(ui, "machine", &lasers)].name, "Generic Diode 400") != NULL,
           "Generic Diode 400 is first");

    gv_doc_init(&d);
    burn_defaults(&o);
    expect(gv_load_svg(&d, "burn/tests/data/two-rects.svg", err, (int)sizeof err) == 0, "two rectangles");
    expect(burn_ui_layers(ui, &d, &o, err, (int)sizeof err) == 0 && burn_ui_count(ui) == 2, "two layer rows");
    expect(olui_layout(ui, w, h), "it still lays out at 640 by 480 with the layers");
    {
        olui_rect r, p0, p1;
        expect(olui_box(ui, olui_find(ui, "layer-0"), &r) && inside(&r, w, h) &&
                   olui_box(ui, olui_find(ui, "layer-1"), &p1) && inside(&p1, w, h) && !overlap(&r, &p1),
               "both layer rows are in the window and clear of each other");
        expect(olui_box(ui, olui_find(ui, "layer-0-power"), &p0) && inside(&p0, w, h), "power is in the first row");
        expect(omv_number(ui, "layer-0-power") == 50.f && omv_number(ui, "layer-0-speed") == 20.f &&
                   omv_number(ui, "layer-0-passes") == 1.f && olui_get_checked(olui_find(ui, "layer-0-on")),
               "defaults in the list: 50%, 20 mm/s, one pass, on");
        expect(!strcmp(olui_get_text(olui_find(ui, "layer-0-name")), "#ff0000") &&
                   !strcmp(olui_get_text(olui_find(ui, "layer-1-name")), "#0000ff"),
               "red then blue");
    }
    olui_set_number(ui, olui_find(ui, "layer-0-power"), 80);
    olui_set_number(ui, olui_find(ui, "layer-1-speed"), 150); /* 15.0 with one decimal */
    olui_set_checked(ui, olui_find(ui, "constant"), 1);
    burn_ui_read(ui, &o, store, 8);
    expect(o.nlayer == 2 && o.constant == 1 && store[0].power == 80.f && store[1].speed == 15.f &&
               store[0].colour == 0xff0000u && store[1].index == 2,
           "the list is read back, including M3");

    /* Sized up, as on an RTG screen: the view takes the room. */
    {
        olui_rect view1, st1, r;
        int big_w = 1024, big_h = 768, ok = 1;
        expect(olui_layout(ui, big_w, big_h), "the window lays out at 1024 by 768");
        for (i = 0; i < N; i++) ok &= olui_box(ui, olui_find(ui, controls[i][0]), &r) && inside(&r, big_w, big_h);
        expect(ok, "at 1024 by 768 every control is in the window");
        expect(olui_box(ui, olui_find(ui, "view"), &view1) && view1.w >= view.w + 384 - 8 && view1.h >= view.h + 288 - 8,
               "the view grows with the window");
        expect(olui_box(ui, olui_find(ui, "status"), &st1) && st1.y + st1.h >= big_h - 40 && st1.w >= big_w - 16,
               "the status line along the bottom, the window's width");
        expect(olui_box(ui, olui_find(ui, "layer-0"), &r) && inside(&r, big_w, big_h),
               "the layer list stays in the bigger window");
    }

    burn_ui_status(line, (int)sizeof line, "Generic Diode 400", 5, 2.0f);
    expect(!strcmp(line, "5 moves for Generic Diode 400, about 2.0 s"), "the status line's wording");
    omv_say(ui, line);
    expect(!strcmp(olui_get_text(olui_find(ui, "status")), line), "omv_say says it on the status line");

    omv_machines_free(&lasers);
    gv_doc_free(&d);
    olui_free(ui);
    printf(fails ? "burn ui: %d failed\n" : "burn ui ok\n", fails);
    return fails != 0;
}
