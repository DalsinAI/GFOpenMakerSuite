/* The mill's three tools, on a 10 mm square. MIT, Copyright (c) 2026 AmiCradle Team. */
#include "cnc_job.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

static int failed;

static void expect(int ok, const char *what)
{
    if (!ok) {
        printf("fail: %s\n", what);
        failed++;
    }
}

static int near(float a, float b)
{
    float d = a - b;
    if (d < 0.f) d = -d;
    return d < 0.6f;
}

static int load_square(gv_doc *d)
{
    char err[160];
    gv_doc_init(d);
    err[0] = 0;
    return gv_load_svg(d, "core/tests/data/rect-10mm.svg", err, (int)sizeof err);
}

int main(void)
{
    gv_doc d;
    gt_path p;
    cnc_opts o;
    char err[160];
    float min[3], max[3];
    expect(cnc_mode_parse("profile") == CNC_PROFILE && cnc_mode_parse("pocket") == CNC_POCKET &&
               cnc_mode_parse("drill") == CNC_DRILL && cnc_mode_parse("fil") == -1,
           "profile, pocket, drill");
    expect(load_square(&d) == 0, "the 10 mm square");
    cnc_defaults(&o);
    o.tool = 0.f;
    o.depth = 1.f;
    o.safe = 5.f;
    gt_init(&p);
    err[0] = 0;
    expect(cnc_path(&p, &d, &o, err, (int)sizeof err) == 0, "a profile on the line");
    expect(near(gt_length(&p, GT_CUT), 46.f), "40 mm round the square, and a 6 mm plunge");
    expect(gt_bounds(&p, min, max) == 0 && min[0] > -0.1f && max[0] < 10.1f && min[2] < -0.9f && max[2] > 4.f,
           "on the square, 1 mm down, a travel at 5");
    o.tool = 2.f;
    expect(cnc_path(&p, &d, &o, err, (int)sizeof err) == 0 && near(gt_length(&p, GT_CUT), 54.f),
           "a 2 mm tool stands 1 mm outside: a 12 mm square");
    o.mode = CNC_POCKET;
    o.tool = 4.f;
    o.stepover = 2.f;
    expect(cnc_path(&p, &d, &o, err, (int)sizeof err) == 0 && near(gt_length(&p, GT_CUT), 44.f),
           "a pocket: a 6 mm loop, a 2 mm loop, and a plunge on each");
    o.tool = 20.f;
    expect(cnc_path(&p, &d, &o, err, (int)sizeof err) == -1 && strstr(err, "does not fit"),
           "a tool wider than the shape is refused");
    o.mode = CNC_DRILL;
    o.tool = 3.f;
    o.depth = 2.f;
    o.stepdown = 1.f;
    expect(cnc_path(&p, &d, &o, err, (int)sizeof err) == 0 && near(gt_length(&p, GT_CUT), 13.f),
           "two plunges, 6 mm then 7 mm, at the centre");
    expect(gt_bounds(&p, min, max) == 0 && near(min[0], 5.f) && near(max[0], 5.f) && near(min[1], 5.f),
           "the drill is at the middle of the square");
    {
        om_machine m;
        gc_stats st;
        om_clear(&m);
        err[0] = 0;
        expect(om_load(&m, "machines/cnc/grbl/driver.om", err, (int)sizeof err) == 0 &&
                   om_load(&m, "machines/cnc/grbl/profiles/generic-300.om", err, (int)sizeof err) == 0,
               "the generic mill");
        cnc_defaults(&o);
        o.tool = 0.f;
        o.depth = 1.f;
        o.feed = 10.f;
        o.plunge = 3.f;
        o.rpm = 12000.f;
        expect(cnc_write("core/tests/data/rect-10mm.svg", "build/cnc-square.gcode", &m, &o, &st, err, (int)sizeof err) ==
                   0,
               "written");
        {
            FILE *f = fopen("build/cnc-square.gcode", "r");
            char text[8000];
            size_t n = f ? fread(text, 1, sizeof text - 1, f) : 0;
            if (f) fclose(f);
            text[n] = 0;
            expect(strstr(text, "G21") && strstr(text, "G90") && strstr(text, "M3 S12000") && strstr(text, "F600") &&
                       strstr(text, "F180") && strstr(text, "G1 X10.000 Y0.000 Z-1.000") && strstr(text, "M5") &&
                       !strstr(text, "M104"),
                   "Grbl mill G-code: spindle, a cut at 10 mm/s, a plunge at 3, no hotend");
        }
        o.rpm = 30000.f;
        expect(cnc_write("core/tests/data/rect-10mm.svg", "build/cnc-fast.gcode", &m, &o, &st, err, (int)sizeof err) ==
                       -1 &&
                   strstr(err, "rpm"),
               "faster than the spindle is refused, and no file");
        expect(fopen("build/cnc-fast.gcode", "r") == NULL, "the refused job is not left behind");
        o.rpm = 12000.f;
        o.start = "M117 hello";
        o.end = "M300 S10";
        expect(cnc_write("core/tests/data/rect-10mm.svg", "build/cnc-lines.gcode", &m, &o, &st, err, (int)sizeof err) ==
                   0,
               "written with the user's lines");
        {
            FILE *f = fopen("build/cnc-lines.gcode", "r");
            char text[8000];
            char *at, *hello, *spindle, *beep, *home;
            size_t n = f ? fread(text, 1, sizeof text - 1, f) : 0;
            if (f) fclose(f);
            text[n] = 0;
            at = strstr(text, "G90");
            hello = strstr(text, "M117 hello");
            spindle = strstr(text, "M3 S");
            beep = strstr(text, "M300 S10");
            home = strstr(text, "G0 X0 Y0");
            expect(at && hello && spindle && beep && home && at < hello && hello < spindle && beep < home,
                   "the user's lines sit after the machine's start and before its end");
        }
        {
            cnc_opts old;
            memset(&old, 0x41, sizeof old);
            old.size = (int)offsetof(cnc_opts, start);
            old.tool = 0.f;
            old.depth = 1.f;
            old.stepdown = 0.f;
            old.stepover = 0.f;
            old.feed = 10.f;
            old.plunge = 3.f;
            old.safe = 0.f;
            old.rpm = 12000.f;
            old.mode = CNC_PROFILE;
            old.x = 0.f;
            old.y = 0.f;
            expect(cnc_write("core/tests/data/rect-10mm.svg", "build/cnc-short.gcode", &m, &old, &st, err,
                             (int)sizeof err) == 0,
                   "a shorter cnc_opts still writes");
            {
                FILE *f = fopen("build/cnc-short.gcode", "r");
                char text[8000];
                size_t n = f ? fread(text, 1, sizeof text - 1, f) : 0;
                if (f) fclose(f);
                text[n] = 0;
                expect(strstr(text, "M3 S12000") && !strstr(text, "M117") && !strstr(text, "M300"),
                       "a short cnc_opts does not read start or end");
            }
        }
    }
    gt_free(&p);
    gv_doc_free(&d);
    if (failed) {
        printf("%d failed\n", failed);
        return 1;
    }
    printf("cnc ok\n");
    return 0;
}
