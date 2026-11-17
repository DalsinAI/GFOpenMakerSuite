/* A host picture of GFOpenMake's window after Slice: the real layout
 * (make_ui.h, GFOpenLayout) recorded as draw operations for
 * slicer/tools/paint.py, and the view as the CPU drawer draws the toolpath
 * (gt_paint_part, fitted to the part: what the pens show without OpenGPU), with the status line
 * make.c's Slice says. Not the Amiga, not OpenGPU.
 *   make_preview out.ops [upto [machines [open]]] [SIZE=WxH]
 * upto: the Layers slider's layer, Show layer ticked (0: all layers);
 * machines: where the printer picker looks (machines/ to start); open:
 * the picker's list shown open; SIZE: the window's inside, 640x480 to start.
 * SETTINGS: a profile's lines put in the settings panel and sliced with,
 * held to the first printer; SIDE: the side's tab (1, Print, to start);
 * TAB: the panel's tab. SHAPE: Parts' shape (0 Box ... 6 Slot) added and
 * OP'd into the part first (0 join, 1 cut, 2 intersect; join to start);
 * FIT=axis:mm and AT=x,y[,z]: Place's Size and At on the added shape first.
 * HOLES: 0 bolt circle, 1 hole row, 2 counterbore cut first, with HD, HN,
 * HPCD, HPITCH, HCB and HDEEP for the Holes fields (as shown).
 * OPEN: a select's id, shown open (the side on Model for "shape").
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gs_edit.h"
#include "gs_gcode.h"
#include "gs_plan.h"
#include "gs_settings.h"
#include "make_ui.h"
#include "omview_machines.h"
#include "omview_settings.h"
#include "omview_ui.h"

#include <math.h>
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

/* As make.c's draw_marks: a blue line between the points, each a white
 * square ringed in black, 5 pixels, as the pens draw them. */
static void put(unsigned char *rgb, int w, int h, int x, int y, int r, int g, int b)
{
    unsigned char *p;
    if (x < 0 || y < 0 || x >= w || y >= h) return;
    p = rgb + ((size_t)y * (size_t)w + (size_t)x) * 3;
    p[0] = (unsigned char)r, p[1] = (unsigned char)g, p[2] = (unsigned char)b;
}

static int paint_marks(unsigned char *rgb, int w, int h, const gs_mesh *m, const gs_cam *cam, const gs_v *pt, int n)
{
    float px[2], py[2];
    int i, dx, dy;
    for (i = 0; i < n; i++)
        if (ge_project(m, cam, w, h, pt[i], &px[i], &py[i])) return -1;
    if (n == 2) {
        int steps = (int)(fabsf(px[1] - px[0]) > fabsf(py[1] - py[0]) ? fabsf(px[1] - px[0]) : fabsf(py[1] - py[0])) + 1;
        for (i = 0; i <= steps; i++) {
            float t = (float)i / (float)steps;
            put(rgb, w, h, (int)(px[0] + (px[1] - px[0]) * t), (int)(py[0] + (py[1] - py[0]) * t), 0x66, 0x88, 0xbb);
        }
    }
    for (i = 0; i < n; i++)
        for (dy = -2; dy <= 2; dy++)
            for (dx = -2; dx <= 2; dx++) {
                int edge = dx == -2 || dx == 2 || dy == -2 || dy == 2;
                put(rgb, w, h, (int)px[i] + dx, (int)py[i] + dy, edge ? 0 : 255, edge ? 0 : 255, edge ? 0 : 255);
            }
    return 0;
}

int main(int argc, char **argv)
{
    olui *ui;
    olui_rect box;
    gs_mesh part;
    gs_cam cam;
    gp_opts o;
    gc_opts go;
    gp_more x;
    gc_more gx;
    gs_settings set;
    om_machine m;
    int have, how;
    gc_stats st;
    gt_path path;
    char err[160], status[180], ppm[256];
    gs_v mark[2];
    int nmark = 0;
    unsigned char *rgb;
    int w = 640, h = 480, upto = argc > 2 ? atoi(argv[2]) : 0;
    FILE *pf;
    omv_machines printers;
    if (argc > 2 && !strncmp(argv[argc - 1], "SIZE=", 5)) {
        if (sscanf(argv[argc - 1] + 5, "%dx%d", &w, &h) != 2 || w < 64 || h < 64) return 2;
        argc--;
        upto = argc > 2 ? atoi(argv[2]) : 0;
    }
    if (argc < 2) return 2;
    memset(&printers, 0, sizeof printers);
    memset(&part, 0, sizeof part);
    if (ge_start(&part, NULL, err, (int)sizeof err)) {
        fprintf(stderr, "%s\n", err);
        return 1;
    }
    if (getenv("SHAPE")) { /* as make.c's Add, then Join, Cut or Intersect */
        ge_scene sc;
        memset(&sc, 0, sizeof sc);
        sc.stock = part;
        sc.has_added = sc.on_added = 1;
        if (ge_shape(&sc.added, atoi(getenv("SHAPE")), &part) == 0 && getenv("FIT")) {
            int axis;
            float mm; /* as Place's Size, before the Join or Cut */
            if (sscanf(getenv("FIT"), "%d:%f", &axis, &mm) != 2 || ge_fit(&sc.added, axis, mm)) return 2;
        }
        if (sc.added.n && getenv("AT")) {
            float pos[3], size[3]; /* as Place's At: x,y[,z] */
            ge_where(&sc.added, pos, size);
            if (sscanf(getenv("AT"), "%f,%f,%f", &pos[0], &pos[1], &pos[2]) < 2) return 2;
            ge_place(&sc.added, pos[0], pos[1], pos[2]);
        }
        if (!sc.added.n ||
            ge_bool(&sc, getenv("OP") ? atoi(getenv("OP")) : 0, NULL)) {
            fprintf(stderr, "SHAPE: that did not build or did not meet the part\n");
            return 1;
        }
        part = sc.stock;
        printf("%s %s: %d triangles\n", ge_shape_name(atoi(getenv("SHAPE"))),
               getenv("OP") && atoi(getenv("OP")) == 1 ? "cut" : getenv("OP") && atoi(getenv("OP")) == 2 ? "met" : "joined",
               part.n);
    }
    if (getenv("HOLES")) { /* as make.c's Holes tab, its fields' values or these */
        ge_holes hs;
        char why[96];
        hs.d = getenv("HD") ? (float)atof(getenv("HD")) : 3.f;
        hs.n = getenv("HN") ? atoi(getenv("HN")) : 4;
        hs.pcd = getenv("HPCD") ? (float)atof(getenv("HPCD")) : 16.f;
        hs.pitch = getenv("HPITCH") ? (float)atof(getenv("HPITCH")) : 8.f;
        hs.cb = getenv("HCB") ? (float)atof(getenv("HCB")) : 16.f;
        hs.deep = getenv("HDEEP") ? (float)atof(getenv("HDEEP")) : 4.f;
        if (ge_drill(&part, atoi(getenv("HOLES")), &hs, why, (int)sizeof why)) {
            fprintf(stderr, "HOLES: %s\n", why);
            return 1;
        }
        printf("holes cut: %d triangles\n", part.n);
    }
    ui = olui_new();
    olui_set_measure(ui, measure, NULL);
    if (!olui_load(ui, gm_markup, gm_css, err, sizeof err) || omv_settings_add(ui, "settings", err, (int)sizeof err)) {
        fprintf(stderr, "%s\n", err);
        return 1;
    }
    gs_settings_default(&set);
    if (getenv("SETTINGS") && gs_settings_parse(&set, getenv("SETTINGS"), err, (int)sizeof err)) {
        fprintf(stderr, "SETTINGS: %s\n", err);
        return 1;
    }
    omv_settings_show(ui, &set);
    olui_tabs_select(ui, olui_find(ui, "side-tabs"), getenv("SIDE") ? atoi(getenv("SIDE")) : 1);
    if (getenv("TAB")) omv_settings_tab(ui, atoi(getenv("TAB")));
    omv_machines_scan(&printers, argc > 3 ? argv[3] : "machines", "printer");
    omv_machines_fill(ui, "machine", &printers, "No printers");
    /* As make.c's Slice: the settings held to the printer, the part sliced
     * with them, then its estimate. */
    have = printers.n > 0 && omv_machines_load(&printers, 0, &m, err, (int)sizeof err) == 0;
    if (have) {
        omv_settings_fit(ui, &m, &set, err, (int)sizeof err);
        if (err[0]) printf("%s\n", err);
    }
    gs_settings_default(&set);
    omv_settings_read(ui, &set);
    gs_settings_opts(&set, &o, &go);
    gs_settings_more(&set, &x);
    gt_init(&path);
    if (gp_part_more(&path, &part, &o, &x)) return 1;
    gs_settings_gc_more(&set, &go, &gx);
    how = gc_estimate_more(&path, have ? &m : NULL, &gx, &st);
    if (upto > path.nlayers) upto = path.nlayers;
    omv_slice_status(status, (int)sizeof status, path.nlayers, st.moves, st.seconds, st.filament, upto,
                     (how & GC_FELL_BACK) != 0);
    omv_say(ui, status);
    if (upto) {
        olui_set_checked(ui, olui_find(ui, "show"), 1);
        olui_set_slider(ui, olui_find(ui, "layer"), omv_slider_of(upto, path.nlayers));
    }
    {
        int k; /* as make.c fills Parts' list, showing the shape added */
        for (k = 0; k < GE_SHAPES; k++) olui_select_add(ui, olui_find(ui, "shape"), ge_shape_name(k), ge_shape_name(k));
        olui_select_set(ui, olui_find(ui, "shape"), getenv("SHAPE") ? atoi(getenv("SHAPE")) : 0);
    }
    {
        static const char *const ids[6] = {"sx", "sy", "sz", "px", "py", "pz"};
        float pos[3], size[3];
        int k; /* as make.c's Place: the part's box and bottom middle */
        ge_where(&part, pos, size);
        for (k = 0; k < 3; k++) {
            olui_set_number(ui, olui_find(ui, ids[k]), (int)(size[k] * 10.f + 0.5f));
            olui_set_number(ui, olui_find(ui, ids[3 + k]), (int)(pos[k] < 0.f ? pos[k] * 10.f - 0.5f : pos[k] * 10.f + 0.5f));
        }
    }
    {
        static const char *const ids[6][2] = {{"hd", "HD"}, {"hn", "HN"}, {"hpcd", "HPCD"},
                                              {"hpitch", "HPITCH"}, {"hcb", "HCB"}, {"hdeep", "HDEEP"}};
        int k;
        for (k = 0; k < 6; k++)
            if (getenv(ids[k][1]))
                olui_set_number(ui, olui_find(ui, ids[k][0]),
                                (int)(atof(getenv(ids[k][1])) * (k == 1 ? 1.0 : 10.0) + 0.5));
    }
    if (!olui_layout(ui, w, h)) return 1;
    if (getenv("OPEN")) {
        olui_select_open(ui, olui_find(ui, getenv("OPEN")), 1);
        if (!olui_layout(ui, w, h)) return 1;
    }
    if (argc > 4 && !strcmp(argv[4], "open")) {
        olui_select_open(ui, olui_find(ui, "machine"), 1);
        if (!olui_layout(ui, w, h)) return 1;
    }
    gs_cam_home(&cam);
    if (getenv("MEASURE")) { /* as make.c's Measure: the tool lit, then each click */
        const char *q = getenv("MEASURE");
        olui_set_selected(ui, olui_find(ui, "orbit"), 0);
        olui_set_selected(ui, olui_find(ui, "measure"), 1);
        ge_facts_line(status, (int)sizeof status, &part);
        /* Each world point to its pixel, then picked there as a click is:
         * the part, or else the bed. */
        while (olui_box(ui, olui_find(ui, "view"), &box) && nmark < 2 &&
               sscanf(q, "%f,%f,%f", &mark[nmark].x, &mark[nmark].y, &mark[nmark].z) == 3) {
            float fx, fy;
            if (ge_project(&part, &cam, box.w, box.h, mark[nmark], &fx, &fy) ||
                (ge_pick_at(&part, &cam, box.w, box.h, (int)fx, (int)fy, &mark[nmark]) < 0 &&
                 ge_pick_bed(&part, &cam, box.w, box.h, (int)fx, (int)fy, &mark[nmark]))) {
                fprintf(stderr, "MEASURE: neither part nor bed under point %d\n", nmark + 1);
                return 2;
            }
            nmark++;
            q = strchr(q, ';');
            if (!q) break;
            q++;
        }
        if (nmark == 2) ge_measure_line(status, (int)sizeof status, mark[0], mark[1]);
        else if (nmark == 1) snprintf(status, sizeof status, "First point set. Click the second.");
        omv_say(ui, status);
    }
    ops = fopen(argv[1], "w");
    if (!ops) return 1;
    fprintf(ops, "W %d %d\n", w, h);
    olui_draw(ui, NULL, draw, NULL);
    if (olui_box(ui, olui_find(ui, "view"), &box) && box.w > 8 && box.h > 8) {
        rgb = (unsigned char *)malloc((size_t)box.w * (size_t)box.h * 3);
        if (rgb && (getenv("MEASURE") ? !gs_view_bed(&part, &cam, rgb, box.w, box.h) && !paint_marks(rgb, box.w, box.h, &part, &cam, mark, nmark)
                                      : !gt_paint_part(&path, &cam, upto, rgb, box.w, box.h))) {
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
