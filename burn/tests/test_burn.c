/* OpenMakeBurn's job: an SVG to a line toolpath and Grbl laser G-code.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "burn_job.h"

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
               o.constant == 0,
           "burn_defaults");

    gv_doc_init(&d);
    expect(gv_load_svg(&d, "core/tests/data/rect-10mm.svg", err, (int)sizeof err) == 0 && d.n == 1, "the 10 mm square");
    gt_init(&p);
    expect(burn_path(&p, &d, &o) == 0 && p.n == 5 && p.m[0].kind == GT_TRAVEL && p.m[0].w == 0.f, "a travel, then the sides");
    expect(p.m[1].kind == GT_CUT && p.m[1].w == 50.f && p.m[4].kind == GT_CUT && p.m[4].w == 50.f, "four cuts at 50%");
    /* Closed: last point is the first again. */
    expect(p.m[4].x == p.m[0].x && p.m[4].y == p.m[0].y, "the last cut closes the square");
    o.passes = 2;
    expect(burn_path(&p, &d, &o) == 0 && p.n == 10, "two passes are two rounds");
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

    if (fails) {
        printf("%d failed\n", fails);
        return 1;
    }
    printf("burn ok\n");
    return 0;
}
