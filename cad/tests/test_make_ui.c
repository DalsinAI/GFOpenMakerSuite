/* GFOpenMake's window, laid out on the host by the real GFOpenLayout.
 * The text is measured at 60% of the font size, as tools/preview.c does;
 * the Amiga's fonts will differ a little.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "make_ui.h"
#include "omview_settings.h"

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

/* Every button make.c hands to olui_on: the Model tab's, then the Print
 * tab's from PRINT_BUTTONS on. */
static const char *const buttons[] = {
    "open", "save", "undo", "orbit", "move", "rot", "scale", "measure", "drop", "centre", "lay",
    "add", "join", "cut", "meet", "prev", "next", "slice", "gcode"
};
#define PRINT_BUTTONS 15
#define TAB_OF(i) ((i) >= PRINT_BUTTONS)

/* The Print tab's other controls: the Layers group's, and the Slice
 * group's machine picker, and what they are. The layer height is the
 * settings panel's. */
static const char *const controls[][2] = {
    {"show", "checkbox"}, {"layer", "slider"}, {"machine", "select"}, {"set-layer-height", "spinner"}
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
    int w = 640, h = 480, i, j, t;
    gs_settings d;
    olui_el *side_tabs;
    olui_set_measure(ui, measure, NULL);
    expect(olui_load(ui, gm_markup, gm_css, err, sizeof err), "the window loads"); if (failed) printf("%s\n", err);
    expect(omv_settings_add(ui, "settings", err, (int)sizeof err) == 0, "the settings panel goes in the Slice group");
    gs_settings_default(&d);
    omv_settings_show(ui, &d);
    side_tabs = olui_find(ui, "side-tabs");
    expect(side_tabs && !strcmp(olui_tag(side_tabs), "tabs"), "the side is two tabs");
    /* Each tab's buttons in the window and clear of each other. */
    for (t = 1; t >= 0; t--) {
        olui_tabs_select(ui, side_tabs, t);
        expect(olui_layout(ui, w, h), "the window lays out at 640 by 480, as make.c opens it");
        for (i = 0; i < NBUTTONS; i++) {
            olui_el *el = olui_find(ui, buttons[i]);
            char what[80];
            if (TAB_OF(i) != t) continue;
            snprintf(what, sizeof what, "%s is a button in the window", buttons[i]);
            expect(el && strcmp(olui_tag(el), "button") == 0 && olui_box(ui, el, &box[i]) && inside(&box[i], w, h),
                   what);
            for (j = 0; el && j < i; j++) {
                if (TAB_OF(j) != t) continue;
                snprintf(what, sizeof what, "%s is clear of %s", buttons[i], buttons[j]);
                expect(!overlap(&box[i], &box[j]), what);
            }
        }
        if (t == 0) {
            /* Parts: the shape to add, a select beside Add, wide enough for "Cylinder". */
            olui_el *shape = olui_find(ui, "shape");
            olui_rect r;
            expect(shape && !strcmp(olui_tag(shape), "select") && olui_box(ui, shape, &r) && inside(&r, w, h) &&
                       r.w >= 90 && !overlap(&r, &box[11]) && r.x < box[11].x,
                   "the shape select is beside Add, wide enough for a name");
            /* Measure beside Rotate and Scale, wide enough for its name. */
            expect(box[6].y == box[5].y && box[7].y == box[5].y && box[7].x > box[6].x && box[7].w >= 7 * 8,
                   "Measure is in the Tool group's second row, after Scale");
            {
                /* Place: a row for X, Y and Z, its Size then its At, as the bench's
                 * Size and place, with room for "-999.9" beside the arrows. */
                static const char *const nums[6] = {"sx", "sy", "sz", "px", "py", "pz"};
                olui_rect n[6], side;
                int a, b;
                char what[80];
                olui_box(ui, olui_find(ui, "side"), &side);
                for (a = 0; a < 6; a++) {
                    olui_el *el = olui_find(ui, nums[a]);
                    snprintf(what, sizeof what, "%s: a number field in the side, room for -999.9", nums[a]);
                    expect(el && !strcmp(olui_tag(el), "spinner") && olui_box(ui, el, &n[a]) && inside(&n[a], w, h) &&
                               n[a].x >= side.x && n[a].x + n[a].w <= side.x + side.w && n[a].w >= 72 && olui_number_decimals(el) == 1,
                           what);
                    for (b = 0; b < NBUTTONS; b++) {
                        if (TAB_OF(b) != 0) continue;
                        snprintf(what, sizeof what, "%s is clear of %s", nums[a], buttons[b]);
                        expect(!overlap(&n[a], &box[b]), what);
                    }
                    for (b = 0; b < a; b++) {
                        snprintf(what, sizeof what, "%s is clear of %s", nums[a], nums[b]);
                        expect(!overlap(&n[a], &n[b]), what);
                    }
                }
                expect(n[0].y == n[3].y && n[1].y == n[4].y && n[2].y == n[5].y && n[0].y < n[1].y && n[1].y < n[2].y &&
                           n[0].x < n[3].x && n[0].x == n[1].x && n[3].x == n[5].x,
                       "a row each for X, Y, Z: Size, then At");
                expect(olui_box(ui, olui_find(ui, "status"), &status) && n[2].y + n[2].h < status.y &&
                           n[2].y + n[2].h <= side.y + side.h,
                       "Place still clear of the status line");
            }
            break;
        }
        for (i = 0; i < omv_settings_tabs(); i++) {
            olui_rect side, r, frame;
            int k;
            omv_settings_tab(ui, i);
            olui_layout(ui, w, h);
            olui_box(ui, olui_find(ui, "side"), &side);
            olui_box(ui, olui_find(ui, "set-tabs"), &frame);
            for (k = 0; k < gs_settings_count(); k++) {
                char id[48], what[96];
                if (omv_settings_tab_of(k) != i) continue;
                snprintf(id, sizeof id, "set-%s", gs_setting_at(k)->key);
                snprintf(what, sizeof what, "%s in the side, clear of Slice", id);
                /* and room after it in the panel's frame for its unit, "mm/s" in Topaz 8 */
                expect(olui_box(ui, olui_find(ui, id), &r) && r.x >= side.x && r.x + r.w + 4 + 34 <= frame.x + frame.w &&
                           r.y >= side.y && r.y + r.h <= side.y + side.h && !overlap(&r, &box[NBUTTONS - 2]) &&
                           !overlap(&r, &box[NBUTTONS - 1]),
                       what);
            }
        }
        omv_settings_tab(ui, 0);
        olui_layout(ui, w, h);
    }
    olui_tabs_select(ui, side_tabs, 1);
    olui_layout(ui, w, h);
    for (i = 0; i < NCONTROLS; i++) {
        olui_el *el = olui_find(ui, controls[i][0]);
        olui_rect r;
        char what[80];
        snprintf(what, sizeof what, "%s is a %s in the window", controls[i][0], controls[i][1]);
        expect(el && strcmp(olui_tag(el), controls[i][1]) == 0 && olui_box(ui, el, &r) && inside(&r, w, h), what);
        for (j = PRINT_BUTTONS; el && j < NBUTTONS; j++) {
            snprintf(what, sizeof what, "%s is clear of %s", controls[i][0], buttons[j]);
            expect(!overlap(&r, &box[j]), what);
        }
    }
    {
        /* 0.2 mm to start, the panel's; the layer not shown. */
        olui_el *lh = olui_find(ui, "set-layer-height");
        expect(lh && olui_number_decimals(lh) == 2 && olui_number_value(lh) == 20, "the layer height starts at 0.20 mm");
        expect(!olui_find(ui, "lh"), "and is only the panel's");
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
        expect(olui_box(ui, olui_find(ui, "side"), &side) && inside(&side, w, h) && side.y + side.h <= status.y &&
                   side.w == OMV_SETTINGS_W,
               "the Print tab fits above the status line at 480, the panel's width");
        olui_tabs_select(ui, side_tabs, 0);
        olui_layout(ui, w, h);
        expect(olui_box(ui, olui_find(ui, "side"), &side) && inside(&side, w, h) && side.y + side.h <= status.y,
               "and the Model tab");
    }
    {
        /* The Holes tab: drill, holes, the patterns and the counterbore,
         * each field with its label and unit in the side, every button
         * clear, above the status line, at 640 by 480 and sized up. */
        static const char *const hole_ids[] = {"hd", "hn", "hpcd", "hpitch", "hcb", "hdeep", "bolt", "row", "cbore"};
        const int nh = (int)(sizeof hole_ids / sizeof hole_ids[0]);
        static const int at[][2] = {{640, 480}, {1912, 1048}};
        int s;
        for (s = 0; s < 2; s++) {
            olui_rect side, st, r[9];
            int a, b;
            char what[96];
            olui_tabs_select(ui, side_tabs, 2);
            expect(olui_layout(ui, at[s][0], at[s][1]), "the Holes tab lays out");
            olui_box(ui, olui_find(ui, "side"), &side);
            olui_box(ui, olui_find(ui, "status"), &st);
            for (a = 0; a < nh; a++) {
                olui_el *el = olui_find(ui, hole_ids[a]);
                snprintf(what, sizeof what, "%s in the side at %d by %d, above the status line", hole_ids[a], at[s][0], at[s][1]);
                expect(el && olui_box(ui, el, &r[a]) && r[a].x >= side.x && r[a].x + r[a].w <= side.x + side.w &&
                           r[a].y + r[a].h <= st.y && !strcmp(olui_tag(el), a < 6 ? "spinner" : "button") &&
                           r[a].w >= (a < 6 ? 72 : 90),
                       what);
                for (b = 0; b < a; b++) {
                    snprintf(what, sizeof what, "%s is clear of %s", hole_ids[a], hole_ids[b]);
                    expect(!overlap(&r[a], &r[b]), what);
                }
            }
            expect(side.w == OMV_SETTINGS_W, "the side keeps its width on the Holes tab");
        }
        olui_tabs_select(ui, side_tabs, 0);
        olui_layout(ui, w, h);
    }
    expect(status.w >= 78 * 8, "the status line holds 78 characters of Topaz 8 (omv_slice_status keeps to them)");

    /* The Clear look (docs/HANDOVER.md). */
    olui_draw(ui, NULL, collect, NULL);
    expect(seen.window_bg == WINDOW_BG, "the window is Clear's light #f9f9f9");
    expect(seen.groups == 4 && seen.legends == 4 && !seen.layers_legend && !seen.slice_legend,
           "the Model tab: four groups with legends, File to Place");
    olui_tabs_select(ui, side_tabs, 1);
    olui_layout(ui, w, h);
    memset(&seen, 0, sizeof seen);
    first_box = 0;
    olui_draw(ui, NULL, collect, NULL);
    expect(seen.groups == 2 && seen.legends == 2 && seen.layers_legend == 1 && seen.slice_legend == 1,
           "the Print tab: Layers and Slice");
    olui_tabs_select(ui, side_tabs, 0);
    olui_layout(ui, w, h);
    memset(&seen, 0, sizeof seen);
    first_box = 0;
    olui_draw(ui, NULL, collect, NULL);
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
            for (t = 0; t < 2; t++) {
                olui_tabs_select(ui, side_tabs, t);
                olui_layout(ui, big_w, big_h);
                for (i = 0; i < NBUTTONS; i++)
                    if (TAB_OF(i) == t) ok &= olui_box(ui, olui_find(ui, buttons[i]), &r) && inside(&r, big_w, big_h);
                for (i = 0; t && i < NCONTROLS; i++)
                    ok &= olui_box(ui, olui_find(ui, controls[i][0]), &r) && inside(&r, big_w, big_h);
            }
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
