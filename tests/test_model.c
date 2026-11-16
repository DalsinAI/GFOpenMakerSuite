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
         * The BSP's splits used to leave T-junctions: 270 open edges on the
         * holed cube. */
        gs_mesh a, b, out;
        char what[96];
        int k;
        memset(&a, 0, sizeof a);
        memset(&b, 0, sizeof b);
        memset(&out, 0, sizeof out);
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
        gs_mesh_free(&a);
        gs_mesh_free(&b);
        gs_mesh_free(&out);
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
    gs_mesh_free(&hole);
    if (fails) {
        fprintf(stderr, "%d failed\n", fails);
        return 1;
    }
    printf("model ok\n");
    return 0;
}
