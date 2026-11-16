/* GFOpenMake's window, laid out on the host by the real GFOpenLayout.
 * The text is measured at 60% of the font size, as tools/preview.c does;
 * the Amiga's fonts will differ a little.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "make_ui.h"

#include <gfopenlayout_ui.h>

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
    int w = 640, h = 480, i, j;
    olui_set_measure(ui, measure, NULL);
    expect(olui_load(ui, gm_markup, gm_css, err, sizeof err), "the window loads");
    expect(olui_layout(ui, w, h), "the window lays out at 640 by 480");
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
    olui_free(ui);
    if (failed) {
        printf("%d failed\n", failed);
        return 1;
    }
    printf("make ui ok\n");
    return 0;
}
