/* GFOpenMakerWorkshop on the Amiga. The window is GFOpenLayout
 * (markup.h): New asks for a drawer, Open for a project.om (or the
 * drawer), Save writes it, the left list is the product's items, and
 * Launch starts the kind's program (the host hook when that app's port
 * is not up; OPENITEM uses the port when it is).
 * The view is the picked item (STL, SVG/DXF or G-code) drawn on the
 * CPU and put down with the pens (core/ui). The window is resizable,
 * 640 by 480 inside at the least.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "workshop_ui.h"
#include "workshop_rexx.h"
#include "omview_ui.h"
#include "omview_window.h"

#include <devices/inputevent.h>
#include <dos/dostags.h>
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

static const char version[] = "$VER: GFOpenWorkshop 0.1.0 (17.11.2026) AmiCradle Team";
static const char title[] = "GFOpenMakerWorkshop 0.1.0";

static struct Window *win;
static workshop ws;
static gs_cam cam;
static int dragging, drag_x, drag_y, drag_shift;

static gw_rexx rexx;
static omr_port *rexx_port;

static void rexx_activate(void *window)
{
    WindowToFront((struct Window *)window);
    ActivateWindow((struct Window *)window);
}

static void rexx_hide(void *window) { WindowToBack((struct Window *)window); }

static void path_join(char *path, int n, const char *drawer, const char *file)
{
    size_t len = drawer ? strlen(drawer) : 0;
    int slash = len && (drawer[len - 1] == '/' || drawer[len - 1] == ':');
    snprintf(path, (size_t)n, slash ? "%s%s" : "%s/%s", drawer ? drawer : "", file ? file : "");
}

static void say_err(olui *ui, const char *err)
{
    snprintf(ws.status, sizeof ws.status, "%s", err);
    omv_say(ui, ws.status);
}

static void refresh(olui *ui)
{
    workshop_ui_refresh(ui, &ws);
    olui_repaint(ui, olui_find(ui, "view"), NULL);
}

static void remember(void)
{
    if (!ws.have) return;
    gpr_recent_touch(&ws.recent, ws.project.path);
    if (ws.recent_path[0]) {
        char err[160];
        gpr_recent_save(&ws.recent, ws.recent_path, err, (int)sizeof err);
    }
}

static void recent_path_init(void)
{
    BPTR lock = GetProgramDir();
    char dir[256];
    if (lock && NameFromLock(lock, dir, sizeof dir))
        path_join(ws.recent_path, (int)sizeof ws.recent_path, dir, "workshop-recent.om");
    else
        snprintf(ws.recent_path, sizeof ws.recent_path, "PROGDIR:workshop-recent.om");
}

static int ask_file(olui *ui, const char *title_text, int save, int drawers, const char *pattern, char *path, int n)
{
    struct FileRequester *fr;
    int got = 0;
    fr = (struct FileRequester *)AllocAslRequestTags(
        ASL_FileRequest, ASLFR_TitleText, (ULONG)title_text, ASLFR_DoSaveMode, (ULONG)save, ASLFR_DrawersOnly,
        (ULONG)drawers, ASLFR_DoPatterns, pattern && pattern[0] ? TRUE : FALSE, ASLFR_InitialPattern,
        (ULONG)(pattern ? pattern : "#?"), TAG_END);
    if (!fr) {
        say_err(ui, "The file requester did not open");
        return 0;
    }
    if (AslRequest(fr, NULL) && fr->fr_Drawer) {
        if (drawers) {
            if (fr->fr_File && fr->fr_File[0])
                path_join(path, n, (const char *)fr->fr_Drawer, (const char *)fr->fr_File);
            else {
                snprintf(path, (size_t)n, "%s", (const char *)fr->fr_Drawer);
                if (n > 1) {
                    size_t L = strlen(path);
                    if (L && (path[L - 1] == '/' || path[L - 1] == ':')) path[L - 1] = 0;
                }
            }
            got = 1;
        } else if (fr->fr_File && fr->fr_File[0]) {
            path_join(path, n, (const char *)fr->fr_Drawer, (const char *)fr->fr_File);
            got = 1;
        }
    }
    FreeAslRequest(fr);
    return got;
}

static void pull_fields(olui *ui)
{
    const char *name, *notes;
    if (!ws.have) return;
    name = olui_get_text(olui_find(ui, "product"));
    notes = olui_get_text(olui_find(ui, "notes"));
    if (name) gpr_set_name(&ws.project, name);
    if (notes) gpr_set_notes(&ws.project, notes);
}

static void on_new(olui *ui, const olui_event *ev, void *userdata)
{
    char path[256], err[160];
    const char *name;
    (void)ev;
    (void)userdata;
    if (!ask_file(ui, "New project drawer", TRUE, 1, NULL, path, (int)sizeof path)) return;
    name = olui_get_text(olui_find(ui, "product"));
    if (name && !name[0]) name = NULL;
    gpr_free(&ws.project);
    gpr_init(&ws.project);
    ws.have = 0;
    ws.picked = -1;
    if (gpr_create(&ws.project, path, name, err, (int)sizeof err)) {
        say_err(ui, err);
        refresh(ui);
        return;
    }
    ws.have = 1;
    ws.status[0] = 0;
    remember();
    refresh(ui);
}

static void on_open(olui *ui, const olui_event *ev, void *userdata)
{
    char path[256], err[160];
    (void)ev;
    (void)userdata;
    if (!ask_file(ui, "Open a project", FALSE, 0, "#?.(om)", path, (int)sizeof path)) return;
    if (gpr_open(&ws.project, path, err, (int)sizeof err)) {
        say_err(ui, err);
        return;
    }
    ws.have = 1;
    ws.picked = ws.project.n ? 0 : -1;
    ws.status[0] = 0;
    gs_cam_home(&cam);
    remember();
    refresh(ui);
}

static void on_save(olui *ui, const olui_event *ev, void *userdata)
{
    char err[160];
    (void)ev;
    (void)userdata;
    if (!ws.have) return;
    pull_fields(ui);
    if (gpr_save(&ws.project, err, (int)sizeof err)) {
        say_err(ui, err);
        return;
    }
    ws.status[0] = 0;
    remember();
    refresh(ui);
}

static void on_product(olui *ui, const olui_event *ev, void *userdata)
{
    const char *name = olui_get_text(olui_find(ui, "product"));
    (void)ev;
    (void)userdata;
    if (!ws.have || !name) return;
    gpr_set_name(&ws.project, name);
}

static void on_notes(olui *ui, const olui_event *ev, void *userdata)
{
    const char *notes = olui_get_text(olui_find(ui, "notes"));
    (void)ev;
    (void)userdata;
    if (!ws.have || !notes) return;
    gpr_set_notes(&ws.project, notes);
}

static void on_recent(olui *ui, const olui_event *ev, void *userdata)
{
    char err[160];
    (void)userdata;
    if (!ev->value || !ev->value[0]) return;
    if (ws.have && !strcmp(ev->value, ws.project.path)) return;
    if (gpr_open(&ws.project, ev->value, err, (int)sizeof err)) {
        say_err(ui, err);
        return;
    }
    ws.have = 1;
    ws.picked = ws.project.n ? 0 : -1;
    ws.status[0] = 0;
    gs_cam_home(&cam);
    remember();
    refresh(ui);
}

static void on_items(olui *ui, const olui_event *ev, void *userdata)
{
    int at;
    (void)userdata;
    if (!ev->value || ev->value[0] < '0' || ev->value[0] > '9') return;
    at = atoi(ev->value);
    if (at == ws.picked) return;
    if (at < 0 || at >= ws.project.n) return;
    ws.picked = at;
    ws.status[0] = 0;
    gs_cam_home(&cam);
    refresh(ui);
}

static void on_add(olui *ui, const olui_event *ev, void *userdata)
{
    char path[256], err[160];
    const char *kind;
    gpr_add_opts o;
    int at;
    (void)ev;
    (void)userdata;
    if (!ws.have) return;
    if (!ask_file(ui, "Add a file", FALSE, 0, "#?", path, (int)sizeof path)) return;
    {
        olui_el *sel = olui_find(ui, "addkind");
        int ix = olui_select_index(sel);
        kind = ix >= 0 ? olui_select_option_value(sel, ix) : NULL;
    }
    if (!kind || !kind[0]) kind = gpr_kind_suggest(path);
    if (!kind) {
        say_err(ui, "pick a kind for this file");
        return;
    }
    pull_fields(ui);
    gpr_add_defaults(&o);
    o.kind = kind;
    o.file = path;
    at = gpr_add(&ws.project, &o, err, (int)sizeof err);
    if (at < 0) {
        say_err(ui, err);
        return;
    }
    ws.picked = at;
    ws.status[0] = 0;
    gs_cam_home(&cam);
    refresh(ui);
}

static void on_remove(olui *ui, const olui_event *ev, void *userdata)
{
    (void)ev;
    (void)userdata;
    if (!ws.have || ws.picked < 0) return;
    gpr_remove(&ws.project, ws.picked);
    ws.picked = -1;
    ws.status[0] = 0;
    refresh(ui);
}

static void on_rename(olui *ui, const olui_event *ev, void *userdata)
{
    const char *name = olui_get_text(olui_find(ui, "itemname"));
    (void)ev;
    (void)userdata;
    if (!ws.have || ws.picked < 0 || !name || !name[0]) return;
    if (gpr_rename(&ws.project, ws.picked, name) < 0) {
        say_err(ui, "That name can't be used");
        return;
    }
    ws.status[0] = 0;
    refresh(ui);
}

static void on_launch(olui *ui, const olui_event *ev, void *userdata)
{
    char err[160], used[64];
    (void)ev;
    (void)userdata;
    if (gw_open_item(&ws, ws.picked, used, (int)sizeof used, err, (int)sizeof err)) {
        say_err(ui, err[0] ? err : "the app did not start");
        return;
    }
    snprintf(ws.status, sizeof ws.status, "Opened in %s", used[0] ? used : "the app");
    omv_say(ui, ws.status);
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
    char err[160];
    (void)userdata;
    (void)el;
    rgb = (unsigned char *)malloc((size_t)w * (size_t)h * 3);
    if (!rgb) return;
    workshop_ui_preview(&ws, &cam, rgb, w, h, err, (int)sizeof err);
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
    workshop_init(&ws);
    recent_path_init();
    if (ws.recent_path[0]) gpr_recent_load(&ws.recent, ws.recent_path, err, (int)sizeof err);
    if (!ui || !workshop_ui_load(ui, err, (int)sizeof err)) {
        if (ui) olui_free(ui);
        workshop_free(&ws);
        fprintf(stderr, "GFOpenWorkshop: %s\n", ui ? err : "GFOpenLayout is not open");
        return 20;
    }
    olui_on(ui, "new", OLUI_CLICK, on_new, NULL);
    olui_on(ui, "open", OLUI_CLICK, on_open, NULL);
    olui_on(ui, "save", OLUI_CLICK, on_save, NULL);
    olui_on(ui, "product", OLUI_CHANGE, on_product, NULL);
    olui_on(ui, "notes", OLUI_CHANGE, on_notes, NULL);
    olui_on(ui, "recent", OLUI_CHANGE, on_recent, NULL);
    olui_on(ui, "items", OLUI_SELECT, on_items, NULL);
    olui_on(ui, "add", OLUI_CLICK, on_add, NULL);
    olui_on(ui, "remove", OLUI_CLICK, on_remove, NULL);
    olui_on(ui, "rename", OLUI_CLICK, on_rename, NULL);
    olui_on(ui, "launch", OLUI_CLICK, on_launch, NULL);
    gs_cam_home(&cam);
    /* Resizable, 640 by 480 inside at the least: a PAL screen, or more on RTG. */
    w = omv_window_open(title, olui_amiga_idcmp());
    if (!w) {
        olui_free(ui);
        workshop_free(&ws);
        fprintf(stderr, "GFOpenWorkshop: the window did not open\n");
        return 20;
    }
    win = w;
    a = olui_amiga_new(ui, w);
    if (!a) {
        CloseWindow(w);
        olui_free(ui);
        workshop_free(&ws);
        fprintf(stderr, "GFOpenWorkshop: GFOpenLayout could not draw the window\n");
        return 20;
    }
    olui_amiga_theme(a, NULL);
    olui_amiga_own(a, olui_find(ui, "view"), draw_view, NULL);
    workshop_ui_refresh(ui, &ws);
    olui_amiga_layout(a);
    gw_rexx_init(&rexx);
    rexx.ws = &ws;
    rexx.version = "GFOpenMakerWorkshop 0.1.0";
    rexx.activate = rexx_activate;
    rexx.hide = rexx_hide;
    rexx.win = w;
    rexx.ui = ui;
    rexx_port = omr_open(GW_REXX_PORT, rexx.version);
    if (rexx_port) {
        rexx.port_name = omr_name(rexx_port);
        omr_attach(rexx_port, gw_rexx_verbs(), gw_rexx_nverbs(), &rexx);
    }
    for (;;) {
        int r;
        ULONG mask = 1UL << w->UserPort->mp_SigBit, got, rmask = omr_mask(rexx_port);
        got = Wait(mask | rmask);
        if (rmask && (got & rmask)) {
            omr_cmd *cmd;
            while ((cmd = omr_get(rexx_port))) omr_dispatch(cmd, gw_rexx_verbs(), gw_rexx_nverbs(), &rexx);
            workshop_ui_refresh(ui, &ws);
            olui_repaint(ui, olui_find(ui, "view"), NULL);
            if (rexx.quit) goto done;
        }
        if (!(got & mask)) {
            olui_amiga_draw(a);
            continue;
        }
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
    if (rexx_port) omr_close(rexx_port);
    if (a) olui_amiga_free(a);
    CloseWindow(w);
    olui_free(ui);
    workshop_free(&ws);
    return 0;
}
