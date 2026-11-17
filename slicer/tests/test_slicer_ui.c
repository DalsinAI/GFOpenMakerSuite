/* GFOpenSlicer's window (slicer/src/markup.h), laid out on the host by
 * the real GFOpenLayout at the size amiga.c opens it: 640 by 480 inside,
 * which with the title bar and sizing border fits a 640 by 512 PAL screen.
 * Its status line and numbers go through core/ui (omview_ui.h); its
 * settings become the toolpath engine's options (slicer_ui.h).
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "markup.h"
#include "omview_ui.h"
#include "slicer_ui.h"

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
    {"path", "input"}, {"open", "button"}, {"slice", "button"}, {"layer", "spinner"}, {"infill", "spinner"},
    {"line", "spinner"}, {"walls", "spinner"}, {"hotend", "spinner"}, {"bed", "spinner"}, {"brim", "spinner"},
    {"machine", "select"}, {"show", "checkbox"}, {"prev", "button"}, {"layerno", "slider"}, {"next", "button"},
    {"gcode", "button"}
};
#define N ((int)(sizeof controls / sizeof controls[0]))

int main(void)
{
    olui *ui = olui_new();
    char err[160];
    olui_rect box[N], view, status;
    int w = 640, h = 480, i, j;
    olui_set_measure(ui, measure, NULL);
    expect(olui_load(ui, gs_markup, gs_css, err, sizeof err), "the window loads");
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
    }
    omv_say(ui, "Sliced: 100 layers");
    expect(!strcmp(olui_get_text(olui_find(ui, "status")), "Sliced: 100 layers"), "omv_say says it on the status line");
    expect(olui_find(ui, "layers") == NULL && olui_find(ui, "skirt") == NULL, "no layer list or skirt: a slider, a brim");
    {
        /* The settings as the engine's options: its defaults, then the
         * window's numbers over them. */
        gp_opts po, pd;
        gc_opts go, gd;
        gp_defaults(&pd);
        gc_defaults(&gd);
        sl_ui_opts(ui, &po, &go);
        expect(po.layer_h == 0.2f && po.infill == 20.f && po.line_w == 0.4f && po.walls == 2 && po.brim == 3 &&
                   po.skins == pd.skins,
               "gp_opts: 0.20 mm layers, 20 % infill, 0.40 mm lines, 2 walls, 3 brim loops, the skins as ever");
        expect(go.hotend == 200.f && go.bed == 60.f && go.layer_h == po.layer_h && go.filament == gd.filament,
               "gc_opts: 200 and 60 degrees, the layers' height, the filament as ever");
        olui_set_number(ui, olui_find(ui, "layer"), 30);
        olui_set_number(ui, olui_find(ui, "walls"), 4);
        sl_ui_opts(ui, &po, &go);
        expect(po.layer_h == 0.3f && go.layer_h == 0.3f && po.walls == 4, "and as they are changed");
    }
    olui_free(ui);
    printf(fails ? "slicer ui: %d failed\n" : "slicer ui ok\n", fails);
    return fails != 0;
}
