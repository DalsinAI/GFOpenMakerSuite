/* GFOpenSlicer on the Amiga. The window is GFOpenLayout (markup.h): Open
 * asks ASL for an STL, Slice makes its toolpath with the engine on as many
 * tasks as there are cores (sl_job.h: gp_part_par, exec tasks here), the
 * slider shows it a layer at a time, and Save G-code writes it for the
 * printer picked (core/ui's machine picker), once the printer says it can.
 * The view is drawn by OpenGPU when gfopengpu.library can (core/ui's
 * omview_scene.h and omview_gpu.h, as GFOpenMake), with the pens when not.
 * The status line, the numbers, the pen drawer and the resizable window
 * are the suite's (core/ui).
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gs.h"
#include "gs_layer.h"
#include "gs_par.h"
#include "markup.h"
#include "omview_gpu.h"
#include "omview_machines.h"
#include "omview_scene.h"
#include "omview_ui.h"
#include "omview_window.h"
#include "sl_job.h"
#include "slicer_ui.h"

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

static const char version[] = GS_VERSTR;
static sl_job job;
static omv_machines printers;
static gs_cam cam;
static struct Window *win;
static int dragging, drag_x, drag_y, drag_shift;
static int show_layer, layer_no = 1;
static gc_opts sliced_go; /* the G-code options the slice was made with */
static omv_gpu gpu;

/* The printer picked, loaded; a 220 mm bed when there is none. 0, or -1. */
static int printer(olui *ui, om_machine *m, char *err, int errlen)
{
    int i = omv_machines_chosen(ui, "machine", &printers);
    if (i < 0) {
        snprintf(err, (size_t)errlen, "No printer: put one in PROGDIR:machines/printer");
        om_clear(m);
        m->area[0] = m->area[1] = 220.f;
        return -1;
    }
    return omv_machines_load(&printers, i, m, err, errlen);
}

static void refresh(olui *ui)
{
    char status[180];
    if (job.sliced) {
        omv_slice_status(status, (int)sizeof status, job.path.nlayers, job.st.moves, job.st.seconds, job.st.filament,
                         show_layer ? layer_no : 0);
        omv_say(ui, status);
    }
    olui_repaint(ui, olui_find(ui, "view"), NULL);
}

/* One run of one pen: one RectFill, at the view's corner in ud. */
static void put_run(void *ud, int x, int y, int n, int pen)
{
    const int *at = ud;
    SetAPen(win->RPort, (UBYTE)pen);
    RectFill(win->RPort, at[0] + x, at[1] + y, at[0] + x + n - 1, at[1] + y);
}

/* The view on OpenGPU: 1, or 0 when it cannot (no library, no room,
 * nothing to draw) and the pens draw it. */
static int draw_gpu(int x, int y, int w, int h)
{
    const gs_mesh *part = job.have_part ? &job.part : NULL;
    const gt_path *path = job.sliced ? &job.path : NULL;
    int need = omv_scene_need(part, path), n;
    if (!gpu.on || need <= 0 || !omv_gpu_size(&gpu, w, h) || !omv_gpu_room(&gpu, need)) return 0;
    n = omv_scene(part, path, &cam, show_layer ? layer_no : 0, gpu.grid, w, h, gpu.verts, gpu.vert_cap, gpu.stream,
                  gpu.stream_cap, (uint32_t)(uintptr_t)gpu.rs->rs_Pixels, (int)gpu.rs->rs_BytesPerRow,
                  (uint32_t)(uintptr_t)gpu.depth);
    return n > 0 && omv_gpu_show(&gpu, n, win->RPort, x, y);
}

/* The toolpath once sliced (layers 1 to the slider's with Show), the part
 * before: by OpenGPU when it can, else drawn by the CPU drawer and put down
 * with the pens a run at a time. Drag turns it, Shift-drag pans. */
static void draw_view(void *userdata, const olui_el *el, int x, int y, int w, int h)
{
    unsigned char *rgb;
    int at[2], rc = -1;
    (void)userdata;
    (void)el;
    if (draw_gpu(x, y, w, h)) return;
    rgb = (unsigned char *)malloc((size_t)w * (size_t)h * 3);
    if (!rgb) return;
    if (job.sliced) rc = gt_paint_part(&job.path, &cam, show_layer ? layer_no : 0, rgb, w, h);
    else if (job.have_part) rc = gs_view_solid(&job.part, &cam, rgb, w, h);
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

/* An ASL requester's file, or 0 when none was chosen. */
static int ask_file(olui *ui, const char *title, int save, const char *pattern, char *path, int n)
{
    struct FileRequester *fr;
    int got = 0;
    fr = (struct FileRequester *)AllocAslRequestTags(ASL_FileRequest, ASLFR_TitleText, (ULONG)title,
                                                     ASLFR_DoSaveMode, (ULONG)save, ASLFR_DoPatterns, TRUE,
                                                     ASLFR_InitialPattern, (ULONG)pattern, TAG_END);
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
    char path[256], err[160];
    om_machine m;
    (void)ev;
    (void)userdata;
    if (!ask_file(ui, "Open an STL", FALSE, "#?.stl", path, (int)sizeof path)) return;
    printer(ui, &m, err, (int)sizeof err);
    if (sl_load(&job, path, m.area[0], m.area[1], err, (int)sizeof err)) {
        omv_say(ui, err);
        return;
    }
    olui_set_text(ui, olui_find(ui, "path"), path);
    gs_cam_home(&cam);
    snprintf(err, sizeof err, "%d triangles. Slice when ready.", job.part.n);
    omv_say(ui, err);
    refresh(ui);
}

static void on_slice(olui *ui, const olui_event *ev, void *userdata)
{
    gp_opts po;
    gc_opts go;
    om_machine m;
    char err[160];
    int have;
    (void)ev;
    (void)userdata;
    have = printer(ui, &m, err, (int)sizeof err) == 0;
    if (job.have_part) gp_place(&job.part, m.area[0], m.area[1]); /* on the bed of the printer picked */
    sl_ui_opts(ui, &po, &go);
    omv_say(ui, "Slicing...");
    if (sl_slice(&job, &po, &go, have ? &m : NULL, gs_par_cores(), err, (int)sizeof err)) {
        omv_say(ui, err);
        olui_repaint(ui, olui_find(ui, "view"), NULL);
        return;
    }
    sliced_go = go;
    sliced_go.layer_h = gl_height(po.layer_h);
    layer_no = job.path.nlayers;
    olui_set_slider(ui, olui_find(ui, "layerno"), 100);
    refresh(ui);
}

static void on_gcode(olui *ui, const olui_event *ev, void *userdata)
{
    om_machine m;
    char path[256], err[160];
    (void)ev;
    (void)userdata;
    if (!job.sliced) {
        omv_say(ui, "Slice first");
        return;
    }
    if (printer(ui, &m, err, (int)sizeof err)) {
        omv_say(ui, err);
        return;
    }
    if (!ask_file(ui, "Save G-code", TRUE, "#?.gcode", path, (int)sizeof path)) return;
    if (sl_write(&job, path, &m, &sliced_go, err, (int)sizeof err)) omv_say(ui, err);
    else omv_say(ui, path);
}

static void on_show(olui *ui, const olui_event *ev, void *userdata)
{
    (void)userdata;
    show_layer = ev->checked;
    refresh(ui);
}

static void on_step(olui *ui, const olui_event *ev, void *userdata)
{
    int n = job.path.nlayers > 0 ? job.path.nlayers : 1;
    (void)ev;
    layer_no += (int)(intptr_t)userdata;
    if (layer_no < 1) layer_no = 1;
    if (layer_no > n) layer_no = n;
    if (!show_layer) olui_set_checked(ui, olui_find(ui, "show"), show_layer = 1);
    olui_set_slider(ui, olui_find(ui, "layerno"), omv_slider_of(layer_no, n));
    refresh(ui);
}

static void on_slide(olui *ui, const olui_event *ev, void *userdata)
{
    (void)userdata;
    layer_no = omv_layer_of(ev->index, job.path.nlayers > 0 ? job.path.nlayers : 1);
    if (!show_layer) olui_set_checked(ui, olui_find(ui, "show"), show_layer = 1);
    refresh(ui);
}

int main(void)
{
    olui *ui;
    olui_amiga *a;
    struct Window *w;
    struct IntuiMessage *m;
    char err[160];
    (void)version;
    ui = olui_new();
    if (!ui || !olui_load(ui, gs_markup, gs_css, err, sizeof err)) {
        if (ui) olui_free(ui);
        fprintf(stderr, "GFOpenSlicer: %s\n", ui ? err : "GFOpenLayout is not open");
        return 20;
    }
    olui_on(ui, "open", OLUI_CLICK, on_open, NULL);
    olui_on(ui, "slice", OLUI_CLICK, on_slice, NULL);
    olui_on(ui, "gcode", OLUI_CLICK, on_gcode, NULL);
    olui_on(ui, "show", OLUI_CHANGE, on_show, NULL);
    olui_on(ui, "prev", OLUI_CLICK, on_step, (void *)(intptr_t)-1);
    olui_on(ui, "next", OLUI_CLICK, on_step, (void *)(intptr_t)1);
    olui_on(ui, "layerno", OLUI_CHANGE, on_slide, NULL);
    sl_init(&job);
    gs_cam_home(&cam);
    /* OpenGPU when the library can draw the view; the pens when not. */
    omv_gpu_open(&gpu, 4096, 512);
    /* Resizable, 640 by 480 inside at the least: a PAL screen, or more on RTG. */
    w = omv_window_open("GFOpenSlicer 0.1.0", olui_amiga_idcmp());
    if (!w) {
        olui_free(ui);
        omv_gpu_close(&gpu);
        fprintf(stderr, "GFOpenSlicer: the window did not open\n");
        return 20;
    }
    win = w;
    a = olui_amiga_new(ui, w);
    if (!a) {
        CloseWindow(w);
        olui_free(ui);
        omv_gpu_close(&gpu);
        fprintf(stderr, "GFOpenSlicer: GFOpenLayout could not draw the window\n");
        return 20;
    }
    olui_amiga_theme(a, NULL);
    olui_amiga_own(a, olui_find(ui, "view"), draw_view, NULL);
    /* The printer picker: every machines/printer/<driver>/profiles/ .om file. */
    if (omv_machines_scan(&printers, "PROGDIR:machines", "printer") < 0) printers.n = 0;
    omv_machines_fill(ui, "machine", &printers, "No printers");
    olui_amiga_layout(a);
    if (printers.skipped) omv_say(ui, printers.why);
    for (;;) {
        int r;
        WaitPort(w->UserPort);
        while ((m = (struct IntuiMessage *)GetMsg(w->UserPort))) {
            WORD mx = m->MouseX, my = m->MouseY;
            ULONG cls = m->Class;
            UWORD code = m->Code;
            UWORD qual = m->Qualifier;
            r = olui_amiga_event(a, m);
            ReplyMsg((struct Message *)m);
            if (r == OLUI_AMIGA_CLOSE) goto done;
            if (cls == IDCMP_MOUSEBUTTONS && code == SELECTDOWN) {
                olui_rect box;
                if (olui_box(ui, olui_find(ui, "view"), &box) &&
                    mx >= w->BorderLeft + box.x && mx < w->BorderLeft + box.x + box.w &&
                    my >= w->BorderTop + box.y && my < w->BorderTop + box.y + box.h) {
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
    sl_free(&job);
    omv_machines_free(&printers);
    olui_free(ui);
    omv_gpu_close(&gpu);
    return 0;
}
