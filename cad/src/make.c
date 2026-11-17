/* GFOpenMake on the Amiga. A 20 mm cube with a hole through it, on a 220 mm
 * plate, in a GFOpenLayout window. Orbit turns the view. Move, rotate and
 * scale change the solid. Drop, Centre and Lay flat put it on the bed.
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
#include "make_ui.h"
#include "omview_gpu.h"
#include "omview_machines.h"
#include "omview_ui.h"

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

static gs_mesh hole;
static gs_mesh added;
static gs_mesh shown;
static int has_added, on_added;
static gs_cam cam;
static struct Window *win;
static int dragging, drag_x, drag_y, drag_shift, drag_moved;
static int gpu_on, on_chip, draw_grid, presented;
static int tool, picked = -1, pushed, vw = 640, vh = 320, said_chip;
static ge_hist *hist;
static char stl_path[256] = "RAM:hole.stl";
/* The Layers group: the height, whether a layer is shown, which, and its walls. */
static float layer_h = 0.2f;
static int show_layer, layer_no = 1;
static gl_walls walls;
/* The Slice group: the part as a toolpath, and what it came to. */
static gt_path sliced;
static int has_slice;
static gc_stats slice_st;
/* The printers under PROGDIR:machines/printer, for Save G-code. */
static omv_machines printers;
/* The view on OpenGPU (core/ui): its picture, depth, vertices and stream. */
static omv_gpu gpu;

static gs_mesh *work(void)
{
    return (has_added && on_added) ? &added : &hole;
}

static void mesh_bounds(gs_mesh *m)
{
    int i, k;
    if (!m->n || !m->t) {
        m->min.x = m->min.y = m->min.z = 0.f;
        m->max = m->min;
        return;
    }
    m->min = m->max = m->t[0].a;
    for (i = 0; i < m->n; i++) {
        gs_v p[3];
        p[0] = m->t[i].a;
        p[1] = m->t[i].b;
        p[2] = m->t[i].c;
        for (k = 0; k < 3; k++) {
            if (p[k].x < m->min.x) m->min.x = p[k].x;
            if (p[k].y < m->min.y) m->min.y = p[k].y;
            if (p[k].z < m->min.z) m->min.z = p[k].z;
            if (p[k].x > m->max.x) m->max.x = p[k].x;
            if (p[k].y > m->max.y) m->max.y = p[k].y;
            if (p[k].z > m->max.z) m->max.z = p[k].z;
        }
    }
}

static void rebuild_shown(void)
{
    int n = hole.n + (has_added ? added.n : 0);
    int i, k = 0;
    gs_tri *t = n ? (gs_tri *)malloc((size_t)n * sizeof *t) : NULL;
    if (n && !t) return;
    for (i = 0; i < hole.n; i++) t[k++] = hole.t[i];
    if (has_added) {
        for (i = 0; i < added.n; i++) t[k++] = added.t[i];
    }
    free(shown.t);
    memset(&shown, 0, sizeof shown);
    shown.t = t;
    shown.n = shown.cap = n;
    mesh_bounds(&shown);
}

static void say(olui *ui, const char *text)
{
    omv_say(ui, text);
}

static void verts_fit(int extra);

static int draw_gpu(int x, int y, int w, int h)
{
    int nv, nw;
    if (!gpu_on || !omv_gpu_size(&gpu, w, h)) return 0;
    rebuild_shown();
    if (has_slice) {
        /* The toolpath alone, fitted to the part, with no grid. */
        if (!omv_gpu_room(&gpu, sliced.n * 6 + 6)) return 0;
        nv = gs_gpu_toolpath(&sliced, &cam, show_layer ? layer_no : 0, 1, w, h, gpu.verts, gpu.vert_cap);
    } else {
        verts_fit(show_layer ? walls.n * 6 : 0);
        nv = gs_gpu_vertices_plate(&shown, &cam, w, h, gpu.verts, gpu.vert_cap);
    }
    if (nv < 3) return 0;
    /* The layer's walls go after the part's, in the same batch. */
    if (!has_slice && show_layer && walls.n) {
        int nw2 = gs_gpu_walls_plate(&walls, &shown, &cam, w, h, gpu.verts + nv * 6, gpu.vert_cap - nv);
        if (nw2 > 0) nv += nw2;
    }
    nw = gs_gpu_stream_plate(gpu.stream, gpu.stream_cap, (uint32_t)(uintptr_t)gpu.verts, nv,
                             (uint32_t)(uintptr_t)gpu.rs->rs_Pixels, (int)gpu.rs->rs_BytesPerRow,
                             (uint32_t)(uintptr_t)gpu.depth, w, h, &shown, &cam, has_slice ? 0 : draw_grid, 1);
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
    rgb = (unsigned char *)malloc((size_t)w * (size_t)h * 3);
    if (!rgb) return;
    vw = w;
    vh = h;
    rebuild_shown();
    if (has_slice ? gt_paint_part(&sliced, &cam, show_layer ? layer_no : 0, rgb, w, h) : gs_view_bed(&shown, &cam, rgb, w, h)) {
        free(rgb);
        return;
    }
    if (!has_slice && show_layer) gl_paint(&walls, &shown, &cam, rgb, w, h);
    /* Grey goes to the three greys, the walls' blue to the window's fourth
     * pen, the Workbench blue. */
    at[0] = x;
    at[1] = y;
    omv_pens(rgb, w, h, put_run, at);
    free(rgb);
}

static void draw_view(void *userdata, const olui_el *el, int x, int y, int w, int h)
{
    (void)userdata;
    (void)el;
    vw = w;
    vh = h;
    if (!draw_gpu(x, y, w, h)) draw_pens(x, y, w, h);
}

enum { TOOL_ORBIT, TOOL_MOVE, TOOL_ROT, TOOL_SCALE };

static void face_line(olui *ui)
{
    char buf[32];
    if (picked >= 0) snprintf(buf, sizeof buf, "face %d", picked);
    else snprintf(buf, sizeof buf, "no face");
    olui_set_text(ui, olui_find(ui, "face"), buf);
}

static const char *tool_label(void)
{
    if (tool == TOOL_MOVE) return "Move. Drag slides. Shift-drag lifts.";
    if (tool == TOOL_ROT) return "Rotate. Drag turns the part.";
    if (tool == TOOL_SCALE) return "Scale. Drag up grows it.";
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

static void refresh(olui *ui)
{
    char status[180];
    recut();
    if (has_slice) {
        omv_slice_status(status, (int)sizeof status, sliced.nlayers, slice_st.moves, slice_st.seconds,
                         slice_st.filament, show_layer ? layer_no : 0);
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
    snprintf(status, sizeof status, "%s, %d triangles%s. %s",
             (has_added && on_added) ? "added part" : "stock",
             work()->n,
             presented ? (on_chip ? ", chip" : ", OpenGPU") : "", tool_label());
    say(ui, status);
    face_line(ui);
    olui_repaint(ui, olui_find(ui, "view"), NULL);
}

/* The two parts as one scene. The meshes move, they are not copied. */
static ge_scene scene_out(void)
{
    ge_scene sc;
    sc.stock = hole;
    sc.added = added;
    sc.has_added = has_added;
    sc.on_added = on_added;
    return sc;
}

static void scene_in(const ge_scene *sc)
{
    hole = sc->stock;
    added = sc->added;
    has_added = sc->has_added;
    on_added = sc->on_added;
}

/* Both parts go in the step, so Undo after Join gives both back. */
static void drop_slice(void)
{
    gt_free(&sliced);
    has_slice = 0;
}

static void remember(void)
{
    drop_slice();
    ge_scene sc = scene_out();
    if (hist && hole.n) ge_hist_push_scene(hist, &sc);
}

/* Takes back the step remember() just made. */
static void forget(void)
{
    ge_scene sc = scene_out();
    if (hist && ge_hist_pop_scene(hist, &sc) == 0) scene_in(&sc);
}

static void verts_fit(int extra)
{
    int need = hole.n + (has_added ? added.n : 0);
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
            remember();
            gs_mesh_free(work());
            *work() = next;
            if (work() == &added) has_added = 1;
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

static void on_undo(olui *ui, const olui_event *ev, void *userdata)
{
    (void)ev;
    ge_scene sc = scene_out();
    (void)userdata;
    if (!hist || ge_hist_pop_scene(hist, &sc)) {
        say(ui, "Nothing to undo");
        return;
    }
    scene_in(&sc);
    drop_slice();
    picked = -1;
    verts_fit(0);
    refresh(ui);
}

/* The chosen tool's button is :selected, lit in the accent. */
static void light_tool(olui *ui)
{
    static const char *const ids[] = { "orbit", "move", "rot", "scale" };
    int i;
    for (i = 0; i < 4; i++) olui_set_selected(ui, olui_find(ui, ids[i]), i == tool);
}

static void on_tool(olui *ui, const olui_event *ev, void *userdata)
{
    (void)ev;
    tool = (int)(intptr_t)userdata;
    light_tool(ui);
    refresh(ui);
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

static void on_height(olui *ui, const olui_event *ev, void *userdata)
{
    olui_el *lh = olui_find(ui, "lh");
    (void)ev;
    (void)userdata;
    layer_h = (float)olui_number_value(lh) / 100.f;
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
    if (gs_view_frame_bed(&hole, &cam, vw, vh, 0.f, 0.f, GS_BED, GS_BED, &f)) return -1;
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

static void take_added(olui *ui, gs_mesh box)
{
    remember();
    if (has_added) gs_mesh_free(&added);
    added = box;
    has_added = 1;
    on_added = 1;
    picked = -1;
    verts_fit(0);
    refresh(ui);
}

static void on_add_cube(olui *ui, const olui_event *ev, void *userdata)
{
    gs_mesh box;
    (void)ev;
    (void)userdata;
    memset(&box, 0, sizeof box);
    if (gm_cube(&box, hole.min.x + 12.f, hole.min.y + 4.f, hole.min.z, 16.f, 16.f, 16.f)) {
        say(ui, "The cube did not build");
        return;
    }
    take_added(ui, box);
}

static void on_add_cyl(olui *ui, const olui_event *ev, void *userdata)
{
    gs_mesh peg;
    float cx, cy, h, r;
    (void)ev;
    (void)userdata;
    memset(&peg, 0, sizeof peg);
    cx = (hole.min.x + hole.max.x) * 0.5f;
    cy = (hole.min.y + hole.max.y) * 0.5f;
    h = (hole.max.z - hole.min.z) + 4.f;
    if (h < 8.f) h = 8.f;
    r = (hole.max.x - hole.min.x) * 0.22f;
    if (r < 3.f) r = 3.f;
    if (gm_cylinder(&peg, cx, cy, hole.min.z - 2.f, r, h, 24)) {
        say(ui, "The cylinder did not build");
        return;
    }
    take_added(ui, peg);
}

static void on_bool(olui *ui, const olui_event *ev, void *userdata)
{
    ge_scene sc = scene_out();
    int rc;
    (void)ev;
    /* Cut removes the selected part from the other one. */
    rc = ge_bool(&sc, (int)(intptr_t)userdata, hist);
    scene_in(&sc);
    if (rc == -1) {
        say(ui, "Add a second part first");
        return;
    }
    if (rc) {
        say(ui, "Those two do not meet");
        return;
    }
    picked = -1;
    verts_fit(0);
    refresh(ui);
}

/* The part as a printer's toolpath, at the Layers height, on as many
 * tasks as there are cores (gs_par: exec tasks here). */
static void on_slice(olui *ui, const olui_event *ev, void *userdata)
{
    gp_opts o;
    gc_opts go;
    (void)ev;
    (void)userdata;
    rebuild_shown();
    gp_defaults(&o);
    o.layer_h = layer_h;
    drop_slice();
    say(ui, "Slicing...");
    if (gp_part_par(&sliced, &shown, &o, gs_par_cores()) || !sliced.n) {
        drop_slice();
        say(ui, "That did not slice: out of memory, or nothing to slice");
        return;
    }
    gc_defaults(&go);
    go.layer_h = layer_h;
    gc_estimate(&sliced, NULL, &go, &slice_st);
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
    gc_opts go;
    char path[256], err[160];
    FILE *f;
    (void)ev;
    (void)userdata;
    if (!has_slice) {
        say(ui, "Slice first");
        return;
    }
    if (omv_machines_chosen(ui, "machine", &printers) < 0) {
        say(ui, "No printer: put one in PROGDIR:machines/printer");
        return;
    }
    if (omv_machines_load(&printers, omv_machines_chosen(ui, "machine", &printers), &m, err, (int)sizeof err)) {
        say(ui, err);
        return;
    }
    gc_defaults(&go);
    go.layer_h = layer_h;
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
        if (!f || gc_write(f, &sliced, &m, &go, NULL, err, (int)sizeof err)) say(ui, f ? err : "Could not write there");
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
    (void)version;
    memset(&hole, 0, sizeof hole);
    /* A file named at launch is opened, never written. Only Save writes. */
    if (argc > 1 && argv[1] && argv[1][0]) {
        if (ge_start(&hole, argv[1], err, (int)sizeof err) == 0) {
            strncpy(stl_path, argv[1], sizeof stl_path - 1);
            stl_path[sizeof stl_path - 1] = 0;
        } else {
            fprintf(stderr, "GFOpenMake: %s\n", err);
        }
    }
    if (!hole.n && ge_start(&hole, NULL, err, (int)sizeof err)) {
        fprintf(stderr, "GFOpenMake: %s\n", err);
        return 20;
    }
    hist = ge_hist_new();
    omv_gpu_open(&gpu, hole.n * 3, 512);
    gpu_on = gpu.on;
    on_chip = gpu.on_chip;
    draw_grid = gpu.grid;
    ui = olui_new();
    if (!ui || !olui_load(ui, gm_markup, gm_css, err, sizeof err)) {
        if (ui) olui_free(ui);
        ge_hist_free(hist);
        gs_mesh_free(&hole);
        gs_mesh_free(&added);
        gs_mesh_free(&shown);
        omv_gpu_close(&gpu);
        fprintf(stderr, "GFOpenMake: %s\n", ui ? err : "GFOpenLayout is not open");
        return 20;
    }
    olui_on(ui, "open", OLUI_CLICK, on_open, NULL);
    olui_on(ui, "save", OLUI_CLICK, on_save, NULL);
    olui_on(ui, "undo", OLUI_CLICK, on_undo, NULL);
    olui_on(ui, "orbit", OLUI_CLICK, on_tool, (void *)(intptr_t)TOOL_ORBIT);
    olui_on(ui, "move", OLUI_CLICK, on_tool, (void *)(intptr_t)TOOL_MOVE);
    olui_on(ui, "rot", OLUI_CLICK, on_tool, (void *)(intptr_t)TOOL_ROT);
    olui_on(ui, "scale", OLUI_CLICK, on_tool, (void *)(intptr_t)TOOL_SCALE);
    olui_on(ui, "drop", OLUI_CLICK, on_drop, NULL);
    olui_on(ui, "centre", OLUI_CLICK, on_centre, NULL);
    olui_on(ui, "lay", OLUI_CLICK, on_lay, NULL);
    olui_on(ui, "addcube", OLUI_CLICK, on_add_cube, NULL);
    olui_on(ui, "addcyl", OLUI_CLICK, on_add_cyl, NULL);
    olui_on(ui, "join", OLUI_CLICK, on_bool, (void *)(intptr_t)0);
    olui_on(ui, "cut", OLUI_CLICK, on_bool, (void *)(intptr_t)1);
    olui_on(ui, "meet", OLUI_CLICK, on_bool, (void *)(intptr_t)2);
    olui_on(ui, "lh", OLUI_CHANGE, on_height, NULL);
    olui_on(ui, "show", OLUI_CHANGE, on_show, NULL);
    olui_on(ui, "prev", OLUI_CLICK, on_step, (void *)(intptr_t)-1);
    olui_on(ui, "next", OLUI_CLICK, on_step, (void *)(intptr_t)1);
    olui_on(ui, "layer", OLUI_CHANGE, on_slide, NULL);
    olui_on(ui, "slice", OLUI_CLICK, on_slice, NULL);
    olui_on(ui, "gcode", OLUI_CLICK, on_gcode, NULL);
    gs_cam_home(&cam);
    w = OpenWindowTags(NULL,
        WA_Title, (ULONG)"GFOpenMake",
        WA_InnerWidth, 640,
        WA_InnerHeight, 580, /* the Slice group under Layers */
        WA_MinWidth, 320,
        WA_MinHeight, 280,
        WA_MaxWidth, (ULONG)~0,
        WA_MaxHeight, (ULONG)~0,
        WA_DragBar, TRUE,
        WA_DepthGadget, TRUE,
        WA_CloseGadget, TRUE,
        WA_SizeGadget, TRUE,
        WA_SizeBBottom, TRUE,
        WA_IDCMP, olui_amiga_idcmp(),
        WA_ReportMouse, TRUE,
        WA_Activate, TRUE,
        TAG_END);
    if (!w) {
        ge_hist_free(hist);
        gs_mesh_free(&hole);
        gs_mesh_free(&added);
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
        ge_hist_free(hist);
        gs_mesh_free(&hole);
        gs_mesh_free(&added);
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
    for (;;) {
        int r, lx, ly;
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
                if (in_view(ui, w, mx, my, NULL, NULL)) {
                    dragging = 1;
                    drag_moved = 0;
                    pushed = 0;
                    drag_x = mx;
                    drag_y = my;
                    drag_shift = (qual & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT)) != 0;
                }
            } else if (cls == IDCMP_MOUSEBUTTONS && code == SELECTUP) {
                if (dragging && tool == TOOL_ORBIT && drag_moved < 4 &&
                    in_view(ui, w, mx, my, &lx, &ly)) {
                    int face = -1;
                    if (has_added) face = ge_pick(&added, &cam, vw, vh, lx, ly);
                    if (face >= 0) on_added = 1;
                    else {
                        face = ge_pick(&hole, &cam, vw, vh, lx, ly);
                        on_added = 0;
                    }
                    picked = face;
                    face_line(ui);
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
                if (tool == TOOL_ORBIT) {
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
    olui_amiga_free(a);
    CloseWindow(w);
    ge_hist_free(hist);
    gs_mesh_free(&hole);
    gs_mesh_free(&added);
    gs_mesh_free(&shown);
    gl_walls_free(&walls);
    drop_slice();
    omv_machines_free(&printers);
    olui_free(ui);
    omv_gpu_close(&gpu);
    return 0;
}
