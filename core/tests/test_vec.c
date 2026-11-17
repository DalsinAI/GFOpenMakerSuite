/* Host checks for the 2D vector SVG reader (gs_vec.h): basic shapes,
 * path commands, units, the Y-up view, layers by colour, and a file.
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

static int load(gv_doc *d, const char *svg, char *err, int errlen)
{
    gv_doc_init(d);
    return gv_load_svg_text(d, svg, -1, err, errlen);
}

int main(void)
{
    gv_doc d;
    char err[128];
    float minx, miny, maxx, maxy;
    int i;

    err[0] = 0;
    gv_doc_init(&d);
    expect(d.size == (int)sizeof d && d.n == 0, "gv_doc_init");
    expect(gv_load_svg_text(&d, "", -1, err, (int)sizeof err) == -1 && err[0], "empty text is not an SVG");
    expect(d.n == 0, "a failed load leaves the document empty");
    expect(gv_load_svg_text(&d, "<svg viewBox=\"0 0 10 10\"/>", -1, err, (int)sizeof err) == -1 &&
               strstr(err, "no paths") != NULL,
           "an empty svg says no paths");
    expect(gv_load_svg_text(&d, "<html></html>", -1, err, (int)sizeof err) == -1, "html is not an SVG");
    expect(gv_load_svg(&d, "core/tests/data/no-such.svg", err, (int)sizeof err) == -1 && err[0],
           "a missing file says why");

    expect(load(&d,
                "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"10mm\" height=\"10mm\" "
                "viewBox=\"0 0 10 10\">"
                "<rect x=\"0\" y=\"0\" width=\"10\" height=\"10\" fill=\"none\" stroke=\"#ff0000\"/>"
                "</svg>",
                err, (int)sizeof err) == 0,
           "a 10 mm red square");
    expect(d.n == 1 && d.p[0].closed && d.p[0].n == 4, "one closed path of four points");
    expect(d.p[0].stroke == 0xff0000 && d.p[0].fill == GV_NONE, "stroke red, no fill");
    expect(gv_path_colour(&d.p[0]) == 0xff0000, "the layer colour is the stroke");
    expect(near(d.width, 10, 0.01f) && near(d.height, 10, 0.01f), "the view is 10 x 10 mm");
    expect(box_near(&d.p[0], 0, 0, 10, 10, 0.05f), "the square fills the view, Y up");
    /* SVG (0,0) is the top-left, so it lands at (0, 10) after the Y flip. */
    expect(near(d.p[0].xy[0], 0, 0.05f) && near(d.p[0].xy[1], 10, 0.05f),
           "the first corner is the top-left, at y = 10");
    expect(d.nlayer == 1 && d.layer[0].colour == 0xff0000 && d.layer[0].n == 1, "one red layer");
    expect(near(d.p[0].width, 1, 0.05f), "the default stroke is 1 mm in this view");
    gv_doc_free(&d);
    expect(d.n == 0 && d.p == NULL && d.size == (int)sizeof d, "gv_doc_free");

    expect(gv_load_svg(&d, "core/tests/data/rect-10mm.svg", err, (int)sizeof err) == 0 && d.n == 1 &&
               d.p[0].stroke == 0xff0000 && box_near(&d.p[0], 0, 0, 10, 10, 0.05f),
           "the same square from a file, with a comment and an xml header");
    gv_doc_free(&d);

    expect(load(&d,
                "<svg width=\"20mm\" height=\"10mm\" viewBox=\"0 0 20 10\">"
                "<line x1=\"0\" y1=\"10\" x2=\"20\" y2=\"0\" stroke=\"#00ff00\" fill=\"none\"/>"
                "</svg>",
                err, (int)sizeof err) == 0,
           "a green line");
    expect(d.n == 1 && !d.p[0].closed && d.p[0].n == 2 && d.p[0].stroke == 0x00ff00, "an open path");
    expect(near(d.p[0].xy[0], 0, 0.05f) && near(d.p[0].xy[1], 0, 0.05f) &&
               near(d.p[0].xy[2], 20, 0.05f) && near(d.p[0].xy[3], 10, 0.05f),
           "SVG (0,10) is the origin, (20,0) is the top-right");
    gv_doc_free(&d);

    expect(load(&d,
                "<svg width=\"20mm\" height=\"20mm\" viewBox=\"0 0 20 20\">"
                "<circle cx=\"10\" cy=\"10\" r=\"10\" fill=\"#0000ff\" stroke=\"none\"/>"
                "</svg>",
                err, (int)sizeof err) == 0,
           "a blue filled circle");
    expect(d.n == 1 && d.p[0].closed && d.p[0].n >= 12 && d.p[0].fill == 0x0000ff &&
               d.p[0].stroke == GV_NONE && gv_path_colour(&d.p[0]) == 0x0000ff,
           "the fill is the layer colour when there is no stroke");
    {
        int ok = 1;
        for (i = 0; i < d.p[0].n; i++) {
            float dx = d.p[0].xy[i * 2] - 10.f, dy = d.p[0].xy[i * 2 + 1] - 10.f;
            float r = sqrtf(dx * dx + dy * dy);
            if (!near(r, 10, 0.08f)) ok = 0;
        }
        expect(ok && box_near(&d.p[0], 0, 0, 20, 20, 0.1f), "every point is 10 mm from the centre");
    }
    gv_doc_free(&d);

    expect(load(&d,
                "<svg width=\"10mm\" height=\"10mm\" viewBox=\"0 0 10 10\">"
                "<path d=\"M 1 9 L 9 9 9 1 1 1 Z\" fill=\"none\" stroke=\"red\"/>"
                "</svg>",
                err, (int)sizeof err) == 0,
           "a path of lines, closed, named red");
    expect(d.n == 1 && d.p[0].closed && d.p[0].n == 4 && d.p[0].stroke == 0xff0000, "four corners");
    expect(near(d.p[0].xy[0], 1, 0.05f) && near(d.p[0].xy[1], 1, 0.05f),
           "M 1 9 is (1, 1) after the Y flip");
    gv_doc_free(&d);

    expect(load(&d,
                "<svg width=\"10mm\" height=\"10mm\" viewBox=\"0 0 10 10\">"
                "<path d=\"M 0 5 C 0 0 10 0 10 5\" fill=\"none\" stroke=\"#000\"/>"
                "</svg>",
                err, (int)sizeof err) == 0,
           "a cubic");
    expect(d.n == 1 && !d.p[0].closed && d.p[0].n > 2, "the cubic was flattened");
    expect(near(d.p[0].xy[0], 0, 0.05f) && near(d.p[0].xy[1], 5, 0.05f), "the cubic starts at (0, 5)");
    expect(near(d.p[0].xy[(d.p[0].n - 1) * 2], 10, 0.05f) &&
               near(d.p[0].xy[(d.p[0].n - 1) * 2 + 1], 5, 0.05f),
           "and ends at (10, 5)");
    gv_doc_free(&d);

    expect(load(&d,
                "<svg width=\"20mm\" height=\"20mm\" viewBox=\"0 0 20 20\">"
                "<path d=\"M 20 10 A 10 10 0 0 1 10 20\" fill=\"none\" stroke=\"black\"/>"
                "</svg>",
                err, (int)sizeof err) == 0,
           "a quarter-circle arc");
    expect(d.n == 1 && d.p[0].n > 2 && d.p[0].stroke == 0x000000, "the arc was flattened");
    {
        int ok = 1;
        expect(near(d.p[0].xy[0], 20, 0.08f) && near(d.p[0].xy[1], 10, 0.08f),
               "the arc starts at (20, 10)");
        expect(near(d.p[0].xy[(d.p[0].n - 1) * 2], 10, 0.08f) &&
                   near(d.p[0].xy[(d.p[0].n - 1) * 2 + 1], 0, 0.08f),
               "and ends at (10, 0), the Y-flip of SVG (10, 20)");
        for (i = 0; i < d.p[0].n; i++) {
            float dx = d.p[0].xy[i * 2] - 10.f, dy = d.p[0].xy[i * 2 + 1] - 10.f;
            float r = sqrtf(dx * dx + dy * dy);
            if (!near(r, 10, 0.15f)) ok = 0;
        }
        expect(ok, "arc points lie on a 10 mm circle about the centre");
    }
    gv_doc_free(&d);

    expect(load(&d,
                "<svg width=\"30mm\" height=\"20mm\" viewBox=\"0 0 30 20\">"
                "<g transform=\"translate(10,0)\" fill=\"none\" stroke=\"#123456\">"
                "<rect x=\"0\" y=\"0\" width=\"10\" height=\"10\"/>"
                "</g></svg>",
                err, (int)sizeof err) == 0,
           "a group with a translate");
    expect(d.n == 1 && d.p[0].stroke == 0x123456 && box_near(&d.p[0], 10, 10, 20, 20, 0.05f),
           "the rect sits at x 10-20, y 10-20 (top of the 20 mm view)");
    gv_doc_free(&d);

    expect(load(&d,
                "<svg width=\"10mm\" height=\"10mm\" viewBox=\"0 0 10 10\">"
                "<rect style=\"fill:none;stroke:#abc;stroke-width:2\" x=\"0\" y=\"0\" "
                "width=\"5\" height=\"5\"/>"
                "</svg>",
                err, (int)sizeof err) == 0,
           "paint from a style attribute, short hex");
    expect(d.n == 1 && d.p[0].stroke == 0xaabbcc && near(d.p[0].width, 2, 0.05f),
           "#abc is #aabbcc, stroke 2 mm");
    gv_doc_free(&d);

    expect(load(&d,
                "<svg width=\"20mm\" height=\"10mm\" viewBox=\"0 0 20 10\">"
                "<rect x=\"0\" y=\"0\" width=\"8\" height=\"8\" fill=\"none\" stroke=\"red\"/>"
                "<line x1=\"0\" y1=\"10\" x2=\"20\" y2=\"10\" stroke=\"blue\" fill=\"none\"/>"
                "</svg>",
                err, (int)sizeof err) == 0,
           "two colours");
    expect(d.n == 2 && d.nlayer == 2 && d.layer[0].colour == 0xff0000 &&
               d.layer[1].colour == 0x0000ff && d.layer[0].n == 1 && d.layer[1].n == 1,
           "layers in the order the colours first appear");
    gv_doc_free(&d);

    expect(load(&d,
                "<svg width=\"10mm\" height=\"10mm\" viewBox=\"0 0 10 10\">"
                "<g stroke=\"green\" fill=\"none\">"
                "<rect x=\"1\" y=\"1\" width=\"2\" height=\"2\"/>"
                "</g></svg>",
                err, (int)sizeof err) == 0,
           "a stroke inherited from a group");
    expect(d.n == 1 && d.p[0].stroke == 0x008000, "green is #008000");
    gv_doc_free(&d);

    expect(load(&d,
                "<svg width=\"10mm\" height=\"10mm\" viewBox=\"0 0 10 10\">"
                "<rect x=\"0\" y=\"0\" width=\"10\" height=\"10\"/>"
                "</svg>",
                err, (int)sizeof err) == 0,
           "the default fill is black");
    expect(d.n == 1 && d.p[0].fill == 0 && d.p[0].stroke == GV_NONE && gv_path_colour(&d.p[0]) == 0,
           "a rect with no paint is a black fill");
    gv_doc_free(&d);

    expect(load(&d,
                "<svg width=\"10mm\" height=\"10mm\" viewBox=\"0 0 10 10\">"
                "<polygon points=\"0,10 10,10 5,0\" fill=\"none\" stroke=\"rgb(255, 0, 0)\"/>"
                "</svg>",
                err, (int)sizeof err) == 0,
           "a polygon and rgb()");
    expect(d.n == 1 && d.p[0].closed && d.p[0].n == 3 && d.p[0].stroke == 0xff0000, "a triangle, red");
    /* SVG (0,10)->(0,0), (10,10)->(10,0), (5,0)->(5,10) */
    expect(box_near(&d.p[0], 0, 0, 10, 10, 0.05f), "the triangle fills the view");
    gv_doc_free(&d);

    expect(load(&d,
                "<svg width=\"10mm\" height=\"10mm\" viewBox=\"0 0 10 10\">"
                "<polyline points=\"0 10 10 10\" stroke=\"#00f\" fill=\"none\"/>"
                "</svg>",
                err, (int)sizeof err) == 0,
           "a polyline, short #00f");
    expect(d.n == 1 && !d.p[0].closed && d.p[0].n == 2 && d.p[0].stroke == 0x0000ff, "an open blue line");
    gv_doc_free(&d);

    expect(load(&d,
                "<svg width=\"20mm\" height=\"10mm\" viewBox=\"0 0 20 10\">"
                "<ellipse cx=\"10\" cy=\"5\" rx=\"10\" ry=\"5\" fill=\"none\" stroke=\"navy\"/>"
                "</svg>",
                err, (int)sizeof err) == 0,
           "an ellipse, named navy");
    expect(d.n == 1 && d.p[0].closed && d.p[0].stroke == 0x000080 &&
               box_near(&d.p[0], 0, 0, 20, 10, 0.15f),
           "the ellipse fills the view");
    gv_doc_free(&d);

    expect(load(&d,
                "<svg width=\"20mm\" height=\"20mm\" viewBox=\"0 0 20 20\">"
                "<g transform=\"scale(2)\">"
                "<rect x=\"0\" y=\"0\" width=\"5\" height=\"5\" fill=\"none\" stroke=\"#f00\" "
                "stroke-width=\"1\"/>"
                "</g></svg>",
                err, (int)sizeof err) == 0,
           "a scale of two");
    expect(d.n == 1 && box_near(&d.p[0], 0, 10, 10, 20, 0.08f),
           "the 5 x 5 rect is 10 mm, at the top of the view");
    expect(near(d.p[0].width, 2, 0.08f), "the stroke width is scaled too");
    gv_doc_free(&d);

    /* A load replaces what was there. */
    expect(load(&d,
                "<svg width=\"10mm\" height=\"10mm\" viewBox=\"0 0 10 10\">"
                "<line x1=\"0\" y1=\"0\" x2=\"1\" y2=\"1\" stroke=\"#000\"/>"
                "</svg>",
                err, (int)sizeof err) == 0 &&
               d.n == 1,
           "a line to replace");
    expect(gv_load_svg_text(&d,
                            "<svg width=\"10mm\" height=\"10mm\" viewBox=\"0 0 10 10\">"
                            "<rect x=\"0\" y=\"0\" width=\"4\" height=\"4\" fill=\"none\" "
                            "stroke=\"#0f0\"/>"
                            "<rect x=\"5\" y=\"5\" width=\"4\" height=\"4\" fill=\"none\" "
                            "stroke=\"#0f0\"/>"
                            "</svg>",
                            -1, err, (int)sizeof err) == 0 &&
               d.n == 2 && d.nlayer == 1 && d.layer[0].n == 2 && d.p[0].stroke == 0x00ff00,
           "a second load replaces the first, two paths one layer");
    gv_doc_free(&d);

    gv_path_box(NULL, &minx, &miny, &maxx, &maxy);
    expect(minx == 0 && miny == 0 && maxx == 0 && maxy == 0 && gv_path_colour(NULL) == GV_NONE,
           "a missing path has no box and no colour");

    if (fails) {
        printf("%d failed\n", fails);
        return 1;
    }
    printf("vec ok\n");
    return 0;
}
