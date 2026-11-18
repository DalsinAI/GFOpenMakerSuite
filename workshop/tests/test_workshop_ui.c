/* GFOpenMakerWorkshop's window (workshop/src/markup.h), laid out on the
 * host by the real GFOpenLayout at the size amiga.c opens it: 640 by 480
 * inside, which with the title bar and sizing border fits a 640 by 512
 * PAL screen, and again at 640 by 512 and 1920 by 1080 (the window is
 * resizable). The item list always shows the known kind groups, plus a
 * group for any other kind the project has.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#include "workshop_ui.h"
#include "omview_ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int fails;

static void expect(int ok, const char *what)
{
    if (!ok) {
        printf("fail: %s\n", what);
        fails++;
    }
}

static int measure(void *userdata, const char *utf8, size_t length, const ol_style *style, ol_text_metrics *metrics)
{
    (void)userdata;
    (void)utf8;
    metrics->width = (ol_unit)(length * (size_t)style->font_size * 60 / 100);
    metrics->ascent = style->font_size * 80 / 100;
    metrics->descent = style->font_size * 22 / 100;
    return 1;
}

static int inside(const olui_rect *r, int w, int h)
{
    return r->w > 0 && r->h > 0 && r->x >= 0 && r->y >= 0 && r->x + r->w <= w && r->y + r->h <= h;
}

static int overlap(const olui_rect *a, const olui_rect *b)
{
    return a->x < b->x + b->w && b->x < a->x + a->w && a->y < b->y + b->h && b->y < a->y + a->h;
}

static int has_text(const olui_el *el, const char *text)
{
    const char *t;
    olui_el *c;
    if (!el) return 0;
    t = olui_get_text(el);
    if (t && !strcmp(t, text)) return 1;
    for (c = olui_first_child(el); c; c = olui_next(c))
        if (has_text(c, text)) return 1;
    return 0;
}

static const char *const controls[][2] = {
    {"new", "button"},       {"open", "button"},   {"save", "button"},     {"product", "input"},
    {"recent", "select"},    {"items", "list"},    {"itemname", "input"},  {"add", "button"},
    {"addkind", "select"},   {"remove", "button"}, {"rename", "button"},   {"launch", "button"},
    {"notes", "input"},      {"view", "div"},      {"status", "div"},
};
#define N ((int)(sizeof controls / sizeof controls[0]))

/* A cube as ASCII STL, enough for gs_stl_load and a solid preview. */
static const char cube_stl[] =
    "solid cube\n"
    "  facet normal 0 0 1\n"
    "    outer loop\n"
    "      vertex 0 0 1\n"
    "      vertex 1 0 1\n"
    "      vertex 0 1 1\n"
    "    endloop\n"
    "  endfacet\n"
    "  facet normal 0 0 1\n"
    "    outer loop\n"
    "      vertex 1 0 1\n"
    "      vertex 1 1 1\n"
    "      vertex 0 1 1\n"
    "    endloop\n"
    "  endfacet\n"
    "  facet normal 0 0 -1\n"
    "    outer loop\n"
    "      vertex 0 0 0\n"
    "      vertex 0 1 0\n"
    "      vertex 1 0 0\n"
    "    endloop\n"
    "  endfacet\n"
    "  facet normal 0 0 -1\n"
    "    outer loop\n"
    "      vertex 1 0 0\n"
    "      vertex 0 1 0\n"
    "      vertex 1 1 0\n"
    "    endloop\n"
    "  endfacet\n"
    "  facet normal 0 -1 0\n"
    "    outer loop\n"
    "      vertex 0 0 0\n"
    "      vertex 1 0 0\n"
    "      vertex 0 0 1\n"
    "    endloop\n"
    "  endfacet\n"
    "  facet normal 0 -1 0\n"
    "    outer loop\n"
    "      vertex 1 0 0\n"
    "      vertex 1 0 1\n"
    "      vertex 0 0 1\n"
    "    endloop\n"
    "  endfacet\n"
    "  facet normal 0 1 0\n"
    "    outer loop\n"
    "      vertex 0 1 0\n"
    "      vertex 0 1 1\n"
    "      vertex 1 1 0\n"
    "    endloop\n"
    "  endfacet\n"
    "  facet normal 0 1 0\n"
    "    outer loop\n"
    "      vertex 1 1 0\n"
    "      vertex 0 1 1\n"
    "      vertex 1 1 1\n"
    "    endloop\n"
    "  endfacet\n"
    "  facet normal -1 0 0\n"
    "    outer loop\n"
    "      vertex 0 0 0\n"
    "      vertex 0 0 1\n"
    "      vertex 0 1 0\n"
    "    endloop\n"
    "  endfacet\n"
    "  facet normal -1 0 0\n"
    "    outer loop\n"
    "      vertex 0 1 0\n"
    "      vertex 0 0 1\n"
    "      vertex 0 1 1\n"
    "    endloop\n"
    "  endfacet\n"
    "  facet normal 1 0 0\n"
    "    outer loop\n"
    "      vertex 1 0 0\n"
    "      vertex 1 1 0\n"
    "      vertex 1 0 1\n"
    "    endloop\n"
    "  endfacet\n"
    "  facet normal 1 0 0\n"
    "    outer loop\n"
    "      vertex 1 1 0\n"
    "      vertex 1 1 1\n"
    "      vertex 1 0 1\n"
    "    endloop\n"
    "  endfacet\n"
    "endsolid cube\n";

static const char plate_svg[] =
    "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"40mm\" height=\"30mm\" viewBox=\"0 0 40 30\">"
    "<rect x=\"2\" y=\"2\" width=\"36\" height=\"26\" fill=\"none\" stroke=\"#0067c0\" stroke-width=\"0.8\"/>"
    "<circle cx=\"20\" cy=\"15\" r=\"8\" fill=\"none\" stroke=\"#c00000\" stroke-width=\"0.8\"/>"
    "</svg>";

static int write_file(const char *path, const char *text)
{
    FILE *f = fopen(path, "w");
    if (!f) return -1;
    if (fputs(text, f) == EOF) {
        fclose(f);
        return -1;
    }
    fclose(f);
    return 0;
}

static int count_lit(const unsigned char *rgb, int n, unsigned char r0, unsigned char g0, unsigned char b0)
{
    int i, lit = 0;
    for (i = 0; i < n; i++)
        if (rgb[i * 3] != r0 || rgb[i * 3 + 1] != g0 || rgb[i * 3 + 2] != b0) lit++;
    return lit;
}

static int layout_ok(olui *ui, int w, int h, const char *what)
{
    olui_rect box[N], view, status, side, bar;
    int i, j, before = fails;
    char line[96];
    snprintf(line, sizeof line, "the window lays out at %s", what);
    if (!olui_layout(ui, w, h)) {
        expect(0, line);
        return 0;
    }
    expect(1, line);
    for (i = 0; i < N; i++) {
        olui_el *el = olui_find(ui, controls[i][0]);
        snprintf(line, sizeof line, "%s is a %s at %s", controls[i][0], controls[i][1], what);
        expect(el && !strcmp(olui_tag(el), controls[i][1]) && olui_box(ui, el, &box[i]) && inside(&box[i], w, h), line);
    }
    olui_box(ui, olui_find(ui, "view"), &view);
    olui_box(ui, olui_find(ui, "status"), &status);
    olui_box(ui, olui_find(ui, "side"), &side);
    olui_box(ui, olui_find(ui, "bar"), &bar);
    snprintf(line, sizeof line, "the view is clear of the side, the bar and the status line at %s", what);
    expect(view.h >= h / 3 && view.w >= w / 3 && !overlap(&view, &side) && !overlap(&view, &status) &&
               !overlap(&view, &bar),
           line);
    snprintf(line, sizeof line, "the item list is in the side, above the status line at %s", what);
    expect(box[5].x >= side.x && box[5].x + box[5].w <= side.x + side.w && box[5].y + box[5].h <= status.y, line);
    snprintf(line, sizeof line, "the status line sits under the view at %s", what);
    expect(status.y >= view.y + view.h - 1 && inside(&status, w, h), line);
    for (i = 0; i < 5; i++) {
        for (j = 0; j < i; j++) {
            snprintf(line, sizeof line, "%s is clear of %s at %s", controls[i][0], controls[j][0], what);
            expect(!overlap(&box[i], &box[j]), line);
        }
        snprintf(line, sizeof line, "%s is clear of the view at %s", controls[i][0], what);
        expect(!overlap(&box[i], &view), line);
    }
    for (i = 7; i <= 11; i++) {
        for (j = 7; j < i; j++) {
            snprintf(line, sizeof line, "%s is clear of %s at %s", controls[i][0], controls[j][0], what);
            expect(!overlap(&box[i], &box[j]), line);
        }
    }
    return fails == before;
}

int main(void)
{
    olui *ui = olui_new();
    char err[160], dir[256], stl[320], svg[320], kicad[320], rec[320];
    workshop w;
    unsigned char *rgb;
    gpr_add_opts o;
    int sizes[][2] = {{640, 480}, {640, 512}, {1920, 1080}};
    int s, lit;
    olui_el *list;
    char *cwd;
    char rm[600];

    workshop_init(&w);
    olui_set_measure(ui, measure, NULL);
    expect(workshop_ui_load(ui, err, (int)sizeof err), "the window loads");
    expect(olui_find(ui, "ver") == NULL, "the name and version are in the title bar, not a row");

    workshop_ui_refresh(ui, &w);
    expect(layout_ok(ui, 640, 480, "640 by 480, as amiga.c opens it"), "layout at the minimum inside size");
    list = olui_find(ui, "items");
    expect(has_text(list, "CAD") && has_text(list, "Print") && has_text(list, "Laser") && has_text(list, "CNC") &&
               has_text(list, "Electronics"),
           "an empty window still shows the five known groups");
    expect(!has_text(list, "render") && !has_text(list, "Render"), "no Render group until a project has that kind");
    expect(!strcmp(olui_get_text(olui_find(ui, "status")), "No project"), "the status starts as No project");
    expect(workshop_launch(&w, err, (int)sizeof err) == -1, "Open in app refuses when no item is picked");

    cwd = getcwd(NULL, 0);
    expect(cwd != NULL, "the working directory");
    snprintf(dir, sizeof dir, "%s/build/ws-ui-proj", cwd ? cwd : ".");
    snprintf(stl, sizeof stl, "%s/body.stl", dir);
    snprintf(svg, sizeof svg, "%s/plate.svg", dir);
    snprintf(kicad, sizeof kicad, "%s/Widget.kicad_pro", dir);
    snprintf(rec, sizeof rec, "%s/workshop-recent.om", dir);
    mkdir("build", 0755);
    snprintf(rm, sizeof rm, "rm -rf %s", dir);
    if (system(rm) != 0) expect(0, "clear the fixture drawer");
    expect(gpr_create(&w.project, dir, "Widget", err, (int)sizeof err) == 0, "a Widget project");
    w.have = 1;
    gpr_set_notes(&w.project, "A shelf bracket");
    expect(write_file(stl, cube_stl) == 0 && write_file(svg, plate_svg) == 0 &&
               write_file(kicad, "(kicad_pro (version 20240101))\n") == 0,
           "fixture files");
    gpr_add_defaults(&o);
    o.kind = "cad";
    o.file = stl;
    o.name = "Body";
    expect(gpr_add(&w.project, &o, err, (int)sizeof err) == 0, err[0] ? err : "add the CAD body");
    gpr_add_defaults(&o);
    o.kind = "laser";
    o.file = svg;
    o.name = "Plate";
    expect(gpr_add(&w.project, &o, err, (int)sizeof err) == 1, err[0] ? err : "add the laser plate");
    gpr_add_defaults(&o);
    o.kind = "electronics";
    o.file = kicad;
    o.name = "Board";
    expect(gpr_add(&w.project, &o, err, (int)sizeof err) == 2, err[0] ? err : "add the KiCad project, referenced");
    gpr_add_defaults(&o);
    o.kind = "render";
    o.file = stl;
    o.name = "Shot";
    o.copy = 0;
    expect(gpr_add(&w.project, &o, err, (int)sizeof err) == 3, err[0] ? err : "add an unknown render item, referenced");
    gpr_add_defaults(&o);
    o.kind = "cnc";
    o.file = svg;
    o.name = "Job";
    o.copy = 0;
    expect(gpr_add(&w.project, &o, err, (int)sizeof err) == 4, err[0] ? err : "add a mill drawing");
    w.picked = 0;
    if (strlen(rec) >= sizeof w.recent_path) expect(0, "the recent path is too long");
    else memcpy(w.recent_path, rec, strlen(rec) + 1);
    gpr_recent_touch(&w.recent, dir);
    workshop_ui_refresh(ui, &w);

    expect(!strcmp(olui_get_text(olui_find(ui, "product")), "Widget"), "the product name is in the field");
    expect(!strcmp(olui_get_text(olui_find(ui, "notes")), "A shelf bracket"), "the notes are in the field");
    expect(has_text(list, "CAD") && has_text(list, "Body  cad/body.stl"), "the CAD group holds Body");
    expect(has_text(list, "Laser") && has_text(list, "Plate  laser/plate.svg"), "the Laser group holds Plate");
    expect(has_text(list, "Electronics") && has_text(list, "Board  Widget.kicad_pro"),
           "Electronics holds the KiCad file, referenced in place");
    expect(has_text(list, "render") && has_text(list, "Shot  body.stl"), "an unknown kind gets its own group");
    expect(olui_list_selected(list) && strstr(olui_get_text(olui_list_selected(list)), "Body"),
           "the picked CAD item is selected");
    expect(workshop_launch(&w, err, (int)sizeof err) == 0 && strstr(workshop_launch_last(&w), "GFOpenMake") &&
               strstr(workshop_launch_last(&w), "body.stl"),
           "Open in app records GFOpenMake and the STL (the host hook)");

    rgb = (unsigned char *)malloc(80 * 60 * 3);
    expect(rgb != NULL, "a preview buffer");
    w.picked = 0;
    expect(workshop_ui_preview(&w, NULL, rgb, 80, 60, err, (int)sizeof err) == 0, err[0] ? err : "a solid preview of the CAD item");
    lit = count_lit(rgb, 80 * 60, 0x14, 0x16, 0x1c);
    expect(lit > 10, "the CAD preview is not an empty view");
    w.picked = 1;
    expect(workshop_ui_preview(&w, NULL, rgb, 80, 60, err, (int)sizeof err) == 0,
           err[0] ? err : "a line preview of the laser drawing");
    lit = count_lit(rgb, 80 * 60, 0x14, 0x16, 0x1c);
    expect(lit > 10, "the laser preview drew strokes");
    w.picked = 2;
    expect(workshop_ui_preview(&w, NULL, rgb, 80, 60, err, (int)sizeof err) == -1 && strstr(err, "gfamigakicad"),
           "a KiCad file is left to gfamigakicad");
    lit = count_lit(rgb, 80 * 60, 0x14, 0x16, 0x1c);
    expect(lit == 0, "the electronics preview stays a dark view");
    w.picked = 3;
    expect(workshop_ui_preview(&w, NULL, rgb, 80, 60, err, (int)sizeof err) == 0,
           "a render item that points at an STL still shows the solid (the file, not the kind)");
    free(rgb);

    w.picked = 2;
    workshop_ui_refresh(ui, &w);
    expect(strstr(olui_get_text(olui_find(ui, "status")), "gfamigakicad opens this") != NULL,
           "the status trusts gfamigakicad with the board");
    expect(workshop_launch(&w, err, (int)sizeof err) == 0 && strstr(workshop_launch_last(&w), "gfamigakicad"),
           "Open in app records gfamigakicad for electronics");
    w.picked = 3;
    workshop_ui_refresh(ui, &w);
    expect(workshop_launch(&w, err, (int)sizeof err) == -1, "the hook refuses a kind with no app");
    w.picked = 4;
    workshop_ui_refresh(ui, &w);
    expect(workshop_launch(&w, err, (int)sizeof err) == 0 && strstr(workshop_launch_last(&w), "GFOpenCNC") &&
               strstr(workshop_launch_last(&w), ".svg"),
           "Open in app records GFOpenCNC for a mill drawing");

    w.picked = 0;
    workshop_ui_refresh(ui, &w);
    for (s = 0; s < 3; s++) {
        char what[40];
        snprintf(what, sizeof what, "%d by %d", sizes[s][0], sizes[s][1]);
        layout_ok(ui, sizes[s][0], sizes[s][1], what);
    }

    workshop_free(&w);
    olui_free(ui);
    free(cwd);
    if (fails) printf("%d failed\n", fails);
    else printf("ok\n");
    return fails ? 1 : 0;
}
