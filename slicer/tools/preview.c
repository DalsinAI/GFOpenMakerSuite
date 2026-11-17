/* Host picture of the GFOpenLayout window. The layout is the real one;
 * this only records the draw operations.
 *   preview empty out.ops
 *   preview sliced out.ops stl [upto]
 * with SIZE=WxH last: the window's inside (640x480 to start). Sliced as
 * amiga.c's Slice does (sl_job, the toolpath engine on 4 tasks) for the
 * suite's printer picked first, and drawn as its view: the toolpath, layers
 * 1 to upto with Show ticked (0: all).
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "markup.h"
#include "omview_machines.h"
#include "omview_ui.h"
#include "sl_job.h"
#include "slicer_ui.h"

#include <gfopenlayout_ui.h>

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
    if (metrics->width < 0) metrics->width = 0;
    return 1;
}

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
        fprintf(ops, "M %d %d %d %d %d %08x %d\n", op->box.x, op->box.y, op->box.w, op->box.h, op->mark, op->argb, op->alpha);
    } else if (op->kind == OLUI_DRAW_CLIP) {
        fprintf(ops, "C %d %d %d %d\n", op->clip.x, op->clip.y, op->clip.w, op->clip.h);
    }
}

int main(int argc, char **argv)
{
    olui *ui;
    char err[160];
    int w = 640, h = 480; /* as amiga.c opens it */
    sl_job job;
    omv_machines printers;
    int upto = 0;
    if (argc > 3 && !strncmp(argv[argc - 1], "SIZE=", 5)) {
        if (sscanf(argv[argc - 1] + 5, "%dx%d", &w, &h) != 2 || w < 64 || h < 64) return 2;
        argc--;
    }
    if (argc < 3) return 2;
    ui = olui_new();
    olui_set_measure(ui, measure, NULL);
    if (!olui_load(ui, gs_markup, gs_css, err, sizeof err)) {
        fprintf(stderr, "%s\n", err);
        return 1;
    }
    memset(&printers, 0, sizeof printers);
    omv_machines_scan(&printers, "machines", "printer");
    omv_machines_fill(ui, "machine", &printers, "No printers");
    sl_init(&job);
    if (strcmp(argv[1], "sliced") == 0) {
        gp_opts po;
        gc_opts go;
        om_machine m;
        char name[64], status[180];
        const char *base;
        if (argc < 4 || omv_machines_load(&printers, 0, &m, err, (int)sizeof err) ||
            sl_load(&job, argv[3], m.area[0], m.area[1], err, (int)sizeof err)) {
            fprintf(stderr, "%s\n", argc < 4 ? "need an stl" : err);
            return 1;
        }
        sl_ui_opts(ui, &po, &go);
        if (sl_slice(&job, &po, &go, &m, 4, err, (int)sizeof err)) {
            fprintf(stderr, "%s\n", err);
            return 1;
        }
        upto = argc > 4 ? atoi(argv[4]) : 0;
        if (upto > job.path.nlayers) upto = job.path.nlayers;
        /* The STL's own name, in an Amiga drawer. */
        base = strrchr(argv[3], '/');
        snprintf(name, sizeof name, "Work:%.26s", base ? base + 1 : argv[3]);
        olui_set_text(ui, olui_find(ui, "path"), name);
        if (upto) {
            olui_set_checked(ui, olui_find(ui, "show"), 1);
            olui_set_slider(ui, olui_find(ui, "layerno"), omv_slider_of(upto, job.path.nlayers));
        }
        omv_slice_status(status, (int)sizeof status, job.path.nlayers, job.st.moves, job.st.seconds, job.st.filament, upto);
        omv_say(ui, status);
        printf("%s (%d tasks)\n", status, job.tasks);
    }
    if (!olui_layout(ui, w, h)) return 1;
    ops = fopen(argv[2], "w");
    if (!ops) return 1;
    fprintf(ops, "W %d %d\n", w, h);
    olui_draw(ui, NULL, draw, NULL);
    if (job.sliced) {
        olui_rect box;
        unsigned char *rgb;
        gs_cam cam;
        if (olui_box(ui, olui_find(ui, "view"), &box) && box.w > 8 && box.h > 8) {
            rgb = (unsigned char *)malloc((size_t)box.w * (size_t)box.h * 3);
            gs_cam_home(&cam);
            if (rgb && !gt_paint_part(&job.path, &cam, upto, rgb, box.w, box.h)) {
                char ppm[256];
                FILE *pf;
                snprintf(ppm, sizeof ppm, "%s.ppm", argv[2]);
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
    sl_free(&job);
    omv_machines_free(&printers);
    olui_free(ui);
    return 0;
}
