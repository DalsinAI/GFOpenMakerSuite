/* OpenMakeBurn's window (burn/src/markup.h), laid out on the host by
 * the real GFOpenLayout at the size amiga.c opens it: 640 by 480 inside,
 * which with the title bar and sizing border fits a 640 by 512 PAL screen.
 * Its status line and numbers go through core/ui (omview_ui.h); its layer
 * list is burn_ui.h.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "burn_ui.h"
#include "burn_material.h"
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
    {"kind", "label"}, {"path", "input"},   {"open", "button"}, {"preview", "button"}, {"machine", "select"},
    {"constant", "checkbox"}, {"cuts_last", "checkbox"}, {"material", "select"}, {"frame", "button"}, {"gcode", "button"},
    {"place-x", "spinner"}, {"place-y", "spinner"}, {"place-rot", "spinner"},
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
    expect(!strcmp(olui_get_text(olui_find(ui, "kind")), "File"), "the Drawing label starts as File");
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
    expect(omv_number(ui, "place-x") == 0.f && omv_number(ui, "place-y") == 0.f &&
               omv_number(ui, "place-rot") == 0.f,
           "the drawing starts unmoved, unrotated");

    memset(&lasers, 0, sizeof lasers);
    expect(omv_machines_scan(&lasers, "machines", "laser") > 0, "the laser profiles");
    expect(omv_machines_fill(ui, "machine", &lasers, "No lasers") > 0, "the picker lists them");
    expect(strstr(lasers.at[omv_machines_chosen(ui, "machine", &lasers)].name, "Generic Diode 400") != NULL,
           "Generic Diode 400 is first");

    {
        burn_materials stocks;
        burn_materials_init(&stocks);
        expect(burn_materials_scan(&stocks, "materials/laser") == 0 && stocks.n == 3, "the materials library");
        expect(burn_ui_materials_fill(ui, "material", &stocks) == 3, "the picker lists them");
        expect(burn_ui_materials_chosen(ui, "material", &stocks) < 0, "None is first");
        burn_materials_free(&stocks);
    }

    gv_doc_init(&d);
    burn_defaults(&o);
    expect(gv_load_svg(&d, "burn/tests/data/two-rects.svg", err, (int)sizeof err) == 0, "two rectangles");
    expect(burn_ui_layers(ui, &d, &o, err, (int)sizeof err) == 0 && burn_ui_count(ui) == 2, "two layer rows");
    burn_ui_kind(ui, "burn/tests/data/two-rects.svg");
    expect(!strcmp(olui_get_text(olui_find(ui, "kind")), "SVG") && olui_layout(ui, w, h),
           "an SVG names the Drawing field SVG");
    {
        olui_rect kr, pr;
        expect(olui_box(ui, olui_find(ui, "kind"), &kr) && olui_box(ui, olui_find(ui, "path"), &pr) &&
                   inside(&kr, w, h) && !overlap(&kr, &pr),
               "the kind label stays in the window, clear of the path");
    }
    burn_ui_kind(ui, "burn/tests/data/two-rects.dxf");
    expect(!strcmp(olui_get_text(olui_find(ui, "kind")), "DXF") && olui_layout(ui, w, h),
           "a DXF names the Drawing field DXF");
    burn_ui_kind(ui, "burn/tests/data/bars-8x2.pgm");
    expect(!strcmp(olui_get_text(olui_find(ui, "kind")), "Picture") && olui_layout(ui, w, h),
           "a picture names the Drawing field Picture");
    {
        olui_rect kr, pr;
        expect(olui_box(ui, olui_find(ui, "kind"), &kr) && olui_box(ui, olui_find(ui, "path"), &pr) &&
                   inside(&kr, w, h) && kr.w > 0 && !overlap(&kr, &pr),
               "Picture still fits in the Drawing row");
    }
    burn_ui_kind(ui, "burn/tests/data/two-rects.svg");
    expect(olui_layout(ui, w, h), "it still lays out at 640 by 480 with the layers");
    {
        olui_rect r, p0, p1;
        expect(olui_box(ui, olui_find(ui, "layer-0"), &r) && inside(&r, w, h) &&
                   olui_box(ui, olui_find(ui, "layer-1"), &p1) && inside(&p1, w, h) && !overlap(&r, &p1),
               "both layer rows are in the window and clear of each other");
        expect(olui_box(ui, olui_find(ui, "layer-0-power"), &p0) && inside(&p0, w, h) &&
                   p0.x + p0.w <= side.x + side.w && r.x + r.w <= side.x + side.w,
               "power is in the first row, and the list stays in the side column");
        {
            olui_rect boxset, pass;
            expect(olui_box(ui, olui_find(ui, "settingsbox"), &boxset) &&
                       olui_box(ui, olui_find(ui, "layer-0-passes"), &pass) &&
                       pass.x >= boxset.x && pass.x + pass.w <= boxset.x + boxset.w,
                   "the pass spinner stays inside the settings box");
            {
                olui_rect kerf;
                expect(olui_box(ui, olui_find(ui, "layer-0-kerf"), &kerf) && inside(&kerf, w, h) &&
                           kerf.x >= boxset.x && kerf.x + kerf.w <= boxset.x + boxset.w,
                       "the kerf spinner stays inside the settings box");
                {
                    olui_rect tabs, tabw;
                    expect(olui_box(ui, olui_find(ui, "layer-0-tabs"), &tabs) && inside(&tabs, w, h) &&
                               tabs.x >= boxset.x && tabs.x + tabs.w <= boxset.x + boxset.w,
                           "the tabs spinner stays inside the settings box");
                    expect(olui_box(ui, olui_find(ui, "layer-0-tabw"), &tabw) && inside(&tabw, w, h) &&
                               tabw.x >= boxset.x && tabw.x + tabw.w <= boxset.x + boxset.w,
                           "the tab width spinner stays inside the settings box");
                }
            }
        }
        expect(omv_number(ui, "layer-0-power") == 50.f && omv_number(ui, "layer-0-speed") == 20.f &&
                   omv_number(ui, "layer-0-passes") == 1.f && olui_get_checked(olui_find(ui, "layer-0-on")) &&
                   olui_select_index(olui_find(ui, "layer-0-mode")) == 0 && omv_number(ui, "layer-0-interval") == 0.2f &&
                   omv_number(ui, "layer-0-kerf") == 0.f && omv_number(ui, "layer-0-tabs") == 0.f &&
                   omv_number(ui, "layer-0-tabw") == 1.f,
               "defaults in the list: 50%, 20 mm/s, one pass, on, line, 0.2 mm gap, no kerf, no tabs, 1 mm tab width");
        expect(!strcmp(olui_get_text(olui_find(ui, "layer-0-name")), "#ff0000") &&
                   !strcmp(olui_get_text(olui_find(ui, "layer-1-name")), "#0000ff"),
               "red then blue");
    }
    olui_set_number(ui, olui_find(ui, "layer-0-power"), 80);
    olui_set_number(ui, olui_find(ui, "layer-1-speed"), 150); /* 15.0 with one decimal */
    olui_set_checked(ui, olui_find(ui, "constant"), 1);
    olui_select_set(ui, olui_find(ui, "layer-0-mode"), 1);
    olui_set_number(ui, olui_find(ui, "layer-0-interval"), 10); /* 0.10 with two decimals */
    olui_set_number(ui, olui_find(ui, "layer-1-kerf"), 20); /* 0.20 with two decimals */
    olui_set_number(ui, olui_find(ui, "layer-1-tabs"), 4);
    olui_set_number(ui, olui_find(ui, "layer-1-tabw"), 150); /* 1.50 with two decimals */
    burn_ui_read(ui, &o, store, 8);
    expect(o.nlayer == 2 && o.constant == 1 && store[0].power == 80.f && store[1].speed == 15.f &&
               store[0].colour == 0xff0000u && store[1].index == 2 && store[0].mode == BURN_FILL &&
               store[0].interval == 0.1f && store[1].mode == BURN_LINE && store[1].kerf == 0.2f &&
               (store[1].set & BURN_SET_KERF) && store[1].tabs == 4 && store[1].tabw == 1.5f &&
               (store[1].set & BURN_SET_TABS) && (store[1].set & BURN_SET_TABW) && o.power == 80.f &&
               o.speed == 20.f && o.mode == BURN_FILL && o.interval == 0.1f,
           "the list is read back, including M3, fill, kerf and tabs");
    olui_set_number(ui, olui_find(ui, "layer-0-kerf"), 20); /* 0.20 */
    olui_set_number(ui, olui_find(ui, "layer-0-tabs"), 4);
    olui_set_number(ui, olui_find(ui, "layer-0-tabw"), 150); /* 1.50 */
    burn_ui_read(ui, &o, store, 8);
    expect(o.kerf == 0.2f && o.tabs == 4 && o.tabw == 1.5f, "the first row's kerf and tabs become the job");
    olui_set_number(ui, olui_find(ui, "place-x"), 50);   /* 5.0 with one decimal */
    olui_set_number(ui, olui_find(ui, "place-y"), -25);  /* -2.5 */
    olui_set_number(ui, olui_find(ui, "place-rot"), 90);
    burn_ui_read(ui, &o, store, 8);
    expect(o.x == 5.f && o.y == -2.5f && o.rot == 90.f, "X, Y and Rot are read back");
    o.x = 1.f;
    o.y = 2.f;
    o.rot = 45.f;
    burn_ui_place(ui, &o);
    burn_ui_read(ui, &o, store, 8);
    expect(o.x == 1.f && o.y == 2.f && o.rot == 45.f, "the place fields take a port move");
    olui_select_set(ui, olui_find(ui, "layer-1-mode"), 2);
    burn_ui_read(ui, &o, store, 8);
    expect(store[1].mode == BURN_OFFSET, "Offset in the list is read as offset fill");

    expect(olui_get_checked(olui_find(ui, "cuts_last")), "Cuts last is on by default");
    olui_set_checked(ui, olui_find(ui, "cuts_last"), 0);
    burn_ui_read(ui, &o, store, 8);
    expect(o.cuts_last == 0, "unchecking Cuts last is read back");
    olui_set_checked(ui, olui_find(ui, "cuts_last"), 1);
    {
        int seq[4], n;
        olui_el *first;
        n = burn_ui_order(ui, seq, 4);
        expect(n == 2 && seq[0] == 0 && seq[1] == 1, "the list starts red then blue");
        expect(olui_find(ui, "layer-0-up") && olui_find(ui, "layer-1-down") &&
                   !strcmp(olui_tag(olui_find(ui, "layer-0-up")), "button"),
               "Up and Dn sit on each layer row");
        expect(burn_ui_move(ui, &d, &o, store, 8, seq, 4, 0, 1, err, (int)sizeof err) == 0, "move red down");
        expect(olui_layout(ui, w, h), "it still lays out after a move");
        n = burn_ui_order(ui, seq, 4);
        first = olui_first_child(olui_find(ui, "layers"));
        expect(n == 2 && seq[0] == 1 && seq[1] == 0 && first && !strcmp(olui_id(first), "layer-1"),
               "Dn puts blue first in the list");
        {
            olui_rect boxset, pass, up;
            expect(olui_box(ui, olui_find(ui, "settingsbox"), &boxset) &&
                       olui_box(ui, olui_find(ui, "layer-0-passes"), &pass) &&
                       olui_box(ui, olui_find(ui, "layer-0-up"), &up) &&
                       pass.x >= boxset.x && pass.x + pass.w <= boxset.x + boxset.w &&
                       up.x >= boxset.x && up.x + up.w <= boxset.x + boxset.w,
                   "Up and the pass spinner stay inside the settings box after a move");
        }
        /* Display order is the list, not the cuts-last burn sort. */
        o.cuts_last = 1;
        expect(burn_ui_layers(ui, &d, &o, err, (int)sizeof err) == 0, "rebuild with cuts last on");
        n = burn_ui_order(ui, seq, 4);
        expect(n == 2 && seq[0] == 1 && seq[1] == 0 && olui_get_checked(olui_find(ui, "cuts_last")),
               "the shown list keeps the user's order when Cuts last is on");
    }
    {
        burn_materials stocks;
        burn_materials_init(&stocks);
        expect(burn_materials_scan(&stocks, "materials/laser") == 0, "rescan the library");
        burn_ui_materials_fill(ui, "material", &stocks);
        /* 3 mm plywood is first by name, index 1 in the picker (0 is None). */
        olui_select_set(ui, olui_find(ui, "material"), 1);
        expect(burn_ui_materials_chosen(ui, "material", &stocks) == 0 &&
                   !strcmp(stocks.at[0].name, "3 mm plywood"),
               "the first library entry is 3 mm plywood");
        burn_ui_material(ui, &o, &stocks.at[0]);
        expect(olui_layout(ui, w, h), "it still lays out with a material applied");
        expect(o.power == 80.f && o.speed == 8.f && o.passes == 3 && o.mode == BURN_LINE &&
                   omv_number(ui, "layer-0-power") == 80.f && omv_number(ui, "layer-1-speed") == 8.f &&
                   omv_number(ui, "layer-0-passes") == 3.f,
               "a material fills the job and both layer rows");
        {
            olui_rect boxset, pass, mat;
            expect(olui_box(ui, olui_find(ui, "settingsbox"), &boxset) &&
                       olui_box(ui, olui_find(ui, "layer-0-passes"), &pass) &&
                       olui_box(ui, olui_find(ui, "material"), &mat) &&
                       pass.x >= boxset.x && pass.x + pass.w <= boxset.x + boxset.w &&
                       mat.x >= boxset.x && mat.x + mat.w <= boxset.x + boxset.w,
                   "the Material picker and the pass spinner stay inside the settings box");
        }
        burn_materials_free(&stocks);
    }

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
