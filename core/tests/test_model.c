/* A 20 mm cube with a hole through it. The hole is a cylinder cut out.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gs_model.h"
#include "gs_view.h"

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

static float volume(const gs_mesh *m)
{
    double s = 0;
    int i;
    for (i = 0; i < m->n; i++) {
        gs_v a = m->t[i].a, b = m->t[i].b, c = m->t[i].c;
        s += (double)a.x * ((double)b.y * c.z - (double)c.y * b.z);
        s += (double)b.x * ((double)c.y * a.z - (double)a.y * c.z);
        s += (double)c.x * ((double)a.y * b.z - (double)b.y * a.z);
    }
    if (s < 0) s = -s;
    return (float)(s / 6.0);
}

/* Odd crossings of a ray along +X means the point is inside. */
static int inside(const gs_mesh *m, float x, float y, float z)
{
    int i, hits = 0;
    for (i = 0; i < m->n; i++) {
        gs_v a = m->t[i].a, b = m->t[i].b, c = m->t[i].c;
        float ay = a.y - y, by = b.y - y, cy = c.y - y;
        float az = a.z - z, bz = b.z - z, cz = c.z - z;
        float area = ay * (bz - cz) + by * (cz - az) + cy * (az - bz);
        float u, v, w, t;
        if (area > -1e-8f && area < 1e-8f) continue;
        u = (y * (bz - cz) + b.y * (cz - z) + c.y * (z - bz) - y * 0) ;
        /* barycentric of (x is free) on the yz projection */
        {
            float den = (b.y - a.y) * (c.z - a.z) - (c.y - a.y) * (b.z - a.z);
            if (den > -1e-8f && den < 1e-8f) continue;
            v = ((y - a.y) * (c.z - a.z) - (c.y - a.y) * (z - a.z)) / den;
            w = ((b.y - a.y) * (z - a.z) - (y - a.y) * (b.z - a.z)) / den;
            u = 1.f - v - w;
            if (u < -1e-5f || v < -1e-5f || w < -1e-5f) continue;
            t = u * a.x + v * b.x + w * c.x;
            if (t > x + 1e-4f) hits++;
        }
    }
    return hits & 1;
}

int main(void)
{
    gs_mesh box, peg, hole;
    gs_cam cam;
    unsigned char *rgb;
    float vol;
    int i, coloured = 0;
    memset(&box, 0, sizeof box);
    memset(&peg, 0, sizeof peg);
    memset(&hole, 0, sizeof hole);
    expect(gm_cube(&box, 0, 0, 0, 20, 20, 20) == 0, "cube");
    expect(gm_cylinder(&peg, 10, 10, -1, 6, 22, 32) == 0, "cylinder");
    expect(gm_subtract(&hole, &box, &peg) == 0, "subtract");
    expect(hole.n > 12, "the hole added triangles");
    vol = volume(&hole);
    fprintf(stderr, "triangles %d  volume %.1f\n", hole.n, vol);
    expect(vol > 5700.f && vol < 5800.f, "volume is the cube minus the peg");
    expect(!inside(&hole, 10.f, 10.f, 10.f), "the middle is the hole");
    expect(inside(&hole, 1.3f, 1.7f, 10.2f), "a corner of the cube is solid");
    {
        gs_mesh left, right, joined, shared, miss;
        float jv, sv;
        memset(&left, 0, sizeof left);
        memset(&right, 0, sizeof right);
        memset(&joined, 0, sizeof joined);
        memset(&shared, 0, sizeof shared);
        memset(&miss, 0, sizeof miss);
        expect(gm_cube(&left, 0, 0, 0, 20, 20, 20) == 0, "left cube");
        expect(gm_cube(&right, 10, 0, 0, 20, 20, 20) == 0, "right cube");
        expect(gm_union(&joined, &left, &right) == 0, "union");
        jv = volume(&joined);
        fprintf(stderr, "union %.1f\n", jv);
        expect(jv > 11900.f && jv < 12100.f, "union is both cubes, overlap once");
        expect(inside(&joined, 4.f, 7.f, 9.f), "union keeps the left cube");
        expect(inside(&joined, 26.f, 7.f, 9.f), "union keeps the right cube");
        expect(gm_intersect(&shared, &left, &right) == 0, "intersect");
        sv = volume(&shared);
        fprintf(stderr, "intersect %.1f\n", sv);
        expect(sv > 3900.f && sv < 4100.f, "intersect is the overlap");
        expect(inside(&shared, 15.f, 8.f, 12.f), "the overlap is solid");
        expect(!inside(&shared, 4.f, 7.f, 9.f), "outside the overlap is empty");
        expect(gm_cube(&miss, 40, 40, 40, 10, 10, 10) == 0, "a cube that misses");
        expect(gm_intersect(&shared, &left, &miss) == 0, "intersect of a miss");
        expect(shared.n == 0, "a miss leaves nothing");
        gs_mesh_free(&left);
        gs_mesh_free(&right);
        gs_mesh_free(&joined);
        gs_mesh_free(&shared);
        gs_mesh_free(&miss);
    }
    {
        /* A holed block far out on the plate: a second boolean on it once
         * lost its faces, for a float there has few digits left. */
        static const float at[3] = {0.f, 64.f, 100.f};
        float first = 0.f;
        int k;
        for (k = 0; k < 3; k++) {
            gs_mesh blk, peg, holed, box, out;
            char what[64];
            float o = at[k], d;
            int ok;
            memset(&blk, 0, sizeof blk), memset(&peg, 0, sizeof peg), memset(&holed, 0, sizeof holed);
            memset(&box, 0, sizeof box), memset(&out, 0, sizeof out);
            expect(gm_cube(&blk, o, o, 8.f, 20, 20, 20) == 0 && gm_cylinder(&peg, o + 10, o + 10, 7.f, 6, 22, 32) == 0 &&
                       gm_subtract(&holed, &blk, &peg) == 0 && gm_cube(&box, o + 12, o + 4, 8.f, 16, 16, 16) == 0,
                   "a holed block and a box");
            snprintf(what, sizeof what, "at %.0f: the join is closed and the same", o);
            /* The box less its 8 x 16 x 16 overlap, plus the hole's share
             * of that overlap, and the same wherever it is. */
            ok = gm_union(&out, &holed, &box) == 0 && gm_open_edges(&out) == 0;
            d = volume(&out) - volume(&holed) - 2048.f;
            if (k == 0) first = d;
            expect(ok && d > 0.f && d < 1000.f && fabsf(d - first) < 1.f, what);
            fprintf(stderr, "chained join at %.0f: %.1f\n", o, volume(&out));
            gs_mesh_free(&out);
            snprintf(what, sizeof what, "at %.0f: the cut is closed", o);
            expect(gm_subtract(&out, &holed, &box) == 0 && gm_open_edges(&out) == 0 && volume(&out) < volume(&holed),
                   what);
            gs_mesh_free(&blk), gs_mesh_free(&peg), gs_mesh_free(&holed), gs_mesh_free(&box), gs_mesh_free(&out);
        }
    }
    {
        /* A plate with a bolt circle cut, then a counterbore: building the
         * holed plate's tree on its first polygon's plane let the hole walls
         * cut its faces into slivers (1620 triangles to 9355), and the cut
         * came out open, with 17000 triangles. */
        gs_mesh plate, tool, one, two;
        float v1;
        memset(&plate, 0, sizeof plate), memset(&tool, 0, sizeof tool);
        memset(&one, 0, sizeof one), memset(&two, 0, sizeof two);
        expect(gm_cube(&plate, 80, 80, 8, 40, 40, 10) == 0 && gm_bolt_circle(&tool, 100, 100, 4, 18, 4, 24, 4) == 0 &&
                   gm_subtract(&one, &plate, &tool) == 0 && gm_open_edges(&one) == 0,
               "a plate with a bolt circle");
        v1 = volume(&one);
        expect(gm_counterbore(&tool, 100, 100, 4, 14.4f, 4, 9, 3.4f) == 0 && gm_subtract(&two, &one, &tool) == 0 &&
                   gm_open_edges(&two) == 0,
               "and a counterbore after it: closed");
        expect(volume(&two) < v1 - 150.f && volume(&two) > v1 - 300.f, "the counterbore took its volume");
        fprintf(stderr, "bolt circle then counterbore: %d then %d triangles\n", one.n, two.n);
        expect(one.n < 2500 && two.n < 8000, "without slivers by the thousand");
        gs_mesh_free(&plate), gs_mesh_free(&tool), gs_mesh_free(&one), gs_mesh_free(&two);
    }
    {
        /* A sliver from a cut once made its own plane split it again, for
         * ever: r = 5 and 5.5 on 32 sides overflowed the stack. */
        static const int sides[2] = {32, 24};
        int si, k;
        for (si = 0; si < 2; si++) {
            for (k = 2; k <= 16; k++) {
                gs_mesh rod, cut;
                float r = (float)k * 0.5f, want;
                char what[64];
                memset(&rod, 0, sizeof rod);
                memset(&cut, 0, sizeof cut);
                want = 8000.f - 10.f * (float)sides[si] * r * r * sinf(6.2831853f / (float)sides[si]);
                snprintf(what, sizeof what, "cube minus r %.1f on %d sides", r, sides[si]);
                expect(gm_cylinder(&rod, 10, 10, -1, r, 22, sides[si]) == 0 && gm_subtract(&cut, &box, &rod) == 0 &&
                           fabsf(volume(&cut) - want) < want * 0.01f && !inside(&cut, 10.f, 10.1f, 10.3f),
                       what);
                gs_mesh_free(&rod);
                gs_mesh_free(&cut);
            }
        }
    }
    {
        /* Watertight: every edge of a boolean's result is used once each way.
         * The BSP's splits used to leave T-junctions: 270 open edges. */
        gs_mesh a, b, c, d, out, part;
        char what[96];
        int k;
        memset(&a, 0, sizeof a);
        memset(&b, 0, sizeof b);
        memset(&c, 0, sizeof c);
        memset(&d, 0, sizeof d);
        memset(&out, 0, sizeof out);
        memset(&part, 0, sizeof part);
        expect(gm_open_edges(&box) == 0, "a cube is closed");
        expect(gm_cube(&a, 0, 0, 0, 20, 20, 20) == 0, "cube to break");
        a.n--;
        expect(gm_open_edges(&a) == 3, "a cube less one triangle has 3 open edges");
        fprintf(stderr, "holed cube: %d open edges\n", gm_open_edges(&hole));
        expect(gm_open_edges(&hole) == 0, "the holed cube is closed");
        expect(gm_cube(&a, 0, 0, 0, 20, 20, 20) == 0 && gm_cube(&b, 10, 5, 5, 20, 10, 10) == 0, "two cubes");
        for (k = 0; k < 3; k++) {
            int rc = k == 0 ? gm_union(&out, &a, &b) : k == 1 ? gm_subtract(&out, &a, &b) : gm_intersect(&out, &a, &b);
            snprintf(what, sizeof what, "%s of two cubes is closed", k == 0 ? "union" : k == 1 ? "cut" : "overlap");
            expect(rc == 0 && gm_open_edges(&out) == 0, what);
        }
        /* Wrench sized: a 114 mm bar, a hole at each end, a boss joined on. */
        expect(gm_cube(&a, 0, 0, 0, 114, 33, 8) == 0 && gm_cylinder(&b, 14, 16.5f, -1, 9, 10, 48) == 0 &&
                   gm_subtract(&part, &a, &b) == 0, "bar with one hole");
        expect(gm_cylinder(&c, 100, 16.5f, -1, 11, 10, 48) == 0 && gm_subtract(&out, &part, &c) == 0, "and the other");
        fprintf(stderr, "bar, two holes: %d open edges\n", gm_open_edges(&out));
        expect(gm_open_edges(&out) == 0, "the bar with two holes is closed");
        expect(gm_cylinder(&d, 57, 16.5f, 4, 8, 12, 40) == 0 && gm_union(&part, &out, &d) == 0, "a boss joined on");
        fprintf(stderr, "bar and boss: %d open edges, volume %.0f\n", gm_open_edges(&part), volume(&part));
        expect(gm_open_edges(&part) == 0, "the bar and boss are closed");
        expect(fabsf(volume(&part) - 26635.6f) < 26635.6f * 0.01f, "the bar keeps its volume");
        gs_mesh_free(&a);
        gs_mesh_free(&b);
        gs_mesh_free(&c);
        gs_mesh_free(&d);
        gs_mesh_free(&out);
        gs_mesh_free(&part);
    }
    {
        /* Binary STL is little-endian on every machine: an 80 byte header,
         * a uint32 count, then 50 bytes a triangle. */
        gs_mesh again;
        unsigned char raw[84 + 50 * 12];
        FILE *f;
        size_t got = 0;
        char err[80];
        unsigned u;
        float x;
        memset(&again, 0, sizeof again);
        expect(gm_cube(&box, 0, 0, 0, 20, 20, 20) == 0 && box.n == 12, "a cube is 12 triangles");
        expect(gm_stl_write(&box, "build/le.stl") == 0, "write le.stl");
        f = fopen("build/le.stl", "rb");
        if (f) {
            got = fread(raw, 1, sizeof raw, f);
            expect(fgetc(f) == EOF, "nothing after the last triangle");
            fclose(f);
        }
        expect(got == sizeof raw, "84 + 50 bytes a triangle");
        expect(raw[80] == 12 && raw[81] == 0 && raw[82] == 0 && raw[83] == 0, "the count is uint32 little-endian");
        u = (unsigned)raw[84 + 12] | (unsigned)raw[85 + 12] << 8 | (unsigned)raw[86 + 12] << 16 | (unsigned)raw[87 + 12] << 24;
        memcpy(&x, &u, 4);
        expect(x == box.t[0].a.x, "the first x is a little-endian float");
        expect(raw[84 + 48] == 0 && raw[84 + 49] == 0, "the attribute is two zero bytes");
        expect(memcmp(raw, "GFOpenMake le", 13) == 0 && raw[13] == 0, "the header names the part");
        expect(raw[79] == 0, "the header is padded with zeros");
        expect(gs_stl_load(&again, "build/le.stl", err, (int)sizeof err) == 0 && again.n == 12 &&
                   memcmp(again.t, box.t, 12 * sizeof *box.t) == 0,
               "the file reads back the same triangles");
        gs_mesh_free(&again);
    }
    {
        /* A triangle too small to have a direction gets a zero normal. */
        gs_mesh dot;
        unsigned char raw[84 + 50];
        float nz = 1.f;
        FILE *f;
        char err[80];
        memset(&dot, 0, sizeof dot);
        dot.t = (gs_tri *)calloc(1, sizeof *dot.t);
        dot.n = dot.cap = 1;
        dot.t[0].b.x = 1e-7f;
        dot.t[0].c.y = 1e-7f;
        expect(gm_stl_write(&dot, "build/dot.stl") == 0, "write dot.stl");
        f = fopen("build/dot.stl", "rb");
        if (f) {
            expect(fread(raw, 1, sizeof raw, f) == sizeof raw, "dot.stl is one triangle");
            fclose(f);
            memcpy(&nz, raw + 84 + 8, 4);
        }
        expect(nz == 0.f && raw[84] == 0 && raw[88] == 0, "its normal is zero");
        gs_mesh_free(&dot);
        expect(gs_stl_load(&dot, "build/dot.stl", err, (int)sizeof err) == 0 && dot.n == 1, "and it still loads");
        gs_mesh_free(&dot);
    }
    {
        /* Text STL as the exporters write it: tabs, CRLF, exponents and
         * signs; a vertex line short of three numbers is refused. */
        static const char *const good =
            "solid t\r\n facet normal 0 0 1\r\n  outer loop\r\n"
            "\tvertex 0 0 0\r\n\tvertex\t1.5e1 -0 +0\r\n   vertex 0.0 1E+1 2.5e-1 \r\n"
            "  endloop\r\n endfacet\r\nendsolid t\r\n";
        static const char *const bad = "solid t\nfacet normal 0 0 1\nouter loop\nvertex 0 0 0\nvertex 1, 2, 3\n";
        gs_mesh t;
        char err[96];
        FILE *f = fopen("build/text.stl", "wb");
        memset(&t, 0, sizeof t);
        if (f) {
            fputs(good, f);
            fclose(f);
        }
        expect(gs_stl_load(&t, "build/text.stl", err, (int)sizeof err) == 0 && t.n == 1, "a text STL loads");
        expect(t.n == 1 && t.t[0].b.x == 15.f && t.t[0].b.y == 0.f && t.t[0].c.y == 10.f && t.t[0].c.z == 0.25f,
               "its numbers, exponents and all");
        gs_mesh_free(&t);
        f = fopen("build/text.stl", "wb");
        if (f) {
            fputs(bad, f);
            fclose(f);
        }
        err[0] = 0;
        expect(gs_stl_load(&t, "build/text.stl", err, (int)sizeof err) == -1 &&
                   strcmp(err, "a vertex line is not three numbers") == 0,
               "a vertex of 1, 2, 3 is refused");
        gs_mesh_free(&t);
    }
    expect(gm_stl_write(&hole, "build/hole.stl") == 0, "stl");
    rgb = (unsigned char *)malloc(480 * 360 * 3);
    expect(rgb != NULL, "picture");
    if (rgb) {
        gs_cam_home(&cam);
        expect(gs_view_solid(&hole, &cam, rgb, 480, 360) == 0, "view");
        for (i = 0; i < 480 * 360 * 3; i += 3)
            if (rgb[i] > 40 || rgb[i + 2] > 40) coloured++;
        expect(coloured > 2000, "the solid covers the picture");
        {
            FILE *f = fopen("build/hole.ppm", "wb");
            expect(f != NULL, "ppm");
            if (f) {
                fprintf(f, "P6\n480 360\n255\n");
                fwrite(rgb, 1, 480 * 360 * 3, f);
                fclose(f);
            }
        }
        free(rgb);
    }
    gs_mesh_free(&box);
    gs_mesh_free(&peg);
    {
        /* Depth: nearer the eye is larger, as gs_view.h says, so the top of
         * a cube hides its bottom and its front hides its back. */
        gs_mesh cube;
        gs_frame f;
        gs_v top = {10, 10, 20}, bottom = {10, 10, 0}, front = {10, 0, 10}, back = {10, 20, 10};
        gs_v t0 = {0, 0, 20}, t1 = {20, 0, 20}, t2 = {20, 20, 20};
        float tx, ty, tz, bx, by, bz, fx, fy, fz, kx, ky, kz;
        int r, g, b;
        static unsigned char pic[240 * 180 * 3];
        memset(&cube, 0, sizeof cube);
        gs_cam_home(&cam);
        expect(gm_cube(&cube, 0, 0, 0, 20, 20, 20) == 0 && gs_view_frame(&cube, &cam, 240, 180, &f) == 0, "a cube, framed");
        gs_view_map(&f, &cam, top, &tx, &ty, &tz);
        gs_view_map(&f, &cam, bottom, &bx, &by, &bz);
        gs_view_map(&f, &cam, front, &fx, &fy, &fz);
        gs_view_map(&f, &cam, back, &kx, &ky, &kz);
        expect(tz > bz && fz > kz, "the top and the front are nearer");
        expect(tz <= f.zmax && kz >= f.zmin, "inside the frame's depths");
        expect(gs_view_solid(&cube, &cam, pic, 240, 180) == 0 && gs_view_colour(&cam, f.origin, t0, t1, t2, &r, &g, &b) == 0,
               "the cube, and its top's colour");
        {
            const unsigned char *q = pic + ((size_t)(int)ty * 240 + (size_t)(int)tx) * 3;
            expect(q[0] == r && q[1] == g && q[2] == b, "the middle of the top shows the top");
        }
        gs_mesh_free(&cube);
    }
    gs_mesh_free(&hole);
    if (fails) {
        fprintf(stderr, "%d failed\n", fails);
        return 1;
    }
    printf("model ok\n");
    return 0;
}
