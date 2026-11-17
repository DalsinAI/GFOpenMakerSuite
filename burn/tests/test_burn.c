/* OpenMakeBurn's job: an SVG to a line toolpath and Grbl laser G-code.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "burn_job.h"
#include "burn_material.h"
#include "burn_image.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fails;

static void expect(int ok, const char *what)
{
    if (!ok) {
        printf("fail: %s\n", what);
        fails++;
    }
}

static char *slurp(const char *path)
{
    FILE *f = fopen(path, "rb");
    long n;
    char *s;
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) || (n = ftell(f)) < 0 || fseek(f, 0, SEEK_SET)) {
        fclose(f);
        return NULL;
    }
    s = (char *)malloc((size_t)n + 1);
    if (!s) {
        fclose(f);
        return NULL;
    }
    if (n && fread(s, 1, (size_t)n, f) != (size_t)n) {
        free(s);
        fclose(f);
        return NULL;
    }
    fclose(f);
    s[n] = 0;
    return s;
}

static int exists(const char *path)
{
    FILE *f = fopen(path, "r");
    if (f) fclose(f);
    return f != NULL;
}

static int load_laser(om_machine *m, char *err, int errlen)
{
    om_clear(m);
    return om_load(m, "machines/laser/grbl/driver.om", err, errlen) ||
           om_load(m, "machines/laser/grbl/profiles/generic-diode-400.om", err, errlen);
}

int main(void)
{
    om_machine m, printer;
    burn_opts o;
    gv_doc d;
    gt_path p;
    gc_stats st;
    char err[160];
    char *g;

    err[0] = 0;
    expect(load_laser(&m, err, (int)sizeof err) == 0 && m.family == OM_LASER, "Generic Diode 400");
    burn_defaults(&o);
    expect(o.size == (int)sizeof o && o.power == 50.f && o.speed == 20.f && o.passes == 1 && o.s100 == 1000.f &&
               o.constant == 0 && o.mode == BURN_LINE && o.interval == 0.2f && o.cuts_last == 1,
           "burn_defaults");

    gv_doc_init(&d);
    expect(gv_load_svg(&d, "core/tests/data/rect-10mm.svg", err, (int)sizeof err) == 0 && d.n == 1, "the 10 mm square");
    gt_init(&p);
    expect(burn_path(&p, NULL, &d, &o) == 0 && p.n == 5 && p.m[0].kind == GT_TRAVEL && p.m[0].w == 0.f, "a travel, then the sides");
    expect(p.m[1].kind == GT_CUT && p.m[1].w == 50.f && p.m[4].kind == GT_CUT && p.m[4].w == 50.f, "four cuts at 50%");
    /* Closed: last point is the first again. */
    expect(p.m[4].x == p.m[0].x && p.m[4].y == p.m[0].y, "the last cut closes the square");
    o.passes = 2;
    expect(burn_path(&p, NULL, &d, &o) == 0 && p.n == 10, "two passes are two rounds");
    gv_doc_free(&d);
    gt_free(&p);

    burn_defaults(&o);
    memset(&st, 0, sizeof st);
    expect(burn_write("core/tests/data/rect-10mm.svg", "build/rect-10mm.gcode", &m, &o, &st, err, (int)sizeof err) == 0,
           "wrote the square");
    g = slurp("build/rect-10mm.gcode");
    expect(g && strstr(g, "; GFOpenMake laser") && strstr(g, "G21") && strstr(g, "G90") && strstr(g, "M4") &&
               !strstr(g, "M3\n") && strstr(g, "S500") && strstr(g, "M5") && strstr(g, "G0 X0 Y0") &&
               strstr(g, "G1 ") && st.moves == 5,
           "M4, S500 at 50%, start and end, five moves");
    /* No S on a G0 line. */
    if (g) {
        const char *s = g;
        int bad = 0;
        while ((s = strstr(s, "G0 ")) != NULL) {
            const char *e = strchr(s, '\n');
            if (!e) e = s + strlen(s);
            if (memchr(s, 'S', (size_t)(e - s))) bad = 1;
            s = *e ? e + 1 : e;
        }
        expect(!bad, "no S on a travel");
    }
    free(g);

    o.constant = 1;
    o.power = 100;
    expect(burn_write("core/tests/data/rect-10mm.svg", "build/rect-m3.gcode", &m, &o, NULL, err, (int)sizeof err) == 0,
           "M3 at 100%");
    g = slurp("build/rect-m3.gcode");
    expect(g && strstr(g, "M3") && !strstr(g, "M4") && strstr(g, "S1000"), "M3 and S1000");
    free(g);

    err[0] = 0;
    om_clear(&printer);
    expect(om_load(&printer, "machines/printer/marlin/driver.om", err, (int)sizeof err) == 0 &&
               om_load(&printer, "machines/printer/marlin/profiles/generic-220.om", err, (int)sizeof err) == 0,
           "the generic Marlin, for a refusal");
    expect(burn_write("core/tests/data/rect-10mm.svg", "build/rect-bad.gcode", &printer, &o, NULL, err,
                      (int)sizeof err) == -1 &&
               strstr(err, "laser") && !exists("build/rect-bad.gcode"),
           "a printer is refused, and no file is left");

    {
        FILE *f = fopen("build/wide.svg", "w");
        expect(f != NULL, "a wide SVG");
        if (f) {
            fputs("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"500mm\" height=\"10mm\" "
                  "viewBox=\"0 0 500 10\">"
                  "<rect x=\"0\" y=\"0\" width=\"500\" height=\"10\" fill=\"none\" stroke=\"#000\"/>"
                  "</svg>",
                  f);
            fclose(f);
        }
        err[0] = 0;
        burn_defaults(&o);
        expect(burn_write("build/wide.svg", "build/wide.gcode", &m, &o, NULL, err, (int)sizeof err) == -1 &&
                   strstr(err, "cannot burn this") && !exists("build/wide.gcode"),
               "a 500 mm job will not fit a 400 mm bed");
    }

    expect(burn_write("core/tests/data/no-such.svg", "build/none.gcode", &m, &o, NULL, err, (int)sizeof err) == -1 &&
               err[0] && !exists("build/none.gcode"),
           "a missing SVG says why");

    {
        burn_layer ly, got;
        burn_layer set[2];
        float *spd = NULL;
        expect(burn_parse_layer("#ff0000,power=80,speed=10,passes=2", &ly, err, (int)sizeof err) == 0 &&
                   ly.colour == 0xff0000u && (ly.set & BURN_SET_POWER) && ly.power == 80.f && ly.speed == 10.f &&
                   ly.passes == 2 && ly.index == 0,
               "parse a colour layer");
        expect(burn_parse_layer("f00,SPEED=15", &ly, err, (int)sizeof err) == 0 && ly.colour == 0xff0000u &&
                   ly.speed == 15.f && !(ly.set & BURN_SET_POWER),
               "a short hex colour, one field");
        expect(burn_parse_layer("2,on=0", &ly, err, (int)sizeof err) == 0 && ly.index == 2 && ly.on == 0 &&
                   (ly.set & BURN_SET_ON) && ly.colour == GV_NONE,
               "parse an index layer, off");
        expect(burn_parse_layer("#ff0000,spped=20", &ly, err, (int)sizeof err) == -1 && strstr(err, "spped=20"),
               "a typo in a layer setting is refused");
        expect(burn_parse_layer("0,power=50", &ly, err, (int)sizeof err) == -1, "layer 0 is not a 1-based index");
        expect(burn_parse_layer("red,power=50", &ly, err, (int)sizeof err) == -1, "a name is not a colour here");

        gv_doc_init(&d);
        expect(gv_load_svg(&d, "burn/tests/data/two-rects.svg", err, (int)sizeof err) == 0 && d.nlayer == 2 &&
                   d.layer[0].colour == 0xff0000u && d.layer[1].colour == 0x0000ffu,
               "two colours, red then blue");
        burn_defaults(&o);
        gt_init(&p);
        expect(burn_path(&p, NULL, &d, &o) == 0 && p.n == 10, "two squares, one pass each, ten moves");
        expect(burn_parse_layer("#ff0000,passes=2", &set[0], err, (int)sizeof err) == 0, "red twice");
        o.layer = set;
        o.nlayer = 1;
        expect(burn_path(&p, NULL, &d, &o) == 0 && p.n == 15, "red twice then blue once is 15 moves");
        expect(burn_parse_layer("#0000ff,on=0", &set[0], err, (int)sizeof err) == 0, "blue off");
        expect(burn_path(&p, NULL, &d, &o) == 0 && p.n == 5, "blue left out leaves the red square");
        expect(burn_parse_layer("1,power=100,speed=10", &set[0], err, (int)sizeof err) == 0 &&
                   burn_parse_layer("2,power=25,speed=40", &set[1], err, (int)sizeof err) == 0,
               "per-layer power and speed");
        o.nlayer = 2;
        o.cuts_last = 0;
        expect(burn_path(&p, &spd, &d, &o) == 0 && p.n == 10 && spd && p.m[1].w == 100.f && p.m[6].w == 25.f &&
                   spd[1] == 10.f && spd[6] == 40.f,
               "red at 100% 10 mm/s, blue at 25% 40 mm/s");
        free(spd);
        burn_layer_of(&o, &d, 0, &got);
        expect(got.power == 100.f && got.speed == 10.f && got.passes == 1 && got.on == 1, "layer_of red");
        burn_layer_of(&o, &d, 1, &got);
        expect(got.power == 25.f && got.speed == 40.f, "layer_of blue");

        burn_defaults(&o);
        o.layer = set;
        o.nlayer = 2;
        o.cuts_last = 0;
        memset(&st, 0, sizeof st);
        expect(burn_write("burn/tests/data/two-rects.svg", "build/two-rects.gcode", &m, &o, &st, err, (int)sizeof err) ==
                       0 &&
                   st.moves == 10,
               "wrote two layers");
        g = slurp("build/two-rects.gcode");
        expect(g && strstr(g, "M4") && strstr(g, "S1000") && strstr(g, "S250") && strstr(g, " F600") &&
                   strstr(g, " F2400") && strstr(g, "S1000") < strstr(g, "S250") &&
                   strstr(g, " F600") < strstr(g, " F2400"),
               "S1000 then S250, F600 then F2400, in list order");
        free(g);

        /* Cuts last (default): lighter blue before heavier red. */
        burn_defaults(&o);
        o.layer = set;
        o.nlayer = 2;
        {
            int run[2];
            expect(burn_heaviness(&set[0]) > burn_heaviness(&set[1]), "red is the heavier cut");
            expect(burn_seq(&o, &d, run, 2) == 0 && run[0] == 1 && run[1] == 0,
                   "cuts last: blue then red");
        }
        expect(burn_path(&p, &spd, &d, &o) == 0 && p.n == 10 && spd && p.m[1].w == 25.f && p.m[6].w == 100.f &&
                   spd[1] == 40.f && spd[6] == 10.f,
               "cuts last burns the lighter blue first");
        free(spd);
        memset(&st, 0, sizeof st);
        expect(burn_write("burn/tests/data/two-rects.svg", "build/two-rects-cuts-last.gcode", &m, &o, &st, err,
                          (int)sizeof err) == 0,
               "wrote cuts last");
        g = slurp("build/two-rects-cuts-last.gcode");
        expect(g && strstr(g, "S250") && strstr(g, "S1000") && strstr(g, "S250") < strstr(g, "S1000") &&
                   strstr(g, " F2400") < strstr(g, " F600"),
               "S250 then S1000 when cuts last is on");
        free(g);

        /* List order overrides when cuts last is off. */
        {
            int user[2] = {1, 0}, run[2];
            o.cuts_last = 0;
            o.seq = user;
            o.nseq = 2;
            expect(burn_seq(&o, &d, run, 2) == 0 && run[0] == 1 && run[1] == 0,
                   "order=2,1 with cuts last off is blue then red");
            expect(burn_path(&p, &spd, &d, &o) == 0 && p.m[1].w == 25.f && p.m[6].w == 100.f, "seq 1,0 is blue first");
            free(spd);
            o.seq = NULL;
            o.nseq = 0;
        }

        /* A fill runs before a line, even when the fill is heavier. */
        expect(burn_parse_layer("1,power=100,speed=10,mode=fill,interval=2", &set[0], err, (int)sizeof err) == 0 &&
                   burn_parse_layer("2,power=10,speed=40", &set[1], err, (int)sizeof err) == 0,
               "heavy red fill, light blue line");
        o.layer = set;
        o.nlayer = 2;
        o.cuts_last = 1;
        o.seq = NULL;
        o.nseq = 0;
        {
            int run[2];
            expect(burn_seq(&o, &d, run, 2) == 0 && run[0] == 0 && run[1] == 1,
                   "cuts last: the fill is first, even when heavier");
        }
        gv_doc_free(&d);
        gt_free(&p);

        {
            int user[1] = {9};
            o.seq = user;
            o.nseq = 1;
            o.layer = NULL;
            o.nlayer = 0;
            err[0] = 0;
            expect(burn_write("burn/tests/data/two-rects.svg", "build/order-bad.gcode", &m, &o, NULL, err,
                              (int)sizeof err) == -1 &&
                       strstr(err, "order: no such layer 10") && !exists("build/order-bad.gcode"),
                   "an order index past the document is refused");
            o.seq = NULL;
            o.nseq = 0;
        }

        expect(burn_parse_layer("9,power=50", &set[0], err, (int)sizeof err) == 0, "a far index");
        o.layer = set;
        o.nlayer = 1;
        err[0] = 0;
        expect(burn_write("burn/tests/data/two-rects.svg", "build/layer-bad.gcode", &m, &o, NULL, err,
                          (int)sizeof err) == -1 &&
                   strstr(err, "layer 9") && !exists("build/layer-bad.gcode"),
               "an index past the document is refused");
    }

    {
        gt_path job, fr;
        burn_defaults(&o);
        gv_doc_init(&d);
        expect(gv_load_svg(&d, "core/tests/data/rect-10mm.svg", err, (int)sizeof err) == 0, "the square, for a frame");
        gt_init(&job);
        gt_init(&fr);
        expect(burn_path(&job, NULL, &d, &o) == 0 && burn_frame_path(&fr, &job) == 0 && fr.n == 5, "a box of five travels");
        expect(fr.m[0].kind == GT_TRAVEL && fr.m[4].kind == GT_TRAVEL && fr.m[4].x == fr.m[0].x &&
                   fr.m[4].y == fr.m[0].y && fr.m[1].w == 0.f,
               "the frame is closed, laser off");
        memset(&st, 0, sizeof st);
        expect(burn_write_path(&fr, NULL, "build/rect-frame.gcode", &m, &o, &st, err, (int)sizeof err) == 0, "wrote the frame");
        g = slurp("build/rect-frame.gcode");
        expect(g && strstr(g, "G0 ") && !strstr(g, "G1 ") && !strstr(g, "M4") && !strstr(g, "M3\n") &&
                   strstr(g, "M5") && st.moves == 5,
               "framing G-code is travels only, laser off");
        free(g);
        gv_doc_free(&d);
        gt_free(&job);
        gt_free(&fr);
    }

    {
        burn_layer ly;
        int i, cuts = 0, trav = 0, off = 0;
        float ymin = 1e9f, ymax = -1e9f, xmin = 1e9f, xmax = -1e9f;
        expect(burn_mode_parse("fill") == BURN_FILL && burn_mode_parse("line") == BURN_LINE &&
                   burn_mode_parse("offset") == BURN_OFFSET && !strcmp(burn_mode_name(BURN_OFFSET), "offset"),
               "mode names");
        expect(burn_parse_layer("#000000,mode=fill,interval=0.2", &ly, err, (int)sizeof err) == 0 &&
                   ly.mode == BURN_FILL && ly.interval == 0.2f && (ly.set & BURN_SET_MODE) &&
                   (ly.set & BURN_SET_INTERVAL),
               "parse a fill layer");
        expect(burn_parse_layer("1,mode=offset,interval=2", &ly, err, (int)sizeof err) == 0 &&
                   ly.mode == BURN_OFFSET && ly.interval == 2.f,
               "parse an offset layer");

        gv_doc_init(&d);
        expect(gv_load_svg(&d, "burn/tests/data/square-fill.svg", err, (int)sizeof err) == 0 && d.n == 1 &&
                   d.p[0].closed,
               "a filled 10 mm square");
        burn_defaults(&o);
        o.mode = BURN_FILL;
        o.interval = 0.2f;
        gt_init(&p);
        expect(burn_path(&p, NULL, &d, &o) == 0 && p.n > 40, "a scan fill has many moves");
        for (i = 0; i < p.n; i++) {
            if (p.m[i].kind == GT_TRAVEL) {
                trav++;
                continue;
            }
            expect(p.m[i].kind == GT_CUT && p.m[i].w == 50.f, "fill cuts at the job's power");
            if (p.m[i].kind != GT_CUT) continue;
            cuts++;
            if (p.m[i].x < xmin) xmin = p.m[i].x;
            if (p.m[i].x > xmax) xmax = p.m[i].x;
            if (p.m[i].y < ymin) ymin = p.m[i].y;
            if (p.m[i].y > ymax) ymax = p.m[i].y;
            if (i && p.m[i - 1].kind == GT_CUT && p.m[i].y != p.m[i - 1].y) off++;
        }
        expect(cuts >= 40 && trav >= 20 && off == 0, "horizontal scan lines, each entered by a travel");
        expect(xmin >= -0.01f && ymin >= -0.01f && xmax <= 10.01f && ymax <= 10.01f, "the hatch stays in the square");
        /* Spacing: each cut's y, unique, about 0.2 mm apart. */
        {
            int ny = 0;
            float ys[256], dlt, min_d = 1e9f, max_d = 0.f;
            for (i = 0; i < p.n; i++) {
                int k, seen = 0;
                if (p.m[i].kind != GT_CUT) continue;
                for (k = 0; k < ny; k++)
                    if (ys[k] == p.m[i].y) seen = 1;
                if (!seen && ny < 256) ys[ny++] = p.m[i].y;
            }
            /* insertion sort */
            for (i = 1; i < ny; i++) {
                float v = ys[i];
                int j = i;
                while (j > 0 && ys[j - 1] > v) {
                    ys[j] = ys[j - 1];
                    j--;
                }
                ys[j] = v;
            }
            for (i = 1; i < ny; i++) {
                dlt = ys[i] - ys[i - 1];
                if (dlt < min_d) min_d = dlt;
                if (dlt > max_d) max_d = dlt;
            }
            expect(ny >= 40 && min_d > 0.19f && max_d < 0.21f, "lines 0.2 mm apart");
        }
        memset(&st, 0, sizeof st);
        expect(burn_write("burn/tests/data/square-fill.svg", "build/square-fill.gcode", &m, &o, &st, err,
                          (int)sizeof err) == 0 &&
                   st.moves == p.n,
               "wrote the fill");
        g = slurp("build/square-fill.gcode");
        expect(g && strstr(g, "M4") && strstr(g, "G1 ") && strstr(g, "S500") && strstr(g, "M5"),
               "fill G-code is M4 at 50%, with G1 cuts");
        if (g) {
            const char *s = g;
            int bad = 0;
            while ((s = strstr(s, "G0 ")) != NULL) {
                const char *e = strchr(s, '\n');
                if (!e) e = s + strlen(s);
                if (memchr(s, 'S', (size_t)(e - s))) bad = 1;
                s = *e ? e + 1 : e;
            }
            expect(!bad, "no S on a fill travel");
        }
        free(g);
        gt_free(&p);

        expect(gv_load_svg(&d, "burn/tests/data/square-hole.svg", err, (int)sizeof err) == 0, "a square with a hole");
        expect(burn_path(&p, NULL, &d, &o) == 0 && p.n > 20, "the hole is filled even-odd");
        for (i = 0; i < p.n; i++) {
            if (p.m[i].kind != GT_CUT) continue;
            /* The hole is 6..14 in both axes; a cut through y=10 should stop at x=6 and resume at 14. */
            if (p.m[i].y > 6.1f && p.m[i].y < 13.9f && p.m[i].x > 6.1f && p.m[i].x < 13.9f)
                off = 1000; /* reuse off as a flag */
        }
        expect(off != 1000, "no cut inside the hole");
        gt_free(&p);

        expect(gv_load_svg(&d, "burn/tests/data/square-fill.svg", err, (int)sizeof err) == 0, "the square, for offset fill");
        o.mode = BURN_OFFSET;
        o.interval = 2.f;
        expect(burn_path(&p, NULL, &d, &o) == 0 && p.n == 15, "three rings, five moves each");
        expect(p.n >= 15 && p.m && p.m[0].kind == GT_TRAVEL && p.m[1].kind == GT_CUT && p.m[1].w == 50.f, "offset rings are cuts at 50%");
        /* The first ring is the 10 mm outline; the last is the 2 mm inner square. */
        {
            float mn[3], mx[3];
            gt_path ring;
            gt_init(&ring);
            /* first five moves */
            for (i = 0; i < 5; i++) gt_add(&ring, p.m[i].kind, p.m[i].x, p.m[i].y, 0.f, p.m[i].w);
            expect(gt_bounds(&ring, mn, mx) == 0 && mn[0] <= 0.01f && mx[0] >= 9.99f && mn[1] <= 0.01f && mx[1] >= 9.99f,
                   "the first ring is the 10 mm outline");
            gt_free(&ring);
            gt_init(&ring);
            for (i = 10; i < 15; i++) gt_add(&ring, p.m[i].kind, p.m[i].x, p.m[i].y, 0.f, p.m[i].w);
            expect(gt_bounds(&ring, mn, mx) == 0 && mn[0] > 3.9f && mn[0] < 4.1f && mx[0] > 5.9f && mx[0] < 6.1f,
                   "the last ring is 2 mm across, 4 mm in");
            gt_free(&ring);
        }
        memset(&st, 0, sizeof st);
        expect(burn_write("burn/tests/data/square-fill.svg", "build/square-offset.gcode", &m, &o, &st, err,
                          (int)sizeof err) == 0 &&
                   st.moves == 15 && (g = slurp("build/square-offset.gcode")) && strstr(g, "M4") && strstr(g, "G1 "),
               "wrote the offset fill");
        free(g);
        gt_free(&p);

        /* A hole: the outer shrinks, the hole grows, they do not fill the hole. */
        expect(gv_load_svg(&d, "burn/tests/data/square-hole.svg", err, (int)sizeof err) == 0, "the holed square, offset");
        o.interval = 1.f;
        expect(burn_path(&p, NULL, &d, &o) == 0 && p.n > 10 && p.n < 50, "offset fill round a hole, not a solid square");
        off = 0;
        for (i = 0; i < p.n; i++) {
            if (p.m[i].kind != GT_CUT) continue;
            /* Strictly inside the 8 mm hole (walls at 6 and 14). Growing the hole
             * moves later rings into the material, never into the opening. */
            if (p.m[i].x > 6.5f && p.m[i].x < 13.5f && p.m[i].y > 6.5f && p.m[i].y < 13.5f) off++;
        }
        expect(off == 0, "no cut inside the hole");
        gv_doc_free(&d);
        gt_free(&p);
    }

    /* The same 10 mm square as a DXF. */
    expect(gv_load(&d, "burn/tests/data/square-10mm.dxf", err, (int)sizeof err) == 0 && d.n == 1 && d.p[0].closed,
           "the 10 mm square as a DXF");
    burn_defaults(&o);
    gt_init(&p);
    expect(burn_path(&p, NULL, &d, &o) == 0 && p.n == 5, "the DXF square is four cuts and a travel");
    memset(&st, 0, sizeof st);
    expect(burn_write("burn/tests/data/square-10mm.dxf", "build/square-dxf.gcode", &m, &o, &st, err, (int)sizeof err) ==
                   0 &&
               st.moves == 5 && (g = slurp("build/square-dxf.gcode")) && strstr(g, "M4") && strstr(g, "G1 "),
           "wrote G-code from a DXF");
    free(g);
    gv_doc_free(&d);
    gt_free(&p);

    {
        burn_material mat;
        burn_materials lib;
        burn_opts o2;
        expect(burn_material_load(&mat, "materials/laser/card.om", err, (int)sizeof err) == 0 &&
                   !strcmp(mat.name, "Card") && mat.power == 40.f && mat.speed == 40.f && mat.passes == 1 &&
                   mat.mode == BURN_LINE && (mat.set & BURN_SET_POWER) && (mat.set & BURN_SET_SPEED),
               "card starting point");
        expect(burn_material_load(&mat, "materials/laser/plywood-3mm.om", err, (int)sizeof err) == 0 &&
                   !strcmp(mat.name, "3 mm plywood") && mat.power == 80.f && mat.speed == 8.f && mat.passes == 3 &&
                   mat.mode == BURN_LINE,
               "3 mm plywood starting point");
        expect(burn_material_load(&mat, "materials/laser/draft-engrave.om", err, (int)sizeof err) == 0 &&
                   !strcmp(mat.name, "Draft engrave") && mat.mode == BURN_FILL && mat.interval == 0.2f &&
                   mat.power == 20.f,
               "draft engrave starting point");
        burn_defaults(&o2);
        burn_material_apply(&mat, &o2);
        expect(o2.power == 20.f && o2.speed == 40.f && o2.mode == BURN_FILL && o2.interval == 0.2f,
               "apply sets the job defaults that were in the profile");
        err[0] = 0;
        expect(burn_material_load(&mat, "materials/laser/no-such.om", err, (int)sizeof err) == -1 &&
                   strstr(err, "cannot open"),
               "a missing profile says why");
        {
            FILE *f = fopen("build/bad-material.om", "w");
            expect(f != NULL, "a typo profile");
            if (f) {
                fputs("name = typo\nspped = 10\n", f);
                fclose(f);
            }
            err[0] = 0;
            expect(burn_material_load(&mat, "build/bad-material.om", err, (int)sizeof err) == -1 &&
                       strstr(err, "spped"),
                   "an unknown key is refused");
        }
        {
            FILE *f = fopen("build/noname-material.om", "w");
            expect(f != NULL, "a nameless profile");
            if (f) {
                fputs("power = 50\n", f);
                fclose(f);
            }
            err[0] = 0;
            expect(burn_material_load(&mat, "build/noname-material.om", err, (int)sizeof err) == -1 &&
                       strstr(err, "wants a name"),
                   "a profile without a name is refused");
        }
        burn_materials_init(&lib);
        expect(burn_materials_scan(&lib, "materials/laser") == 0 && lib.n == 3, "the shipped library has three profiles");
        expect(!strcmp(lib.at[0].name, "3 mm plywood") && !strcmp(lib.at[1].name, "Card") &&
                   !strcmp(lib.at[2].name, "Draft engrave"),
               "the library is sorted by name");
        burn_materials_free(&lib);
    }

    {
        burn_image im;
        float *spd = NULL;
        burn_image_init(&im);
        expect(burn_is_image("burn/tests/data/bars-8x2.pgm") && !burn_is_image("core/tests/data/rect-10mm.svg"),
               "a PGM is a picture, an SVG is not");
        expect(burn_image_load(&im, "burn/tests/data/bars-8x2.pgm", err, (int)sizeof err) == 0 && im.cols == 8 &&
                   im.rows == 2 && im.grey[0] == 1.f && im.grey[4] == 0.f,
               "the 8 by 2 bars, black then white on the top row");
        burn_defaults(&o);
        o.power = 100.f;
        o.interval = 1.f;
        gt_init(&p);
        expect(burn_image_path(&p, &spd, &im, &o) == 0 && p.n == 4 && spd && p.m[0].kind == GT_TRAVEL &&
                   p.m[0].w == 0.f && p.m[0].x == 0.f && p.m[0].y == 1.f && p.m[1].kind == GT_CUT && p.m[1].w == 100.f &&
                   p.m[1].x == 4.f && p.m[1].y == 1.f,
               "top row: travel to 0,1, cut to 4,1 at 100%");
        expect(p.m[2].kind == GT_TRAVEL && p.m[2].w == 0.f && p.m[2].x == 8.f && p.m[2].y == 0.f &&
                   p.m[3].kind == GT_CUT && p.m[3].w == 100.f && p.m[3].x == 4.f && p.m[3].y == 0.f && spd[1] == 20.f,
               "bottom row snakes: travel to 8,0, cut to 4,0, laser off on the travel");
        free(spd);
        o.invert = 1;
        expect(burn_image_path(&p, NULL, &im, &o) == 0 && p.n == 4 && p.m[1].x == 8.f && p.m[1].y == 1.f &&
                   p.m[0].x == 4.f && p.m[3].x == 0.f,
               "invert burns the other half");
        o.invert = 0;
        o.width = 16.f;
        expect(burn_image_px(&o, 8) == 2.f && burn_image_path(&p, NULL, &im, &o) == 0 && p.m[1].x == 8.f,
               "width=16 mm makes each pixel 2 mm");
        memset(&st, 0, sizeof st);
        o.width = 0.f;
        expect(burn_write("burn/tests/data/bars-8x2.pgm", "build/bars.gcode", &m, &o, &st, err, (int)sizeof err) == 0 &&
                   st.moves == 4 && (g = slurp("build/bars.gcode")) && strstr(g, "M4") && strstr(g, "S1000") &&
                   strstr(g, "G0") && strstr(g, "G1 "),
               "wrote G-code from a PGM");
        expect(g && strstr(g, "G0") && strstr(g, "S0"), "travels are G0, and S0 appears for a dark travel");
        free(g);
        {
            FILE *f = fopen("build/white.pgm", "w");
            expect(f != NULL, "a white picture");
            if (f) {
                fputs("P2\n1 1\n255\n255\n", f);
                fclose(f);
            }
            err[0] = 0;
            expect(burn_write("build/white.pgm", "build/white.gcode", &m, &o, NULL, err, (int)sizeof err) == -1 &&
                       strstr(err, "no paths to burn") && !exists("build/white.gcode"),
                   "an all-white picture has nothing to burn");
        }
        burn_image_free(&im);
        gt_free(&p);
    }

    expect(!strcmp(burn_drawing_kind(NULL), "File") && !strcmp(burn_drawing_kind(""), "File") &&
               !strcmp(burn_drawing_kind("notes.txt"), "File"),
           "no path, or an unknown name, is File");
    expect(!strcmp(burn_drawing_kind("burn/tests/data/two-rects.svg"), "SVG") &&
               !strcmp(burn_drawing_kind("Work:Box.SVG"), "SVG"),
           "an SVG, whatever the case");
    expect(!strcmp(burn_drawing_kind("burn/tests/data/two-rects.dxf"), "DXF") &&
               !strcmp(burn_drawing_kind("part.DXF"), "DXF"),
           "a DXF");
    expect(!strcmp(burn_drawing_kind("burn/tests/data/bars-8x2.pgm"), "Picture") &&
               !strcmp(burn_drawing_kind("a.PBM"), "Picture") &&
               !strcmp(burn_drawing_kind("a.ppm"), "Picture"),
           "a Netpbm picture");

    if (fails) {
        printf("%d failed\\n", fails);
        return 1;
    }
    printf("burn ok\n");
    return 0;
}
