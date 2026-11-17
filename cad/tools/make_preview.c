/* A host picture of GFOpenMake's window after Slice: the real layout
 * (make_ui.h, GFOpenLayout) recorded as draw operations for
 * slicer/tools/paint.py, and the view as the CPU drawer draws the toolpath
 * (gt_paint_part, fitted to the part: what the pens show without OpenGPU), with the status line
 * make.c's Slice says. Not the Amiga, not OpenGPU.
 *   make_preview out.ops [upto [machines [open]]]
 * upto: the Layers slider's layer, Show layer ticked (0: all layers);
 * machines: where the printer picker looks (machines/ to start); open:
 * the picker's list shown open.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gs_edit.h"
#include "gs_gcode.h"
#include "gs_plan.h"
#include "make_ui.h"
#include "omview_machines.h"
#include "omview_ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static FILE *ops;

static int measure(void *userdata, const char *utf8, size_t length, const ol_style *style, ol_text_metrics *metrics)
{
    (void)userdata;
    (void)utf8;
    metrics->width = (ol_unit)(length * (size_t)style->font_size * 60 / 100);
    metrics->ascent = style->font_size * 80 / 100;
    metrics->descent = style->font_size * 22 / 100;
    return 1;
}

static void draw(void *userdata, const olui_draw_op *op)
{
    int i;
    (void)userdata;
    if (op->alpha == 0) return;
    if (op->kind == OLUI_DRAW_BOX) {
        fprintf(ops, "B %d %d %d %d %d", op->box.x, op->box.y, op->box.w, op->box.h, op->stops);
        for (i = 0; i < op->stops && i < OLUI_MAX_STOPS; i++) fprintf(ops, " %08x %d", op->stop_argb[i], op->stop_pos[i]);
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
    olui_rect box;
    gs_mesh part;
    gs_cam cam;
    gp_opts o;
    gc_opts go;
    gc_stats st;
    gt_path path;
    char err[160], status[180], ppm[256];
    unsigned char *rgb;
    int w = 640, h = 480, upto = argc > 2 ? atoi(argv[2]) : 0;
    FILE *pf;
    omv_machines printers;
    if (argc < 2) return 2;
    memset(&printers, 0, sizeof printers);
    memset(&part, 0, sizeof part);
    if (ge_start(&part, NULL, err, (int)sizeof err)) {
        fprintf(stderr, "%s\n", err);
        return 1;
    }
    /* As make.c's Slice: the part at the Layers height, then its estimate. */
    gp_defaults(&o);
    o.layer_h = 0.2f;
    gt_init(&path);
    if (gp_part(&path, &part, &o)) return 1;
    gc_defaults(&go);
    go.layer_h = o.layer_h;
    gc_estimate(&path, NULL, &go, &st);
    if (upto > path.nlayers) upto = path.nlayers;

    ui = olui_new();
    olui_set_measure(ui, measure, NULL);
    if (!olui_load(ui, gm_markup, gm_css, err, sizeof err)) {
        fprintf(stderr, "%s\n", err);
        return 1;
    }
    omv_machines_scan(&printers, argc > 3 ? argv[3] : "machines", "printer");
    omv_machines_fill(ui, "machine", &printers, "No printers");
    omv_slice_status(status, (int)sizeof status, path.nlayers, st.moves, st.seconds, st.filament, upto);
    omv_say(ui, status);
    if (upto) {
        olui_set_checked(ui, olui_find(ui, "show"), 1);
        olui_set_slider(ui, olui_find(ui, "layer"), omv_slider_of(upto, path.nlayers));
    }
    if (!olui_layout(ui, w, h)) return 1;
    if (argc > 4 && !strcmp(argv[4], "open")) {
        olui_select_open(ui, olui_find(ui, "machine"), 1);
        if (!olui_layout(ui, w, h)) return 1;
    }
    ops = fopen(argv[1], "w");
    if (!ops) return 1;
    fprintf(ops, "W %d %d\n", w, h);
    olui_draw(ui, NULL, draw, NULL);
    if (olui_box(ui, olui_find(ui, "view"), &box) && box.w > 8 && box.h > 8) {
        rgb = (unsigned char *)malloc((size_t)box.w * (size_t)box.h * 3);
        gs_cam_home(&cam);
        if (rgb && !gt_paint_part(&path, &cam, upto, rgb, box.w, box.h)) {
            snprintf(ppm, sizeof ppm, "%s.ppm", argv[1]);
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
    fclose(ops);
    printf("%s\n", status);
    gt_free(&path);
    gs_mesh_free(&part);
    omv_machines_free(&printers);
    olui_free(ui);
    return 0;
}
