/* G-code for a printer: start and end code, speeds held to the machine,
 * filament, retraction, and the estimates. MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gs_gcode.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static int fails;

static void expect(int cond, const char *msg)
{
    if (!cond) {
        fprintf(stderr, "fail: %s\n", msg);
        fails++;
    }
}

static int near(float a, float b, float tol) { return fabsf(a - b) < tol; }

static char text[16384];

/* A sink into memory, as a program would give openmakegcode.library. */
typedef struct {
    char buf[16384];
    long n, room;               /* room: what it takes before it fails */
} sink;

static long into(const char *s, long n, void *ud)
{
    sink *k = ud;
    long take = n;
    if (take > k->room) take = k->room;
    if (k->n + take > (long)sizeof k->buf - 1) take = (long)sizeof k->buf - 1 - k->n;
    memcpy(k->buf + k->n, s, (size_t)take);
    k->n += take;
    k->room -= take;
    k->buf[k->n] = 0;
    return take;
}

static int count(const char *what)
{
    int n = 0;
    const char *s = text;
    while ((s = strstr(s, what)) != NULL) n++, s++;
    return n;
}

int main(void)
{
    om_machine m;
    gc_opts o;
    gc_stats st, est;
    gt_path p;
    float sq[] = {0, 0, 10, 0, 10, 10, 0, 10}, per = 0.4f * 0.2f / (3.14159265f * 0.875f * 0.875f);
    gl_loop square = {sq, 4, 1};
    char err[128];
    FILE *f;
    size_t got;

    om_clear(&m);
    expect(om_load(&m, "machines/printer/marlin/driver.om", err, (int)sizeof err) == 0 &&
               om_load(&m, "machines/printer/marlin/profiles/generic-220.om", err, (int)sizeof err) == 0,
           "the generic Marlin printer");
    gc_defaults(&o);
    o.hotend = 205;
    o.speed[GT_TRAVEL] = 500.f; /* more than the machine's 300 */
    o.speed[GT_OUTER] = 25.f;
    o.speed[GT_INFILL] = 60.f;
    o.speed[GT_SKIN] = 40.f;
    o.retract = 1.f;
    o.retract_speed = 20.f;
    o.retract_min = 2.f;

    gt_init(&p);
    gt_layer(&p);
    gt_loop(&p, GT_OUTER, &square, 0.2f, 0.4f);           /* 40 mm */
    gt_add(&p, GT_TRAVEL, 50, 50, 0.2f, 0);               /* 70.7 mm: retracts */
    gt_add(&p, GT_INFILL, 60, 50, 0.2f, 0.4f);            /* 10 mm */
    gt_layer(&p);
    gt_add(&p, GT_TRAVEL, 60, 50, 0.4f, 0);               /* 0.2 mm up: does not */
    gt_add(&p, GT_SKIN, 70, 50, 0.4f, 0.4f);              /* 10 mm */

    f = fopen("build/test.gcode", "w");
    expect(f && gc_write(f, &p, &m, &o, &st, err, (int)sizeof err) == 0, "written");
    if (f) fclose(f);
    f = fopen("build/test.gcode", "r");
    got = f ? fread(text, 1, sizeof text - 1, f) : 0;
    if (f) fclose(f);
    text[got] = 0;

    expect(strstr(text, "M104 S205\n") && strstr(text, "M190 S60\n") && strstr(text, "G28"), "the machine's start code, filled in");
    expect(strstr(text, "M84") > strstr(text, "; LAYER 2"), "its end code, last");
    expect(got > 4 && strcmp(text + got - 4, "M84\n") == 0 && !strstr(text, "\n\n"), "no empty lines, and M84 the last");
    expect(count("; LAYER ") == 2 && count("G92 E0") >= 2, "each layer marked, E from 0 on each");
    expect(count("G1 X") == 6, "six lines laid down");
    expect(count(" F18000") >= 1 && !strstr(text, "F30000"), "travels held to the machine's 300 mm/s");
    expect(strstr(text, " F1500") != NULL && strstr(text, " F3600") != NULL, "walls at 25 mm/s, infill at 60");
    expect(count(" F1200") == 2, "one retraction and its return, at 20 mm/s");
    expect(strstr(text, "Z0.400") != NULL, "the second layer's height");
    expect(near(st.filament, 60.f * per, 1e-3f), "filament: 60 mm of line 0.4 by 0.2");
    expect(near(st.seconds, 40.f / 25 + 70.7107f / 300 + 10.f / 60 + 0.2f / 5 + 10.f / 40 + 2.f / 20, 1e-3f),
           "the time: travels at the machine's 300 mm/s, the lift at its Z's 5, and the retraction");
    gc_estimate(&p, &m, &o, &est);
    expect(est.seconds == st.seconds && est.filament == st.filament && est.moves == st.moves && st.moves > 6,
           "the estimate is what was written");
    printf("%d moves, %.1f s, %.3f mm of filament\n", st.moves, st.seconds, st.filament);

    {
        static sink k;
        gc_stats ks;
        k.room = 1L << 30;
        expect(gc_write_to(into, &k, &p, &m, &o, &ks, err, (int)sizeof err) == 0 && k.n == (long)got &&
                   strcmp(k.buf, text) == 0 && ks.moves == st.moves && ks.seconds == st.seconds,
               "gc_write_to gives a sink what gc_write gives a file");
        k.n = 0;
        k.room = 100;
        err[0] = 0;
        expect(gc_write_to(into, &k, &p, &m, &o, NULL, err, (int)sizeof err) == -1 && strstr(err, "written") &&
                   k.n == 100,
               "a sink that takes no more is said so");
    }

    {
        om_job job;
        char why[96];
        gc_job(&p, &m, &o, &job);
        expect(job.min[0] == 0 && job.max[0] == 70 && job.max[1] == 50 && job.min[2] == 0.2f && job.max[2] == 0.4f,
               "the job's box, travels and all");
        expect(job.speed[OM_X] == 300.f && job.speed[OM_E] == 20.f && job.hotend == 205.f && job.bed == 60.f &&
                   job.extruders == 1 && job.needs == OM_HEATED_BED,
               "its speeds as written, temperatures and needs");
        expect(om_check(&m, &job, why, (int)sizeof why) == OM_OK, "fits the generic Marlin");
        gt_add(&p, GT_SKIN, 230, 50, 0.4f, 0.4f);
        o.hotend = 300.f;
        gc_job(&p, &m, &o, &job);
        expect(om_check(&m, &job, why, (int)sizeof why) == (OM_TOO_BIG | OM_TOO_HOT) && strstr(why, "230"),
               "230 mm on a 220 mm bed, and too hot, said so");
        printf("om_check: %s\n", why);
    }

    m.family = OM_LASER;
    f = fopen("build/test-laser.gcode", "w");
    expect(f && gc_write(f, &p, &m, &o, NULL, err, (int)sizeof err) == -1 && strstr(err, "printer"), "a laser is refused");
    if (f) fclose(f);
    gt_free(&p);
    if (fails) {
        printf("%d failed\n", fails);
        return 1;
    }
    printf("gcode ok\n");
    return 0;
}
