/* GFOpenSlicer on the Amiga. The window is GFOpenLayout (markup.h): Open
 * asks ASL for an STL, Slice makes its toolpath with the engine on the
 * port's task count (one a core until SET tasks), the slider shows layers
 * 1 to its place, and Save G-code writes it for the printer picked
 * (core/ui's machine picker), once the printer says it can.
 * Support, Start and End on the Settings side are the shell's SUPPORT, START
 * and END. The view is the part on the plate. Once sliced, the toolpath is laid on
 * that solid, in the same frame. OpenGPU draws it when gfopengpu.library
 * can, and the pens when not. A file named at launch is opened the way
 * Open opens one.
 * The status line, the numbers, the pen drawer and the resizable window
 * are the suite's (core/ui).
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gs.h"
#include "gs_layer.h"
#include "gs_par.h"
#include "markup.h"
#include "gs_gpu.h"
#include "omview_gpu.h"
#include "omview_machines.h"
#include "omview_settings.h"
#include "omview_ui.h"
#include "omview_window.h"
#include "sl_job.h"
#include "slicer_rexx.h"
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
static int vw = 640, vh = 320;
static omv_gpu gpu;

/* ARexx: the port OPENMAKESLICER.1, and the command table in slicer_rexx.c. */
static os_rexx rexx;
static omr_port *rexx_port;
static char stl_path[256];

static void rexx_activate(void *win)
{
    WindowToFront((struct Window *)win);
    ActivateWindow((struct Window *)win);
}

static void rexx_hide(void *win) { WindowToBack((struct Window *)win); }

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
                         show_layer ? layer_no : 0, job.how == GC_FELL_BACK);
        omv_say(ui, status);
    }
    olui_repaint(ui, olui_find(ui, "view"), NULL);
}

/* The port's layer picture. A slice that just finished shows the top layer.
 * Anything else keeps the checkbox and the slider, held to the layers there. */
static void apply_layers(olui *ui)
{
    int n = job.path.nlayers > 0 ? job.path.nlayers : 1;
    if (rexx.fresh && job.sliced && job.path.nlayers > 0) layer_no = job.path.nlayers;
    else {
        show_layer = rexx.layers ? 1 : 0;
        layer_no = rexx.layer_no >= 1 ? rexx.layer_no : 1;
        if (layer_no > n) layer_no = n;
    }
    rexx.fresh = 0;
    rexx.layers = show_layer ? 1 : 0;
    rexx.layer_no = layer_no < 1 ? 1 : layer_no;
    if (olui_find(ui, "show")) olui_set_checked(ui, olui_find(ui, "show"), show_layer);
    if (olui_find(ui, "layerno")) olui_set_slider(ui, olui_find(ui, "layerno"), omv_slider_of(layer_no, n));
}

/* One run of one pen: one RectFill, at the view's corner in ud. */
static void put_run(void *ud, int x, int y, int n, int pen)
{
    const int *at = ud;
    SetAPen(win->RPort, (UBYTE)pen);
    RectFill(win->RPort, at[0] + x, at[1] + y, at[0] + x + n - 1, at[1] + y);
}

/* The view on OpenGPU: the part on the plate, and the toolpath on it once
 * sliced. 1, or 0 when it cannot and the pens draw the same picture. */
static int draw_gpu(int x, int y, int w, int h)
{
    gs_mesh none;
    const gs_mesh *part = job.have_part ? &job.part : NULL;
    int nv = 0, nw, need = 0;
    if (job.have_part) need += job.part.n * 3;
    if (job.sliced) need += job.path.n * 6 + 6;
    if (!part) {
        memset(&none, 0, sizeof none);
        part = &none;
    }
    if (!gpu.on || need < 3 || !omv_gpu_size(&gpu, w, h) || !omv_gpu_room(&gpu, need)) return 0;
    if (job.have_part) {
        nv = gs_gpu_vertices_plate(&job.part, &cam, w, h, gpu.verts, gpu.vert_cap);
        if (nv < 0) nv = 0;
    }
    if (job.sliced && nv + 6 <= gpu.vert_cap) {
        int nt = gs_gpu_toolpath_on(&job.path, part, &cam, show_layer ? layer_no : 0, w, h, gpu.verts + nv * 6,
                                    gpu.vert_cap - nv);
        if (nt > 0) nv += nt;
    }
    if (nv < 3) return 0;
    nw = gs_gpu_stream_plate(gpu.stream, gpu.stream_cap, (uint32_t)(uintptr_t)gpu.verts, nv,
                             (uint32_t)(uintptr_t)gpu.rs->rs_Pixels, (int)gpu.rs->rs_BytesPerRow,
                             (uint32_t)(uintptr_t)gpu.depth, w, h, part, &cam, gpu.grid, 1);
    return nw > 0 && omv_gpu_show(&gpu, nw, win->RPort, x, y);
}

/* The part on the plate, and once sliced the toolpath laid on it (layers
 * 1 to the slider's with Show). The pens draw it when OpenGPU cannot.
 * Drag turns it, Shift-drag pans. */
static void draw_view(void *userdata, const olui_el *el, int x, int y, int w, int h)
{
    gs_mesh none;
    const gs_mesh *part;
    unsigned char *rgb;
    int at[2], rc;
    (void)userdata;
    (void)el;
    if (w > 4) vw = w;
    if (h > 4) vh = h;
    if (draw_gpu(x, y, w, h)) return;
    rgb = (unsigned char *)malloc((size_t)w * (size_t)h * 3);
    if (!rgb) return;
    if (job.have_part) part = &job.part;
    else {
        memset(&none, 0, sizeof none);
        part = &none;
    }
    rc = gs_view_bed(part, &cam, rgb, w, h);
    if (!rc && job.sliced) rc = gt_paint_on(&job.path, part, &cam, show_layer ? layer_no : 0, rgb, w, h);
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

/* An STL, loaded onto the bed of the printer picked (220 mm when there is
 * none). The same path Open and a file named at launch take. */
static void open_stl(olui *ui, const char *path)
{
    char err[160];
    om_machine m;
    printer(ui, &m, err, (int)sizeof err);
    if (sl_load(&job, path, m.area[0], m.area[1], err, (int)sizeof err)) {
        omv_say(ui, err);
        return;
    }
    snprintf(stl_path, sizeof stl_path, "%s", path);
    olui_set_text(ui, olui_find(ui, "path"), path);
    gs_cam_home(&cam);
    snprintf(err, sizeof err, "%d triangles. Slice when ready.", job.part.n);
    omv_say(ui, err);
    refresh(ui);
}

static void on_open(olui *ui, const olui_event *ev, void *userdata)
{
    char path[256];
    (void)ev;
    (void)userdata;
    if (!ask_file(ui, "Open an STL", FALSE, "#?.stl", path, (int)sizeof path)) return;
    open_stl(ui, path);
}

/* Support, and the user's own lines, from the Settings side. */
static void take_own(olui *ui)
{
    char start[240], end[240];
    int support = 0;
    sl_ui_job_read(ui, &support, start, (int)sizeof start, end, (int)sizeof end);
    job.support = support;
    sl_own(&job, start, end);
}

static void show_own(olui *ui) { sl_ui_job_show(ui, job.support, job.start, job.end); }

static void on_slice(olui *ui, const olui_event *ev, void *userdata)
{
    gs_settings set;
    om_machine m;
    char err[160];
    int have;
    (void)ev;
    (void)userdata;
    have = printer(ui, &m, err, (int)sizeof err) == 0;
    if (job.have_part) gp_place(&job.part, m.area[0], m.area[1]); /* on the bed of the printer picked */
    if (have) omv_settings_fit(ui, &m, &set, err, (int)sizeof err); /* the fields held to the printer */
    sl_ui_settings(ui, &set);
    take_own(ui);
    omv_say(ui, "Slicing...");
    {
        int tasks = rexx.tasks;
        if (tasks < 1) tasks = 1;
        if (tasks > 64) tasks = 64;
        if (sl_slice(&job, &set, have ? &m : NULL, tasks, err, (int)sizeof err)) {
            omv_say(ui, err);
            olui_repaint(ui, olui_find(ui, "view"), NULL);
            return;
        }
    }
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
    take_own(ui);
    if (sl_write(&job, path, &m, err, (int)sizeof err)) omv_say(ui, err);
    else omv_say(ui, path);
}

static void on_show(olui *ui, const olui_event *ev, void *userdata)
{
    (void)userdata;
    show_layer = ev->checked;
    refresh(ui);
}

/* Another printer picked: the settings held to it, and why said. */
static void on_machine(olui *ui, const olui_event *ev, void *userdata)
{
    om_machine m;
    gs_settings s;
    char err[160];
    (void)ev;
    (void)userdata;
    if (printer(ui, &m, err, (int)sizeof err)) {
        omv_say(ui, err);
        return;
    }
    omv_settings_fit(ui, &m, &s, err, (int)sizeof err);
    if (err[0]) omv_say(ui, err);
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

int main(int argc, char **argv)
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
    if (sl_ui_init(ui, err, (int)sizeof err)) {
        olui_free(ui);
        fprintf(stderr, "GFOpenSlicer: %s\n", err);
        return 20;
    }
    olui_on(ui, "open", OLUI_CLICK, on_open, NULL);
    olui_on(ui, "machine", OLUI_CHANGE, on_machine, NULL);
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
    else on_machine(ui, NULL, NULL);
    {
        char path[256];
        int got = omv_startup_file(argc, argv, path, (int)sizeof path);
        if (got < 0) omv_say(ui, "The file name is too long");
        else if (got) open_stl(ui, path);
    }
    /* ARexx: Wait() on the window and OPENMAKESLICER.1 together. */
    os_rexx_init(&rexx);
    rexx.job = &job;
    rexx.version = "GFOpenSlicer 0.1.0";
    rexx.activate = rexx_activate;
    rexx.hide = rexx_hide;
    rexx.win = w;
    rexx.ui = ui;
    rexx.tasks = gs_par_cores();
    if (rexx.tasks < 1) rexx.tasks = 1;
    if (rexx.tasks > 64) rexx.tasks = 64;
    sl_ui_settings(ui, &rexx.set);
    rexx_port = omr_open(OS_REXX_PORT, rexx.version);
    if (rexx_port) {
        rexx.port_name = omr_name(rexx_port);
        omr_attach(rexx_port, os_rexx_verbs(), os_rexx_nverbs(), &rexx);
    }
    for (;;) {
        int r;
        ULONG mask = 1UL << w->UserPort->mp_SigBit, got, rmask = omr_mask(rexx_port);
        got = Wait(mask | rmask);
        if (rmask && (got & rmask)) {
            omr_cmd *cmd;
            char why[160];
            snprintf(rexx.file, sizeof rexx.file, "%s", stl_path);
            sl_ui_settings(ui, &rexx.set);
            take_own(ui);
            rexx.have_machine = printer(ui, &rexx.machine, why, (int)sizeof why) == 0;
            rexx.layers = show_layer ? 1 : 0;
            rexx.layer_no = layer_no < 1 ? 1 : layer_no;
            rexx.fresh = 0;
            rexx.look = -1;
            while ((cmd = omr_get(rexx_port))) omr_dispatch(cmd, os_rexx_verbs(), os_rexx_nverbs(), &rexx);
            omv_settings_show(ui, &rexx.set);
            show_own(ui);
            if (rexx.have_machine && omv_machines_show(ui, "machine", &printers, rexx.machine.name)) {
                gs_settings held;
                char note[160];
                note[0] = 0;
                omv_settings_fit(ui, &rexx.machine, &held, note, (int)sizeof note);
                if (note[0]) omv_say(ui, note);
            }
            strncpy(stl_path, rexx.file, sizeof stl_path - 1);
            stl_path[sizeof stl_path - 1] = 0;
            if (olui_find(ui, "path")) olui_set_text(ui, olui_find(ui, "path"), stl_path);
            apply_layers(ui);
            if (rexx.look >= 0 && rexx.look <= 3) {
                if (rexx.look == 3) {
                    gs_mesh none;
                    const gs_mesh *part;
                    if (job.have_part) part = &job.part;
                    else {
                        memset(&none, 0, sizeof none);
                        part = &none;
                    }
                    gs_cam_frame(&cam, part, vw, vh);
                } else gs_cam_look(&cam, rexx.look);
                rexx.home = 0;
            } else if (rexx.home) {
                gs_cam_home(&cam);
                rexx.home = 0;
            }
            refresh(ui);
            if (rexx.quit) goto done;
        }
        if (!(got & mask)) continue;
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
    omr_close(rexx_port);
    olui_amiga_free(a);
    CloseWindow(w);
    sl_free(&job);
    omv_machines_free(&printers);
    olui_free(ui);
    omv_gpu_close(&gpu);
    return 0;
}
