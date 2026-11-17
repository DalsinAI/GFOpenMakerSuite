/* OpenMakeBurn on the Amiga. The window is GFOpenLayout (markup.h): Open
 * asks ASL for an SVG or a DXF, Preview builds the line toolpath at the layer
 * settings, Frame writes a travel around the job with the laser off, and
 * Save G-code writes Grbl laser G-code for the laser picked (core/ui's
 * machine picker). The view is drawn by OpenGPU when gfopengpu.library
 * can (core/ui's omview_scene.h and omview_gpu.h), with the pens when not.
 * The status line, the numbers, the pen drawer and the resizable window
 * are the suite's (core/ui).
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "burn_job.h"
#include "burn_ui.h"
#include "burn_material.h"
#include "markup.h"
#include "omview_gpu.h"
#include "omview_machines.h"
#include "omview_scene.h"
#include "omview_ui.h"
#include "omview_window.h"

#include <devices/inputevent.h>
#include <proto/asl.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <libraries/asl.h>
#include <intuition/intuition.h>

#include <gfopenlayout_ui.h>
#include <gfopenlayout_ui_amiga.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char version[] = "OpenMakeBurn 0.1.0";
static char svg_path[256];
static gv_doc doc;
static int have_doc;
static gt_path path;
static int have_path;
static burn_opts opts;
static burn_layer layers[64];
static int layer_seq[64];
static omv_machines lasers;
static burn_materials stocks;
static gs_cam cam;
static struct Window *win;
static int dragging, drag_x, drag_y, drag_shift;
static omv_gpu gpu;

static void hook_shifts(olui *ui);

static void read_opts(olui *ui)
{
    burn_defaults(&opts);
    burn_ui_read(ui, &opts, layers, 64);
    opts.nseq = burn_ui_order(ui, layer_seq, 64);
    opts.seq = opts.nseq ? layer_seq : NULL;
}

static void on_shift(olui *ui, const olui_event *ev, void *userdata)
{
    const char *id;
    char err[160];
    int L = 0, dir = 0;
    const char *s;
    (void)userdata;
    if (!have_doc) return;
    id = (ev && ev->id) ? ev->id : "";
    if (strncmp(id, "layer-", 6) != 0) return;
    s = id + 6;
    if (*s < '0' || *s > '9') return;
    while (*s >= '0' && *s <= '9') L = L * 10 + (*s++ - '0');
    if (!strcmp(s, "-up")) dir = -1;
    else if (!strcmp(s, "-down")) dir = 1;
    else return;
    read_opts(ui);
    if (burn_ui_move(ui, &doc, &opts, layers, 64, layer_seq, 64, L, dir, err, (int)sizeof err)) {
        omv_say(ui, err);
        return;
    }
    hook_shifts(ui);
}

static void hook_shifts(olui *ui)
{
    char id[32];
    int i, n = burn_ui_count(ui);
    for (i = 0; i < n; i++) {
        snprintf(id, sizeof id, "layer-%d-up", i);
        if (olui_find(ui, id)) olui_on(ui, id, OLUI_CLICK, on_shift, NULL);
        snprintf(id, sizeof id, "layer-%d-down", i);
        if (olui_find(ui, id)) olui_on(ui, id, OLUI_CLICK, on_shift, NULL);
    }
}

static void on_material(olui *ui, const olui_event *ev, void *userdata)
{
    int i;
    (void)ev;
    (void)userdata;
    i = burn_ui_materials_chosen(ui, "material", &stocks);
    if (i < 0) return;
    burn_ui_material(ui, &opts, &stocks.at[i]);
}

/* The laser picked, loaded; a 400 mm bed when there is none. 0, or -1. */
static int laser(olui *ui, om_machine *m, char *err, int errlen)
{
    int i = omv_machines_chosen(ui, "machine", &lasers);
    if (i < 0) {
        snprintf(err, (size_t)errlen, "No laser: put one in PROGDIR:machines/laser");
        om_clear(m);
        m->area[0] = m->area[1] = 400.f;
        return -1;
    }
    return omv_machines_load(&lasers, i, m, err, errlen);
}

static int preview_job(olui *ui, om_machine *m, char *err, int errlen)
{
    float *speeds = NULL;
    gc_laser L;
    gc_laser_more more;
    gc_stats st;
    char status[180];
    int have;
    if (!have_doc) {
        snprintf(err, (size_t)errlen, "Open a drawing first");
        return -1;
    }
    have = laser(ui, m, err, errlen) == 0;
    read_opts(ui);
    gt_free(&path);
    have_path = 0;
    if (burn_path(&path, &speeds, &doc, &opts)) {
        snprintf(err, (size_t)errlen, "out of memory");
        free(speeds);
        return -1;
    }
    if (path.n < 2) {
        snprintf(err, (size_t)errlen, "no paths to burn");
        free(speeds);
        gt_free(&path);
        return -1;
    }
    have_path = 1;
    gc_laser_defaults(&L);
    L.constant = opts.constant;
    L.s100 = opts.s100;
    L.speed = opts.speed;
    L.travel = opts.travel;
    gc_laser_more_defaults(&more, &L);
    more.speed = speeds;
    memset(&st, 0, sizeof st);
    gc_laser_estimate_more(&path, have ? m : NULL, &more, &st);
    free(speeds);
    burn_ui_status(status, (int)sizeof status, have ? m->name : "the laser", st.moves, st.seconds);
    omv_say(ui, status);
    olui_repaint(ui, olui_find(ui, "view"), NULL);
    return 0;
}

static void refresh(olui *ui)
{
    olui_repaint(ui, olui_find(ui, "view"), NULL);
}

static void put_run(void *ud, int x, int y, int n, int pen)
{
    const int *at = ud;
    SetAPen(win->RPort, (UBYTE)pen);
    RectFill(win->RPort, at[0] + x, at[1] + y, at[0] + x + n - 1, at[1] + y);
}

static int draw_gpu(int x, int y, int w, int h)
{
    const gt_path *p = have_path ? &path : NULL;
    int need = omv_scene_need(NULL, p), n;
    if (!gpu.on || need <= 0 || !omv_gpu_size(&gpu, w, h) || !omv_gpu_room(&gpu, need)) return 0;
    n = omv_scene(NULL, p, &cam, 0, gpu.grid, w, h, gpu.verts, gpu.vert_cap, gpu.stream, gpu.stream_cap,
                  (uint32_t)(uintptr_t)gpu.rs->rs_Pixels, (int)gpu.rs->rs_BytesPerRow,
                  (uint32_t)(uintptr_t)gpu.depth);
    return n > 0 && omv_gpu_show(&gpu, n, win->RPort, x, y);
}

/* The job once previewed: by OpenGPU when it can, else drawn by the CPU
 * drawer and put down with the pens a run at a time. Drag turns it,
 * Shift-drag pans. */
static void draw_view(void *userdata, const olui_el *el, int x, int y, int w, int h)
{
    unsigned char *rgb;
    int at[2], rc = -1;
    (void)userdata;
    (void)el;
    if (draw_gpu(x, y, w, h)) return;
    rgb = (unsigned char *)malloc((size_t)w * (size_t)h * 3);
    if (!rgb) return;
    if (have_path) rc = gt_paint_part(&path, &cam, 0, rgb, w, h);
    else {
        memset(rgb, 0x14, (size_t)w * (size_t)h * 3);
        rc = 0;
    }
    if (!rc) {
        at[0] = x;
        at[1] = y;
        omv_pens(rgb, w, h, put_run, at);
    }
    free(rgb);
}

static void path_join(char *path, int n, const char *drawer, const char *file)
{
    size_t len = drawer ? strlen(drawer) : 0;
    int slash = len && (drawer[len - 1] == '/' || drawer[len - 1] == ':');
    snprintf(path, (size_t)n, slash ? "%s%s" : "%s/%s", drawer ? drawer : "", file ? file : "");
}

static int ask_file(olui *ui, const char *title, int save, const char *pattern, char *path, int n)
{
    struct FileRequester *fr;
    int got = 0;
    fr = (struct FileRequester *)AllocAslRequestTags(ASL_FileRequest, ASLFR_TitleText, (ULONG)title, ASLFR_DoSaveMode,
                                                     (ULONG)save, ASLFR_DoPatterns, TRUE, ASLFR_InitialPattern,
                                                     (ULONG)pattern, TAG_END);
    if (!fr) {
        omv_say(ui, "The file requester did not open");
        return 0;
    }
    if (AslRequest(fr, NULL) && fr->fr_Drawer && fr->fr_File) {
        path_join(path, n, (const char *)fr->fr_Drawer, (const char *)fr->fr_File);
        got = 1;
    }
    FreeAslRequest(fr);
    return got;
}

static void on_open(olui *ui, const olui_event *ev, void *userdata)
{
    char pathbuf[256], err[160];
    om_machine m;
    (void)ev;
    (void)userdata;
    if (!ask_file(ui, "Open a drawing", FALSE, "#?.(svg|dxf)", pathbuf, (int)sizeof pathbuf)) return;
    burn_materials_free(&stocks);
    gv_doc_free(&doc);
    gv_doc_init(&doc);
    have_doc = 0;
    gt_free(&path);
    have_path = 0;
    if (gv_load(&doc, pathbuf, err, (int)sizeof err)) {
        omv_say(ui, err);
        refresh(ui);
        return;
    }
    have_doc = 1;
    snprintf(svg_path, sizeof svg_path, "%s", pathbuf);
    olui_set_text(ui, olui_find(ui, "path"), svg_path);
    burn_defaults(&opts);
    if (burn_ui_layers(ui, &doc, &opts, err, (int)sizeof err)) {
        omv_say(ui, err);
        return;
    }
    hook_shifts(ui);
    gs_cam_home(&cam);
    if (preview_job(ui, &m, err, (int)sizeof err)) omv_say(ui, err);
}

static void on_preview(olui *ui, const olui_event *ev, void *userdata)
{
    om_machine m;
    char err[160];
    (void)ev;
    (void)userdata;
    if (preview_job(ui, &m, err, (int)sizeof err)) omv_say(ui, err);
}

static void on_gcode(olui *ui, const olui_event *ev, void *userdata)
{
    om_machine m;
    char pathbuf[256], err[160];
    gc_stats st;
    (void)ev;
    (void)userdata;
    if (!have_doc) {
        omv_say(ui, "Open a drawing first");
        return;
    }
    if (laser(ui, &m, err, (int)sizeof err)) {
        omv_say(ui, err);
        return;
    }
    if (!ask_file(ui, "Save G-code", TRUE, "#?.gcode", pathbuf, (int)sizeof pathbuf)) return;
    read_opts(ui);
    memset(&st, 0, sizeof st);
    if (burn_write(svg_path, pathbuf, &m, &opts, &st, err, (int)sizeof err)) omv_say(ui, err);
    else omv_say(ui, pathbuf);
}

static void on_frame(olui *ui, const olui_event *ev, void *userdata)
{
    om_machine m;
    gt_path fr;
    char pathbuf[256], err[160], status[180];
    gc_stats st;
    (void)ev;
    (void)userdata;
    if (!have_path && preview_job(ui, &m, err, (int)sizeof err)) {
        omv_say(ui, err);
        return;
    }
    if (laser(ui, &m, err, (int)sizeof err)) {
        omv_say(ui, err);
        return;
    }
    if (!ask_file(ui, "Save framing G-code", TRUE, "#?.gcode", pathbuf, (int)sizeof pathbuf)) return;
    gt_init(&fr);
    read_opts(ui);
    memset(&st, 0, sizeof st);
    if (burn_frame_path(&fr, &path) || burn_write_path(&fr, NULL, pathbuf, &m, &opts, &st, err, (int)sizeof err)) {
        omv_say(ui, err);
        gt_free(&fr);
        return;
    }
    burn_ui_status(status, (int)sizeof status, m.name, st.moves, st.seconds);
    omv_say(ui, status);
    gt_free(&fr);
}

int main(void)
{
    olui *ui;
    olui_amiga *a;
    struct Window *w;
    struct IntuiMessage *msg;
    char err[160];
    (void)version;
    ui = olui_new();
    if (!ui || !olui_load(ui, burn_markup, burn_css, err, sizeof err)) {
        if (ui) olui_free(ui);
        fprintf(stderr, "OpenMakeBurn: %s\n", ui ? err : "GFOpenLayout is not open");
        return 20;
    }
    olui_on(ui, "open", OLUI_CLICK, on_open, NULL);
    olui_on(ui, "preview", OLUI_CLICK, on_preview, NULL);
    olui_on(ui, "gcode", OLUI_CLICK, on_gcode, NULL);
    olui_on(ui, "frame", OLUI_CLICK, on_frame, NULL);
    olui_on(ui, "material", OLUI_CHANGE, on_material, NULL);
    gv_doc_init(&doc);
    gt_init(&path);
    burn_defaults(&opts);
    gs_cam_home(&cam);
    omv_gpu_open(&gpu, 4096, 512);
    /* Resizable, 640 by 480 inside at the least: a PAL screen, or more on RTG. */
    w = omv_window_open("OpenMakeBurn 0.1.0", olui_amiga_idcmp());
    if (!w) {
        olui_free(ui);
        omv_gpu_close(&gpu);
        fprintf(stderr, "OpenMakeBurn: the window did not open\n");
        return 20;
    }
    win = w;
    a = olui_amiga_new(ui, w);
    if (!a) {
        CloseWindow(w);
        olui_free(ui);
        omv_gpu_close(&gpu);
        fprintf(stderr, "OpenMakeBurn: GFOpenLayout could not draw the window\n");
        return 20;
    }
    olui_amiga_theme(a, NULL);
    olui_amiga_own(a, olui_find(ui, "view"), draw_view, NULL);
    if (omv_machines_scan(&lasers, "PROGDIR:machines", "laser") < 0) lasers.n = 0;
    omv_machines_fill(ui, "machine", &lasers, "No lasers");
    burn_materials_init(&stocks);
    if (burn_materials_scan(&stocks, "PROGDIR:materials/laser") < 0) stocks.n = 0;
    burn_ui_materials_fill(ui, "material", &stocks);
    if (stocks.skipped) omv_say(ui, stocks.why);
    olui_amiga_layout(a);
    if (lasers.skipped) omv_say(ui, lasers.why);
    for (;;) {
        int r;
        WaitPort(w->UserPort);
        while ((msg = (struct IntuiMessage *)GetMsg(w->UserPort))) {
            WORD mx = msg->MouseX, my = msg->MouseY;
            ULONG cls = msg->Class;
            UWORD code = msg->Code;
            UWORD qual = msg->Qualifier;
            r = olui_amiga_event(a, msg);
            ReplyMsg((struct Message *)msg);
            if (r == OLUI_AMIGA_CLOSE) goto done;
            if (cls == IDCMP_MOUSEBUTTONS && code == SELECTDOWN) {
                olui_rect box;
                if (olui_box(ui, olui_find(ui, "view"), &box) && mx >= w->BorderLeft + box.x &&
                    mx < w->BorderLeft + box.x + box.w && my >= w->BorderTop + box.y &&
                    my < w->BorderTop + box.y + box.h) {
                    dragging = 1;
                    drag_x = mx;
                    drag_y = my;
                    drag_shift = (qual & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT)) != 0;
                }
            } else if (cls == IDCMP_MOUSEBUTTONS && code == SELECTUP) {
                dragging = 0;
            } else if (cls == IDCMP_MOUSEMOVE && dragging) {
                float dx = (float)(mx - drag_x), dy = (float)(my - drag_y);
                if (drag_shift) gs_cam_pan(&cam, dx / 400.f, -dy / 400.f);
                else gs_cam_orbit(&cam, dx * 0.01f, dy * 0.01f);
                drag_x = mx;
                drag_y = my;
                olui_repaint(ui, olui_find(ui, "view"), NULL);
            }
        }
        olui_amiga_draw(a);
    }
done:
    olui_amiga_free(a);
    CloseWindow(w);
    burn_materials_free(&stocks);
    gv_doc_free(&doc);
    gt_free(&path);
    omv_machines_free(&lasers);
    olui_free(ui);
    omv_gpu_close(&gpu);
    return 0;
}
