/* Host checks for STL loading and the core's pictures: a binary cube, an
 * ASCII pyramid, a binary file whose header says "solid", the solid view
 * (faces, orbit, pan) and the G-code painter. They were test_slice's, when
 * the first slicer (gs_slice) was here. MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gs.h"
#include "gs_view.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fails;

static void expect(int cond, const char *what)
{
    if (!cond) {
        fprintf(stderr, "FAIL %s\n", what);
        fails++;
    }
}

static void tri(FILE *f, float ax, float ay, float az, float bx, float by, float bz, float cx, float cy, float cz)
{
    float r[12];
    unsigned char attr[2] = {0, 0};
    memset(r, 0, sizeof r);
    r[3] = ax; r[4] = ay; r[5] = az;
    r[6] = bx; r[7] = by; r[8] = bz;
    r[9] = cx; r[10] = cy; r[11] = cz;
    fwrite(r, 4, 12, f);
    fwrite(attr, 1, 2, f);
}

static void cube_stl(const char *path)
{
    FILE *f = fopen(path, "wb");
    unsigned char hdr[84];
    unsigned n = 12;
    memset(hdr, 0, sizeof hdr);
    memcpy(hdr, "gfopenslicer", 12);
    hdr[80] = 12;
    fwrite(hdr, 1, 84, f);
    /* outward-enough; repair turns it out if not */
    tri(f, 0, 0, 0, 10, 0, 0, 0, 10, 0);
    tri(f, 10, 0, 0, 10, 10, 0, 0, 10, 0);
    tri(f, 0, 0, 10, 0, 10, 10, 10, 0, 10);
    tri(f, 10, 0, 10, 0, 10, 10, 10, 10, 10);
    tri(f, 0, 0, 0, 0, 0, 10, 10, 0, 0);
    tri(f, 10, 0, 0, 0, 0, 10, 10, 0, 10);
    tri(f, 0, 10, 0, 10, 10, 0, 0, 10, 10);
    tri(f, 10, 10, 0, 10, 10, 10, 0, 10, 10);
    tri(f, 0, 0, 0, 0, 10, 0, 0, 0, 10);
    tri(f, 0, 10, 0, 0, 10, 10, 0, 0, 10);
    tri(f, 10, 0, 0, 10, 0, 10, 10, 10, 0);
    tri(f, 10, 10, 0, 10, 0, 10, 10, 10, 10);
    (void)n;
    fclose(f);
}

static void pyramid_stl(const char *path)
{
    FILE *f = fopen(path, "w");
    fprintf(f, "solid pyramid\n");
    fprintf(f, "facet normal 0 0 -1\nouter loop\nvertex 0 0 0\nvertex 10 0 0\nvertex 0 10 0\nendloop\nendfacet\n");
    fprintf(f, "facet normal 0 0 -1\nouter loop\nvertex 10 0 0\nvertex 10 10 0\nvertex 0 10 0\nendloop\nendfacet\n");
    fprintf(f, "facet normal 0 -1 0\nouter loop\nvertex 0 0 0\nvertex 5 5 10\nvertex 10 0 0\nendloop\nendfacet\n");
    fprintf(f, "facet normal 1 0 0\nouter loop\nvertex 10 0 0\nvertex 5 5 10\nvertex 10 10 0\nendloop\nendfacet\n");
    fprintf(f, "facet normal 0 1 0\nouter loop\nvertex 10 10 0\nvertex 5 5 10\nvertex 0 10 0\nendloop\nendfacet\n");
    fprintf(f, "facet normal -1 0 0\nouter loop\nvertex 0 10 0\nvertex 5 5 10\nvertex 0 0 0\nendloop\nendfacet\n");
    fprintf(f, "endsolid\n");
    fclose(f);
}

int main(void)
{
    cube_stl("/tmp/gs-cube.stl");
    pyramid_stl("/tmp/gs-pyr.stl");

    {
        /* A first layer as G-code: a 10 mm square's wall and five lines
         * of infill, drawn on the bed by the G-code painter. */
        static const char gcode[] =
            "; LAYER 0 Z0.1\nG0 X0 Y0\nG1 X10 Y0 E1\nG1 X10 Y10 E2\nG1 X0 Y10 E3\nG1 X0 Y0 E4\n"
            "G0 X1 Y1\nG1 X9 Y9 E5\nG0 X1 Y3\nG1 X7 Y9 E6\nG0 X3 Y1\nG1 X9 Y7 E7\nG0 X1 Y5\nG1 X5 Y9 E8\n"
            "G0 X5 Y1\nG1 X9 Y5 E9\n; LAYER 1 Z0.3\n";
        unsigned char *rgb = (unsigned char *)malloc(180 * 140 * 3);
        int segs, brass = 0, i;
        segs = gs_view_paint(gcode, (int)strlen(gcode), rgb, 180, 140);
        expect(segs == 9, "the view draws the layer's nine lines, not the travels");
        for (i = 0; i < 180 * 140; i++)
            if (rgb[i * 3] == 232 && rgb[i * 3 + 1] == 184) brass++;
        expect(brass > 40, "the lines show on the bed");
        expect(rgb[0] == 20 && rgb[2] == 28, "the corner of the bed stays empty");
        free(rgb);
    }

    {
        gs_mesh m;
        gs_cam cam;
        unsigned char *a, *b;
        int shades[8], i, used = 0, diff = 0;
        char err[80];
        expect(gs_stl_load(&m, "/tmp/gs-cube.stl", err, (int)sizeof err) == 0, "cube loads for the solid");
        gs_mesh_repair(&m);
        a = (unsigned char *)malloc(160 * 120 * 3);
        b = (unsigned char *)malloc(160 * 120 * 3);
        gs_cam_home(&cam);
        expect(gs_view_solid(&m, &cam, a, 160, 120) == 0, "the solid draws");
        expect(gs_view_on_gpu() == 0, "this machine has no chip");
        memset(shades, 0, sizeof shades);
        for (i = 0; i < 160 * 120; i++) {
            if (a[i * 3] == 20) continue;
            shades[a[i * 3] >> 5]++;
        }
        for (i = 0; i < 8; i++)
            if (shades[i] > 40) used++;
        expect(used >= 2, "the cube shows more than one face");
        gs_cam_orbit(&cam, 0.8f, 0.f);
        gs_view_solid(&m, &cam, b, 160, 120);
        for (i = 0; i < 160 * 120 * 3; i++) diff += a[i] != b[i];
        expect(diff > 500, "orbiting the camera moves the solid");
        gs_cam_home(&cam);
        gs_view_solid(&m, &cam, a, 160, 120);
        gs_cam_pan(&cam, 0.35f, 0.f);
        gs_view_solid(&m, &cam, b, 160, 120);
        diff = 0;
        for (i = 0; i < 160 * 120 * 3; i++) diff += a[i] != b[i];
        expect(diff > 500, "panning moves the point we look at");
        free(a);
        free(b);
        gs_mesh_free(&m);
    }

    {
        /* Some exporters start a binary header with "solid". The size
         * 84 + 50 a triangle says it is binary all the same. */
        FILE *f = fopen("/tmp/gs-cube.stl", "r+b");
        gs_mesh m;
        char err[160];
        if (f) {
            fwrite("solid part, facet normals", 1, 25, f);
            fclose(f);
        }
        expect(gs_stl_load(&m, "/tmp/gs-cube.stl", err, (int)sizeof err) == 0 && m.n == 12,
               "a binary file whose header says solid loads as binary");
        gs_mesh_free(&m);
        expect(gs_stl_load(&m, "/tmp/gs-pyr.stl", err, (int)sizeof err) == 0 && m.n == 6, "ASCII still loads");
        gs_mesh_free(&m);
        cube_stl("/tmp/gs-cube.stl");
    }

    if (fails) {
        fprintf(stderr, "%d failed\n", fails);
        return 1;
    }
    printf("ok\n");
    return 0;
}
