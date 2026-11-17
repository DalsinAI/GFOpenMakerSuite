/* GFOpenMake on the Amiga. A 20 mm cube with a hole through it, on a 220 mm
 * plate, in a GFOpenLayout window: the first of up to 24 parts, as the
 * bench's list (gs_parts.h). A click picks a part. Orbit turns the view.
 * Move, rotate and scale change the picked part. Drop, Centre and Lay flat
 * put it on the bed. Join, Cut and Intersect take the picked part as the
 * tool and the one before it as the stock (ge_combine).
 *
 * The triangles are submitted as OGPU_OP_TRIANGLES, with a depth buffer,
 * whenever gfopengpu.library can draw them. The picture is shown through
 * a retained surface, so a chip and the library's CPU back end take the
 * same batch. The pen drawer is only the fallback.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gs_edit.h"
#include "gs_gcode.h"
#include "gs_gpu.h"
#include "gs_layer.h"
#include "gs_model.h"
#include "gs_par.h"
#include "gs_plan.h"
#include "gs_settings.h"
#include "make_rexx.h"
#include "make_ui.h"
#include "omview_gpu.h"
#include "omview_machines.h"
#include "omview_settings.h"
#include "omview_ui.h"
#include "omview_window.h"

#include <devices/inputevent.h>
#include <proto/asl.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <intuition/intuition.h>
#include <libraries/asl.h>
#include <exec/libraries.h>
#include <exec/memory.h>

#include <gfopenlayout_ui.h>
#include <gfopenlayout_ui_amiga.h>

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char version[] = "$VER: GFOpenMake 0.1.0 (8.11.2026) AmiCradle Team";

/* The bench's parts (gs_parts.h): up to 24, one picked; shown is every
 * shown part's triangles, for the view, picking and the slicer. */
static gs_parts *parts;
static gs_mesh shown, none;
static gs_tint tints[GS_PARTS_MAX]; /* shown's faces in each part's metal */
static int ntints;
static gs_cam cam;
static struct Window *win;
static int dragging, drag_x, drag_y, drag_shift, drag_moved;
static int gpu_on, on_chip, draw_grid, presented;
static int tool, picked = -1, pushed, vw = 640, vh = 320, said_chip;
static ge_steps *steps;

/* ARexx: the port GFOPENMAKE.1, and the command table in make_rexx.c. */
static gm_rexx rexx;
static omr_port *rexx_port;

static void rexx_activate(void *win)
{
    WindowToFront((struct Window *)win);
    ActivateWindow((struct Window *)win);
}

static void rexx_hide(void *win) { WindowToBack((struct Window *)win); }
static char stl_path[256] = "RAM:hole.stl";
/* The Layers group: the height, whether a layer is shown, which, and its walls. */
static float layer_h = 0.2f;
static int show_layer, layer_no = 1;
static gl_walls walls;
/* The Slice group: the part as a toolpath, and what it came to. */
static gt_path sliced;
static int has_slice;
static gc_stats slice_st;
static int slice_how; /* gc_estimate_more's: GC_FELL_BACK, the time short */
/* The printers under PROGDIR:machines/printer, for Save G-code. */
static omv_machines printers;
/* The view on OpenGPU (core/ui): its picture, depth, vertices and stream. */
static omv_gpu gpu;

/* The picked part, or an empty mesh with none (nothing then changes). */
static gs_mesh *work(void)
{
    gs_mesh *m = gs_parts_mesh(parts, gs_parts_picked(parts));
    if (m) return m;
    gs_mesh_free(&none);
    return &none;
}

static const char *work_name(void)
{
    const char *n = gs_parts_name(parts, gs_parts_picked(parts));
    return n ? n : "No part";
}

static void rebuild_shown(void)
{
    gs_parts_scene(parts, &shown);
    ntints = gm_tints(parts, tints, GS_PARTS_MAX);
}

/* Section: the picture hides whatever is above section_z. The parts are
 * not cut. Split does that. */
static int section_on;
static float section_z = 10.f;
static gs_mesh section_mesh;

static const gs_mesh *view_of(void)
{
    if (!section_on) return &shown;
    gs_mesh_free(&section_mesh);
    if (ge_clip_below(&section_mesh, &shown, section_z)) return &shown;
    return &section_mesh;
}

static void say(olui *ui, const char *text)
{
    omv_say(ui, text);
}

static void verts_fit(int extra);

static int draw_gpu(int x, int y, int w, int h)
{
    int nv, nw;
    const gs_mesh *vm;
    if (!gpu_on || !omv_gpu_size(&gpu, w, h)) return 0;
    rebuild_shown();
    vm = view_of();
    if (has_slice) {
        /* The toolpath alone, fitted to the part, with no grid. */
        if (!omv_gpu_room(&gpu, sliced.n * 6 + 6)) return 0;
        nv = gs_gpu_toolpath(&sliced, &cam, show_layer ? layer_no : 0, 1, w, h, gpu.verts, gpu.vert_cap);
    } else {
        verts_fit(show_layer ? walls.n * 6 : 0);
        nv = gs_gpu_vertices_plate_tint(vm, &cam, w, h, section_on ? NULL : tints, section_on ? 0 : ntints, gpu.verts,
                                        gpu.vert_cap);
    }
    if (nv < 3) return 0;
    /* The layer's walls go after the part's, in the same batch. */
    if (!has_slice && show_layer && walls.n) {
        int nw2 = gs_gpu_walls_plate(&walls, &shown, &cam, w, h, gpu.verts + nv * 6, gpu.vert_cap - nv);
        if (nw2 > 0) nv += nw2;
    }
    nw = gs_gpu_stream_plate(gpu.stream, gpu.stream_cap, (uint32_t)(uintptr_t)gpu.verts, nv,
                             (uint32_t)(uintptr_t)gpu.rs->rs_Pixels, (int)gpu.rs->rs_BytesPerRow,
                             (uint32_t)(uintptr_t)gpu.depth, w, h, vm, &cam, has_slice ? 0 : draw_grid, 1);
    if (!omv_gpu_show(&gpu, nw, win->RPort, x, y)) return 0;
    presented = 1;
    return 1;
}

/* One run of one pen: one RectFill, at the view's corner in ud. */
static void put_run(void *ud, int x, int y, int n, int pen)
{
    const int *at = ud;
    SetAPen(win->RPort, (UBYTE)pen);
    RectFill(win->RPort, at[0] + x, at[1] + y, at[0] + x + n - 1, at[1] + y);
}

static void draw_pens(int x, int y, int w, int h)
{
    unsigned char *rgb;
    int at[2];
    const gs_mesh *vm;
    rgb = (unsigned char *)malloc((size_t)w * (size_t)h * 3);
    if (!rgb) return;
    vw = w;
    vh = h;
    rebuild_shown();
    vm = view_of();
    if (has_slice ? gt_paint_part(&sliced, &cam, show_layer ? layer_no : 0, rgb, w, h) : gs_view_bed(vm, &cam, rgb, w, h)) {
        free(rgb);
        return;
    }
    if (!has_slice && show_layer) gl_paint(&walls, &shown, &cam, rgb, w, h);
    /* The pens keep the way-coloured picture, not the metals: four pens
     * cannot hold six metals, and a dark metal's faces would go to the
     * background's pen. Grey goes to the three greys, the walls' blue to
     * the window's fourth pen, the Workbench blue. */
    at[0] = x;
    at[1] = y;
    omv_pens(rgb, w, h, put_run, at);
    free(rgb);
}

/* Measure's points, and the line between them once there are two. */
static gs_v mark[2];
static int marks;

static void draw_marks(int x, int y, int w, int h)
{
    float px[2], py[2];
    int i, in = 0;
    struct RastPort *rp = win->RPort;
    if (has_slice) return;
    for (i = 0; i < marks; i++) {
        if (ge_project(&shown, &cam, w, h, mark[i], &px[i], &py[i]) ||
            px[i] < 2.f || py[i] < 2.f || px[i] > (float)(w - 3) || py[i] > (float)(h - 3)) continue;
        in |= 1 << i;
    }
    SetAPen(rp, 3);
    if (in == 3) {
        Move(rp, x + (int)px[0], y + (int)py[0]);
        Draw(rp, x + (int)px[1], y + (int)py[1]);
    }
    SetAPen(rp, 2);
    for (i = 0; i < marks; i++) {
        if (in & (1 << i)) RectFill(rp, x + (int)px[i] - 2, y + (int)py[i] - 2, x + (int)px[i] + 2, y + (int)py[i] + 2);
    }
}

static void draw_view(void *userdata, const olui_el *el, int x, int y, int w, int h)
{
    (void)userdata;
    (void)el;
    vw = w;
    vh = h;
    if (!draw_gpu(x, y, w, h)) draw_pens(x, y, w, h);
    draw_marks(x, y, w, h);
}

enum { TOOL_ORBIT, TOOL_MOVE, TOOL_ROT, TOOL_SCALE, TOOL_MEASURE, TOOL_PULL };

/* Place's Size and At: the picked part's box and its bottom middle, in
 * tenths of a mm (the fields' one place). Setting them sends nothing. */
static const char *const place_ids[6] = {"sx", "sy", "sz", "px", "py", "pz"};

static int tenths(float v)
{
    return (int)(v < 0.f ? v * 10.f - 0.5f : v * 10.f + 0.5f);
}

static void face_line(olui *ui)
{
    char buf[32];
    float pos[3], size[3];
    int k;
    if (picked >= 0) snprintf(buf, sizeof buf, "face %d", picked);
    else snprintf(buf, sizeof buf, "no face");
    olui_set_text(ui, olui_find(ui, "face"), buf);
    ge_where(work(), pos, size);
    for (k = 0; k < 3; k++) {
        olui_set_number(ui, olui_find(ui, place_ids[k]), tenths(size[k]));
        olui_set_number(ui, olui_find(ui, place_ids[3 + k]), tenths(pos[k]));
    }
}

static const char *tool_label(void)
{
    if (tool == TOOL_MOVE) return "Move. Drag slides. Shift-drag lifts.";
    if (tool == TOOL_ROT) return "Rotate. Drag turns the part.";
    if (tool == TOOL_SCALE) return "Scale. Drag up grows it.";
    if (tool == TOOL_MEASURE) return "Measure. Click two points on the part or bed.";
    if (tool == TOOL_PULL) return "Pull. Drag the face along its normal. Escape leaves.";
    return "Orbit. Drag turns the view. A click picks a face.";
}

/* Cuts the shown layer again, after the part or the height changed. */
static void recut(void)
{
    int n;
    rebuild_shown();
    n = gl_layers(&shown, layer_h);
    if (layer_no > n) layer_no = n;
    if (layer_no < 1) layer_no = 1;
    if (show_layer) gl_cut(&shown, layer_h, layer_no, &walls);
    else gl_walls_free(&walls);
}

/* The Parts tab from the list: the list's lines, the picked part's
 * name, metal and Hide or Show. Remade only when the list has changed. */
static void sync_parts(olui *ui)
{
    static unsigned long was;
    unsigned long h = 5381;
    int n = gs_parts_count(parts), at = gs_parts_picked(parts), i;
    const char *p;
    for (i = 0; i < n; i++) {
        for (p = gs_parts_name(parts, i); *p; p++) h = h * 33 + (unsigned char)*p;
        h = h * 33 + (unsigned long)(gs_parts_shown(parts, i) * 8 + gs_parts_colour(parts, i));
    }
    h = h * 33 + (unsigned long)(n * GS_PARTS_MAX + at);
    if (h == was) return;
    was = h;
    gm_parts_list(ui, parts);
}

static void refresh(olui *ui)
{
    char status[180];
    sync_parts(ui);
    olui_set_disabled(ui, olui_find(ui, "undo"), !ge_steps_back(steps));
    olui_set_disabled(ui, olui_find(ui, "redo"), !ge_steps_ahead(steps));
    recut();
    if (has_slice) {
        omv_slice_status(status, (int)sizeof status, sliced.nlayers, slice_st.moves, slice_st.seconds,
                         slice_st.filament, show_layer ? layer_no : 0, (slice_how & GC_FELL_BACK) != 0);
        say(ui, status);
        face_line(ui);
        olui_repaint(ui, olui_find(ui, "view"), NULL);
        return;
    }
    if (show_layer) {
        gl_status(status, (int)sizeof status, &shown, layer_h, layer_no, &walls);
        say(ui, status);
        face_line(ui);
        olui_repaint(ui, olui_find(ui, "view"), NULL);
        return;
    }
    if (tool == TOOL_MEASURE && marks == 2) ge_measure_line(status, (int)sizeof status, mark[0], mark[1]);
    else if (tool == TOOL_MEASURE && marks == 1) snprintf(status, sizeof status, "First point set. Click the second.");
    else if (tool == TOOL_MEASURE) ge_facts_line(status, (int)sizeof status, work());
    else snprintf(status, sizeof status, "%s, %d triangles%s. %s",
             work_name(),
             work()->n,
             presented ? (on_chip ? ", chip" : ", OpenGPU") : "", tool_label());
    say(ui, status);
    face_line(ui);
    olui_repaint(ui, olui_find(ui, "view"), NULL);
}

static void drop_slice(void)
{
    gt_free(&sliced);
    has_slice = 0;
}

/* A step for Undo, before a change: the whole list, so Undo after Join
 * gives both parts back. */
static void remember(void)
{
    drop_slice();
    marks = 0;
    if (steps && gs_parts_count(parts)) ge_steps_push(steps, parts);
}

/* Takes back the step remember() just made. */
static void forget(void)
{
    gs_parts *was = ge_steps_forget(steps, parts);
    if (was) parts = was;
    marks = 0;
}

static void verts_fit(int extra)
{
    int i, need = 0;
    for (i = 0; i < gs_parts_count(parts); i++) need += gs_parts_mesh(parts, i)->n;
    omv_gpu_room(&gpu, need * 3 + extra);
}

static void path_join(char *path, int n, const char *drawer, const char *file)
{
    size_t len = drawer ? strlen(drawer) : 0;
    int slash = len && (drawer[len - 1] == '/' || drawer[len - 1] == ':');
    snprintf(path, (size_t)n, slash ? "%s%s" : "%s/%s", drawer ? drawer : "", file ? file : "");
}

static void on_open(olui *ui, const olui_event *ev, void *userdata)
{
    struct FileRequester *fr;
    char path[256], err[160];
    gs_mesh next;
    (void)ev;
    (void)userdata;
    fr = (struct FileRequester *)AllocAslRequestTags(ASL_FileRequest,
        ASLFR_TitleText, (ULONG)"Open an STL",
        ASLFR_DoPatterns, TRUE,
        ASLFR_InitialPattern, (ULONG)"#?.stl",
        TAG_END);
    if (!fr) {
        say(ui, "The file requester did not open");
        return;
    }
    if (AslRequest(fr, NULL) && fr->fr_Drawer && fr->fr_File) {
        path_join(path, (int)sizeof path, (const char *)fr->fr_Drawer, (const char *)fr->fr_File);
        memset(&next, 0, sizeof next);
        if (ge_open(&next, path, err, (int)sizeof err)) {
            say(ui, err[0] ? err : "Could not open that STL");
        } else {
            if (gs_parts_count(parts) >= GS_PARTS_MAX) {
                gs_mesh_free(&next);
                say(ui, "Twenty-four parts is the limit.");
                FreeAslRequest(fr);
                return;
            }
            remember();
            gs_parts_add(parts, &next, (const char *)fr->fr_File);
            picked = -1;
            strncpy(stl_path, path, sizeof stl_path - 1);
            stl_path[sizeof stl_path - 1] = 0;
            verts_fit(0);
            refresh(ui);
        }
    }
    FreeAslRequest(fr);
}

static void on_save(olui *ui, const olui_event *ev, void *userdata)
{
    struct FileRequester *fr;
    char path[256];
    (void)ev;
    (void)userdata;
    fr = (struct FileRequester *)AllocAslRequestTags(ASL_FileRequest,
        ASLFR_TitleText, (ULONG)"Save an STL",
        ASLFR_DoSaveMode, TRUE,
        ASLFR_DoPatterns, TRUE,
        ASLFR_InitialPattern, (ULONG)"#?.stl",
        TAG_END);
    if (!fr) {
        say(ui, "The file requester did not open");
        return;
    }
    if (AslRequest(fr, NULL) && fr->fr_Drawer && fr->fr_File) {
        path_join(path, (int)sizeof path, (const char *)fr->fr_Drawer, (const char *)fr->fr_File);
        if (gm_stl_write(work(), path)) say(ui, "Could not save that STL");
        else {
            strncpy(stl_path, path, sizeof stl_path - 1);
            stl_path[sizeof stl_path - 1] = 0;
            say(ui, path);
        }
    }
    FreeAslRequest(fr);
}

enum { PART_SHOW = 6, PART_DELETE };

/* Undo (userdata 0) and Redo (1): the list as it was, or as it was again. */
static void on_undo(olui *ui, const olui_event *ev, void *userdata)
{
    int redo = (int)(intptr_t)userdata;
    gs_parts *was = redo ? ge_steps_redo(steps, parts) : ge_steps_undo(steps, parts);
    (void)ev;
    if (!was) {
        say(ui, redo ? "Nothing to redo" : "Nothing to undo");
        return;
    }
    parts = was;
    marks = 0;
    drop_slice();
    picked = -1;
    verts_fit(0);
    refresh(ui);
}

/* The chosen tool's button is :selected, lit in the accent. */
static void light_tool(olui *ui)
{
    static const char *const ids[] = { "orbit", "move", "rot", "scale", "measure", "fdrag" };
    int i;
    for (i = 0; i < 6; i++) olui_set_selected(ui, olui_find(ui, ids[i]), i == tool);
}

static void on_tool(olui *ui, const olui_event *ev, void *userdata)
{
    (void)ev;
    tool = (int)(intptr_t)userdata;
    marks = 0;
    light_tool(ui);
    refresh(ui);
}

/* Drag to pull, as the bench's Pull mode (P, or Face's Drag): a drag in
 * the view pulls the picked face along its normal, by as many mm as the
 * pointer goes that way (ge_face_axis, ge_drag_mm), the part redrawn as it
 * goes; one step for Undo. Escape, or Drag again, leaves it. */
static gs_mesh pull_base; /* the part as the drag found it */
static float pull_sx, pull_sy, pull_mm;
static int pull_on, pull_x, pull_y;

static void set_pull(olui *ui, int on)
{
    if (on && (picked < 0 || !work()->n)) {
        say(ui, "Click a face on the part, then Drag.");
        return;
    }
    if (!on && tool != TOOL_PULL) return;
    tool = on ? TOOL_PULL : TOOL_ORBIT;
    marks = 0;
    light_tool(ui);
    refresh(ui);
    say(ui, on ? "Drag to pull that face along its normal. Escape leaves Pull." : "Pull is off.");
}

static void on_fdrag(olui *ui, const olui_event *ev, void *userdata)
{
    (void)ev;
    (void)userdata;
    set_pull(ui, tool != TOOL_PULL);
}

/* The drag starts at x, y on the screen: 0, or -1 and why said. */
static int pull_start(olui *ui, int x, int y)
{
    rebuild_shown();
    if (has_slice || picked < 0 || picked >= work()->n) {
        say(ui, "Click a face on the part, then drag it.");
        return -1;
    }
    if (ge_face_axis(&shown, work(), picked, &cam, vw, vh, &pull_sx, &pull_sy)) {
        say(ui, "Turn the view a little. Pull cannot see along that face.");
        return -1;
    }
    if (ge_copy(&pull_base, work())) return -1;
    pull_on = 1;
    pull_mm = 0.f;
    pull_x = x;
    pull_y = y;
    return 0;
}

/* The pointer at x, y: the part as the drag found it, pulled that far. */
static void pull_move(olui *ui, int x, int y)
{
    char line[96];
    float mm = ge_drag_mm(pull_sx, pull_sy, (float)(x - pull_x), (float)(y - pull_y));
    if (mm == pull_mm) return;
    if (!pushed) {
        remember();
        pushed = 1;
    }
    if (ge_copy(work(), &pull_base)) return;
    pull_mm = mm;
    if (mm == 0.f) snprintf(line, sizeof line, "Drag along the face's normal to pull it.");
    else if (ge_pull(work(), picked, mm, line, (int)sizeof line) < 0) ge_copy(work(), &pull_base);
    refresh(ui);
    say(ui, line);
}

/* The button up: under 0.2 mm the pull is dropped, as the bench's. */
static void pull_end(olui *ui)
{
    pull_on = 0;
    gs_mesh_free(&pull_base);
    if (!pushed) return;
    pushed = 0;
    if (pull_mm < 0.2f && pull_mm > -0.2f) {
        forget();
        refresh(ui);
        say(ui, "That pull was under 0.2 mm, so it was dropped.");
    }
}

static void on_drop(olui *ui, const olui_event *ev, void *userdata)
{
    (void)ev;
    (void)userdata;
    remember();
    ge_drop(work());
    refresh(ui);
}

static void on_centre(olui *ui, const olui_event *ev, void *userdata)
{
    (void)ev;
    (void)userdata;
    remember();
    ge_centre(work());
    refresh(ui);
}

static void on_lay(olui *ui, const olui_event *ev, void *userdata)
{
    int face = picked >= 0 ? picked : ge_largest_face(work());
    gs_v was_min, was_max;
    int same;
    gs_mesh *m;
    (void)ev;
    (void)userdata;
    if (face < 0) {
        say(ui, "No face to lay");
        return;
    }
    m = work();
    was_min = m->min;
    was_max = m->max;
    remember();
    if (ge_lay_face(m, face)) {
        forget();
        say(ui, "That face will not lie flat");
        return;
    }
    same = fabsf(m->min.x - was_min.x) < 1e-3f && fabsf(m->min.y - was_min.y) < 1e-3f &&
           fabsf(m->min.z - was_min.z) < 1e-3f && fabsf(m->max.x - was_max.x) < 1e-3f &&
           fabsf(m->max.y - was_max.y) < 1e-3f && fabsf(m->max.z - was_max.z) < 1e-3f;
    if (same) {
        forget();
        say(ui, "That face is already flat on the bed");
        return;
    }
    picked = face;
    refresh(ui);
}

/* The slider is 1 to 100 along the part's layers; Prev and Next step one. */
static void slider_to_layer(olui *ui)
{
    int n = gl_layers(&shown, layer_h);
    olui_set_slider(ui, olui_find(ui, "layer"), omv_slider_of(layer_no, n));
}

/* The print settings panel's, over the defaults. */
static void settings(olui *ui, gs_settings *s)
{
    gs_settings_default(s);
    omv_settings_read(ui, s);
}

/* The settings panel's layer height: the layers shown are cut at it. */
static void on_height(olui *ui, const olui_event *ev, void *userdata)
{
    gs_settings s;
    (void)ev;
    (void)userdata;
    settings(ui, &s);
    layer_h = s.layer_h;
    if (layer_h < GL_MIN_H) layer_h = GL_MIN_H;
    if (layer_h > GL_MAX_H) layer_h = GL_MAX_H;
    drop_slice();
    refresh(ui);
    slider_to_layer(ui);
}

static void on_show(olui *ui, const olui_event *ev, void *userdata)
{
    (void)userdata;
    show_layer = ev->checked;
    refresh(ui);
}

static void on_step(olui *ui, const olui_event *ev, void *userdata)
{
    (void)ev;
    layer_no += (int)(intptr_t)userdata;
    if (!show_layer) olui_set_checked(ui, olui_find(ui, "show"), show_layer = 1);
    refresh(ui);
    slider_to_layer(ui);
}

static void on_slide(olui *ui, const olui_event *ev, void *userdata)
{
    int n = gl_layers(&shown, layer_h);
    (void)userdata;
    layer_no = omv_layer_of(ev->index, n);
    if (!show_layer) olui_set_checked(ui, olui_find(ui, "show"), show_layer = 1);
    refresh(ui);
}

static int axes(float b[3][2])
{
    gs_frame f;
    float ox, oy, oz, px, py, pz;
    gs_v o, p;
    int k;
    static const float step[3][3] = {{1.f, 0.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 0.f, 1.f}};
    if (vw < 4 || vh < 4) return -1;
    rebuild_shown();
    if (gs_view_frame_bed(&shown, &cam, vw, vh, 0.f, 0.f, GS_BED, GS_BED, &f)) return -1;
    o = f.origin;
    gs_view_map(&f, &cam, o, &ox, &oy, &oz);
    for (k = 0; k < 3; k++) {
        p.x = o.x + step[k][0];
        p.y = o.y + step[k][1];
        p.z = o.z + step[k][2];
        gs_view_map(&f, &cam, p, &px, &py, &pz);
        b[k][0] = px - ox;
        b[k][1] = py - oy;
    }
    return 0;
}

static void drag_edit(olui *ui, float dx, float dy)
{
    if (dx == 0.f && dy == 0.f) return;
    if (!pushed) {
        remember();
        pushed = 1;
    }
    if (tool == TOOL_MOVE) {
        float b[3][2];
        if (axes(b)) return;
        if (drag_shift) {
            float len2 = b[2][0] * b[2][0] + b[2][1] * b[2][1];
            float mz = len2 < 1e-4f ? 0.f : (dx * b[2][0] + dy * b[2][1]) / len2;
            ge_move(work(), 0.f, 0.f, mz);
        } else {
            float a00 = b[0][0] * b[0][0] + b[0][1] * b[0][1];
            float a01 = b[0][0] * b[1][0] + b[0][1] * b[1][1];
            float a11 = b[1][0] * b[1][0] + b[1][1] * b[1][1];
            float rx = dx * b[0][0] + dy * b[0][1];
            float ry = dx * b[1][0] + dy * b[1][1];
            float det = a00 * a11 - a01 * a01;
            float mx, my;
            if (det < 1e-4f && det > -1e-4f) return;
            mx = (rx * a11 - ry * a01) / det;
            my = (a00 * ry - a01 * rx) / det;
            ge_move(work(), mx, my, 0.f);
        }
    } else if (tool == TOOL_ROT) {
        ge_rotate(work(), dy * 0.01f, 0.f, dx * 0.01f);
    } else if (tool == TOOL_SCALE) {
        float s = expf(-dy * 0.005f);
        ge_scale(work(), s, s, s);
    }
    refresh(ui);
}

static int in_view(olui *ui, struct Window *w, int mx, int my, int *lx, int *ly)
{
    olui_rect box;
    int x0, y0;
    if (!olui_box(ui, olui_find(ui, "view"), &box)) return 0;
    x0 = w->BorderLeft + box.x;
    y0 = w->BorderTop + box.y;
    if (mx < x0 || my < y0 || mx >= x0 + box.w || my >= y0 + box.h) return 0;
    if (lx) *lx = mx - x0;
    if (ly) *ly = my - y0;
    vw = box.w;
    vh = box.h;
    return 1;
}

/* Size: the part scaled evenly to the new size (ge_fit); At: moved there. */
static void on_place(olui *ui, const olui_event *ev, void *userdata)
{
    int which = (int)(intptr_t)userdata;
    float v = omv_number(ui, place_ids[which]), pos[3], size[3];
    (void)ev;
    ge_where(work(), pos, size);
    if (which < 3 ? (v - size[which] < 0.05f && size[which] - v < 0.05f) : (v - pos[which - 3] < 0.05f && pos[which - 3] - v < 0.05f))
        return;
    remember();
    if (which < 3) {
        if (ge_fit(work(), which, v)) {
            say(ui, "That part is too thin to size that way.");
            face_line(ui);
            return;
        }
    } else {
        pos[which - 3] = v;
        ge_place(work(), pos[0], pos[1], pos[2]);
    }
    refresh(ui);
}

/* Holes: the bench's bolt circle, hole row, counterbore, one drill or
 * countersink, cut through the picked part (ge_drill), with Undo; refused,
 * the status line says why and nothing changes. Washer is a new part. */
static void on_holes(olui *ui, const olui_event *ev, void *userdata)
{
    int kind = (int)(intptr_t)userdata;
    ge_holes h;
    gs_mesh copy;
    char why[96];
    (void)ev;
    why[0] = 0;
    memset(&h, 0, sizeof h);
    h.d = omv_number(ui, "hd");
    h.n = (int)(omv_number(ui, "hn") + 0.5f);
    h.pcd = omv_number(ui, "hpcd");
    h.pitch = omv_number(ui, "hpitch");
    h.cb = omv_number(ui, "hcb");
    h.deep = omv_number(ui, "hdeep");
    h.mouth = h.cb; /* the countersink's mouth is the Bore field, as the bench */
    memset(&copy, 0, sizeof copy);
    if (ge_copy(&copy, work()) || ge_drill(&copy, kind, &h, why, (int)sizeof why)) {
        gs_mesh_free(&copy);
        say(ui, why[0] ? why : "Not enough memory for the holes.");
        return;
    }
    remember();
    gs_mesh_free(work());
    *work() = copy;
    picked = -1;
    refresh(ui);
    if (kind == GE_BOLT_CIRCLE)
        snprintf(why, sizeof why, "%d holes %.1f mm on a %.1f mm circle, cut.", h.n, (double)h.d, (double)h.pcd);
    else if (kind == GE_HOLE_ROW)
        snprintf(why, sizeof why, "%d holes %.1f mm, %.1f mm apart, cut.", h.n, (double)h.d, (double)h.pitch);
    else if (kind == GE_DRILL)
        snprintf(why, sizeof why, "A %.1f mm hole, cut through.", (double)h.d);
    else if (kind == GE_COUNTERSINK)
        snprintf(why, sizeof why, "A %.1f mm hole with a %.1f mm countersink, cut.", (double)h.d, (double)h.mouth);
    else
        snprintf(why, sizeof why, "A %.1f mm hole with a %.1f mm counterbore %.1f deep, cut.", (double)h.d, (double)h.cb,
                 (double)h.deep);
    say(ui, why);
}

/* The bench's washer: a new part on the bed, past the parts shown. */
static void on_washer(olui *ui, const olui_event *ev, void *userdata)
{
    char line[120];
    (void)ev;
    (void)userdata;
    remember();
    if (ge_washer(parts, line, (int)sizeof line) < 0) {
        forget();
        say(ui, line[0] ? line : "The washer did not build.");
        return;
    }
    picked = -1;
    verts_fit(0);
    refresh(ui);
    say(ui, line);
}

/* Section hides the picture above At. Split cuts the picked part there. */
static void section_say(olui *ui)
{
    char line[96];
    if (!section_on) {
        say(ui, "Section is off.");
        return;
    }
    snprintf(line, sizeof line, "Section at %.1f mm. Everything above the plane is hidden.", (double)section_z);
    say(ui, line);
}

static void set_section_z(olui *ui, float z)
{
    if (z < 0.f) z = 0.f;
    if (z > 220.f) z = 220.f;
    section_z = (float)(long)(z * 10.f + 0.5f) / 10.f;
    olui_set_number(ui, olui_find(ui, "sectz"), (int)(section_z * 10.f + 0.5f));
}

static void on_sect(olui *ui, const olui_event *ev, void *userdata)
{
    (void)userdata;
    section_on = ev->checked;
    section_z = omv_number(ui, "sectz");
    refresh(ui);
    section_say(ui);
}

static void on_sectz(olui *ui, const olui_event *ev, void *userdata)
{
    (void)ev;
    (void)userdata;
    section_z = omv_number(ui, "sectz");
    if (!section_on) olui_set_checked(ui, olui_find(ui, "sect"), section_on = 1);
    refresh(ui);
    section_say(ui);
}

static void on_split(olui *ui, const olui_event *ev, void *userdata)
{
    char line[120];
    (void)ev;
    (void)userdata;
    remember();
    if (ge_split(parts, omv_number(ui, "sectz"), 1, line, (int)sizeof line) < 0) {
        forget();
        say(ui, line);
        return;
    }
    picked = -1;
    verts_fit(0);
    refresh(ui);
    say(ui, line);
}

static void on_plist(olui *ui, const olui_event *ev, void *userdata)
{
    (void)userdata;
    if (!ev->value || atoi(ev->value) == gs_parts_picked(parts)) return;
    gs_parts_pick(parts, atoi(ev->value));
    marks = 0;
    picked = -1; /* the face was the other part's */
    refresh(ui);
}

/* Return in the name field: the picked part's new name, a step for Undo. */
static void on_pname(olui *ui, const olui_event *ev, void *userdata)
{
    const char *name = olui_get_text(olui_find(ui, "pname"));
    (void)ev;
    (void)userdata;
    if (!name || !*name || !strcmp(name, work_name())) return;
    remember();
    if (gs_parts_rename(parts, gs_parts_picked(parts), name) < 0) {
        forget();
        say(ui, "That name can't be used");
        return;
    }
    refresh(ui);
}

/* A metal, Hide or Show, and Delete for the picked part: each a step. */
static void on_part(olui *ui, const olui_event *ev, void *userdata)
{
    int what = (int)(intptr_t)userdata, at = gs_parts_picked(parts);
    (void)ev;
    if (at < 0 || (what == PART_DELETE && gs_parts_count(parts) < 2)) return;
    remember();
    if (what == PART_DELETE) {
        gs_parts_remove(parts, at);
        picked = -1;
        verts_fit(0);
    } else if (what == PART_SHOW) {
        gs_parts_show(parts, at, !gs_parts_shown(parts, at));
        verts_fit(0);
    } else
        gs_parts_set_colour(parts, at, what);
    refresh(ui);
}

/* A new part, picked: Parts' Add, Extrude's Add. */
/* Arrange, as the bench's (ge_duplicate to ge_solo): Copy, Line X, Line
 * Y and Ring with Arrange's copies and gap, Mirror, Turn (90 degrees about
 * Z), Stack and Solo, on the picked part; each a step for Undo, and taken
 * back with the status line saying why when refused. */
enum { AR_COPY, AR_LINEX, AR_LINEY, AR_RING, AR_MIRROR, AR_TURN, AR_STACK, AR_SOLO };

static void on_arrange(olui *ui, const olui_event *ev, void *userdata)
{
    int what = (int)(intptr_t)userdata, n = (int)(omv_number(ui, "acount") + 0.5f), was = gs_parts_count(parts), r;
    float gap = omv_number(ui, "agap");
    char line[120];
    (void)ev;
    remember();
    r = what == AR_COPY    ? ge_duplicate(parts, line, (int)sizeof line)
        : what == AR_LINEX ? ge_line(parts, 0, n, gap, line, (int)sizeof line)
        : what == AR_LINEY ? ge_line(parts, 1, n, gap, line, (int)sizeof line)
        : what == AR_RING  ? ge_ring(parts, n, line, (int)sizeof line)
        : what == AR_MIRROR ? ge_mirror(parts, line, (int)sizeof line)
        : what == AR_TURN  ? ge_turn(parts, 2, 90.f, line, (int)sizeof line)
        : what == AR_STACK ? ge_stack(parts, line, (int)sizeof line)
                           : ge_solo(parts, line, (int)sizeof line);
    if (r < 0) {
        forget();
        say(ui, line);
        return;
    }
    if (gs_parts_count(parts) != was) picked = -1; /* the copy picked: its faces unclicked */
    verts_fit(0);
    refresh(ui);
    say(ui, line);
}

static void take_added(olui *ui, gs_mesh box, const char *name)
{
    if (gs_parts_count(parts) >= GS_PARTS_MAX) {
        gs_mesh_free(&box);
        say(ui, "Twenty-four parts is the limit.");
        return;
    }
    remember();
    gs_parts_add(parts, &box, name);
    picked = -1;
    verts_fit(0);
    refresh(ui);
}

/* Extrude's Add, On top or Cut: Add is a new part on the bed, to place;
 * On top and Cut change the picked part, as Holes does. */
/* Pull (userdata 0): the face clicked in Orbit moved along its normal by
 * Face's distance, its plane with it, as the bench's Pull by a distance.
 * Chamfer (1): its edges cut in by that distance, as the bench's. */
static void on_pull(olui *ui, const olui_event *ev, void *userdata)
{
    char line[96];
    float mm = omv_number(ui, "fmm");
    (void)ev;
    remember();
    if ((userdata ? ge_chamfer(work(), picked, mm, line, (int)sizeof line)
                  : ge_pull(work(), picked, mm, line, (int)sizeof line)) < 0) {
        forget();
        say(ui, line);
        return;
    }
    refresh(ui);
    say(ui, line);
}

static void on_extrude(olui *ui, const olui_event *ev, void *userdata)
{
    int op = (int)(intptr_t)userdata, kind = olui_select_index(olui_find(ui, "xkind"));
    gs_mesh out;
    char line[96];
    (void)ev;
    memset(&out, 0, sizeof out);
    if (ge_extrude(work(), kind, op, omv_number(ui, "xw"), omv_number(ui, "xd"),
                   omv_number(ui, "xh"), &out, line, (int)sizeof line)) {
        gs_mesh_free(&out);
        say(ui, line);
        return;
    }
    if (op == GM_ON_BED) take_added(ui, out, kind == GM_ROUND ? "Round" : kind == GM_SLOT ? "Slot" : "Block");
    else {
        remember();
        gs_mesh_free(work());
        *work() = out;
        picked = -1;
        refresh(ui);
    }
    say(ui, line);
}

/* Parts' Add: the shape chosen, placed from the picked part (ge_shape), as
 * a new part, picked, to join, cut or intersect with it. */
static void on_add(olui *ui, const olui_event *ev, void *userdata)
{
    gs_mesh part;
    int kind = olui_select_index(olui_find(ui, "shape"));
    char why[64];
    (void)ev;
    (void)userdata;
    memset(&part, 0, sizeof part);
    if (ge_shape(&part, kind, work())) {
        gs_mesh_free(&part);
        snprintf(why, sizeof why, "The %s did not build", ge_shape_name(kind) ? ge_shape_name(kind) : "shape");
        say(ui, why);
        return;
    }
    take_added(ui, part, ge_shape_name(kind));
}

/* Join, Cut, Intersect, as the bench's: the picked part is the tool, the
 * one before it in the list the stock (ge_combine). */
static void on_bool(olui *ui, const olui_event *ev, void *userdata)
{
    gs_parts *was = gs_parts_copy(parts);
    char line[120];
    (void)ev;
    if (!was) {
        say(ui, "Not enough memory to keep a step for Undo.");
        return;
    }
    if (ge_combine(parts, (int)(intptr_t)userdata, line, (int)sizeof line)) {
        gs_parts_free(was);
        say(ui, line);
        return;
    }
    drop_slice();
    marks = 0;
    if (steps) ge_steps_push(steps, was);
    gs_parts_free(was);
    picked = -1;
    verts_fit(0);
    refresh(ui);
    say(ui, line);
}

/* The printer picked, loaded; or -1 and why. */
static int printer(olui *ui, om_machine *m, char *err, int errlen)
{
    int i = omv_machines_chosen(ui, "machine", &printers);
    if (i < 0) {
        snprintf(err, (size_t)errlen, "No printer: put one in PROGDIR:machines/printer");
        return -1;
    }
    return omv_machines_load(&printers, i, m, err, errlen) ? -1 : 0;
}

/* Another printer picked: the settings held to it, and why said. */
static void on_machine(olui *ui, const olui_event *ev, void *userdata)
{
    om_machine m;
    gs_settings s;
    char why[160];
    (void)ev;
    (void)userdata;
    if (printer(ui, &m, why, (int)sizeof why)) {
        say(ui, why);
        return;
    }
    omv_settings_fit(ui, &m, &s, why, (int)sizeof why);
    if (why[0]) say(ui, why);
}

static long to_file(const char *t, long n, void *ud)
{
    return (long)fwrite(t, 1, (size_t)n, (FILE *)ud);
}

/* The part as a printer's toolpath, at the Layers height, on as many
 * tasks as there are cores (gs_par: exec tasks here). */
static void on_slice(olui *ui, const olui_event *ev, void *userdata)
{
    gp_opts o;
    gc_opts go;
    gp_more x;
    gc_more gx;
    gs_settings s;
    om_machine m;
    char why[160];
    int have;
    (void)ev;
    (void)userdata;
    rebuild_shown();
    have = printer(ui, &m, why, (int)sizeof why) == 0;
    if (have) omv_settings_fit(ui, &m, &s, why, (int)sizeof why); /* held to the printer */
    settings(ui, &s);
    gs_settings_opts(&s, &o, &go);
    gs_settings_more(&s, &x);
    drop_slice();
    say(ui, "Slicing...");
    if (gp_part_par_more(&sliced, &shown, &o, &x, gs_par_cores()) || !sliced.n) {
        drop_slice();
        say(ui, "That did not slice: out of memory, or nothing to slice");
        return;
    }
    gs_settings_gc_more(&s, &go, &gx);
    slice_how = gc_estimate_more(&sliced, have ? &m : NULL, &gx, &slice_st);
    has_slice = 1;
    refresh(ui);
}

/* The toolpath as G-code for the printer picked, checked against it
 * first. */
static void on_gcode(olui *ui, const olui_event *ev, void *userdata)
{
    struct FileRequester *fr;
    om_machine m;
    om_job job;
    gp_opts o;
    gc_opts go;
    gc_more gx;
    gs_settings s;
    char path[256], err[160];
    FILE *f;
    (void)ev;
    (void)userdata;
    if (!has_slice) {
        say(ui, "Slice first");
        return;
    }
    if (printer(ui, &m, err, (int)sizeof err)) {
        say(ui, err);
        return;
    }
    settings(ui, &s);
    gs_settings_opts(&s, &o, &go);
    gs_settings_gc_more(&s, &go, &gx);
    gc_job(&sliced, &m, &go, &job);
    if (om_check(&m, &job, err, (int)sizeof err) & ~OM_TOO_FAST) {
        say(ui, err);
        return;
    }
    fr = (struct FileRequester *)AllocAslRequestTags(ASL_FileRequest, ASLFR_TitleText, (ULONG) "Save G-code",
                                                     ASLFR_DoSaveMode, TRUE, ASLFR_DoPatterns, TRUE,
                                                     ASLFR_InitialPattern, (ULONG) "#?.gcode", TAG_END);
    if (!fr) {
        say(ui, "The file requester did not open");
        return;
    }
    if (AslRequest(fr, NULL) && fr->fr_Drawer && fr->fr_File) {
        path_join(path, (int)sizeof path, (const char *)fr->fr_Drawer, (const char *)fr->fr_File);
        f = fopen(path, "w");
        if (!f || gc_write_more(to_file, f, &sliced, &m, &gx, NULL, err, (int)sizeof err)) say(ui, f ? err : "Could not write there");
        else say(ui, path);
        if (f) fclose(f);
    }
    FreeAslRequest(fr);
}

int main(int argc, char **argv)
{
    olui *ui;
    olui_amiga *a;
    struct Window *w;
    struct IntuiMessage *m;
    char err[160];
    gs_mesh first;
    const char *first_name = "Holed cube";
    (void)version;
    memset(&first, 0, sizeof first);
    /* A file named at launch is opened, never written. Only Save writes. */
    if (argc > 1 && argv[1] && argv[1][0]) {
        if (ge_start(&first, argv[1], err, (int)sizeof err) == 0) {
            first_name = (const char *)FilePart((STRPTR)argv[1]);
            strncpy(stl_path, argv[1], sizeof stl_path - 1);
            stl_path[sizeof stl_path - 1] = 0;
        } else {
            fprintf(stderr, "GFOpenMake: %s\n", err);
        }
    }
    if (!first.n && ge_start(&first, NULL, err, (int)sizeof err)) {
        fprintf(stderr, "GFOpenMake: %s\n", err);
        return 20;
    }
    parts = gs_parts_new();
    steps = ge_steps_new();
    if (!parts || gs_parts_add(parts, &first, first_name) < 0) {
        fprintf(stderr, "GFOpenMake: not enough memory\n");
        gs_mesh_free(&first);
        gs_parts_free(parts);
        ge_steps_free(steps);
        return 20;
    }
    omv_gpu_open(&gpu, gs_parts_mesh(parts, 0)->n * 3, 512);
    gpu_on = gpu.on;
    on_chip = gpu.on_chip;
    draw_grid = gpu.grid;
    ui = olui_new();
    if (!ui || !olui_load(ui, gm_markup(), gm_css, err, sizeof err) ||
        omv_settings_add(ui, "settings", err, (int)sizeof err)) {
        if (ui) olui_free(ui);
        ge_steps_free(steps);
        gs_parts_free(parts);
        gs_mesh_free(&shown);
        omv_gpu_close(&gpu);
        fprintf(stderr, "GFOpenMake: %s\n", ui ? err : "GFOpenLayout is not open");
        return 20;
    }
    {
        gs_settings s;
        gs_settings_default(&s);
        omv_settings_show(ui, &s);
    }
    olui_on(ui, "open", OLUI_CLICK, on_open, NULL);
    olui_on(ui, "save", OLUI_CLICK, on_save, NULL);
    olui_on(ui, "undo", OLUI_CLICK, on_undo, (void *)(intptr_t)0);
    olui_on(ui, "redo", OLUI_CLICK, on_undo, (void *)(intptr_t)1);
    olui_on(ui, "orbit", OLUI_CLICK, on_tool, (void *)(intptr_t)TOOL_ORBIT);
    olui_on(ui, "move", OLUI_CLICK, on_tool, (void *)(intptr_t)TOOL_MOVE);
    olui_on(ui, "rot", OLUI_CLICK, on_tool, (void *)(intptr_t)TOOL_ROT);
    olui_on(ui, "scale", OLUI_CLICK, on_tool, (void *)(intptr_t)TOOL_SCALE);
    olui_on(ui, "measure", OLUI_CLICK, on_tool, (void *)(intptr_t)TOOL_MEASURE);
    olui_on(ui, "drop", OLUI_CLICK, on_drop, NULL);
    olui_on(ui, "centre", OLUI_CLICK, on_centre, NULL);
    olui_on(ui, "lay", OLUI_CLICK, on_lay, NULL);
    olui_on(ui, "add", OLUI_CLICK, on_add, NULL);
    olui_on(ui, "bolt", OLUI_CLICK, on_holes, (void *)(intptr_t)GE_BOLT_CIRCLE);
    olui_on(ui, "row", OLUI_CLICK, on_holes, (void *)(intptr_t)GE_HOLE_ROW);
    olui_on(ui, "cbore", OLUI_CLICK, on_holes, (void *)(intptr_t)GE_COUNTERBORE);
    olui_on(ui, "drill", OLUI_CLICK, on_holes, (void *)(intptr_t)GE_DRILL);
    olui_on(ui, "csink", OLUI_CLICK, on_holes, (void *)(intptr_t)GE_COUNTERSINK);
    olui_on(ui, "washer", OLUI_CLICK, on_washer, NULL);
    olui_on(ui, "sect", OLUI_CHANGE, on_sect, NULL);
    olui_on(ui, "sectz", OLUI_CHANGE, on_sectz, NULL);
    olui_on(ui, "split", OLUI_CLICK, on_split, NULL);
    {
        int k;
        for (k = 0; k < 6; k++) olui_on(ui, place_ids[k], OLUI_CHANGE, on_place, (void *)(intptr_t)k);
    }
    {
        int k;
        for (k = 0; k < GE_SHAPES; k++) olui_select_add(ui, olui_find(ui, "shape"), ge_shape_name(k), ge_shape_name(k));
        olui_select_set(ui, olui_find(ui, "shape"), 0);
        olui_select_add(ui, olui_find(ui, "xkind"), "Block", "Block");
        olui_select_add(ui, olui_find(ui, "xkind"), "Round", "Round");
        olui_select_add(ui, olui_find(ui, "xkind"), "Slot", "Slot");
        olui_select_set(ui, olui_find(ui, "xkind"), 0);
    }
    olui_on(ui, "xadd", OLUI_CLICK, on_extrude, (void *)(intptr_t)GM_ON_BED);
    olui_on(ui, "xon", OLUI_CLICK, on_extrude, (void *)(intptr_t)GM_ON_TOP);
    olui_on(ui, "xcut", OLUI_CLICK, on_extrude, (void *)(intptr_t)GM_FROM_TOP);
    olui_on(ui, "plist", OLUI_SELECT, on_plist, NULL);
    olui_on(ui, "pname", OLUI_SUBMIT, on_pname, NULL);
    olui_on(ui, "pshow", OLUI_CLICK, on_part, (void *)(intptr_t)PART_SHOW);
    olui_on(ui, "pdel", OLUI_CLICK, on_part, (void *)(intptr_t)PART_DELETE);
    {
        static const char *const sw[6] = {"c0", "c1", "c2", "c3", "c4", "c5"};
        int c;
        for (c = 0; c < 6; c++) olui_on(ui, sw[c], OLUI_CLICK, on_part, (void *)(intptr_t)c);
    }
    olui_on(ui, "pull", OLUI_CLICK, on_pull, NULL);
    olui_on(ui, "chamfer", OLUI_CLICK, on_pull, (void *)(intptr_t)1);
    olui_on(ui, "fdrag", OLUI_CLICK, on_fdrag, NULL);
    {
        static const char *const ar[] = {"dup", "linex", "liney", "ring", "mirror", "turn", "stack", "solo"};
        int k;
        for (k = 0; k < 8; k++) olui_on(ui, ar[k], OLUI_CLICK, on_arrange, (void *)(intptr_t)k);
    }
    olui_on(ui, "join", OLUI_CLICK, on_bool, (void *)(intptr_t)0);
    olui_on(ui, "cut", OLUI_CLICK, on_bool, (void *)(intptr_t)1);
    olui_on(ui, "meet", OLUI_CLICK, on_bool, (void *)(intptr_t)2);
    olui_on(ui, "set-layer-height", OLUI_CHANGE, on_height, NULL);
    olui_on(ui, "machine", OLUI_CHANGE, on_machine, NULL);
    olui_on(ui, "show", OLUI_CHANGE, on_show, NULL);
    olui_on(ui, "prev", OLUI_CLICK, on_step, (void *)(intptr_t)-1);
    olui_on(ui, "next", OLUI_CLICK, on_step, (void *)(intptr_t)1);
    olui_on(ui, "layer", OLUI_CHANGE, on_slide, NULL);
    olui_on(ui, "slice", OLUI_CLICK, on_slice, NULL);
    olui_on(ui, "gcode", OLUI_CLICK, on_gcode, NULL);
    gs_cam_home(&cam);
    /* Resizable, 640 by 480 inside at the least: a PAL screen, or more on RTG. */
    w = omv_window_open("GFOpenMake 0.1.0", olui_amiga_idcmp());
    if (!w) {
        ge_steps_free(steps);
        gs_parts_free(parts);
        gs_mesh_free(&shown);
        olui_free(ui);
        omv_gpu_close(&gpu);
        fprintf(stderr, "GFOpenMake: the window did not open\n");
        return 20;
    }
    win = w;
    a = olui_amiga_new(ui, w);
    if (!a) {
        CloseWindow(w);
        ge_steps_free(steps);
        gs_parts_free(parts);
        gs_mesh_free(&shown);
        olui_free(ui);
        omv_gpu_close(&gpu);
        fprintf(stderr, "GFOpenMake: GFOpenLayout could not draw the window\n");
        return 20;
    }
    olui_amiga_theme(a, NULL);
    olui_amiga_own(a, olui_find(ui, "view"), draw_view, NULL);
    light_tool(ui);
    /* The printer picker: every machines/printer/<driver>/profiles/ .om file. */
    if (omv_machines_scan(&printers, "PROGDIR:machines", "printer") < 0) printers.n = 0;
    omv_machines_fill(ui, "machine", &printers, "No printers");
    olui_amiga_layout(a);
    refresh(ui);
    if (printers.skipped) say(ui, printers.why);
    else if (printers.n) on_machine(ui, NULL, NULL); /* the settings held to the first printer */
    /* ARexx: Wait() on the window and GFOPENMAKE.1 together. */
    gm_rexx_init(&rexx);
    rexx.parts = &parts;
    rexx.steps = steps;
    rexx.version = "GFOpenMake 0.1.0";
    rexx.activate = rexx_activate;
    rexx.hide = rexx_hide;
    rexx.win = w;
    rexx.ui = ui;
    snprintf(rexx.file, sizeof rexx.file, "%s", stl_path);
    rexx.face = picked;
    rexx_port = omr_open(GM_REXX_PORT, rexx.version);
    if (rexx_port) {
        rexx.port_name = omr_name(rexx_port);
        omr_attach(rexx_port, gm_rexx_verbs(), gm_rexx_nverbs(), &rexx);
    }
    for (;;) {
        int r, lx, ly;
        ULONG mask = 1UL << w->UserPort->mp_SigBit, got, rmask = omr_mask(rexx_port);
        got = Wait(mask | rmask);
        if (rmask && (got & rmask)) {
            omr_cmd *cmd;
            char why[160];
            rexx.face = picked;
            snprintf(rexx.file, sizeof rexx.file, "%s", stl_path);
            settings(ui, &rexx.set);
            rexx.have_machine = printer(ui, &rexx.machine, why, (int)sizeof why) == 0;
            while ((cmd = omr_get(rexx_port))) omr_dispatch(cmd, gm_rexx_verbs(), gm_rexx_nverbs(), &rexx);
            omv_settings_show(ui, &rexx.set);
            picked = rexx.face;
            strncpy(stl_path, rexx.file, sizeof stl_path - 1);
            stl_path[sizeof stl_path - 1] = 0;
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
            if (cls == IDCMP_VANILLAKEY && r != OLUI_AMIGA_USED) {
                /* Amiga-Z, Amiga-Shift-Z and Amiga-Y, when no field took them */
                int k = gm_command_key(code, (qual & (IEQUALIFIER_LCOMMAND | IEQUALIFIER_RCOMMAND)) != 0,
                                       (qual & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT)) != 0);
                if (k == GM_KEY_PULL || k == GM_KEY_LEAVE) set_pull(ui, k == GM_KEY_PULL && tool != TOOL_PULL);
                else if (k == GM_KEY_SECTION) {
                    section_on = !section_on;
                    olui_set_checked(ui, olui_find(ui, "sect"), section_on);
                    refresh(ui);
                    section_say(ui);
                } else if (k == GM_KEY_SECT_DOWN || k == GM_KEY_SECT_UP) {
                    set_section_z(ui, section_z + (k == GM_KEY_SECT_UP ? 1.f : -1.f));
                    if (!section_on) olui_set_checked(ui, olui_find(ui, "sect"), section_on = 1);
                    refresh(ui);
                    section_say(ui);
                } else if (k) on_undo(ui, NULL, (void *)(intptr_t)(k == GM_KEY_REDO));
            }
            if (cls == IDCMP_MOUSEBUTTONS && code == SELECTDOWN) {
                if (in_view(ui, w, mx, my, NULL, NULL)) {
                    if (tool == TOOL_PULL) pull_start(ui, mx, my);
                    dragging = 1;
                    drag_moved = 0;
                    pushed = 0;
                    drag_x = mx;
                    drag_y = my;
                    drag_shift = (qual & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT)) != 0;
                }
            } else if (cls == IDCMP_MOUSEBUTTONS && code == SELECTUP) {
                if (pull_on) pull_end(ui);
                if (dragging && tool == TOOL_MEASURE && drag_moved < 4 && !has_slice &&
                    in_view(ui, w, mx, my, &lx, &ly)) {
                    gs_v p;
                    rebuild_shown();
                    /* Off the part, the bed under the pointer, as the bench. */
                    if (ge_pick_at(&shown, &cam, vw, vh, lx, ly, &p) < 0 && ge_pick_bed(&shown, &cam, vw, vh, lx, ly, &p))
                        say(ui, "Neither the part nor the bed is under the pointer.");
                    else {
                        if (marks == 2) marks = 0;
                        mark[marks++] = p;
                        refresh(ui);
                    }
                } else if (dragging && (tool == TOOL_ORBIT || tool == TOOL_PULL) && drag_moved < 4 &&
                    in_view(ui, w, mx, my, &lx, &ly)) {
                    /* The nearest face under the pointer in the shown parts;
                     * its part is picked, as a click on the bench. */
                    int face, k;
                    rebuild_shown();
                    face = ge_pick(&shown, &cam, vw, vh, lx, ly);
                    for (k = 0; face >= 0 && k < gs_parts_count(parts); k++) {
                        int n = gs_parts_shown(parts, k) ? gs_parts_mesh(parts, k)->n : 0;
                        if (face < n) {
                            gs_parts_pick(parts, k);
                            break;
                        }
                        face -= n;
                    }
                    picked = face;
                    refresh(ui);
                }
                dragging = 0;
                pushed = 0;
            } else if (cls == IDCMP_MOUSEMOVE && dragging) {
                float dx = (float)(mx - drag_x), dy = (float)(my - drag_y);
                int adx = mx - drag_x, ady = my - drag_y;
                if (adx < 0) adx = -adx;
                if (ady < 0) ady = -ady;
                drag_moved += adx + ady;
                drag_shift = (qual & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT)) != 0;
                if (tool == TOOL_PULL) {
                    if (pull_on) pull_move(ui, mx, my);
                } else if (tool == TOOL_ORBIT || tool == TOOL_MEASURE) {
                    if (drag_shift) gs_cam_pan(&cam, dx / 400.f, -dy / 400.f);
                    else gs_cam_orbit(&cam, dx * 0.01f, dy * 0.01f);
                    olui_repaint(ui, olui_find(ui, "view"), NULL);
                } else drag_edit(ui, dx, dy);
                drag_x = mx;
                drag_y = my;
            }
        }
        olui_amiga_draw(a);
        if (presented && !said_chip) {
            said_chip = 1;
            refresh(ui);
        }
    }
done:
    omr_close(rexx_port);
    olui_amiga_free(a);
    CloseWindow(w);
    ge_steps_free(steps);
    gs_parts_free(parts);
    gs_mesh_free(&shown);
    gs_mesh_free(&none);
    gs_mesh_free(&pull_base);
    gs_mesh_free(&section_mesh);
    gl_walls_free(&walls);
    drop_slice();
    omv_machines_free(&printers);
    olui_free(ui);
    omv_gpu_close(&gpu);
    return 0;
}
