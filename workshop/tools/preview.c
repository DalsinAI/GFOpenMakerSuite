/* Host render of GFOpenMakerWorkshop's window (workshop/src/markup.h).
 * Usage: preview W H out.ops [project-drawer] [item]
 * item is a 0-based index, or a name. The picked item's preview is drawn
 * into the view (STL as a solid, SVG/DXF as strokes, G-code as a
 * toolpath, a Netpbm picture as ink; KiCad is listed only) and pasted as a P picture. This is a
 * host render, not an Amiga run.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "workshop_ui.h"
#include "omview_ui.h"

#include <gfopenlayout_ui.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static FILE *ops;

static void draw(void *userdata, const olui_draw_op *op)
{
    int i;
    (void)userdata;
    if (op->alpha == 0) return;
    if (op->kind == OLUI_DRAW_BOX) {
        fprintf(ops, "B %d %d %d %d %d", op->box.x, op->box.y, op->box.w, op->box.h, op->stops);
        for (i = 0; i < op->stops && i < OLUI_MAX_STOPS; i++)
            fprintf(ops, " %08x %d", op->stop_argb[i], op->stop_pos[i]);
        fprintf(ops, " %08x %d %d %d\n", op->border_argb, op->border, op->radius, op->alpha);
    } else if (op->kind == OLUI_DRAW_TEXT && op->text && op->length) {
        fprintf(ops, "T %d %d %d %08x %d ", op->box.x, op->baseline, op->font_pixels, op->argb, op->alpha);
        fwrite(op->text, 1, op->length, ops);
        fputc('\n', ops);
    } else if (op->kind == OLUI_DRAW_MARK) {
        fprintf(ops, "M %d %d %d %d %d %08x %d\n", op->box.x, op->box.y, op->box.w, op->box.h, op->mark, op->argb,
                op->alpha);
    } else if (op->kind == OLUI_DRAW_CLIP) {
        fprintf(ops, "C %d %d %d %d\n", op->clip.x, op->clip.y, op->clip.w, op->clip.h);
    }
}

int main(int argc, char **argv)
{
    olui *ui;
    workshop w;
    char err[160];
    int width, height;
    olui_rect box;

    if (argc < 4) {
        fprintf(stderr, "usage: preview W H out.ops [project] [item]\n");
        return 1;
    }
    width = atoi(argv[1]);
    height = atoi(argv[2]);
    if (width < 64 || height < 64) {
        fprintf(stderr, "the size is too small\n");
        return 1;
    }
    ui = olui_new();
    workshop_init(&w);
    if (!ui || !workshop_ui_load(ui, err, (int)sizeof err)) {
        fprintf(stderr, "workshop preview: %s\n", ui ? err : "GFOpenLayout is not open");
        return 1;
    }
    if (argc >= 5) {
        if (gpr_open(&w.project, argv[4], err, (int)sizeof err)) {
            fprintf(stderr, "workshop preview: %s\n", err);
            workshop_free(&w);
            olui_free(ui);
            return 1;
        }
        w.have = 1;
        w.picked = w.project.n ? 0 : -1;
        if (argc >= 6) {
            int at = gpr_find(&w.project, argv[5]);
            if (at < 0 && argv[5][0] >= '0' && argv[5][0] <= '9') at = atoi(argv[5]);
            if (at >= 0 && at < w.project.n) w.picked = at;
            else {
                fprintf(stderr, "workshop preview: no item %s\n", argv[5]);
                workshop_free(&w);
                olui_free(ui);
                return 1;
            }
        }
        gpr_recent_touch(&w.recent, w.project.path);
    }
    workshop_ui_refresh(ui, &w);
    if (!olui_layout(ui, width, height)) {
        fprintf(stderr, "workshop preview: the window did not lay out\n");
        workshop_free(&w);
        olui_free(ui);
        return 1;
    }
    ops = fopen(argv[3], "w");
    if (!ops) return 1;
    fprintf(ops, "W %d %d\n", width, height);
    olui_draw(ui, NULL, draw, ops);
    if (olui_box(ui, olui_find(ui, "view"), &box) && box.w > 8 && box.h > 8) {
        unsigned char *rgb = (unsigned char *)malloc((size_t)box.w * (size_t)box.h * 3);
        if (rgb) {
            char why[160];
            why[0] = 0;
            workshop_ui_preview(&w, NULL, rgb, box.w, box.h, why, (int)sizeof why);
            {
                char ppm[256];
                FILE *pf;
                snprintf(ppm, sizeof ppm, "%s.ppm", argv[3]);
                pf = fopen(ppm, "wb");
                if (pf) {
                    fprintf(pf, "P6\n%d %d\n255\n", box.w, box.h);
                    fwrite(rgb, 1, (size_t)box.w * (size_t)box.h * 3, pf);
                    fclose(pf);
                    fprintf(ops, "P %d %d %d %d %s\n", box.x, box.y, box.w, box.h, ppm);
                }
            }
            free(rgb);
        }
    }
    fclose(ops);
    workshop_free(&w);
    olui_free(ui);
    return 0;
}
