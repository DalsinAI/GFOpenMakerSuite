/* Host checks for the ASCII DXF reader (gs_vec.h): lines, arcs, circles,
 * polylines and bulge, blocks, colours, units, and a refused binary file.
 * Written against Autodesk's published DXF reference, never from another
 * application's source.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gs_vec.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static int fails;

static void expect(int ok, const char *what)
{
    if (!ok) {
        printf("fail: %s\n", what);
        fails++;
    }
}

static int near(float a, float b, float e)
{
    float d = a - b;
    if (d < 0) d = -d;
    return d <= e;
}

static int box_near(const gv_path *p, float x0, float y0, float x1, float y1, float e)
{
    float minx, miny, maxx, maxy;
    gv_path_box(p, &minx, &miny, &maxx, &maxy);
    return near(minx, x0, e) && near(miny, y0, e) && near(maxx, x1, e) && near(maxy, y1, e);
}

static int path_hits(const gv_path *p, float x, float y, float e)
{
    int i;
    for (i = 0; i < p->n; i++)
        if (near(p->xy[i * 2], x, e) && near(p->xy[i * 2 + 1], y, e)) return 1;
    return 0;
}

int main(void)
{
    gv_doc d;
    char err[160];

    err[0] = 0;
    gv_doc_init(&d);
    expect(gv_load_dxf_text(&d, "", -1, err, (int)sizeof err) == -1 && err[0], "empty text is not a DXF");
    expect(d.n == 0, "a failed load leaves the document empty");
    expect(gv_load_dxf(&d, "core/tests/data/no-such.dxf", err, (int)sizeof err) == -1 && err[0],
           "a missing file says why");
    err[0] = 0;
    expect(gv_load_dxf(&d, "core/tests/data/binary.dxf", err, (int)sizeof err) == -1 && strstr(err, "binary"),
           "binary DXF is refused");

    expect(gv_load_dxf(&d, "core/tests/data/square-10mm.dxf", err, (int)sizeof err) == 0 && d.n == 1 && d.p[0].closed &&
               d.p[0].n == 4,
           "a 10 mm closed square");
    expect(d.p[0].stroke == 0xff0000 && d.p[0].fill == GV_NONE, "ACI 1 is red, as a stroke");
    expect(gv_path_colour(&d.p[0]) == 0xff0000, "the layer colour is the stroke");
    expect(box_near(&d.p[0], 0, 0, 10, 10, 0.05f), "the square is 0..10 mm, Y up, no flip");
    expect(near(d.width, 10, 0.05f) && near(d.height, 10, 0.05f), "the view is 10 x 10 mm");
    expect(d.nlayer == 1 && d.layer[0].colour == 0xff0000, "one red layer");
    gv_doc_free(&d);

    expect(gv_load_dxf(&d, "core/tests/data/square-inch.dxf", err, (int)sizeof err) == 0 && d.n == 1 &&
               box_near(&d.p[0], 0, 0, 10, 10, 0.05f),
           "$INSUNITS inches become millimetres");
    gv_doc_free(&d);

    expect(gv_load(&d, "core/tests/data/square-10mm.dxf", err, (int)sizeof err) == 0 && d.n == 1 &&
               d.p[0].stroke == 0xff0000,
           "gv_load picks a DXF from the file");
    gv_doc_free(&d);
    expect(gv_load(&d, "core/tests/data/rect-10mm.svg", err, (int)sizeof err) == 0 && d.n == 1,
           "gv_load still reads an SVG");
    gv_doc_free(&d);

    expect(gv_load_dxf(&d, "core/tests/data/line-arc-circle.dxf", err, (int)sizeof err) == 0 && d.n == 3,
           "a line, a circle and an arc");
    expect(d.p[0].n == 2 && !d.p[0].closed && d.p[0].stroke == 0xff0000 && box_near(&d.p[0], 0, 0, 10, 0, 0.05f),
           "the line is 10 mm on layer CUT (red)");
    expect(d.p[1].closed && d.p[1].stroke == 0x0000ff && d.p[1].n > 8 && box_near(&d.p[1], 0, 0, 10, 10, 0.08f),
           "the circle is 10 mm across on layer MARK (blue)");
    expect(!d.p[2].closed && d.p[2].stroke == 0x0000ff && path_hits(&d.p[2], 25, 0, 0.08f) &&
               path_hits(&d.p[2], 20, 5, 0.08f),
           "the arc goes CCW from 0 to 90, ending at (20, 5)");
    expect(d.nlayer == 2, "two colours, two layers");
    gv_doc_free(&d);

    expect(gv_load_dxf(&d, "core/tests/data/bulge-semi.dxf", err, (int)sizeof err) == 0 && d.n == 1 && !d.p[0].closed &&
               d.p[0].stroke == 0x00ff00,
           "a bulged polyline");
    {
        float minx, miny, maxx, maxy;
        gv_path_box(&d.p[0], &minx, &miny, &maxx, &maxy);
        expect(path_hits(&d.p[0], 0, 0, 0.05f) && path_hits(&d.p[0], 10, 0, 0.05f) && near(minx, 0, 0.05f) &&
                   near(maxx, 10, 0.05f) && near(maxy, 0, 0.05f) && near(miny, -5, 0.05f),
               "bulge 1 is the CCW semicircle, peaking at y = -5");
    }
    gv_doc_free(&d);

    expect(gv_load_dxf(&d, "core/tests/data/poly-vertex.dxf", err, (int)sizeof err) == 0 && d.n == 1 && d.p[0].closed &&
               d.p[0].stroke == 0x00ff00 && box_near(&d.p[0], 0, 0, 10, 10, 0.05f),
           "a POLYLINE/VERTEX square, true colour 420");
    gv_doc_free(&d);

    expect(gv_load_dxf(&d, "core/tests/data/insert-line.dxf", err, (int)sizeof err) == 0 && d.n == 1 &&
               d.p[0].n == 2 && box_near(&d.p[0], 10, 20, 15, 20, 0.05f) && d.p[0].stroke == 0xff0000,
           "INSERT places the block's line at (10, 20)");
    gv_doc_free(&d);

    {
        static const char *bare = "0\nLINE\n8\n0\n62\n1\n10\n0\n20\n0\n11\n4\n21\n0\n";
        expect(gv_load_dxf_text(&d, bare, -1, err, (int)sizeof err) == 0 && d.n == 1 &&
                   box_near(&d.p[0], 0, 0, 4, 0, 0.05f),
               "a bare entity list, no SECTION, is still a DXF");
        gv_doc_free(&d);
        expect(gv_load_text(&d, bare, -1, err, (int)sizeof err) == 0 && d.n == 1, "gv_load_text sniffs a DXF");
        gv_doc_free(&d);
    }

    expect(gv_load_dxf(&d, "burn/tests/data/square-hole.dxf", err, (int)sizeof err) == 0 && d.n == 2 && d.p[0].closed &&
               d.p[1].closed && d.nlayer == 1,
           "two closed black squares, one hole");
    gv_doc_free(&d);

    if (fails) {
        printf("%d failed\n", fails);
        return 1;
    }
    printf("dxf ok\n");
    return 0;
}
