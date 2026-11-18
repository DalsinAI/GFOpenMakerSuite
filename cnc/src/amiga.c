/* OpenMakeCNC on the Amiga. The window is GFOpenLayout (markup.h): Open
 * asks for an SVG or a DXF, Preview builds the cut, and Save G-code writes
 * it for the mill picked. The view is the toolpath, drawn with the pens
 * (core/ui). The window is resizable, 640 by 480 inside at the least.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "cnc_job.h"
#include "cnc_ui.h"
#include "markup.h"
#include "omview_machines.h"
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char version[] = "$VER: GFOpenCNC 0.1.0 (18.11.2026) AmiCradle Team";
static const char title[] = "GFOpenCNC 0.1.0";

static struct Window *win;
static char drawing[256];
static gv_doc doc;
static int have_doc;
static gt_path path;
static int have_path;
static gc_stats stats;
static cnc_opts opts;
static omv_machines mills;
static gs_cam cam;
static int dragging, drag_x, drag_y, drag_shift;

static void path_join(char *path, int n, const char *drawer, const char *file)
{
    if (!drawer || !drawer[0]) snprintf(path, (size_t)n, "%s", file ? file : "");
    else if (drawer[strlen(drawer) - 1] == ':' || drawer[strlen(drawer) - 1] == '/')
        snprintf(path, (size_t)n, "%s%s", drawer, file ? file : "");
    else snprintf(path, (size_t)n, "%s/%s", drawer, file ? file : "");
}

static int ask_file(olui *ui, const char *title_text, int save, const char *pattern, char *path, int n)
{
    struct FileRequester *fr;
    int got = 0;
    fr = (struct FileRequester *)AllocAslRequestTags(
        ASL_FileRequest, ASLFR_TitleText, (ULONG)title_text, ASLFR_DoSaveMode, (ULONG)save, ASLFR_DoPatterns, TRUE,
        ASLFR_InitialPattern, (ULONG)(pattern ? pattern : "#?"), TAG_END);
    if (!fr) {
        omv_say(ui, "The file requester did not open");
        return 0;
    }
    if (AslRequest(fr, NULL) && fr->fr_File && fr->fr_File[0]) {
        path_join(path, n, (const char *)fr->fr_Drawer, (const char *)fr->fr_File);
        got = 1;
    }
    FreeAslRequest(fr);
    return got;
}

static int chosen_mill(olui *ui, om_machine *m, char *err, int errlen)
{
    int i = omv_machines_chosen(ui, "machine", &mills);
    if (i < 0) {
        snprintf(err, (size_t)errlen, "No mill is picked.");
        return -1;
    }
    return omv_machines_load(&mills, i, m, err, errlen);
}

static void on_open(olui *ui, const olui_event *ev, void *userdata)
{
    (void)ev;
    (void)userdata;
    if (!ask_file(ui, "Open a drawing", 0, "#?.svg|#?.dxf", drawing, (int)sizeof drawing)) return;
    olui_set_text(ui, olui_find(ui, "path"), drawing);
    cnc_ui_kind(ui, drawing);
    have_doc = 0;
    have_path = 0;
    omv_say(ui, drawing);
}

static void on_mode(olui *ui, const olui_event *ev, void *userdata)
{
    (void)ev;
    cnc_ui_mode(ui, (int)(intptr_t)userdata);
}

static void on_preview(olui *ui, const olui_event *ev, void *userdata)
{
    om_machine m;
    char err[160], line[96];
    const char *file;
    (void)ev;
    (void)userdata;
    file = olui_get_text(olui_find(ui, "path"));
    if (file && file[0]) snprintf(drawing, sizeof drawing, "%s", file);
    if (!drawing[0]) {
        omv_say(ui, "Open a drawing.");
        return;
    }
    cnc_ui_read(ui, &opts);
    err[0] = 0;
    gv_doc_free(&doc);
    gv_doc_init(&doc);
    if (gv_load(&doc, drawing, err, (int)sizeof err)) {
        omv_say(ui, err[0] ? err : "Could not open that drawing");
        return;
    }
    have_doc = 1;
    cnc_ui_kind(ui, drawing);
    if (cnc_path(&path, &doc, &opts, err, (int)sizeof err)) {
        have_path = 0;
        omv_say(ui, err[0] ? err : "Could not plan that cut");
        return;
    }
    have_path = 1;
    if (chosen_mill(ui, &m, err, (int)sizeof err)) m.name[0] = 0;
    cnc_ui_status(line, (int)sizeof line, m.name, opts.mode, path.n, 0.f);
    omv_say(ui, line);
    olui_repaint(ui, olui_find(ui, "view"), NULL);
}

static void on_gcode(olui *ui, const olui_event *ev, void *userdata)
{
    om_machine m;
    char err[160], out[256], line[96];
    const char *file;
    (void)ev;
    (void)userdata;
    file = olui_get_text(olui_find(ui, "path"));
    if (file && file[0]) snprintf(drawing, sizeof drawing, "%s", file);
    if (!drawing[0]) {
        omv_say(ui, "Open a drawing.");
        return;
    }
    if (chosen_mill(ui, &m, err, (int)sizeof err)) {
        omv_say(ui, err[0] ? err : "No mill is picked.");
        return;
    }
    if (!ask_file(ui, "Save G-code", 1, "#?.nc|#?.gcode", out, (int)sizeof out)) return;
    cnc_ui_read(ui, &opts);
    err[0] = 0;
    if (cnc_write(drawing, out, &m, &opts, &stats, err, (int)sizeof err)) {
        omv_say(ui, err[0] ? err : "Could not write there");
        return;
    }
    have_path = 1;
    cnc_ui_status(line, (int)sizeof line, m.name, opts.mode, stats.moves, stats.seconds);
    omv_say(ui, line);
}

static void put_run(void *ud, int x, int y, int n, int pen)
{
    const int *at = ud;
    SetAPen(win->RPort, (UBYTE)pen);
    RectFill(win->RPort, at[0] + x, at[1] + y, at[0] + x + n - 1, at[1] + y);
}

static void draw_view(void *userdata, const olui_el *el, int x, int y, int w, int h)
{
    unsigned char *rgb;
    int at[2];
    (void)userdata;
    (void)el;
    rgb = (unsigned char *)malloc((size_t)w * (size_t)h * 3);
    if (!rgb) return;
    if (have_path) gt_paint_part(&path, &cam, 0, rgb, w, h);
    else {
        gs_mesh none;
        memset(&none, 0, sizeof none);
        gs_view_bed(&none, &cam, rgb, w, h);
    }
    at[0] = x;
    at[1] = y;
    omv_pens(rgb, w, h, put_run, at);
    free(rgb);
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
    gv_doc_init(&doc);
    gt_init(&path);
    cnc_defaults(&opts);
    if (!ui || !olui_load(ui, cnc_markup, cnc_css, err, (int)sizeof err)) {
        if (ui) olui_free(ui);
        fprintf(stderr, "GFOpenCNC: %s\n", ui ? err : "GFOpenLayout is not open");
        return 20;
    }
    cnc_ui_mode(ui, CNC_PROFILE);
    memset(&mills, 0, sizeof mills);
    if (omv_machines_scan(&mills, "PROGDIR:machines", "cnc") < 0) mills.n = 0;
    omv_machines_fill(ui, "machine", &mills, "No mills");
    olui_on(ui, "open", OLUI_CLICK, on_open, NULL);
    olui_on(ui, "preview", OLUI_CLICK, on_preview, NULL);
    olui_on(ui, "gcode", OLUI_CLICK, on_gcode, NULL);
    olui_on(ui, "profile", OLUI_CLICK, on_mode, (void *)(intptr_t)CNC_PROFILE);
    olui_on(ui, "pocket", OLUI_CLICK, on_mode, (void *)(intptr_t)CNC_POCKET);
    olui_on(ui, "drill", OLUI_CLICK, on_mode, (void *)(intptr_t)CNC_DRILL);
    gs_cam_home(&cam);
    w = omv_window_open(title, olui_amiga_idcmp());
    if (!w) {
        olui_free(ui);
        omv_machines_free(&mills);
        fprintf(stderr, "GFOpenCNC: the window did not open\n");
        return 20;
    }
    win = w;
    a = olui_amiga_new(ui, w);
    if (!a) {
        CloseWindow(w);
        olui_free(ui);
        omv_machines_free(&mills);
        fprintf(stderr, "GFOpenCNC: GFOpenLayout could not draw the window\n");
        return 20;
    }
    olui_amiga_theme(a, NULL);
    olui_amiga_own(a, olui_find(ui, "view"), draw_view, NULL);
    olui_amiga_layout(a);
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
            } else if (cls == IDCMP_MOUSEBUTTONS && code == SELECTUP)
                dragging = 0;
            else if (cls == IDCMP_MOUSEMOVE && dragging) {
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
    if (a) olui_amiga_free(a);
    CloseWindow(w);
    olui_free(ui);
    gv_doc_free(&doc);
    gt_free(&path);
    omv_machines_free(&mills);
    return 0;
}
