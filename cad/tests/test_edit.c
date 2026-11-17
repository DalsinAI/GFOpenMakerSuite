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
    ge_hist_free(h);
    gs_mesh_free(&m);
    gs_mesh_free(&box);
    if (fails) {
        fprintf(stderr, "%d failed\n", fails);
        return 1;
    }
    printf("edit ok\n");
    return 0;
}
