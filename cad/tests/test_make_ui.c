/* GFOpenMake's window, laid out on the host by the real GFOpenLayout.
 * The text is measured at 60% of the font size, as tools/preview.c does;
 * the Amiga's fonts will differ a little.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "make_ui.h"

#include <gfopenlayout_ui.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int failed;

static void expect(int ok, const char *what)
{
    if (!ok) {
        printf("fail: %s\n", what);
        failed++;
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

/* Every id make.c hands to olui_on. */
static const char *const buttons[] = {
    "open", "save", "undo", "orbit", "move", "rot", "scale", "drop", "centre", "lay",
    "addcube", "addcyl", "join", "cut", "meet", "prev", "next", "slice", "gcode"
};

/* The Layers group's other controls, and the Slice group's machine
 * picker, and what they are. */
static const char *const controls[][2] = {
    {"lh", "spinner"}, {"show", "checkbox"}, {"layer", "slider"}, {"machine", "select"}
};
#define NCONTROLS ((int)(sizeof controls / sizeof controls[0]))
#define NBUTTONS ((int)(sizeof buttons / sizeof buttons[0]))

/* What the window's draw list says, collected by element. */
#define WINDOW_BG 0xfff9f9f9u
#define ACCENT 0xff0067c0u
static struct {
    uint32_t window_bg;     /* the first box, the whole window's */
    int groups;             /* fieldsets edged in Clear's #d0d0d0 */
    const olui_el *last_group;
    int legends;            /* File, Tool, Parts, Place, Layers */
    int layers_legend;
    int slice_legend;
    uint32_t save_bg, save_text;
    uint32_t move_bg, open_bg, open_border;
    int open_radius;
    int face_label_x, face_label_y;
} seen;

static int first_box;

static void collect(void *userdata, const olui_draw_op *op)
{
    const char *id = op->el ? olui_id(op->el) : NULL;
    const char *tag = op->el ? olui_tag(op->el) : "";
    (void)userdata;
    if (op->kind == OLUI_DRAW_BOX) {
        uint32_t top = op->stops ? op->stop_argb[0] : 0;
        if (!first_box) {
            first_box = 1;
            seen.window_bg = top;
        }
        /* A fieldset with a legend is drawn as its edges, square: GFOpenLayout
         * breaks the top edge for the legend and does not round it. */
        if (!strcmp(tag, "fieldset") && op->el != seen.last_group && op->stops == 1 && top == 0xffd0d0d0u) {
            seen.groups++;
            seen.last_group = op->el;
        }
        if (id && !strcmp(id, "save")) seen.save_bg = top;
        if (id && !strcmp(id, "move")) seen.move_bg = top;
        if (id && !strcmp(id, "open")) {
            seen.open_bg = top;
            seen.open_border = op->border_argb;
            seen.open_radius = op->radius;
        }
    } else if (op->kind == OLUI_DRAW_TEXT && op->text) {
        if (id && !strcmp(id, "save")) seen.save_text = op->argb;
        if (!strcmp(tag, "legend")) seen.legends++;
        if (!strcmp(tag, "legend") && op->length == 6 && !memcmp(op->text, "Layers", 6)) seen.layers_legend++;
        if (!strcmp(tag, "legend") && op->length == 5 && !memcmp(op->text, "Slice", 5)) seen.slice_legend++;
        if (!strcmp(tag, "label") && op->length == 4 && !memcmp(op->text, "Face", 4)) {
            seen.face_label_x = op->box.x;
            seen.face_label_y = op->baseline;
        }
    }
}

static int inside(const olui_rect *r, int w, int h)
{
    return r->w > 0 && r->h > 0 && r->x >= 0 && r->y >= 0 && r->x + r->w <= w && r->y + r->h <= h;
}

static int overlap(const olui_rect *a, const olui_rect *b)
{
    return a->x < b->x + b->w && b->x < a->x + a->w && a->y < b->y + b->h && b->y < a->y + a->h;
}

int main(void)
{
    olui *ui = olui_new();
    char err[160];
    olui_rect box[NBUTTONS], view, status;
    /* 640 by 480 inside: with the title bar and the sizing border (about
     * 21 px in Topaz 8) it fits a 640 by 512 PAL screen. */
    int w = 640, h = 480, i, j;
    olui_set_measure(ui, measure, NULL);
    expect(olui_load(ui, gm_markup, gm_css, err, sizeof err), "the window loads"); if (failed) printf("%s\n", err);
    expect(olui_layout(ui, w, h), "the window lays out at 640 by 480, as make.c opens it");
    for (i = 0; i < NBUTTONS; i++) {
        olui_el *el = olui_find(ui, buttons[i]);
        char what[80];
        snprintf(what, sizeof what, "%s is a button in the window", buttons[i]);
        expect(el && strcmp(olui_tag(el), "button") == 0 && olui_box(ui, el, &box[i]) && inside(&box[i], w, h), what);
        for (j = 0; el && j < i; j++) {
            snprintf(what, sizeof what, "%s is clear of %s", buttons[i], buttons[j]);
            expect(!overlap(&box[i], &box[j]), what);
        }
    }
    for (i = 0; i < NCONTROLS; i++) {
        olui_el *el = olui_find(ui, controls[i][0]);
        olui_rect r;
        char what[80];
        snprintf(what, sizeof what, "%s is a %s in the window", controls[i][0], controls[i][1]);
        expect(el && strcmp(olui_tag(el), controls[i][1]) == 0 && olui_box(ui, el, &r) && inside(&r, w, h), what);
        for (j = 0; el && j < NBUTTONS; j++) {
            snprintf(what, sizeof what, "%s is clear of %s", controls[i][0], buttons[j]);
            expect(!overlap(&r, &box[j]), what);
        }
    }
    {
        /* Height from 0.08 to 0.8 mm, 0.2 to start; the layer not shown. */
        olui_el *lh = olui_find(ui, "lh");
        expect(lh && olui_number_decimals(lh) == 2 && olui_number_value(lh) == 20, "the layer height starts at 0.20 mm");
        olui_set_number(ui, lh, 1);
        expect(lh && olui_number_value(lh) == 8, "and is held to 0.08 mm");
        olui_set_number(ui, lh, 500);
        expect(lh && olui_number_value(lh) == 80, "and to 0.8 mm");
        olui_set_number(ui, lh, 20);
        expect(!olui_get_checked(olui_find(ui, "show")), "no layer is shown at the start");
    }
    expect(olui_find(ui, "face") != NULL, "there is a face readout");
    expect(olui_box(ui, olui_find(ui, "view"), &view) && inside(&view, w, h) && view.h >= h / 2,
           "the view has at least half the height");
    expect(olui_box(ui, olui_find(ui, "status"), &status) && inside(&status, w, h) && status.y >= view.y + view.h,
           "the status line is under the view");
    expect(status.y + status.h >= h - 16, "the status line is along the bottom");
    {
        olui_rect pick, sl;
        expect(olui_box(ui, olui_find(ui, "machine"), &pick) && olui_box(ui, olui_find(ui, "slice"), &sl) &&
                   pick.y + pick.h <= sl.y && pick.w >= 150 && pick.y + pick.h <= status.y,
               "the printer picker is over Slice, wide enough for a name, above the status line");
    }
    {
        olui_rect side;
        expect(olui_box(ui, olui_find(ui, "side"), &side) && inside(&side, w, h) && side.y + side.h <= status.y,
               "every group fits above the status line at 480");
    }
    expect(status.w >= 78 * 8, "the status line holds 78 characters of Topaz 8 (omv_slice_status keeps to them)");

    /* The Clear look (docs/HANDOVER.md). */
    olui_draw(ui, NULL, collect, NULL);
    expect(seen.window_bg == WINDOW_BG, "the window is Clear's light #f9f9f9");
    expect(seen.groups == 6 && seen.legends == 6 && seen.layers_legend == 1 && seen.slice_legend == 1,
           "the controls are in six groups with legends, Layers and Slice among them");
    expect(strcmp(olui_get_text(olui_find(ui, "slice")), "Slice") == 0 &&
               strcmp(olui_get_text(olui_find(ui, "gcode")), "Save G-code") == 0,
           "the Slice group: Slice, and Save G-code");
    expect(strcmp(olui_get_text(olui_find(ui, "save")), "Save STL") == 0, "the primary button says Save STL");
    expect(seen.save_bg == ACCENT && seen.save_text == 0xffffffffu, "Save STL is the accent primary button");
    expect(seen.open_bg == 0xfffefefeu && seen.open_border == 0xffd0d0d0u && seen.open_radius == 4,
           "the other buttons are Clear's, with radius 4");
    expect(seen.move_bg != ACCENT, "no tool is lit before one is chosen");
    {
        olui_rect face;
        expect(olui_box(ui, olui_find(ui, "face"), &face) && seen.face_label_x > 0 && seen.face_label_x < face.x &&
                   seen.face_label_y > face.y && seen.face_label_y <= face.y + face.h,
               "the face readout has its label beside it");
    }
    olui_set_selected(ui, olui_find(ui, "move"), 1);
    olui_layout(ui, w, h);
    memset(&seen, 0, sizeof seen);
    first_box = 0;
    olui_draw(ui, NULL, collect, NULL);
    expect(seen.move_bg == ACCENT, "the chosen tool is lit in the accent");

    /* Sized up, as the window opens on an RTG screen (the screen below its
     * title bar, less the borders): the same controls, the side as wide,
     * the view taking the rest, the status line still along the bottom. */
    {
        static const int sizes[][2] = {{1016, 736}, {1912, 1048}, {2552, 1408}};
        olui_rect side0, view1, side1, st1, r;
        int s;
        olui_box(ui, olui_find(ui, "side"), &side0);
        for (s = 0; s < 3; s++) {
            int big_w = sizes[s][0], big_h = sizes[s][1], ok = 1;
            char what[96];
            snprintf(what, sizeof what, "the window lays out at %d by %d", big_w, big_h);
            expect(olui_layout(ui, big_w, big_h), what);
            for (i = 0; i < NBUTTONS; i++) ok &= olui_box(ui, olui_find(ui, buttons[i]), &r) && inside(&r, big_w, big_h);
            for (i = 0; i < NCONTROLS; i++) ok &= olui_box(ui, olui_find(ui, controls[i][0]), &r) && inside(&r, big_w, big_h);
            snprintf(what, sizeof what, "at %d by %d every control is in the window", big_w, big_h);
            expect(ok, what);
            snprintf(what, sizeof what, "at %d by %d the side keeps its width; the view takes the rest", big_w, big_h);
            expect(olui_box(ui, olui_find(ui, "view"), &view1) && olui_box(ui, olui_find(ui, "side"), &side1) &&
                       side1.w == side0.w && view1.w >= view.w + (big_w - w) - 8 && view1.h >= view.h + (big_h - h) - 8,
                   what);
            snprintf(what, sizeof what, "at %d by %d the status line along the bottom, the window's width", big_w, big_h);
            expect(olui_box(ui, olui_find(ui, "status"), &st1) && st1.y + st1.h >= big_h - 16 && st1.w >= big_w - 16,
                   what);
        }
    }
    olui_free(ui);
    if (failed) {
        printf("%d failed\n", failed);
        return 1;
    }
    printf("make ui ok\n");
    return 0;
}
