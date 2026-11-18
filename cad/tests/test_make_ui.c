/* GFOpenMake's window, laid out on the host by the real GFOpenLayout.
 * The text is measured at 60% of the font size, as tools/preview.c does;
 * the Amiga's fonts will differ a little.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "make_ui.h"
#include "omview_settings.h"

#include <gfopenlayout_ui.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
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

/* Every button make.c hands to olui_on: the Model tab's, the Parts tab's
 * from PARTS_BUTTONS on, then the Print tab's from PRINT_BUTTONS on. */
static const char *const buttons[] = {
    "open", "save", "undo", "redo", "orbit", "move", "rot", "scale", "measure", "drop", "centre", "lay", "shelf",
    "vtop", "vfront", "vside", "vframe", "metal", "clay", "wire",
    "add", "join", "cut", "meet", "pshow", "pdel", "c0", "c1", "c2", "c3", "c4", "c5",
    "dup", "linex", "liney", "ring", "mirror", "turn", "stack", "solo",
    "prev", "next", "slice", "gcode"
};
#define PARTS_BUTTONS 20
#define PRINT_BUTTONS 40
#define TAB_OF(i) ((i) >= PRINT_BUTTONS ? GM_TAB_PRINT : (i) >= PARTS_BUTTONS ? GM_TAB_PARTS : GM_TAB_MODEL)

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
    int legends;            /* File, Tool, Place, Layers */
    olui_rect tabbtn[8];    /* the tab strip's buttons, in order */
    int tabbtns;
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
        if (!strcmp(tag, "tabbutton") && seen.tabbtns < 8 &&
            (!seen.tabbtns || seen.tabbtn[seen.tabbtns - 1].x != op->box.x))
            seen.tabbtn[seen.tabbtns++] = op->box;
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
    int w = 640, h = 480, i, j, t, ti;
    gs_settings d;
    olui_el *side_tabs;
    olui_set_measure(ui, measure, NULL);
    expect(olui_load(ui, gm_markup(), gm_css, err, sizeof err), "the window loads"); if (failed) printf("%s\n", err);
    expect(omv_settings_add(ui, "settings", err, (int)sizeof err) == 0, "the settings panel goes in the Slice group");
    gs_settings_default(&d);
    omv_settings_show(ui, &d);
    side_tabs = olui_find(ui, "side-tabs");
    expect(side_tabs && !strcmp(olui_tag(side_tabs), "tabs"), "the side is tabs");
    /* Each tab's buttons in the window and clear of each other. */
    for (ti = 0; ti < 3; ti++) {
        static const int order[3] = {GM_TAB_PRINT, GM_TAB_PARTS, GM_TAB_MODEL};
        t = order[ti];
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
        if (t == GM_TAB_PARTS) {
            /* Make: the shape to add, a select beside Add, wide enough for "Cylinder". */
            olui_el *shape = olui_find(ui, "shape");
            olui_rect r;
            expect(shape && !strcmp(olui_tag(shape), "select") && olui_box(ui, shape, &r) && inside(&r, w, h) &&
                       r.w >= 90 && !overlap(&r, &box[PARTS_BUTTONS]) && r.x < box[PARTS_BUTTONS].x,
                   "the shape select is beside Add, wide enough for a name");
            continue;
        }
        if (t == GM_TAB_MODEL) {
            /* Measure beside Rotate and Scale, wide enough for its name. */
            expect(box[7].y == box[6].y && box[8].y == box[6].y && box[8].x > box[7].x && box[8].w >= 7 * 8,
                   "Measure is in the Tool group's second row, after Scale");
            {
                olui_el *shelf = olui_find(ui, "shelfsel");
                olui_rect r, side, st;
                olui_box(ui, olui_find(ui, "side"), &side);
                olui_box(ui, olui_find(ui, "status"), &st);
                expect(shelf && !strcmp(olui_tag(shelf), "select") && olui_box(ui, shelf, &r) && inside(&r, w, h) &&
                           r.x >= side.x && r.x + r.w <= side.x + side.w && r.w >= 90 &&
                           r.y + r.h <= st.y && !overlap(&r, &box[12]),
                       "the shelf list is beside Shelf, in the side, above the status line");
            }
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
                        if (TAB_OF(b) != GM_TAB_MODEL) continue;
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
    /* the longest name in machines/printer, as make.c fills the picker */
    olui_select_add(ui, olui_find(ui, "machine"), "Generic Marlin 220", "Generic Marlin 220");
    olui_select_add(ui, olui_find(ui, "machine"), "Generic RepRapFirmware 300", "Generic RepRapFirmware 300");
    olui_tabs_select(ui, side_tabs, GM_TAB_PRINT);
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
        olui_rect pick, sl, side;
        expect(olui_box(ui, olui_find(ui, "machine"), &pick) && olui_box(ui, olui_find(ui, "slice"), &sl) &&
                   olui_box(ui, olui_find(ui, "side"), &side) && pick.y + pick.h <= sl.y && pick.w >= 150 &&
                   pick.y + pick.h <= status.y && pick.x + pick.w <= side.x + side.w,
               "the printer picker is over Slice, wide enough for a name, in the side, above the status line");
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
        static const char *const hole_ids[] = {"hd", "hn", "hpcd", "hpitch", "hcb", "hdeep", "bolt", "row", "drill", "cbore", "csink", "washer"};
        const int nh = (int)(sizeof hole_ids / sizeof hole_ids[0]);
        static const int at[][2] = {{640, 480}, {1912, 1048}};
        int s;
        for (s = 0; s < 2; s++) {
            olui_rect side, st, r[12];
            int a, b;
            char what[96];
            olui_tabs_select(ui, side_tabs, GM_TAB_HOLES);
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
    {
        /* The Shape tab (the bench's Extrude, and its face tools): the sketch's shape, width, depth and height,
         * Add, On top and Cut; Face's distance, Pull, Chamfer and Drag (drag to pull); and
         * Add, On top and Cut, each in the side and clear, above the status
         * line, at 640 by 480 and sized up. */
        static const char *const ids[] = {"xkind", "xw", "xd", "xh", "xadd", "xon", "xcut", "xdraw", "fmm", "pull", "chamfer", "fdrag", "sect", "sectz", "split"};
        static const char tags[] = "snnnbbbbnbbbcnb"; /* select, number, button, checkbox */
        const int nx = (int)(sizeof ids / sizeof ids[0]);
        static const int at[][2] = {{640, 480}, {1912, 1048}};
        int s;
        for (s = 0; s < 2; s++) {
            olui_rect side, st, r[sizeof ids / sizeof ids[0]];
            int a, b;
            char what[96];
            olui_tabs_select(ui, side_tabs, GM_TAB_SHAPE);
            expect(olui_layout(ui, at[s][0], at[s][1]), "the Shape tab lays out");
            olui_box(ui, olui_find(ui, "side"), &side);
            olui_box(ui, olui_find(ui, "status"), &st);
            for (a = 0; a < nx; a++) {
                olui_el *el = olui_find(ui, ids[a]);
                int least = tags[a] == 'b' ? 60 : tags[a] == 'c' ? 36 : 72;
                const char *tag = tags[a] == 's' ? "select" : tags[a] == 'n' ? "spinner" : tags[a] == 'c' ? "checkbox" : "button";
                snprintf(what, sizeof what, "%s in the side at %d by %d, above the status line", ids[a], at[s][0], at[s][1]);
                expect(el && olui_box(ui, el, &r[a]) && r[a].x >= side.x && r[a].x + r[a].w <= side.x + side.w &&
                           r[a].y + r[a].h <= st.y && !strcmp(olui_tag(el), tag) && r[a].w >= least,
                       what);
                for (b = 0; b < a; b++) {
                    snprintf(what, sizeof what, "%s is clear of %s", ids[a], ids[b]);
                    expect(!overlap(&r[a], &r[b]), what);
                }
            }
            expect(side.w == OMV_SETTINGS_W, "the side keeps its width on the Shape tab");
        }
        olui_tabs_select(ui, side_tabs, 0);
        olui_layout(ui, w, h);
    }
    {
        /* The Parts tab: Make's select and buttons, the list (24 parts, the
         * bench's limit, scrolled), the name field, six swatches, Hide and
         * Delete, and Arrange's copies, gap and eight buttons, in the side, clear, above the status line, at 640 by 480
         * and full HD. And the tab strip: five tabs, each in the side, clear
         * of the next, wide enough for its five letters in Topaz 8. */
        static const char *const ids[] = {"plist", "pname", "shape", "acount", "agap"};
        static const int at[][2] = {{640, 480}, {1912, 1048}};
        olui_el *list = olui_find(ui, "plist");
        int s, k;
        char what[96];
        expect(list && !strcmp(olui_tag(list), "list") && !strcmp(olui_tag(olui_find(ui, "pname")), "input"),
               "the parts list is a list, the name a field");
        gs_parts *ps = gs_parts_new();
        for (k = 0; ps && k < GS_PARTS_MAX; k++) {
            gs_mesh m;
            memset(&m, 0, sizeof m);
            m.t = calloc(1, sizeof *m.t);
            m.n = m.cap = 1;
            gs_parts_add(ps, &m, NULL);
        }
        expect(gs_parts_count(ps) == 24, "24 parts, the bench's most");
        gs_parts_rename(ps, 23, "Bracket");
        gs_parts_set_colour(ps, 23, 4);
        gs_parts_show(ps, 23, 0);
        gm_parts_list(ui, ps);
        expect(olui_list_selected(list) && !strcmp(olui_get_text(olui_list_selected(list)), "Bracket (hidden)") &&
                   !strcmp(olui_get_text(olui_find(ui, "pname")), "Bracket") &&
                   !strcmp(olui_get_text(olui_find(ui, "pshow")), "Show") && olui_is_selected(olui_find(ui, "c4")) &&
                   !olui_is_selected(olui_find(ui, "c0")),
               "the list: the picked part selected, its name, Show for a hidden part, its metal lit");
        for (s = 0; s < 2; s++) {
            olui_rect side, st, r[5], b[NBUTTONS];
            int a, c;
            olui_tabs_select(ui, side_tabs, GM_TAB_PARTS);
            expect(olui_layout(ui, at[s][0], at[s][1]), "the Parts tab lays out");
            olui_box(ui, olui_find(ui, "side"), &side);
            olui_box(ui, olui_find(ui, "status"), &st);
            for (a = 0; a < 5; a++) {
                snprintf(what, sizeof what, "%s in the side at %d by %d, above the status line", ids[a], at[s][0], at[s][1]);
                expect(olui_box(ui, olui_find(ui, ids[a]), &r[a]) && r[a].x >= side.x && r[a].x + r[a].w <= side.x + side.w &&
                           r[a].y + r[a].h <= st.y,
                       what);
            }
            expect(r[0].h >= 6 * 14 && r[1].w >= 120, "the list shows six rows or more; the name field has room");
            for (a = PARTS_BUTTONS; a < PRINT_BUTTONS; a++) {
                snprintf(what, sizeof what, "%s in the side at %d by %d, clear", buttons[a], at[s][0], at[s][1]);
                expect(olui_box(ui, olui_find(ui, buttons[a]), &b[a]) && b[a].x >= side.x &&
                           b[a].x + b[a].w <= side.x + side.w && b[a].y + b[a].h <= st.y && b[a].w >= (a >= PARTS_BUTTONS + 6 && a < PARTS_BUTTONS + 12 ? 24 : 8 * (int)strlen(olui_get_text(olui_find(ui, buttons[a]))) + 2) &&
                           !overlap(&b[a], &r[0]) && !overlap(&b[a], &r[1]) && !overlap(&b[a], &r[2]) &&
                           !overlap(&b[a], &r[3]) && !overlap(&b[a], &r[4]),
                       what);
                for (c = PARTS_BUTTONS; c < a; c++) {
                    snprintf(what, sizeof what, "%s is clear of %s", buttons[a], buttons[c]);
                    expect(!overlap(&b[a], &b[c]), what);
                }
            }
            expect(!overlap(&r[0], &r[1]) && !overlap(&r[0], &r[2]), "the list clear of the field and the select");
            snprintf(what, sizeof what, "the Parts tab ends above the status line, in the window, at %d by %d", at[s][0], at[s][1]);
            expect(side.y + side.h <= st.y && st.y + st.h <= at[s][1] && st.h <= 24, what);
            for (a = PARTS_BUTTONS + 12; a < PRINT_BUTTONS; a++) {
                snprintf(what, sizeof what, "%s on one line at %d by %d", buttons[a], at[s][0], at[s][1]);
                expect(b[a].h <= 24, what);
            }
            snprintf(what, sizeof what, "Arrange's copies and gap at %d by %d: clear, wide enough for 80.0", at[s][0], at[s][1]);
            expect(!overlap(&r[3], &r[4]) && !overlap(&r[3], &r[0]) && !overlap(&r[4], &r[1]) && r[3].w >= 52 && r[4].w >= 52 &&
                       !strcmp(olui_tag(olui_find(ui, "acount")), "spinner"),
                   what);
            memset(&seen, 0, sizeof seen);
            first_box = 0;
            olui_draw(ui, NULL, collect, NULL);
            snprintf(what, sizeof what, "five tabs in the strip at %d by %d", at[s][0], at[s][1]);
            expect(seen.tabbtns == 5, what);
            for (a = 0; a < seen.tabbtns; a++) {
                snprintf(what, sizeof what, "tab %d in the side, clear, room for five Topaz 8 letters at %d by %d", a,
                         at[s][0], at[s][1]);
                expect(seen.tabbtn[a].x >= side.x && seen.tabbtn[a].x + seen.tabbtn[a].w <= side.x + side.w &&
                           seen.tabbtn[a].w >= 5 * 8 + 2 && (!a || !overlap(&seen.tabbtn[a], &seen.tabbtn[a - 1])),
                       what);
            }
        }
        olui_list_clear(ui, list);
        gs_parts_free(ps);
        olui_tabs_select(ui, side_tabs, GM_TAB_MODEL);
        olui_layout(ui, w, h);
        memset(&seen, 0, sizeof seen);
        first_box = 0;
    }
    expect(status.w >= 78 * 8, "the status line holds 78 characters of Topaz 8 (omv_slice_status keeps to them)");

    /* The Clear look (docs/HANDOVER.md). */
    olui_draw(ui, NULL, collect, NULL);
    expect(seen.window_bg == WINDOW_BG, "the window is Clear's light #f9f9f9");
    expect(seen.groups == 3 && seen.legends == 3 && !seen.layers_legend && !seen.slice_legend,
           "the Model tab: three groups with legends, File, Tool, Place");
    olui_tabs_select(ui, side_tabs, GM_TAB_PRINT);
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
    {
        /* File: Open and Save STL, then Undo and Redo, each on one line
         * (no name wrapped) and wide enough for its name in Topaz 8. */
        static const char *const file[4] = {"open", "save", "undo", "redo"};
        olui_rect f[4];
        int a, ok = 1;
        for (a = 0; a < 4; a++)
            ok &= olui_box(ui, olui_find(ui, file[a]), &f[a]) && f[a].h == f[0].h && f[a].h <= 22 &&
                  f[a].w >= 8 * (int)strlen(olui_get_text(olui_find(ui, file[a]))) + 2;
        expect(ok && f[1].y == f[0].y && f[1].x > f[0].x && f[3].y == f[2].y && f[3].x > f[2].x && f[2].y > f[0].y &&
                   !strcmp(olui_get_text(olui_find(ui, "redo")), "Redo"),
               "File: Open and Save STL, then Undo and Redo, none wrapped");
    }
    /* The keys, as the Amiga's: Amiga-Z undoes, Amiga-Shift-Z and Amiga-Y redo;
     * without Amiga they are a field's letters. */
    expect(gm_command_key('z', 1, 0) == GM_KEY_UNDO && gm_command_key('Z', 1, 1) == GM_KEY_REDO &&
               gm_command_key('y', 1, 0) == GM_KEY_REDO && gm_command_key('Y', 1, 0) == GM_KEY_REDO &&
               gm_command_key('z', 0, 0) == 0 && gm_command_key('x', 1, 0) == 0,
           "Amiga-Z undoes; Amiga-Shift-Z and Amiga-Y redo");
    expect(gm_command_key('p', 0, 0) == GM_KEY_PULL && gm_command_key('P', 0, 1) == GM_KEY_PULL &&
               gm_command_key(27, 0, 0) == GM_KEY_LEAVE && gm_command_key('p', 1, 0) == 0,
           "P drags to pull, as the bench's; Escape leaves it");
    expect(gm_command_key('c', 0, 0) == GM_KEY_SECTION && gm_command_key('[', 0, 0) == GM_KEY_SECT_DOWN &&
               gm_command_key(']', 0, 0) == GM_KEY_SECT_UP && gm_command_key('c', 1, 0) == 0,
           "C sections, and the brackets move the plane");
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
            for (t = 0; t < 5; t++) {
                olui_tabs_select(ui, side_tabs, t);
                olui_layout(ui, big_w, big_h);
                for (i = 0; i < NBUTTONS; i++)
                    if (TAB_OF(i) == t) ok &= olui_box(ui, olui_find(ui, buttons[i]), &r) && inside(&r, big_w, big_h);
                for (i = 0; t == GM_TAB_PRINT && i < NCONTROLS; i++)
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
    { /* The view's colours: one run per shown part, in the list's order. */
        gs_parts *ps = gs_parts_new();
        gs_tint t[GS_PARTS_MAX];
        gs_mesh m[3];
        int k, n;
        for (k = 0; k < 3; k++) {
            memset(&m[k], 0, sizeof m[k]);
            m[k].t = (gs_tri *)calloc(12, sizeof(gs_tri)); /* a cube's twelve faces; where, no matter */
            m[k].n = m[k].cap = m[k].t ? 12 : 0;
        }
        expect(ps && gs_parts_add(ps, &m[0], "A") == 0 && gs_parts_add(ps, &m[1], "B") == 1 &&
                   gs_parts_add(ps, &m[2], "C") == 2,
               "three parts");
        gs_parts_set_colour(ps, 2, 5);
        n = gm_tints(ps, t, GS_PARTS_MAX);
        expect(n == 3 && t[0].n == 12 && t[0].rgb == gs_parts_rgb(0) && t[1].rgb == gs_parts_rgb(1) &&
                   t[2].rgb == gs_parts_rgb(5),
               "three runs, each part's metal");
        gs_parts_show(ps, 1, 0);
        n = gm_tints(ps, t, GS_PARTS_MAX);
        expect(n == 2 && t[0].rgb == gs_parts_rgb(0) && t[1].n == 12 && t[1].rgb == gs_parts_rgb(5),
               "a hidden part has no run: the next part's faces follow on");
        expect(gm_tints(ps, t, 1) == 1 && gm_tints(NULL, t, 4) == 0, "no more runs than room");
        gs_parts_free(ps);
    }
    if (failed) {
        printf("%d failed\n", failed);
        return 1;
    }
    printf("make ui ok\n");
    return 0;
}
