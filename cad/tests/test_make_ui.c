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
    "addcube", "addcyl", "join", "cut", "meet"
};
#define NBUTTONS ((int)(sizeof buttons / sizeof buttons[0]))

/* What the window's draw list says, collected by element. */
#define WINDOW_BG 0xfff9f9f9u
#define ACCENT 0xff0067c0u
static struct {
    uint32_t window_bg;     /* the first box, the whole window's */
    int groups;             /* fieldsets edged in Clear's #d0d0d0 */
    const olui_el *last_group;
    int legends;            /* File, Tool, Parts, Place */
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
    int w = 640, h = 520, i, j;
    olui_set_measure(ui, measure, NULL);
    expect(olui_load(ui, gm_markup, gm_css, err, sizeof err), "the window loads");
    expect(olui_layout(ui, w, h), "the window lays out at 640 by 520, as make.c opens it");
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
    expect(olui_find(ui, "face") != NULL, "there is a face readout");
    expect(olui_box(ui, olui_find(ui, "view"), &view) && inside(&view, w, h) && view.h >= h / 2,
           "the view has at least half the height");
    expect(olui_box(ui, olui_find(ui, "status"), &status) && inside(&status, w, h) && status.y >= view.y + view.h,
           "the status line is under the view");
    expect(status.y + status.h >= h - 16, "the status line is along the bottom");

    /* The Clear look (docs/HANDOVER.md). */
    olui_draw(ui, NULL, collect, NULL);
    expect(seen.window_bg == WINDOW_BG, "the window is Clear's light #f9f9f9");
    expect(seen.groups == 4 && seen.legends == 4, "the buttons are in four groups with legends");
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
    olui_free(ui);
    if (failed) {
        printf("%d failed\n", failed);
        return 1;
    }
    printf("make ui ok\n");
    return 0;
}
