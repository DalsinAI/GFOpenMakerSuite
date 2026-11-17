/* Host picture of the OpenMakeBurn window. The layout is the real one;
 * this only records the draw operations.
 *   preview empty out.ops
 *   preview job out.ops svg
 * with SIZE=WxH last: the window's inside (640x480 to start). Job as
 * amiga.c's Preview does (burn_path at the layer settings) for the suite's
 * laser picked first, and drawn as its view: the toolpath, framed to the
 * job. With GPU=1 in the environment the view is the OpenGPU batch
 * amiga.c submits (omview_scene.h), its triangles filled here on the host
 * with a depth buffer: not a chip, not OpenGPU.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "burn_job.h"
#include "burn_ui.h"
#include "burn_material.h"
#include "markup.h"
#include "omview_machines.h"
#include "gs_gpu.h"
#include "omview_scene.h"
#include "omview_ui.h"

#include <gfopenlayout_ui.h>

#include <stdint.h>
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
        fprintf(ops, "M %d %d %d %d %d %08x %d\n", op->box.x, op->box.y, op->box.w, op->box.h, op->mark, op->argb,
                op->alpha);
    } else if (op->kind == OLUI_DRAW_CLIP) {
        fprintf(ops, "C %d %d %d %d\n", op->clip.x, op->clip.y, op->clip.w, op->clip.h);
    }
}

static void tri(unsigned char *rgb, uint32_t *zb, int w, int h, const uint32_t *v)
{
    float x[3], y[3], area;
    int i, px, py, x0, x1, y0, y1;
    uint32_t c = v[2];
    for (i = 0; i < 3; i++) {
        x[i] = (float)(int32_t)v[i * 6] / 65536.f;
        y[i] = (float)(int32_t)v[i * 6 + 1] / 65536.f;
    }
    area = (x[1] - x[0]) * (y[2] - y[0]) - (x[2] - x[0]) * (y[1] - y[0]);
    if (area == 0.f) return;
    x0 = (int)(x[0] < x[1] ? (x[0] < x[2] ? x[0] : x[2]) : (x[1] < x[2] ? x[1] : x[2]));
    x1 = (int)(x[0] > x[1] ? (x[0] > x[2] ? x[0] : x[2]) : (x[1] > x[2] ? x[1] : x[2])) + 1;
    y0 = (int)(y[0] < y[1] ? (y[0] < y[2] ? y[0] : y[2]) : (y[1] < y[2] ? y[1] : y[2]));
    y1 = (int)(y[0] > y[1] ? (y[0] > y[2] ? y[0] : y[2]) : (y[1] > y[2] ? y[1] : y[2])) + 1;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > w) x1 = w;
    if (y1 > h) y1 = h;
    for (py = y0; py < y1; py++)
        for (px = x0; px < x1; px++) {
            float fx = (float)px + .5f, fy = (float)py + .5f;
            float a = ((x[1] - fx) * (y[2] - fy) - (x[2] - fx) * (y[1] - fy)) / area;
            float b = ((x[2] - fx) * (y[0] - fy) - (x[0] - fx) * (y[2] - fy)) / area;
            float g = 1.f - a - b;
            uint32_t z;
            if (a < 0.f || b < 0.f || g < 0.f) continue;
            z = (uint32_t)(a * (float)v[5] + b * (float)v[11] + g * (float)v[17]);
            if (z <= zb[py * w + px]) continue;
            zb[py * w + px] = z;
            rgb[(py * w + px) * 3] = (unsigned char)(c >> 16);
            rgb[(py * w + px) * 3 + 1] = (unsigned char)(c >> 8);
            rgb[(py * w + px) * 3 + 2] = (unsigned char)c;
        }
}

static int paint_batch(const gt_path *p, const gs_cam *cam, unsigned char *rgb, int w, int h)
{
    int need = omv_scene_need(NULL, p), i, n, nv;
    uint32_t *v = calloc((size_t)need * 6, 4), *st = calloc(65536, 4), *zb = calloc((size_t)w * (size_t)h, 4);
    n = v && st && zb ? omv_scene(NULL, p, cam, 0, 0, w, h, v, need, st, 65536, 0x10000, w * 4, 0x20000) : -1;
    nv = n > 0 ? gs_gpu_toolpath(p, cam, 0, 1, w, h, v, need) : -1;
    for (i = 0; i < w * h; i++) {
        rgb[i * 3] = 0x14;
        rgb[i * 3 + 1] = 0x16;
        rgb[i * 3 + 2] = 0x1c;
    }
    for (i = 0; i + 2 < nv; i += 3) tri(rgb, zb, w, h, v + i * 6);
    free(v);
    free(st);
    free(zb);
    return nv > 0 ? 0 : -1;
}

int main(int argc, char **argv)
{
    olui *ui;
    char err[160];
    int w = 640, h = 480;
    gv_doc d;
    gt_path path;
    int have_path = 0;
    omv_machines lasers;
    burn_opts o;
    burn_layer store[64];
    if (argc > 3 && !strncmp(argv[argc - 1], "SIZE=", 5)) {
        if (sscanf(argv[argc - 1] + 5, "%dx%d", &w, &h) != 2 || w < 64 || h < 64) return 2;
        argc--;
    }
    if (argc < 3) return 2;
    ui = olui_new();
    olui_set_measure(ui, measure, NULL);
    if (!olui_load(ui, burn_markup, burn_css, err, sizeof err)) {
        fprintf(stderr, "%s\n", err);
        return 1;
    }
    memset(&lasers, 0, sizeof lasers);
    omv_machines_scan(&lasers, "machines", "laser");
    omv_machines_fill(ui, "machine", &lasers, "No lasers");
    {
        burn_materials stocks;
        burn_materials_init(&stocks);
        burn_materials_scan(&stocks, "materials/laser");
        burn_ui_materials_fill(ui, "material", &stocks);
        burn_materials_free(&stocks);
    }
    gv_doc_init(&d);
    gt_init(&path);
    burn_defaults(&o);
    if (strcmp(argv[1], "job") == 0) {
        om_machine m;
        float *speeds = NULL;
        gc_laser L;
        gc_laser_more more;
        gc_stats st;
        char name[64], status[180];
        const char *base;
        if (argc < 4 || omv_machines_load(&lasers, 0, &m, err, (int)sizeof err) ||
            gv_load(&d, argv[3], err, (int)sizeof err)) {
            fprintf(stderr, "%s\n", argc < 4 ? "need a drawing" : err);
            return 1;
        }
        if (burn_ui_layers(ui, &d, &o, err, (int)sizeof err)) {
            fprintf(stderr, "%s\n", err);
            return 1;
        }
        if (getenv("MODE") || getenv("INTERVAL")) {
            int i, n = burn_ui_count(ui);
            char id[32];
            int mode = getenv("MODE") && !strcmp(getenv("MODE"), "offset") ? 2
                       : getenv("MODE") && !strcmp(getenv("MODE"), "fill")  ? 1
                                                                            : 0;
            float gap = getenv("INTERVAL") ? (float)atof(getenv("INTERVAL")) : 0.2f;
            int dec;
            for (i = 0; i < n; i++) {
                snprintf(id, sizeof id, "layer-%d-mode", i);
                if (getenv("MODE") && olui_find(ui, id)) olui_select_set(ui, olui_find(ui, id), mode);
                snprintf(id, sizeof id, "layer-%d-interval", i);
                if (getenv("INTERVAL") && olui_find(ui, id)) {
                    dec = olui_number_decimals(olui_find(ui, id));
                    {
                        float d = 1.f;
                        int k;
                        for (k = 0; k < dec; k++) d *= 10.f;
                        olui_set_number(ui, olui_find(ui, id), (int)(gap * d + 0.5f));
                    }
                }
            }
        }
        burn_ui_read(ui, &o, store, 64);
        if (burn_path(&path, &speeds, &d, &o) || path.n < 2) {
            fprintf(stderr, "no paths to burn\n");
            free(speeds);
            return 1;
        }
        have_path = 1;
        gc_laser_defaults(&L);
        L.constant = o.constant;
        L.s100 = o.s100;
        L.speed = o.speed;
        L.travel = o.travel;
        gc_laser_more_defaults(&more, &L);
        more.speed = speeds;
        memset(&st, 0, sizeof st);
        gc_laser_estimate_more(&path, &m, &more, &st);
        free(speeds);
        base = strrchr(argv[3], '/');
        snprintf(name, sizeof name, "Work:%.26s", base ? base + 1 : argv[3]);
        olui_set_text(ui, olui_find(ui, "path"), name);
        burn_ui_status(status, (int)sizeof status, m.name, st.moves, st.seconds);
        omv_say(ui, status);
        printf("%s\n", status);
    }
    if (!olui_layout(ui, w, h)) return 1;
    ops = fopen(argv[2], "w");
    if (!ops) return 1;
    fprintf(ops, "W %d %d\n", w, h);
    olui_draw(ui, NULL, draw, NULL);
    if (have_path) {
        olui_rect box;
        unsigned char *rgb;
        gs_cam cam;
        if (olui_box(ui, olui_find(ui, "view"), &box) && box.w > 8 && box.h > 8) {
            rgb = (unsigned char *)malloc((size_t)box.w * (size_t)box.h * 3);
            gs_cam_home(&cam);
            if (rgb && !(getenv("GPU") ? paint_batch(&path, &cam, rgb, box.w, box.h)
                                       : gt_paint_part(&path, &cam, 0, rgb, box.w, box.h))) {
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
    gv_doc_free(&d);
    gt_free(&path);
    omv_machines_free(&lasers);
    olui_free(ui);
    return 0;
}
