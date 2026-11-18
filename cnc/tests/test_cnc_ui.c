/* OpenMakeCNC's window at 640 by 480, as amiga.c opens it.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "cnc_ui.h"
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

static const char *const controls[][2] = {
    {"kind", "label"}, {"path", "input"}, {"open", "button"}, {"preview", "button"}, {"machine", "select"},
    {"profile", "button"}, {"pocket", "button"}, {"drill", "button"}, {"tool", "spinner"}, {"depth", "spinner"},
    {"stepdown", "spinner"}, {"stepover", "spinner"}, {"feed", "spinner"}, {"plunge", "spinner"}, {"safe", "spinner"},
    {"rpm", "spinner"}, {"gcode", "button"}, {"x", "spinner"}, {"y", "spinner"},
};
#define N ((int)(sizeof controls / sizeof controls[0]))

int main(void)
{
    olui *ui = olui_new();
    char err[160], line[96];
    olui_rect box[N], view, status, side;
    int w = 640, h = 480, i, j;
    cnc_opts o;
    omv_machines mills;

    olui_set_measure(ui, measure, NULL);
    expect(olui_load(ui, cnc_markup, cnc_css, err, sizeof err), "the window loads");
    expect(olui_layout(ui, w, h), "640 by 480, as the window opens");
    cnc_ui_mode(ui, CNC_PROFILE);
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
    expect(olui_box(ui, olui_find(ui, "view"), &view) && inside(&view, w, h) && view.w > 200, "the view takes the rest");
    expect(olui_box(ui, olui_find(ui, "side"), &side) && side.w == 236 && side.x + side.w <= view.x, "the side is 236, left of the view");
    expect(olui_box(ui, olui_find(ui, "status"), &status) && inside(&status, w, h) && status.y >= view.y + view.h &&
               status.w >= 78 * 8,
           "the status line is under the view and holds 78 characters");
    expect(olui_is_selected(olui_find(ui, "profile")) && !olui_is_selected(olui_find(ui, "pocket")), "Profile starts lit");
    cnc_ui_read(ui, &o);
    expect(o.mode == CNC_PROFILE && o.tool > 2.9f && o.tool < 3.1f && o.depth > 0.9f && o.depth < 1.1f &&
               o.rpm > 9999.f && o.rpm < 10001.f,
           "the fields are a 3 mm tool, 1 mm deep, 10000 rpm");
    cnc_ui_mode(ui, CNC_POCKET);
    cnc_ui_read(ui, &o);
    expect(o.mode == CNC_POCKET && olui_is_selected(olui_find(ui, "pocket")), "Pocket lights, and is the cut");
    cnc_ui_kind(ui, "work/square.svg");
    expect(!strcmp(olui_get_text(olui_find(ui, "kind")), "SVG"), "an svg is labelled SVG");
    cnc_ui_kind(ui, "work/square.dxf");
    expect(!strcmp(olui_get_text(olui_find(ui, "kind")), "DXF"), "a dxf is labelled DXF");
    cnc_ui_status(line, (int)sizeof line, "Generic Grbl Mill 300", CNC_PROFILE, 7, 7.f);
    expect(!strcmp(line, "Generic Grbl Mill 300, profile. 7 moves, 7 s.") && strlen(line) <= 78, "the status line");
    memset(&mills, 0, sizeof mills);
    expect(omv_machines_scan(&mills, "machines", "cnc") == 1 && !strcmp(mills.at[0].name, "Generic Grbl Mill 300"),
           "the mill picker finds the generic mill");
    omv_machines_free(&mills);
    olui_free(ui);
    if (fails) {
        printf("%d failed\n", fails);
        return 1;
    }
    printf("cnc ui ok\n");
    return 0;
}
