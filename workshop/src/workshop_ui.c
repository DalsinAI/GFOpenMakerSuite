/* GFOpenMakerWorkshop's window helpers: see workshop_ui.h.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "workshop_ui.h"
#include "markup.h"
#include "omview_ui.h"

#include "gs.h"
#include "gs_vec.h"

#if defined(__amigaos__) || defined(AMIGA) || defined(__AMIGA__)
#include <dos/dostags.h>
#include <proto/dos.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const known_kinds[] = {"cad", "print", "laser", "cnc", "electronics"};
#define N_KNOWN ((int)(sizeof known_kinds / sizeof known_kinds[0]))

void workshop_init(workshop *w)
{
    memset(w, 0, sizeof *w);
    gpr_init(&w->project);
    gpr_recent_init(&w->recent);
    w->picked = -1;
}

void workshop_free(workshop *w)
{
    if (!w) return;
    gpr_free(&w->project);
    w->have = 0;
    w->picked = -1;
    w->last_launch[0] = 0;
}

int workshop_ui_load(olui *ui, char *err, int errlen)
{
    return olui_load(ui, workshop_markup, workshop_css, err, errlen);
}

static int ends_ci(const char *s, const char *suf)
{
    size_t n, m;
    if (!s || !suf) return 0;
    n = strlen(s);
    m = strlen(suf);
    if (n < m) return 0;
    s += n - m;
    while (*suf) {
        int a = *s++, b = *suf++;
        if (a >= 'A' && a <= 'Z') a += 'a' - 'A';
        if (b >= 'A' && b <= 'Z') b += 'a' - 'A';
        if (a != b) return 0;
    }
    return 1;
}

static int kind_is(const char *kind, const char *want)
{
    if (!kind || !want) return 0;
    while (*kind || *want) {
        int a = *kind ? *kind++ : 0, b = *want ? *want++ : 0;
        if (a >= 'A' && a <= 'Z') a += 'a' - 'A';
        if (b >= 'A' && b <= 'Z') b += 'a' - 'A';
        if (a != b) return 0;
    }
    return 1;
}

static void fill_dark(unsigned char *rgb, int w, int h)
{
    int i, n = w * h;
    for (i = 0; i < n; i++) {
        rgb[i * 3] = 0x14;
        rgb[i * 3 + 1] = 0x16;
        rgb[i * 3 + 2] = 0x1c;
    }
}

static void item_line(char *line, int n, const gpr_item *it)
{
    snprintf(line, (size_t)n, "%s  %s", it->name[0] ? it->name : "(unnamed)", it->file);
}

static int already_group(const char *kind, const char *const *seen, int nseen)
{
    int i;
    for (i = 0; i < nseen; i++)
        if (kind_is(seen[i], kind)) return 1;
    return 0;
}

static void fill_items(olui *ui, workshop *w)
{
    olui_el *list = olui_find(ui, "items"), *pick = NULL;
    const char *unknown[GPR_ITEMS_MAX];
    int nunk = 0, i, k;
    char gval[40];
    olui_list_clear(ui, list);
    for (k = 0; k < N_KNOWN; k++) {
        olui_el *group;
        snprintf(gval, sizeof gval, "kind:%s", known_kinds[k]);
        group = olui_list_add(ui, list, NULL, gpr_kind_group(known_kinds[k]), gval);
        if (!group) continue;
        olui_item_open(ui, group, 1);
        for (i = 0; w->have && i < w->project.n; i++) {
            char line[GPR_NAME_MAX + GPR_PATH_MAX + 4], iv[12];
            olui_el *el;
            if (!kind_is(w->project.items[i].kind, known_kinds[k])) continue;
            item_line(line, (int)sizeof line, &w->project.items[i]);
            snprintf(iv, sizeof iv, "%d", i);
            el = olui_list_add(ui, list, group, line, iv);
            if (i == w->picked) pick = el;
        }
    }
    for (i = 0; w->have && i < w->project.n; i++) {
        const char *kind = w->project.items[i].kind;
        if (gpr_kind_known(kind)) continue;
        if (!already_group(kind, unknown, nunk) && nunk < GPR_ITEMS_MAX) unknown[nunk++] = kind;
    }
    for (k = 0; k < nunk; k++) {
        const char *kind = unknown[k];
        const char *title = gpr_kind_group(kind);
        olui_el *group;
        snprintf(gval, sizeof gval, "kind:%s", kind);
        group = olui_list_add(ui, list, NULL, title[0] ? title : kind, gval);
        if (!group) continue;
        olui_item_open(ui, group, 1);
        for (i = 0; i < w->project.n; i++) {
            char line[GPR_NAME_MAX + GPR_PATH_MAX + 4], iv[12];
            olui_el *el;
            if (!kind_is(w->project.items[i].kind, kind)) continue;
            item_line(line, (int)sizeof line, &w->project.items[i]);
            snprintf(iv, sizeof iv, "%d", i);
            el = olui_list_add(ui, list, group, line, iv);
            if (i == w->picked) pick = el;
        }
    }
    if (pick) olui_list_select(ui, list, pick);
}


static void fill_recent(olui *ui, workshop *w)
{
    olui_el *sel = olui_find(ui, "recent");
    int i;
    olui_select_clear(ui, sel);
    olui_select_add(ui, sel, "Recent", "");
    for (i = 0; i < w->recent.n; i++) olui_select_add(ui, sel, w->recent.path[i], w->recent.path[i]);
    olui_select_set(ui, sel, 0);
}

void workshop_ui_refresh(olui *ui, workshop *w)
{
    const gpr_item *it = (w->have && w->picked >= 0 && w->picked < w->project.n) ? &w->project.items[w->picked] : NULL;
    int can_item = it != NULL;
    int can_launch = can_item && gpr_kind_app(it->kind) != NULL;
    char defstat[160];

    if (w->picked >= w->project.n) w->picked = -1;
    olui_set_text(ui, olui_find(ui, "product"), w->have ? w->project.name : "");
    olui_set_text(ui, olui_find(ui, "notes"), w->have ? w->project.notes : "");
    olui_set_text(ui, olui_find(ui, "itemname"), it ? it->name : "");
    olui_set_disabled(ui, olui_find(ui, "save"), !w->have);
    olui_set_disabled(ui, olui_find(ui, "add"), !w->have);
    olui_set_disabled(ui, olui_find(ui, "notes"), !w->have);
    olui_set_disabled(ui, olui_find(ui, "product"), !w->have);
    olui_set_disabled(ui, olui_find(ui, "remove"), !can_item);
    olui_set_disabled(ui, olui_find(ui, "rename"), !can_item);
    olui_set_disabled(ui, olui_find(ui, "itemname"), !can_item);
    olui_set_disabled(ui, olui_find(ui, "launch"), !can_launch);
    fill_items(ui, w);
    fill_recent(ui, w);
    if (w->status[0]) {
        omv_say(ui, w->status);
        return;
    }
    if (!w->have) snprintf(defstat, sizeof defstat, "No project");
    else if (w->project.n == 0) snprintf(defstat, sizeof defstat, "%s — no items yet", w->project.name);
    else snprintf(defstat, sizeof defstat, "%s — %d item%s%s", w->project.name, w->project.n, w->project.n == 1 ? "" : "s",
                  w->project.dirty ? ", not saved" : "");
    omv_say(ui, defstat);
}

static void plot(unsigned char *rgb, int w, int h, int x, int y, unsigned colour)
{
    unsigned char *p;
    if (x < 0 || y < 0 || x >= w || y >= h) return;
    p = rgb + ((size_t)y * (size_t)w + (size_t)x) * 3;
    p[0] = (unsigned char)((colour >> 16) & 0xff);
    p[1] = (unsigned char)((colour >> 8) & 0xff);
    p[2] = (unsigned char)(colour & 0xff);
}

static void line_to(unsigned char *rgb, int w, int h, int x0, int y0, int x1, int y1, unsigned colour)
{
    int dx = x1 - x0, dy = y1 - y0, sx = dx < 0 ? -1 : 1, sy = dy < 0 ? -1 : 1, err, e2;
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    err = (dx > dy ? dx : -dy) / 2;
    for (;;) {
        plot(rgb, w, h, x0, y0, colour);
        if (x0 == x1 && y0 == y1) break;
        e2 = err;
        if (e2 > -dx) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dy) {
            err += dx;
            y0 += sy;
        }
    }
}

static int paint_vec(const char *path, unsigned char *rgb, int w, int h, char *err, int errlen)
{
    gv_doc d;
    float minx, miny, maxx, maxy, sx, sy, s, ox, oy;
    int i, j, pad = w < h ? w / 16 : h / 16;
    gv_doc_init(&d);
    if (gv_load(&d, path, err, errlen)) {
        gv_doc_free(&d);
        return -1;
    }
    minx = d.minx;
    miny = d.miny;
    maxx = d.maxx;
    maxy = d.maxy;
    if (maxx <= minx) {
        minx = 0.f;
        maxx = d.width > 0.f ? d.width : 1.f;
    }
    if (maxy <= miny) {
        miny = 0.f;
        maxy = d.height > 0.f ? d.height : 1.f;
    }
    sx = (float)(w - 2 * pad) / (maxx - minx);
    sy = (float)(h - 2 * pad) / (maxy - miny);
    s = sx < sy ? sx : sy;
    ox = (float)pad - minx * s + ((float)(w - 2 * pad) - (maxx - minx) * s) * 0.5f;
    oy = (float)h - (float)pad + miny * s - ((float)(h - 2 * pad) - (maxy - miny) * s) * 0.5f;
    for (i = 0; i < d.n; i++) {
        const gv_path *p = &d.p[i];
        unsigned colour = p->stroke != GV_NONE ? p->stroke : (p->fill != GV_NONE ? p->fill : 0x00e0e0e0u);
        for (j = 1; j < p->n; j++) {
            float x0 = p->xy[(j - 1) * 2], y0 = p->xy[(j - 1) * 2 + 1];
            float x1 = p->xy[j * 2], y1 = p->xy[j * 2 + 1];
            line_to(rgb, w, h, (int)(ox + x0 * s + 0.5f), (int)(oy - y0 * s + 0.5f), (int)(ox + x1 * s + 0.5f),
                    (int)(oy - y1 * s + 0.5f), colour);
        }
        if (p->closed && p->n > 2) {
            float x0 = p->xy[(p->n - 1) * 2], y0 = p->xy[(p->n - 1) * 2 + 1];
            float x1 = p->xy[0], y1 = p->xy[1];
            line_to(rgb, w, h, (int)(ox + x0 * s + 0.5f), (int)(oy - y0 * s + 0.5f), (int)(ox + x1 * s + 0.5f),
                    (int)(oy - y1 * s + 0.5f), colour);
        }
    }
    gv_doc_free(&d);
    return 0;
}

static int paint_stl(const char *path, const gs_cam *cam, unsigned char *rgb, int w, int h, char *err, int errlen)
{
    gs_mesh m;
    gs_cam home;
    int rc;
    memset(&m, 0, sizeof m);
    if (gs_stl_load(&m, path, err, errlen)) return -1;
    if (!cam) {
        gs_cam_home(&home);
        cam = &home;
    }
    rc = gs_view_solid(&m, cam, rgb, w, h);
    gs_mesh_free(&m);
    if (rc) {
        if (err && errlen > 0) snprintf(err, (size_t)errlen, "the solid could not be drawn");
        return -1;
    }
    return 0;
}

static int read_all(const char *path, char **out, int *len, char *err, int errlen)
{
    FILE *f = fopen(path, "rb");
    long n;
    char *buf;
    *out = NULL;
    *len = 0;
    if (!f) {
        if (err && errlen > 0) snprintf(err, (size_t)errlen, "cannot open the file");
        return -1;
    }
    if (fseek(f, 0, SEEK_END) || (n = ftell(f)) < 0 || n > 4 * 1024 * 1024) {
        fclose(f);
        if (err && errlen > 0) snprintf(err, (size_t)errlen, "the file is too large");
        return -1;
    }
    if (fseek(f, 0, SEEK_SET)) {
        fclose(f);
        return -1;
    }
    buf = (char *)malloc((size_t)n + 1);
    if (!buf) {
        fclose(f);
        if (err && errlen > 0) snprintf(err, (size_t)errlen, "out of memory");
        return -1;
    }
    if (n && fread(buf, 1, (size_t)n, f) != (size_t)n) {
        free(buf);
        fclose(f);
        if (err && errlen > 0) snprintf(err, (size_t)errlen, "cannot read the file");
        return -1;
    }
    buf[n] = 0;
    fclose(f);
    *out = buf;
    *len = (int)n;
    return 0;
}

static int paint_gcode(const char *path, unsigned char *rgb, int w, int h, char *err, int errlen)
{
    char *text = NULL;
    int n = 0, rc;
    if (read_all(path, &text, &n, err, errlen)) return -1;
    rc = gs_view_paint(text, n, rgb, w, h);
    free(text);
    if (rc) {
        if (err && errlen > 0) snprintf(err, (size_t)errlen, "the toolpath could not be drawn");
        return -1;
    }
    return 0;
}

int workshop_ui_preview(const workshop *w, const gs_cam *cam, unsigned char *rgb, int width, int height, char *err,
                        int errlen)
{
    char path[GPR_PATH_MAX + 1];
    const gpr_item *it;
    if (rgb && width > 0 && height > 0) fill_dark(rgb, width, height);
    if (!w || !w->have || w->picked < 0 || w->picked >= w->project.n || !rgb || width <= 0 || height <= 0) return -1;
    it = &w->project.items[w->picked];
    if (gpr_resolve(&w->project, w->picked, path, (int)sizeof path)) return -1;
    if (ends_ci(path, ".stl")) return paint_stl(path, cam, rgb, width, height, err, errlen);
    if (ends_ci(path, ".svg") || ends_ci(path, ".dxf")) return paint_vec(path, rgb, width, height, err, errlen);
    if (ends_ci(path, ".gcode") || ends_ci(path, ".nc") || ends_ci(path, ".gc"))
        return paint_gcode(path, rgb, width, height, err, errlen);
    (void)it;
    if (err && errlen > 0) snprintf(err, (size_t)errlen, "no preview for this file");
    return -1;
}

int workshop_launch(workshop *w, char *err, int errlen)
{
    const gpr_item *it;
    const char *app;
    char file[GPR_PATH_MAX + 1];
    char cmd[512];
#if defined(__amigaos__) || defined(AMIGA) || defined(__AMIGA__)
    BPTR lock;
#endif
    w->last_launch[0] = 0;
    if (!w->have || w->picked < 0 || w->picked >= w->project.n) {
        if (err && errlen > 0) snprintf(err, (size_t)errlen, "no item is picked");
        return -1;
    }
    it = &w->project.items[w->picked];
    app = gpr_kind_app(it->kind);
    if (!app) {
        if (err && errlen > 0) snprintf(err, (size_t)errlen, "no app for this kind");
        return -1;
    }
    if (gpr_resolve(&w->project, w->picked, file, (int)sizeof file)) {
        if (err && errlen > 0) snprintf(err, (size_t)errlen, "the file path is too long");
        return -1;
    }
#if defined(__amigaos__) || defined(AMIGA) || defined(__AMIGA__)
    snprintf(cmd, sizeof cmd, "PROGDIR:%s", app);
    lock = Lock((STRPTR)cmd, ACCESS_READ);
    if (lock) {
        UnLock(lock);
        snprintf(cmd, sizeof cmd, "Run >NIL: PROGDIR:%s \"%s\"", app, file);
    } else {
        snprintf(cmd, sizeof cmd, "Run >NIL: %s \"%s\"", app, file);
    }
    snprintf(w->last_launch, sizeof w->last_launch, "%s %s", app, file);
    if (SystemTags((STRPTR)cmd, TAG_END) == -1) {
        if (err && errlen > 0) snprintf(err, (size_t)errlen, "the app did not start");
        return -1;
    }
    return 0;
#else
    snprintf(cmd, sizeof cmd, "%s %s", app, file);
    snprintf(w->last_launch, sizeof w->last_launch, "%s", cmd);
    (void)err;
    (void)errlen;
    return 0;
#endif
}

const char *workshop_launch_last(const workshop *w) { return w && w->last_launch[0] ? w->last_launch : ""; }
