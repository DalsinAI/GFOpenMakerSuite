/* Move, turn, scale, drop, centre, lay flat, undo.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gs_edit.h"
#include "gs_model.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fails;

static void expect(int cond, const char *msg)
{
    if (!cond) {
        fprintf(stderr, "fail: %s\n", msg);
        fails++;
    }
}

static int near(float a, float b)
{
    float d = a - b;
    if (d < 0.f) d = -d;
    return d < 0.02f;
}

static void one(gs_mesh *m, float x, float y, float z)
{
    memset(m, 0, sizeof *m);
    m->t = (gs_tri *)calloc(1, sizeof(gs_tri));
    m->n = m->cap = 1;
    m->t[0].a = (gs_v){x, y, z};
    m->t[0].b = (gs_v){x + 2.f, y, z};
    m->t[0].c = (gs_v){x, y + 2.f, z};
    m->min = m->t[0].a;
    m->max = m->t[0].b;
    if (m->t[0].c.y > m->max.y) m->max.y = m->t[0].c.y;
}

static float signed_volume(const gs_mesh *m)
{
    double s = 0;
    int i;
    for (i = 0; i < m->n; i++) {
        gs_v a = m->t[i].a, b = m->t[i].b, c = m->t[i].c;
        s += (double)a.x * ((double)b.y * c.z - (double)c.y * b.z) + (double)b.x * ((double)c.y * a.z - (double)a.y * c.z) +
             (double)c.x * ((double)a.y * b.z - (double)b.y * a.z);
    }
    return (float)(s / 6.0);
}

/* Every edge met once each way: closed, no edge split on one side only. */
static int closed(const gs_mesh *m)
{
    int i, j, k, l;
    for (i = 0; i < m->n; i++)
        for (k = 0; k < 3; k++) {
            const gs_v *a = k == 0 ? &m->t[i].a : k == 1 ? &m->t[i].b : &m->t[i].c;
            const gs_v *b = k == 0 ? &m->t[i].b : k == 1 ? &m->t[i].c : &m->t[i].a;
            int back = 0;
            for (j = 0; j < m->n; j++)
                for (l = 0; l < 3; l++) {
                    const gs_v *c = l == 0 ? &m->t[j].a : l == 1 ? &m->t[j].b : &m->t[j].c;
                    const gs_v *e = l == 0 ? &m->t[j].b : l == 1 ? &m->t[j].c : &m->t[j].a;
                    back += !memcmp(c, b, sizeof *c) && !memcmp(e, a, sizeof *e);
                }
            if (back != 1) return 0;
        }
    return 1;
}

int main(void)
{
    gs_mesh m, box;
    ge_hist *h;
    float z0;
    memset(&m, 0, sizeof m);
    memset(&box, 0, sizeof box);
    one(&m, 0.f, 0.f, 3.f);
    ge_move(&m, 5.f, 0.f, 0.f);
    expect(near(m.t[0].a.x, 5.f), "move x");
    expect(near(m.min.z, 3.f), "move keeps z");
    ge_drop(&m);
    expect(near(m.min.z, 0.f) && near(m.t[0].a.z, 0.f), "drop onto the bed");
    ge_centre(&m);
    expect(near((m.min.x + m.max.x) * 0.5f, GS_BED * 0.5f), "centre x");
    expect(near((m.min.y + m.max.y) * 0.5f, GS_BED * 0.5f), "centre y");
    z0 = m.min.z;
    ge_scale(&m, 2.f, 2.f, 2.f);
    expect(near(m.max.x - m.min.x, 4.f), "scale width");
    expect(near(m.min.z, z0), "scale keeps the centre's z");
    h = ge_hist_new();
    expect(h != NULL, "history");
    expect(ge_hist_push(h, &m) == 0, "push");
    ge_move(&m, 0.f, 0.f, 4.f);
    expect(ge_hist_pop(h, &m) == 0, "pop");
    expect(near(m.min.z, z0), "undo puts it back");
    expect(ge_hist_depth(h) == 0, "history empty");
    expect(gm_cube(&box, 0.f, 0.f, 0.f, 20.f, 20.f, 20.f) == 0, "cube");
    expect(ge_lay_face(&box, 0) == 0, "lay the top face");
    expect(near(box.t[0].a.z, 0.f) && near(box.t[0].b.z, 0.f) && near(box.t[0].c.z, 0.f), "that face is on the bed");
    expect(near(box.min.z, 0.f), "nothing under the bed");
    expect(near(box.max.z, 20.f), "the cube is still 20 mm tall");
    {
        gs_cam cam;
        unsigned char *rgb;
        int face = -1, x, y;
        gs_cam_home(&cam);
        rgb = (unsigned char *)malloc(160 * 120 * 3);
        expect(rgb != NULL && gs_view_bed(&box, &cam, rgb, 160, 120) == 0, "plate");
        if (rgb) {
            for (y = 0; y < 120 && face < 0; y++) {
                for (x = 0; x < 160; x++) {
                    unsigned char *p = rgb + ((size_t)y * 160 + (size_t)x) * 3;
                    if (p[0] > 80) {
                        face = ge_pick(&box, &cam, 160, 120, x, y);
                        break;
                    }
                }
            }
            free(rgb);
        }
        expect(face >= 0 && face < box.n, "a click hits a face");
    }
    {
        /* A click on the middle of the top picks the top, not the bottom under it. */
        gs_mesh cube;
        gs_cam cam;
        gs_frame f;
        gs_v mid = {110, 110, 20};
        float x, y, z;
        int face;
        memset(&cube, 0, sizeof cube);
        gs_cam_home(&cam);
        expect(gm_cube(&cube, 100, 100, 0, 20, 20, 20) == 0 &&
                   gs_view_frame_bed(&cube, &cam, 320, 240, 0.f, 0.f, GS_BED, GS_BED, &f) == 0,
               "a cube on the plate");
        gs_view_map(&f, &cam, mid, &x, &y, &z);
        face = ge_pick(&cube, &cam, 320, 240, (int)x, (int)y);
        expect(face >= 0 && near(cube.t[face].a.z, 20.f) && near(cube.t[face].b.z, 20.f) && near(cube.t[face].c.z, 20.f),
               "the top is picked");
        gs_mesh_free(&cube);
    }
    {
        /* Launching with a file opens it and leaves it as it was. */
        gs_mesh start, part;
        FILE *f;
        long before = -1, after = -2;
        char err[160];
        memset(&start, 0, sizeof start);
        memset(&part, 0, sizeof part);
        expect(gm_cube(&part, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f) == 0 && gm_stl_write(&part, "build/start.stl") == 0, "a file to open");
        if ((f = fopen("build/start.stl", "rb"))) {
            fseek(f, 0, SEEK_END);
            before = ftell(f);
            fclose(f);
        }
        expect(ge_start(&start, "build/start.stl", err, (int)sizeof err) == 0, "start with that file");
        expect(start.n == 12 && near(start.min.x, 1.f) && near(start.min.z, 0.f) && near(start.max.z, 6.f), "the window shows that file, on the plate");
        if ((f = fopen("build/start.stl", "rb"))) {
            fseek(f, 0, SEEK_END);
            after = ftell(f);
            fclose(f);
        }
        expect(before == after, "the file is not written over");
        remove("build/missing.stl");
        expect(ge_start(&start, "build/missing.stl", err, (int)sizeof err) == -1 && err[0], "a missing file says why");
        expect((f = fopen("build/missing.stl", "rb")) == NULL, "and is not made");
        if (f) fclose(f);
        expect(GS_BED == 220.f, "the plate is 220 mm, as on the bench");
        expect(ge_start(&start, NULL, err, (int)sizeof err) == 0 && start.n > 12, "no file starts with the holed cube");
        expect(near((start.min.x + start.max.x) * 0.5f, 110.f) && near((start.min.y + start.max.y) * 0.5f, 110.f),
               "in the middle of the plate");
        expect(near(start.min.z, 8.f) && near(start.max.z - start.min.z, 20.f), "8 mm off the plate");
        gs_mesh_free(&start);
        gs_mesh_free(&part);
    }
    {
        /* Join, cut and intersect keep both parts for undo. */
        static const int ops[3] = {0, 1, 2};
        int k;
        for (k = 0; k < 3; k++) {
            ge_scene sc;
            ge_hist *hh = ge_hist_new();
            char what[64];
            memset(&sc, 0, sizeof sc);
            expect(gm_cube(&sc.stock, 0.f, 0.f, 0.f, 20.f, 20.f, 20.f) == 0 &&
                       gm_cube(&sc.added, 10.f, 5.f, 5.f, 20.f, 10.f, 10.f) == 0, "two cubes");
            sc.has_added = sc.on_added = 1;
            snprintf(what, sizeof what, "op %d combines the two", ops[k]);
            expect(ge_bool(&sc, ops[k], hh) == 0 && !sc.has_added && sc.stock.n > 0 && sc.added.n == 0, what);
            expect(ge_hist_depth(hh) == 1, "one step to undo");
            snprintf(what, sizeof what, "undo after op %d brings both parts back", ops[k]);
            expect(ge_hist_pop_scene(hh, &sc) == 0 && sc.has_added && sc.on_added && sc.stock.n == 12 &&
                       sc.added.n == 12 && near(sc.stock.max.x, 20.f) && near(sc.added.min.x, 10.f) &&
                       near(sc.added.max.x, 30.f), what);
            ge_scene_free(&sc);
            ge_hist_free(hh);
        }
    }
    {
        /* Two parts that do not meet: nothing changes, nothing to undo. */
        ge_scene sc;
        ge_hist *hh = ge_hist_new();
        memset(&sc, 0, sizeof sc);
        gm_cube(&sc.stock, 0.f, 0.f, 0.f, 5.f, 5.f, 5.f);
        gm_cube(&sc.added, 40.f, 40.f, 0.f, 5.f, 5.f, 5.f);
        sc.has_added = 1;
        expect(ge_bool(&sc, 2, hh) != 0 && sc.has_added && sc.stock.n == 12 && sc.added.n == 12 &&
                   ge_hist_depth(hh) == 0, "a miss leaves both parts and no step");
        sc.has_added = 0;
        expect(ge_bool(&sc, 0, hh) == -1, "join wants a second part");
        ge_scene_free(&sc);
        ge_hist_free(hh);
    }
    {
        /* Open welds, turns the outside out, and drops the part on the
         * plate. It does not close cracks a boolean left. */
        gs_mesh part, got;
        char err[160];
        int i;
        memset(&part, 0, sizeof part);
        memset(&got, 0, sizeof got);
        expect(gm_cube(&part, 2.f, 2.f, 5.f, 10.f, 10.f, 10.f) == 0, "a cube 5 mm up");
        for (i = 0; i < part.n; i++) {
            gs_v t = part.t[i].b;
            part.t[i].b = part.t[i].c;
            part.t[i].c = t;
        }
        expect(signed_volume(&part) < 0.f, "saved inside out");
        expect(gm_stl_write(&part, "build/flip.stl") == 0, "write flip.stl");
        expect(ge_open(&got, "build/flip.stl", err, (int)sizeof err) == 0 && got.n == 12, "open flip.stl");
        expect(near(got.min.z, 0.f) && near(got.max.z, 10.f) && near(got.min.x, 2.f), "an opened part sits on the plate");
        expect(signed_volume(&got) > 999.f, "an opened part has its outside out");
        expect(ge_open(&got, "build/none.stl", err, (int)sizeof err) == -1 && got.n == 0, "a missing file opens nothing");
        expect(ge_shelf_count() == 11 && ge_shelf_name(0) && !strcmp(ge_shelf_name(0), "NASA wrench") &&
                   ge_shelf_name(10) && !strcmp(ge_shelf_name(10), "Block with hole") && !ge_shelf_name(11) &&
                   !ge_shelf_name(-1),
               "the shelf is the bench's eleven");
        expect(ge_shelf_load(&got, 11, err, (int)sizeof err) == -1 && got.n == 0, "past the shelf loads nothing");
        expect(ge_shelf_load(&got, 10, err, (int)sizeof err) == 0 && got.n > 0 && near(got.min.z, 0.f),
               "the block with a hole loads and sits on the plate");
        gs_mesh_free(&part);
        gs_mesh_free(&got);
    }
    {
        /* Parts' shapes: each closed, outside out, about the stock. */
        gs_mesh stock, part;
        int k;
        char what[80];
        memset(&stock, 0, sizeof stock);
        memset(&part, 0, sizeof part);
        expect(gm_cube(&stock, 100.f, 100.f, 0.f, 20.f, 20.f, 20.f) == 0, "a stock cube");
        expect(ge_shape_name(GE_BOX) && !strcmp(ge_shape_name(GE_BOX), "Box") &&
                   !strcmp(ge_shape_name(GE_SLOT), "Slot") && !ge_shape_name(GE_SHAPES) && !ge_shape_name(-1),
               "the shapes' names");
        for (k = 0; k < GE_SHAPES; k++) {
            snprintf(what, sizeof what, "%s: closed, outside out, at the stock", ge_shape_name(k));
            expect(ge_shape(&part, k, &stock) == 0 && gm_open_edges(&part) == 0 && signed_volume(&part) > 0.f &&
                       part.max.x > 100.f && part.min.x < 128.f && part.max.y > 100.f && part.min.y < 120.f,
                   what);
        }
        expect(ge_shape(&part, GE_CYLINDER, &stock) == 0 && part.min.z < 0.f && part.max.z > 20.f, "the cylinder goes through");
        expect(ge_shape(&part, GE_SLOT, &stock) == 0 && part.min.z < 0.f && part.max.z > 20.f &&
                   part.max.x - part.min.x > part.max.y - part.min.y,
               "the slot goes through, along X");
        expect(ge_shape(&part, GE_SPHERE, &stock) == 0 && near(part.min.z, 0.f), "the sphere sits on the plate");
        expect(ge_shape(&part, GE_SHAPES, &stock) == -1, "no such shape");
        {
            /* Each shape cuts and joins the holed starting part cleanly. */
            gs_mesh start, out;
            char e[80];
            memset(&start, 0, sizeof start);
            memset(&out, 0, sizeof out);
            expect(ge_start(&start, NULL, e, (int)sizeof e) == 0, "the starting part");
            for (k = 0; k < GE_SHAPES; k++) {
                int c, j;
                expect(ge_shape(&part, k, &start) == 0, "a shape at the start");
                c = gm_subtract(&out, &start, &part) == 0 && gm_open_edges(&out) == 0 &&
                    signed_volume(&out) <= signed_volume(&start) + 1.f;
                gs_mesh_free(&out);
                j = gm_union(&out, &start, &part) == 0 && gm_open_edges(&out) == 0 &&
                    signed_volume(&out) >= signed_volume(&start) - 1.f &&
                    signed_volume(&out) <= signed_volume(&start) + signed_volume(&part) + 1.f;
                gs_mesh_free(&out);
                snprintf(what, sizeof what, "%s cuts and joins the start, closed", ge_shape_name(k));
                expect(c && j, what);
            }
            gs_mesh_free(&start);
        }
        gs_mesh_free(&stock);
        expect(ge_shape(&part, GE_CONE, &stock) == 0 && part.n > 0, "with no stock, at the plate's origin");
        gs_mesh_free(&part);
    }
    {
        /* Size and place: the box's bottom middle and its size, set. */
        gs_mesh p;
        float pos[3], size[3], vol;
        memset(&p, 0, sizeof p);
        expect(gm_cube(&p, 10.f, 20.f, 5.f, 4.f, 8.f, 2.f) == 0, "a part to place");
        ge_where(&p, pos, size);
        expect(near(pos[0], 12.f) && near(pos[1], 24.f) && near(pos[2], 5.f), "where: bottom middle");
        expect(near(size[0], 4.f) && near(size[1], 8.f) && near(size[2], 2.f), "where: size");
        ge_place(&p, 100.f, 90.f, 0.f);
        ge_where(&p, pos, size);
        expect(near(pos[0], 100.f) && near(pos[1], 90.f) && near(pos[2], 0.f) && near(size[1], 8.f), "placed");
        vol = signed_volume(&p);
        expect(ge_fit(&p, 1, 16.f) == 0, "fit Y to 16");
        ge_where(&p, pos, size);
        expect(near(size[0], 8.f) && near(size[1], 16.f) && near(size[2], 4.f), "fit scales evenly");
        expect(near(pos[0], 100.f) && near(pos[1], 90.f) && near(pos[2], 0.f), "fit keeps the bottom middle");
        expect(near(signed_volume(&p), vol * 8.f), "fit: eight times the volume");
        expect(ge_fit(&p, 1, 0.f) == -1 && ge_fit(&p, 1, -2.f) == -1 && ge_fit(&p, 3, 5.f) == -1 &&
                   ge_fit(&p, -1, 5.f) == -1,
               "fit refuses no size and no axis");
        ge_where(&p, pos, size);
        expect(near(size[1], 16.f), "a refused fit leaves the part");
        gs_mesh_free(&p);
        expect(ge_fit(&p, 0, 5.f) == -1, "fit refuses an empty part");
        ge_where(&p, pos, size);
        expect(pos[0] == 0.f && size[2] == 0.f, "where: an empty part is nowhere");
    }
    {
        /* The hole tools on a 40 x 40 x 10 plate up off the plate at z 8. */
        gs_mesh plate, tool, before;
        ge_holes hs;
        char why[96];
        float v0;
        memset(&plate, 0, sizeof plate);
        memset(&tool, 0, sizeof tool);
        memset(&before, 0, sizeof before);
        memset(&hs, 0, sizeof hs);
        hs.d = 4.f, hs.n = 4, hs.pcd = 24.f, hs.pitch = 10.f, hs.cb = 9.f, hs.deep = 3.f;
        expect(gm_cube(&plate, 80.f, 80.f, 8.f, 40.f, 40.f, 10.f) == 0, "a plate");
        v0 = signed_volume(&plate);
        expect(ge_hole_tool(&tool, GE_BOLT_CIRCLE, &hs, &plate, why, (int)sizeof why) == 0 && tool.min.z < 8.f &&
                   tool.max.z > 18.f && near((tool.min.x + tool.max.x) * 0.5f, 100.f) && near(tool.max.x, 114.f),
               "bolt circle: through the plate, about its middle");
        expect(ge_hole_tool(&tool, GE_HOLE_ROW, &hs, &plate, why, (int)sizeof why) == 0 && near(tool.min.x, 83.f) &&
                   near(tool.max.x, 117.f),
               "hole row: four along X, 10 apart");
        expect(ge_hole_tool(&tool, GE_COUNTERBORE, &hs, &plate, why, (int)sizeof why) == 0 && tool.max.z > 18.f &&
                   tool.min.z < 8.f && near(tool.max.x - tool.min.x, 9.f),
               "counterbore: through, the bore at the top");
        ge_copy(&before, &plate);
        expect(ge_drill(&plate, GE_BOLT_CIRCLE, &hs, why, (int)sizeof why) == 0 && gm_open_edges(&plate) == 0 &&
                   signed_volume(&plate) < v0 - 4 * 10.f * 12.f,
               "bolt circle: four holes cut, closed");
        expect(ge_drill(&plate, GE_COUNTERBORE, &hs, why, (int)sizeof why) == 0 && gm_open_edges(&plate) == 0,
               "and a counterbore in the middle");
        hs.pcd = 6.f;
        expect(ge_drill(&before, GE_BOLT_CIRCLE, &hs, why, (int)sizeof why) == -1 && strstr(why, "overlap") &&
                   near(signed_volume(&before), v0),
               "holes that overlap: refused, said, the part kept");
        hs.pcd = 44.f;
        expect(ge_hole_tool(&tool, GE_BOLT_CIRCLE, &hs, &before, why, (int)sizeof why) == -1 && strstr(why, "wider"),
               "a pitch circle wider than the part");
        hs.pitch = 14.f;
        expect(ge_hole_tool(&tool, GE_HOLE_ROW, &hs, &before, why, (int)sizeof why) == -1 && strstr(why, "wider"),
               "a row wider than the part");
        hs.pitch = 3.f;
        expect(ge_hole_tool(&tool, GE_HOLE_ROW, &hs, &before, why, (int)sizeof why) == -1 && strstr(why, "overlap"),
               "a row that overlaps");
        hs.cb = 4.2f;
        expect(ge_hole_tool(&tool, GE_COUNTERBORE, &hs, &before, why, (int)sizeof why) == -1 && strstr(why, "wider than the drill"),
               "a bore no wider than the drill");
        hs.cb = 9.f, hs.deep = 10.f;
        expect(ge_hole_tool(&tool, GE_COUNTERBORE, &hs, &before, why, (int)sizeof why) == -1 && strstr(why, "deep"),
               "a bore as deep as the part");
        hs.deep = 3.f, hs.cb = 44.f;
        expect(ge_hole_tool(&tool, GE_COUNTERBORE, &hs, &before, why, (int)sizeof why) == -1 && strstr(why, "wider than the part"),
               "a bore wider than the part");
        gs_mesh_free(&plate);
        expect(ge_drill(&plate, GE_COUNTERBORE, &hs, why, (int)sizeof why) == -1 && why[0], "no part to drill");
        gs_mesh_free(&tool);
        gs_mesh_free(&before);
    }
    {
        /* Measure: a point on the part under the pointer, where a point
         * shows in the view, and the lines said. */
        gs_mesh cube;
        gs_cam cam;
        gs_v top = {110, 104, 20}, side = {100, 112, 6}, at;
        float x, y, x2, y2;
        char line[100];
        int face;
        memset(&cube, 0, sizeof cube);
        gs_cam_home(&cam);
        expect(gm_cube(&cube, 100, 100, 0, 20, 20, 20) == 0 && ge_project(&cube, &cam, 320, 240, top, &x, &y) == 0 &&
                   x > 0.f && x < 320.f && y > 0.f && y < 240.f,
               "a point on the top shows in the view");
        face = ge_pick_at(&cube, &cam, 320, 240, (int)x, (int)y, &at);
        expect(face >= 0 && fabsf(at.x - 110.f) < 2.f && fabsf(at.y - 104.f) < 2.f && near(at.z, 20.f),
               "and picked there is that point to a pixel, on the top");
        expect(ge_pick(&cube, &cam, 320, 240, (int)x, (int)y) == face, "the same face as ge_pick");
        expect(ge_project(&cube, &cam, 320, 240, side, &x2, &y2) == 0 && (x2 != x || y2 != y), "another point elsewhere");
        expect(ge_pick_at(&cube, &cam, 320, 240, 2, 2, &at) == -1, "nothing under a corner of the view");
        {
            /* Off the part, as the bench: the point on the bed under the pointer. */
            gs_v bed = {40, 180, 0};
            float bx, by;
            expect(ge_project(&cube, &cam, 320, 240, bed, &bx, &by) == 0 &&
                       ge_pick_at(&cube, &cam, 320, 240, (int)bx, (int)by, &at) == -1,
                   "a point on the bed is off the part");
            expect(ge_pick_bed(&cube, &cam, 320, 240, (int)bx, (int)by, &at) == 0 && fabsf(at.x - 40.f) < 2.f &&
                       fabsf(at.y - 180.f) < 2.f && at.z == 0.f,
                   "and picked on the bed there, to a pixel");
            expect(ge_pick_bed(&cube, &cam, 320, 240, 2, 2, &at) == -1, "off the bed is nothing");
            expect(ge_pick_bed(NULL, &cam, 320, 240, (int)bx, (int)by, &at) == 0, "the bed is there with no part");
        }
        ge_measure_line(line, (int)sizeof line, top, side);
        expect(!strcmp(line, "19.0 mm between the two points (X 10.0, Y 8.0, Z 14.0)."), "the distance said");
        ge_facts_line(line, (int)sizeof line, &cube);
        expect(!strcmp(line, "8.00 cm3, 9.9 g of PLA. 20.0 x 20.0 x 20.0 mm, 12 triangles."), "the part's facts said");
        expect(strlen(line) <= 78, "in the status line's 78 characters");
        gs_mesh_free(&cube);
        ge_facts_line(line, (int)sizeof line, &cube);
        expect(!strcmp(line, "No part to measure."), "no part");
    }
    {
        /* Extrude: Add puts the solid on the bed mid-plate, On stands it on
         * the part's top and joins, Cut takes it down from the top. */
        gs_mesh part, out;
        gs_facts f;
        char line[100];
        memset(&part, 0, sizeof part);
        memset(&out, 0, sizeof out);
        gm_cube(&part, 100, 100, 8, 20, 20, 20);
        expect(ge_extrude(&part, GM_RECT, GM_ON_BED, 30, 10, 5, &out, line, (int)sizeof line) == 0 &&
                   near(out.min.x, 95.f) && near(out.max.y, 115.f) && near(out.min.z, 0.f) && near(out.max.z, 5.f) &&
                   !strcmp(line, "A block 30.0 x 10.0, 5.0 mm tall, is on the bed. Place it, then Join or Cut."),
               "extrude Add: a block on the bed, mid-plate");
        expect(ge_extrude(&part, GM_ROUND, GM_ON_TOP, 10, 0, 6, &out, line, (int)sizeof line) == 0 &&
                   near(out.max.z, 34.f) && gm_facts(&out, &f) == 0 && f.volume > 8400.f &&
                   !strcmp(line, "A round 10.0 across stands 6.0 mm on the part, joined."),
               "extrude On: a boss joined on the top");
        expect(ge_extrude(&part, GM_SLOT, GM_FROM_TOP, 16, 6, 4, &out, line, (int)sizeof line) == 0 &&
                   gm_facts(&out, &f) == 0 && f.volume < 8000.f - 300.f && f.volume > 8000.f - 400.f &&
                   !strcmp(line, "A slot 16.0 x 6.0 cut 4.0 mm down from the top."),
               "extrude Cut: a slot pocket in the top");
        expect(ge_extrude(&part, GM_RECT, GM_FROM_TOP, 6, 6, 30, &out, line, (int)sizeof line) == 0 &&
                   gm_facts(&out, &f) == 0 && fabsf(f.volume - (8000.f - 720.f)) < 2.f &&
                   !strcmp(line, "A block 6.0 x 6.0 cut right through."),
               "extrude Cut deeper than the part goes right through");
        expect(ge_extrude(&part, GM_RECT, GM_ON_BED, 0.5f, 10, 5, &out, line, (int)sizeof line) == -1 &&
                   !strcmp(line, "The footprint has to be 1 to 200 mm each way."),
               "a footprint under 1 mm is refused");
        expect(ge_extrude(&part, GM_RECT, GM_ON_BED, 10, 10, 121, &out, line, (int)sizeof line) == -1 &&
                   !strcmp(line, "The height has to be 0.4 to 120 mm."),
               "a height over 120 mm is refused");
        expect(ge_extrude(&part, GM_RECT, GM_FROM_TOP, 40, 40, 30, &out, line, (int)sizeof line) == -1 &&
                   !strcmp(line, "That cut takes the whole part."),
               "a cut wider than the part is refused");
        {
            float cx, cy, w, d;
            expect(ge_footprint(GM_RECT, 0.f, 0.f, 20.f, 10.f, &cx, &cy, &w, &d, line, (int)sizeof line) == 0 &&
                       near(cx, 10.f) && near(cy, 5.f) && near(w, 20.f) && near(d, 10.f),
                   "a dragged block is the box between the two points");
            expect(ge_footprint(GM_ROUND, 40.f, 50.f, 40.f, 55.f, &cx, &cy, &w, &d, line, (int)sizeof line) == 0 &&
                       near(cx, 40.f) && near(cy, 50.f) && near(w, 10.f),
                   "a dragged round is centred on the first point, the diameter twice the radius");
            expect(ge_footprint(GM_RECT, 0.f, 0.f, 0.4f, 10.f, &cx, &cy, &w, &d, line, (int)sizeof line) == -1 &&
                       strstr(line, "under 1 mm"),
                   "a drag under 1 mm is refused");
            expect(ge_extrude_at(&part, GM_RECT, GM_ON_BED, 40.f, 50.f, 20.f, 10.f, 5.f, &out, line, (int)sizeof line) == 0 &&
                       near((out.min.x + out.max.x) * 0.5f, 40.f) && near((out.min.y + out.max.y) * 0.5f, 50.f) &&
                       near(out.min.z, 0.f),
                   "extrude at a dragged place, not the middle of the plate");
            expect(ge_sketch_ghost(&part, GM_RECT, GM_ON_BED, 40.f, 50.f, 20.f, 10.f, 5.f, &out, line, (int)sizeof line) == 0 &&
                       part.n == 12 && near(out.min.x, 30.f) && near(out.max.x, 50.f) && near(out.min.y, 45.f) &&
                       near(out.max.y, 55.f) && near(out.min.z, 0.f) && near(out.max.z, 5.f) && gm_open_edges(&out) == 0,
                   "the ghost is the block, and the part is not changed");
            expect(ge_sketch_ghost(&part, GM_ROUND, GM_ON_TOP, 110.f, 110.f, 10.f, 0.f, 6.f, &out, line, (int)sizeof line) == 0 &&
                       near(out.min.z, 27.6f) && near(out.max.z, 34.f) && part.n == 12,
                   "the ghost of On stands on the top, and is not joined");
            expect(ge_sketch_ghost(&part, GM_RECT, GM_ON_BED, 0.5f, 10.f, 0.5f, 10.f, 5.f, &out, line, (int)sizeof line) == -1 &&
                       strstr(line, "1 to 200"),
                   "a ghost under 1 mm is refused");
        }
        gs_mesh_free(&part);
        gs_mesh_free(&out);
    }
    {
        /* The bench's Join, Cut and Intersect on a parts list: the picked
         * part is the tool, the one before it the stock. */
        gs_parts *ps = gs_parts_new();
        gs_mesh a, b;
        gs_facts f;
        char line[100];
        memset(&a, 0, sizeof a);
        memset(&b, 0, sizeof b);
        gm_cube(&a, 0, 0, 0, 20, 20, 20);
        gs_parts_add(ps, &a, "Plate");
        expect(ge_combine(ps, 1, line, (int)sizeof line) == -1 && !strcmp(line, "Need two parts. The picked one is the tool."),
               "combine needs two parts");
        gm_cube(&b, 10, 0, 0, 20, 20, 20);
        gs_parts_add(ps, &b, "Block");
        expect(ge_combine(ps, 1, line, (int)sizeof line) == 0 && gs_parts_count(ps) == 1 &&
                   !strcmp(gs_parts_name(ps, 0), "Plate") && gs_parts_colour(ps, 0) == 0 && gs_parts_picked(ps) == 0 &&
                   gm_facts(gs_parts_mesh(ps, 0), &f) == 0 && fabsf(f.volume - 4000.f) < 1.f &&
                   !strncmp(line, "Cut Block out of Plate.", 23),
               "Cut: the tool out of the stock, which keeps its name and colour");
        gm_cube(&b, 100, 100, 0, 5, 5, 5);
        gs_parts_add(ps, &b, "Far");
        expect(ge_combine(ps, 2, line, (int)sizeof line) == -2 && gs_parts_count(ps) == 2 &&
                   !strcmp(line, "Those two do not meet, so the solid was left as it was."),
               "parts that do not meet are left as they were");
        gs_parts_remove(ps, 1);
        gm_cube(&b, 5, 5, 10, 4, 4, 20);
        gs_parts_add(ps, &b, "Peg");
        gs_parts_pick(ps, 0); /* the first picked: the tool, the one after it the stock */
        expect(ge_combine(ps, 2, line, (int)sizeof line) == 0 && gs_parts_count(ps) == 1 &&
                   !strcmp(gs_parts_name(ps, 0), "Peg and Plate"),
               "Intersect: named for both; the first part picked uses the one after");
        gm_cube(&b, 0, 0, 20, 4, 4, 4);
        gs_parts_add(ps, &b, NULL);
        expect(ge_combine(ps, 0, line, (int)sizeof line) == 0 && !strncmp(line, "Joined into Peg and Plate.", 26),
               "Join");
        {
            /* Undo and redo over the list. */
            ge_steps *st = ge_steps_new();
            gs_parts *was;
            int i;
            expect(st && ge_steps_undo(st, ps) == NULL && ge_steps_redo(st, ps) == NULL, "nothing to undo or redo yet");
            ge_steps_push(st, ps);
            gm_cube(&b, 50, 50, 0, 5, 5, 5);
            gs_parts_add(ps, &b, "New");
            was = ge_steps_undo(st, ps);
            expect(was && gs_parts_count(was) == 1 && ge_steps_back(st) == 0 && ge_steps_ahead(st) == 1,
                   "undo hands back the list before");
            ps = ge_steps_redo(st, was);
            expect(ps && gs_parts_count(ps) == 2 && !strcmp(gs_parts_name(ps, 1), "New") && ge_steps_ahead(st) == 0 &&
                       ge_steps_back(st) == 1,
                   "redo hands back the one undone");
            was = ge_steps_undo(st, ps);
            ge_steps_push(st, was);
            expect(ge_steps_ahead(st) == 0, "a new step empties redo");
            ps = was;
            ge_steps_push(st, ps);
            gs_parts_rename(ps, 0, "Changed");
            ps = ge_steps_forget(st, ps);
            expect(ps && !strcmp(gs_parts_name(ps, 0), "Peg and Plate") && ge_steps_ahead(st) == 0,
                   "forget takes the step back and keeps nothing to redo");
            for (i = 0; i < 40; i++) ge_steps_push(st, ps);
            expect(ge_steps_back(st) == 32, "32 steps back at most");
            ge_steps_free(st);
        }
        gs_parts_free(ps);
    }
    ge_hist_free(h);
    gs_mesh_free(&m);
    gs_mesh_free(&box);
    if (fails) {
        fprintf(stderr, "%d failed\n", fails);
        return 1;
    }
    {
        /* Pull, as the bench's Pull by a distance: the picked face and every
         * face in its plane along its normal, the points the walls share with
         * it too, so the part stays closed. */
        gs_mesh c;
        char line[120];
        float pos[3], size[3], v0;
        int top = -1, k;
        memset(&c, 0, sizeof c);
        expect(gm_cube(&c, 100.f, 100.f, 0.f, 20.f, 20.f, 20.f) == 0, "a cube to pull");
        for (k = 0; k < c.n && top < 0; k++)
            if (near(c.t[k].a.z, 20.f) && near(c.t[k].b.z, 20.f) && near(c.t[k].c.z, 20.f)) top = k;
        v0 = signed_volume(&c);
        expect(ge_pull(&c, top, 5.f, line, (int)sizeof line) == 4 && strstr(line, "Pulled 5.0 mm") != NULL,
               "the top pulled 5 mm: its four corners moved");
        ge_where(&c, pos, size);
        expect(near(size[2], 25.f) && near(size[0], 20.f) && fabsf(signed_volume(&c) - v0 * 1.25f) < 1.f,
               "25 mm tall, as wide, a quarter more in it: closed");
        expect(ge_pull(&c, top, -10.f, line, (int)sizeof line) == 4, "pushed 10 mm back");
        ge_where(&c, pos, size);
        expect(near(size[2], 15.f) && fabsf(signed_volume(&c) - v0 * 0.75f) < 1.f, "15 mm tall");
        expect(ge_pull(&c, top, 0.01f, line, (int)sizeof line) == -1 && strstr(line, "at least 0.1 mm"),
               "too little a pull is refused");
        expect(ge_pull(&c, -1, 5.f, line, (int)sizeof line) == -1 && strstr(line, "Click a face"),
               "no face, no pull");
        expect(ge_pull(&c, c.n, 5.f, line, (int)sizeof line) == -1, "nor past the last face");
        ge_where(&c, pos, size);
        expect(near(size[2], 15.f), "refused pulls leave the part");
        gs_mesh_free(&c);
    }
    {
        /* Drag to pull, as the bench's Pull mode: a drag on the screen is so
         * many mm along the picked face's normal, the way the normal shows. */
        gs_mesh c;
        gs_cam cam;
        gs_v mid = {110.f, 110.f, 20.f}, up = {110.f, 110.f, 27.f};
        float sx, sy, x0, y0, x1, y1;
        int top = -1, front = -1, k;
        memset(&c, 0, sizeof c);
        expect(gm_cube(&c, 100.f, 100.f, 0.f, 20.f, 20.f, 20.f) == 0, "a cube to drag");
        for (k = 0; k < c.n; k++) {
            if (top < 0 && near(c.t[k].a.z, 20.f) && near(c.t[k].b.z, 20.f) && near(c.t[k].c.z, 20.f)) top = k;
            if (front < 0 && near(c.t[k].a.y, 100.f) && near(c.t[k].b.y, 100.f) && near(c.t[k].c.y, 100.f)) front = k;
        }
        gs_cam_home(&cam);
        expect(ge_face_axis(&c, &c, top, &cam, 320, 240, &sx, &sy) == 0 && sx * sx + sy * sy > 0.1f,
               "the top's normal shows on the screen");
        expect(ge_project(&c, &cam, 320, 240, mid, &x0, &y0) == 0 && ge_project(&c, &cam, 320, 240, up, &x1, &y1) == 0,
               "the top's middle, and 7 mm above it");
        expect(near(ge_drag_mm(sx, sy, x1 - x0, y1 - y0), 7.f), "dragged to 7 mm above: 7 mm");
        expect(near(ge_drag_mm(sx, sy, x0 - x1, y0 - y1), -7.f), "dragged the other way: pushed in 7 mm");
        expect(ge_drag_mm(sx, sy, -sy * 40.f, sx * 40.f) == 0.f, "across the normal: no pull");
        k = (int)(ge_drag_mm(sx, sy, (x1 - x0) * 0.4321f, (y1 - y0) * 0.4321f) * 1000.f + 0.5f);
        expect(k == 3000, "a tenth of a mm at a time");
        expect(ge_face_axis(&c, &c, -1, &cam, 320, 240, &sx, &sy) == -1, "no face, no axis");
        cam.pitch = 1.4f;
        expect(ge_face_axis(&c, &c, top, &cam, 320, 240, &sx, &sy) == -1, "the top seen from above: refused");
        cam.pitch = 0.f;
        cam.yaw = 0.f;
        expect(ge_face_axis(&c, &c, front, &cam, 320, 240, &sx, &sy) == -1, "a side seen end on: refused");
        expect(ge_face_axis(&c, &c, top, &cam, 320, 240, &sx, &sy) == 0, "while the top shows");
        gs_mesh_free(&c);
    }
    {
        /* A joined part's top, cut up by the join, with the walls' points on
         * its edges (not at its corners): pushed in, still closed. */
        ge_scene sc;
        char line[120];
        float pos[3], size[3];
        int top = -1, k;
        memset(&sc, 0, sizeof sc);
        expect(gm_cube(&sc.stock, 0.f, 0.f, 0.f, 20.f, 20.f, 20.f) == 0 &&
                   gm_cube(&sc.added, 5.f, 5.f, 10.f, 10.f, 10.f, 10.f) == 0, "a cube and one inside it, flush on top");
        sc.has_added = sc.on_added = 1;
        expect(ge_bool(&sc, 0, NULL) == 0, "joined");
        for (k = 0; k < sc.stock.n && top < 0; k++)
            if (near(sc.stock.t[k].a.z, 20.f) && near(sc.stock.t[k].b.z, 20.f) && near(sc.stock.t[k].c.z, 20.f)) top = k;
        expect(ge_pull(&sc.stock, top, -5.f, line, (int)sizeof line) > 0, "its top pushed in 5 mm");
        ge_where(&sc.stock, pos, size);
        expect(near(size[2], 15.f) && fabsf(signed_volume(&sc.stock) - 6000.f) < 1.f, "15 mm tall, 6000 cubic mm: closed");
        expect(sc.stock.max.z < 15.01f, "no point left at the old top");
        ge_scene_free(&sc);
    }
    {
        /* An 80 mm box joined over the holed cube, its walls cut up by the
         * join below the top: the top pushed in past those points flattens
         * them on the new top, no wall folded over it. */
        ge_scene sc;
        char line[120];
        float pos[3], size[3];
        int top = -1, k;
        memset(&sc, 0, sizeof sc);
        expect(ge_start(&sc.stock, NULL, line, (int)sizeof line) == 0 && ge_shape(&sc.added, 0, &sc.stock) == 0 &&
                   ge_fit(&sc.added, 0, 80.f) == 0,
               "the holed cube and an 80 mm box");
        ge_where(&sc.added, pos, size);
        ge_place(&sc.added, 110.f, 110.f, pos[2]);
        sc.has_added = sc.on_added = 1;
        expect(ge_bool(&sc, 0, NULL) == 0, "joined");
        for (k = 0; k < sc.stock.n && top < 0; k++)
            if (sc.stock.t[k].a.z > sc.stock.max.z - 1e-3f && sc.stock.t[k].b.z > sc.stock.max.z - 1e-3f &&
                sc.stock.t[k].c.z > sc.stock.max.z - 1e-3f)
                top = k;
        expect(ge_pull(&sc.stock, top, -40.f, line, (int)sizeof line) > 0, "its top pushed in 40 mm");
        ge_where(&sc.stock, pos, size);
        expect(near(size[2], 40.f) && fabsf(signed_volume(&sc.stock) - 80.f * 80.f * 40.f) < 20.f,
               "40 mm tall, 80 by 80 by 40 in it: nothing folded");
        ge_scene_free(&sc);
    }
    {
        /* Chamfer: the picked face's edges cut back at 45 degrees by the
         * distance, its height kept, the walls cut where the bevel meets
         * them; closed. (The bench lowers the face and leans the walls.) */
        gs_mesh c, t;
        char line[120];
        float pos[3], size[3];
        int top = -1, k, ok = 1;
        memset(&c, 0, sizeof c);
        memset(&t, 0, sizeof t);
        expect(gm_cube(&c, 100.f, 100.f, 0.f, 20.f, 20.f, 20.f) == 0, "a cube to chamfer");
        for (k = 0; k < c.n && top < 0; k++)
            if (near(c.t[k].a.z, 20.f) && near(c.t[k].b.z, 20.f) && near(c.t[k].c.z, 20.f)) top = k;
        expect(ge_chamfer(&c, top, 2.f, line, (int)sizeof line) == 0 && strstr(line, "by 2.0 mm"), "the top chamfered 2 mm");
        ge_where(&c, pos, size);
        expect(near(size[2], 20.f) && near(c.min.z, 0.f) && near(size[0], 20.f), "as tall, its foot where it was, as wide");
        for (k = 0; k < c.n * 3; k++) {
            const gs_v *p = k % 3 == 0 ? &c.t[k / 3].a : k % 3 == 1 ? &c.t[k / 3].b : &c.t[k / 3].c;
            if (near(p->z, 20.f)) ok &= p->x > 101.99f && p->x < 118.01f && p->y > 101.99f && p->y < 118.01f;
            else ok &= p->z < 0.01f || near(p->z, 18.f);
        }
        /* four edges' prisms, 2 by 2 by 20 halved, less the corners counted twice */
        expect(ok && fabsf(signed_volume(&c) - (8000.f - 160.f + 4.f * 8.f / 3.f)) < 0.5f,
               "the top 2 mm in from every side, the walls cut at 18 mm; the volume a chamfered cube's");
        expect(ge_chamfer(&c, top, 50.f, line, (int)sizeof line) == -1 && strstr(line, "too big"),
               "the bench's most, 20 mm, is as much as the 20 mm part has: refused");
        expect(ge_chamfer(&c, top, 8.f, line, (int)sizeof line) == -1 && strstr(line, "too big"),
               "8 mm in from each side of a 16 mm face: refused");
        ge_where(&c, pos, size);
        expect(near(size[2], 20.f), "refused chamfers leave the part");
        one(&t, 0.f, 0.f, 3.f);
        expect(ge_chamfer(&t, 0, 1.f, line, (int)sizeof line) == -1 && strstr(line, "no edge to chamfer"),
               "a lone face has no edge to chamfer");
        expect(ge_chamfer(&c, -1, 1.f, line, (int)sizeof line) == -1 && strstr(line, "Click a face"), "no face, no chamfer");
        gs_mesh_free(&c);
        gs_mesh_free(&t);
    }
    {
        /* The holed cube's top: its outside edges and the hole's rim both
         * chamfered (the rim opened out), closed, as tall. */
        gs_mesh c;
        char line[120];
        float pos[3], size[3], v0;
        int top = -1, k, was;
        memset(&c, 0, sizeof c);
        expect(ge_start(&c, NULL, line, (int)sizeof line) == 0 && closed(&c), "the holed cube, closed");
        for (k = 0; k < c.n && top < 0; k++)
            if (near(c.t[k].a.z, c.max.z) && near(c.t[k].b.z, c.max.z) && near(c.t[k].c.z, c.max.z)) top = k;
        v0 = signed_volume(&c);
        was = c.n;
        expect(ge_chamfer(&c, top, 1.f, line, (int)sizeof line) == 0, "its top chamfered 1 mm");
        ge_where(&c, pos, size);
        expect(closed(&c) && near(size[2], 20.f) && c.n > was && signed_volume(&c) < v0 - 30.f &&
                   signed_volume(&c) > v0 - 200.f,
               "closed, as tall, the walls cut, about the bevels' volume taken");
        gs_mesh_free(&c);
    }
    {
        /* Arrange, as the bench's: Duplicate, Line, Ring, Mirror, Turn,
         * Stack and Solo, each on the picked part of the list. */
        gs_parts *ps = gs_parts_new();
        gs_mesh c, b;
        char line[120];
        float pos[3], size[3], v0;
        int k, n;
        memset(&c, 0, sizeof c);
        memset(&b, 0, sizeof b);
        expect(gm_cube(&c, 40.f, 40.f, 0.f, 20.f, 10.f, 5.f) == 0 && gm_cube(&b, 150.f, 150.f, 0.f, 30.f, 30.f, 8.f) == 0,
               "a block and a base");
        expect(ps && gs_parts_add(ps, &b, "Base") == 0 && gs_parts_add(ps, &c, "Block") == 1, "two parts, Block picked");
        gs_parts_set_colour(ps, 1, 4);
        v0 = signed_volume(gs_parts_mesh(ps, 1));

        expect(ge_duplicate(ps, line, (int)sizeof line) == 0 && gs_parts_count(ps) == 3 && gs_parts_picked(ps) == 2 &&
                   !strcmp(gs_parts_name(ps, 2), "Block") && gs_parts_colour(ps, 2) == 4 && strstr(line, "Block is a copy"),
               "Duplicate: a third part, the copy, picked, its name and metal");
        ge_where(gs_parts_mesh(ps, 2), pos, size);
        expect(near(pos[0], 50.f + 26.f) && near(pos[1], 45.f) && near(size[0], 20.f), "beside it, 6 mm on in X");
        gs_parts_remove(ps, 2);
        gs_parts_pick(ps, 1);

        expect(ge_line(ps, 1, 3, 4.f, line, (int)sizeof line) == 0 && gs_parts_count(ps) == 4 && gs_parts_picked(ps) == 3 &&
                   strstr(line, "3 of Block along Y, 4.0 mm apart"),
               "Line Y: three in all, the last picked");
        ge_where(gs_parts_mesh(ps, 3), pos, size);
        expect(near(pos[0], 50.f) && near(pos[1], 45.f + 28.f), "each 10 + 4 mm on in Y");
        gs_parts_pick(ps, 1);
        expect(ge_line(ps, 0, 30, 500.f, line, (int)sizeof line) == 0 && gs_parts_count(ps) == 11 &&
                   strstr(line, "8 of Block along X, 80.0 mm apart"),
               "at most eight in a line, at most 80 mm apart");
        for (k = 10; k >= 2; k--) gs_parts_remove(ps, k);
        gs_parts_pick(ps, 1);

        expect(ge_ring(ps, 4, line, (int)sizeof line) == 0 && gs_parts_count(ps) == 5 && strstr(line, "Ring of 4 around"),
               "Ring of 4 about the bed's middle");
        ge_where(gs_parts_mesh(ps, 3), pos, size); /* half way round: opposite the first */
        expect(near(pos[0], 220.f - 50.f) && near(pos[1], 220.f - 45.f) && near(size[0], 20.f) && near(size[1], 10.f),
               "the second copy opposite, turned half round");
        ge_where(gs_parts_mesh(ps, 2), pos, size); /* a quarter round: turned on its side */
        expect(near(size[0], 10.f) && near(size[1], 20.f), "the first copy a quarter round, turned with it");
        for (k = 4; k >= 2; k--) gs_parts_remove(ps, k);
        gs_parts_pick(ps, 1);
        ge_place(gs_parts_mesh(ps, 1), 110.f, 110.f, 0.f);
        expect(ge_ring(ps, 3, line, (int)sizeof line) == 0 && strstr(line, "on the centre") && gs_parts_count(ps) == 4,
               "a part on the centre: the copies on a circle round it");
        ge_where(gs_parts_mesh(ps, 2), pos, size);
        expect(fabsf(sqrtf((pos[0] - 110.f) * (pos[0] - 110.f) + (pos[1] - 110.f) * (pos[1] - 110.f)) - 34.4f) < 0.05f,
               "on a circle clear of it: 20 x 1.42 + 6 mm");
        for (k = 3; k >= 2; k--) gs_parts_remove(ps, k);
        gs_parts_pick(ps, 1);
        ge_place(gs_parts_mesh(ps, 1), 50.f, 45.f, 0.f);

        expect(ge_mirror(ps, line, (int)sizeof line) == 0 && strstr(line, "Block mirrored through X"), "Mirror");
        ge_where(gs_parts_mesh(ps, 1), pos, size);
        expect(near(pos[0], 50.f) && near(size[0], 20.f) && fabsf(signed_volume(gs_parts_mesh(ps, 1)) - v0) < 0.01f &&
                   closed(gs_parts_mesh(ps, 1)),
               "in place, as big, still outward: its volume the same sign");

        expect(ge_turn(ps, 2, 90.f, line, (int)sizeof line) == 0 && strstr(line, "turned 90 degrees around Z"), "Turn");
        ge_where(gs_parts_mesh(ps, 1), pos, size);
        expect(near(size[0], 10.f) && near(size[1], 20.f) && near(pos[0], 50.f) && near(pos[1], 45.f),
               "on its side, where it was");

        expect(ge_stack(ps, line, (int)sizeof line) == 0 && strstr(line, "Block is stacked on Base"), "Stack");
        ge_where(gs_parts_mesh(ps, 1), pos, size);
        expect(near(pos[0], 165.f) && near(pos[1], 165.f) && near(pos[2], 8.f), "on the base's top, over its middle");

        expect(ge_solo(ps, line, (int)sizeof line) == 0 && gs_parts_shown(ps, 0) == 0 && gs_parts_shown(ps, 1) == 1 &&
                   strstr(line, "Only Block is showing"),
               "Solo: the others hidden");
        expect(ge_solo(ps, line, (int)sizeof line) == 0 && gs_parts_shown(ps, 0) == 1 && strstr(line, "Every part"),
               "Solo again: every part shown");
        gs_parts_remove(ps, 0);
        expect(ge_stack(ps, line, (int)sizeof line) == -1 && strstr(line, "base"), "one part: no base to stack on");
        expect(ge_solo(ps, line, (int)sizeof line) == -1 && strstr(line, "only part"), "nor anything to solo from");
        for (n = 1; n < GS_PARTS_MAX; n++) ge_duplicate(ps, line, (int)sizeof line);
        expect(gs_parts_count(ps) == GS_PARTS_MAX && ge_duplicate(ps, line, (int)sizeof line) == -1 &&
                   strstr(line, "limit") && ge_ring(ps, 3, line, (int)sizeof line) == -1,
               "24 parts is the limit");
        gs_parts_free(ps);
    }
    {
        /* The bench's single Drill and Countersink, aimed where the face was
         * clicked (or the part's middle), and its Washer. */
        gs_mesh plate, tool;
        ge_holes hs;
        gs_parts *ps = gs_parts_new();
        char why[120];
        float v0, pos[3], size[3];
        memset(&plate, 0, sizeof plate);
        memset(&tool, 0, sizeof tool);
        memset(&hs, 0, sizeof hs);
        expect(gm_cube(&plate, 80.f, 80.f, 0.f, 40.f, 40.f, 10.f) == 0, "a 40 x 40 x 10 plate");
        v0 = signed_volume(&plate);
        hs.d = 6.f;
        expect(ge_hole_tool(&tool, GE_DRILL, &hs, &plate, why, (int)sizeof why) == 0 &&
                   near((tool.min.x + tool.max.x) * 0.5f, 100.f) && near(tool.max.x - tool.min.x, 6.f) && tool.min.z < 0.f &&
                   tool.max.z > 10.f,
               "Drill: through the part's middle");
        hs.aimed = 1, hs.ax = 88.f, hs.ay = 110.f;
        expect(ge_hole_tool(&tool, GE_DRILL, &hs, &plate, why, (int)sizeof why) == 0 &&
                   near((tool.min.x + tool.max.x) * 0.5f, 88.f) && near((tool.min.y + tool.max.y) * 0.5f, 110.f),
               "aimed: where the face was clicked");
        expect(ge_drill(&plate, GE_DRILL, &hs, why, (int)sizeof why) == 0 && gm_open_edges(&plate) == 0 &&
                   fabsf(signed_volume(&plate) - (v0 - 3.14159f * 9.f * 10.f)) < 4.f,
               "a 6 mm hole cut through, closed");
        hs.ax = 150.f;
        expect(ge_drill(&plate, GE_DRILL, &hs, why, (int)sizeof why) == -1 && strstr(why, "misses"), "a drill off the part");
        hs.aimed = 0, hs.d = 4.f, hs.mouth = 10.f;
        expect(ge_hole_tool(&tool, GE_COUNTERSINK, &hs, &plate, why, (int)sizeof why) == 0 && near(tool.max.x - tool.min.x, 10.4f) &&
                   tool.max.z > 10.f && tool.min.z < 0.f,
               "Countersink: through, 10 mm across the mouth at the top");
        v0 = signed_volume(&plate);
        expect(ge_drill(&plate, GE_COUNTERSINK, &hs, why, (int)sizeof why) == 0 && gm_open_edges(&plate) == 0 &&
                   signed_volume(&plate) < v0 - 3.14159f * 4.f * 10.f + 1.f && signed_volume(&plate) > v0 - 3.14159f * 4.f * 10.f - 130.f,
               "the hole and its 90 degree mouth cut, closed");
        hs.mouth = 4.2f;
        expect(ge_hole_tool(&tool, GE_COUNTERSINK, &hs, &plate, why, (int)sizeof why) == -1 && strstr(why, "wider than the drill"),
               "a mouth no wider than the drill");
        hs.mouth = 30.f;
        expect(ge_hole_tool(&tool, GE_COUNTERSINK, &hs, &plate, why, (int)sizeof why) == -1 && strstr(why, "deeper than the part"),
               "a mouth so wide its cone goes through");
        gs_mesh_free(&tool);

        expect(ps && gs_parts_add(ps, &plate, "Plate") == 0, "the plate in a list");
        expect(ge_washer(ps, why, (int)sizeof why) == 0 && gs_parts_count(ps) == 2 && gs_parts_picked(ps) == 1 &&
                   !strcmp(gs_parts_name(ps, 1), "Washer") && strstr(why, "32 mm washer, 14 mm hole, 8 mm thick"),
               "Washer: a new part, picked");
        ge_where(gs_parts_mesh(ps, 1), pos, size);
        expect(near(size[0], 32.f) && near(size[2], 8.f) && near(pos[2], 0.f) && pos[0] - 16.f >= 120.f + 7.9f &&
                   gm_open_edges(gs_parts_mesh(ps, 1)) == 0 &&
                   fabsf(signed_volume(gs_parts_mesh(ps, 1)) - 3.14159f * (256.f - 49.f) * 8.f) < 60.f,
               "on the bed, past the plate, a ring 32 across with a 14 mm hole");
        gs_parts_free(ps);
    }
    {
        gs_mesh cube, clip, heavy;
        gs_parts *ps;
        char why[120];
        float pos[3], size[3];
        int i, k, under = 1;
        memset(&cube, 0, sizeof cube);
        memset(&clip, 0, sizeof clip);
        expect(gm_cube(&cube, 0.f, 0.f, 0.f, 20.f, 20.f, 20.f) == 0, "a cube to section");
        expect(ge_clip_below(&clip, &cube, 10.f) == 0 && clip.n > 0, "the section view cuts");
        for (i = 0; i < clip.n; i++) {
            gs_v p[3];
            p[0] = clip.t[i].a;
            p[1] = clip.t[i].b;
            p[2] = clip.t[i].c;
            for (k = 0; k < 3; k++)
                if (p[k].z > 10.05f) under = 0;
        }
        expect(under && clip.max.z > 9.5f && clip.min.z < 0.2f, "nothing above 10 mm, the cut is at the plane");
        gs_mesh_free(&clip);
        ps = gs_parts_new();
        expect(ps && gs_parts_add(ps, &cube, "Cube") == 0, "the cube in a list");
        expect(ge_split(ps, 0.1f, 1, why, (int)sizeof why) == -1 && strstr(why, "misses"), "a section that misses");
        expect(ge_split(ps, 10.f, 1, why, (int)sizeof why) == 0 && gs_parts_count(ps) == 2 && gs_parts_picked(ps) == 1 &&
                   strstr(why, "10.0") && !strcmp(gs_parts_name(ps, 0), "Cube lower") &&
                   !strcmp(gs_parts_name(ps, 1), "Cube upper"),
               "Split: two halves, the upper one picked");
        ge_where(gs_parts_mesh(ps, 0), pos, size);
        expect(near(size[0], 20.f) && near(size[2], 10.f) && near(pos[2], 0.f) && gm_open_edges(gs_parts_mesh(ps, 0)) == 0,
               "the lower half is 20 by 10, closed, on the bed");
        ge_where(gs_parts_mesh(ps, 1), pos, size);
        expect(near(size[2], 10.f) && near(pos[2], 10.f) && gm_open_edges(gs_parts_mesh(ps, 1)) == 0 &&
                   gs_parts_colour(ps, 0) == gs_parts_colour(ps, 1),
               "the upper half sits on the cut, the same metal");
        memset(&heavy, 0, sizeof heavy);
        heavy.n = 2501;
        heavy.t = (gs_tri *)calloc(2501, sizeof(gs_tri));
        heavy.t[0].a = (gs_v){0, 0, 0};
        heavy.t[0].b = (gs_v){1, 0, 0};
        heavy.t[0].c = (gs_v){0, 1, 20};
        expect(heavy.t && gs_parts_add(ps, &heavy, "Heavy") >= 0 &&
                   ge_split(ps, 10.f, 1, why, (int)sizeof why) == -1 && strstr(why, "2,500"),
               "over 2,500 triangles is not split");
        gs_parts_free(ps);
    }
    if (fails) { /* the checks after the first count too */
        fprintf(stderr, "%d failed\n", fails);
        return 1;
    }
    printf("edit ok\n");
    return 0;
}
