/* G-code for a printer, and for a Grbl laser: start and end code, speeds
 * held to the machine, filament, retraction, M3/M4 and S, and the
 * estimates. MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gs_gcode.h"

#include <math.h>
#include <stddef.h>
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

static int near(float a, float b, float tol) { return fabsf(a - b) < tol; }

/* gcode.c's planner memory, built here with GC_CALLOC=no_room_calloc: it
 * fails while no_room is set, as a full Amiga would. */
static int no_room;

void *no_room_calloc(size_t n, size_t size);
void *no_room_calloc(size_t n, size_t size)
{
    return no_room ? NULL : calloc(n, size);
}

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


/* 1 if g never fires on a travel or at the end. why is set when not. */
static int laser_safe(const char *g, char *why, int nwhy)
{
    const char *s = g;
    int on = 0, had_m5 = 0;
    while (*s) {
        const char *line = s, *e, *p;
        int is_g0 = 0, is_g1 = 0, is_m3 = 0, is_m4 = 0, is_m5 = 0, sv = -1;
        while (*s && *s != '\n') s++;
        e = s;
        if (*s == '\n') s++;
        p = line;
        while (p < e && (*p == ' ' || *p == '\t')) p++;
        if (p >= e || *p == ';') continue;
        if (p[0] == 'G' && p[1] == '0' && (p[2] == ' ' || p[2] == '\n' || p[2] == 0 || p[2] == '\t')) is_g0 = 1;
        if (p[0] == 'G' && p[1] == '1' && (p[2] == ' ' || p[2] == '\n' || p[2] == 0 || p[2] == '\t')) is_g1 = 1;
        if (p[0] == 'M' && p[1] == '3' && (p[2] == ' ' || p[2] == '\n' || p[2] == 0 || p[2] == '\t' || p[2] == 'S'))
            is_m3 = 1;
        if (p[0] == 'M' && p[1] == '4' && (p[2] == ' ' || p[2] == '\n' || p[2] == 0 || p[2] == '\t' || p[2] == 'S'))
            is_m4 = 1;
        if (p[0] == 'M' && p[1] == '5' && (p[2] == ' ' || p[2] == '\n' || p[2] == 0 || p[2] == '\t')) is_m5 = 1;
        {
            const char *q = line;
            while (q < e) {
                if (*q == 'S' && (q == line || !(q[-1] >= 'A' && q[-1] <= 'Z'))) {
                    const char *n = q + 1;
                    if (*n >= '0' && *n <= '9') {
                        sv = 0;
                        while (*n >= '0' && *n <= '9') sv = sv * 10 + (*n - '0'), n++;
                    }
                }
                q++;
            }
        }
        if (sv >= 0) on = sv > 0;
        if (is_m5) {
            on = 0;
            had_m5 = 1;
        }
        if ((is_m3 || is_m4) && sv == 0) on = 0;
        if (is_g0 && sv > 0) {
            snprintf(why, (size_t)nwhy, "a travel has S%d", sv);
            return 0;
        }
        if (is_g0) on = 0;
        if (is_g1 && on) had_m5 = 0;
        (void)is_g1;
    }
    if (on) {
        snprintf(why, (size_t)nwhy, "the laser is still on at the end");
        return 0;
    }
    if (!had_m5) {
        snprintf(why, (size_t)nwhy, "no M5 at the end");
        return 0;
    }
    return 1;
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

    {
        /* No acceleration in the profile: the time is distance over speed. */
        om_machine flat = m;
        memset(flat.accel, 0, sizeof flat.accel);
        gc_estimate(&p, &flat, &o, &est);
        expect(near(est.seconds, 40.f / 25 + 70.7107f / 300 + 10.f / 60 + 0.2f / 5 + 10.f / 40 + 2.f / 20, 1e-3f),
               "the time: travels at the machine's 300 mm/s, the lift at its Z's 5, and the retraction");

        /* How each time was got, said: at the set speeds, with the
         * machine's acceleration, or at the set speeds because there was
         * no memory to plan with (the same time as with none given). */
        expect(gc_estimate_how(&p, &flat, &o, &st) == GC_AT_SPEEDS && st.seconds == est.seconds,
               "no acceleration given: at the set speeds, and said so");
        expect(gc_estimate_how(&p, NULL, &o, &st) == GC_AT_SPEEDS, "no machine: at the set speeds");
        expect(gc_estimate_how(&p, &m, &o, &st) == GC_WITH_ACCEL && st.seconds > est.seconds + 0.1f,
               "the profile's acceleration: counted, and said so");
        no_room = 1;
        expect(gc_estimate_how(&p, &m, &o, &st) == GC_FELL_BACK, "no memory to plan with: said so");
        no_room = 0;
        expect(near(st.seconds, est.seconds, 1e-4f) && st.moves > 6, "and then the time at the set speeds");
    }
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
    expect(st.seconds > est.seconds + 0.1f, "with the profile's acceleration and jerk it takes longer");
    gc_estimate(&p, &m, &o, &est);
    expect(est.seconds == st.seconds && est.filament == st.filament && est.moves == st.moves && st.moves > 6,
           "the estimate is what was written");
    printf("%d moves, %.1f s, %.3f mm of filament\n", st.moves, st.seconds, st.filament);

    {
        /* The first layer at 15 mm/s: its wall (25) and infill (60) both
         * slowed, travels not, the second layer's skin (40) as it was. */
        static sink k;
        gc_more x, part;
        gc_stats xs;
        om_machine flat = m;
        gc_more_defaults(&x, &o);
        expect(x.size == (int)sizeof x && x.o == &o && x.first == 0.f, "the more's defaults: as o alone");
        k.room = 1L << 30;
        expect(gc_write_more(into, &k, &p, &m, &x, &xs, err, (int)sizeof err) == 0 && !strcmp(k.buf, text) &&
                   xs.seconds == st.seconds,
               "no first-layer speed: what gc_write writes");
        x.first = 15.f;
        k.n = 0;
        expect(gc_write_more(into, &k, &p, &m, &x, &xs, err, (int)sizeof err) == 0, "written with a first-layer speed");
        {
            const char *two = strstr(k.buf, "; LAYER 2");
            int slow = 0;
            const char *s = k.buf;
            while ((s = strstr(s, "G1 X")) != NULL && (!two || s < two)) {
                const char *fe = strstr(s, " F");
                slow += fe && !strncmp(fe, " F900\n", 6);
                s++;
            }
            expect(slow == 5, "the first layer's five lines at 15 mm/s");
            expect(two && strstr(two, " F2400\n") && strstr(k.buf, " F18000\n"), "the second layer's skin at 40, travels at 300");
        }
        memset(flat.accel, 0, sizeof flat.accel);
        expect(gc_estimate_more(&p, &flat, &x, &xs) == GC_AT_SPEEDS &&
                   near(xs.seconds, 40.f / 15 + 70.7107f / 300 + 10.f / 15 + 0.2f / 5 + 10.f / 40 + 2.f / 20, 1e-3f),
               "and its time: the first layer's 50 mm at 15 mm/s");
        expect(gc_estimate_more(&p, &m, &x, &est) == GC_WITH_ACCEL && gc_write_more(into, &k, &p, &m, &x, &xs, err, 128) == 0 &&
                   est.seconds == xs.seconds,
               "the estimate is what was written");
        x.first = 500.f;
        expect(gc_estimate_more(&p, &flat, &x, &xs) == GC_AT_SPEEDS &&
                   near(xs.seconds, 40.f / 25 + 70.7107f / 300 + 10.f / 60 + 0.2f / 5 + 10.f / 40 + 2.f / 20, 1e-3f),
               "a first-layer speed above the others speeds nothing up");
        /* An older program's shorter gc_more: no first-layer speed. */
        part = x;
        part.first = 15.f;
        part.size = (int)offsetof(gc_more, first);
        expect(gc_estimate_more(&p, &flat, &part, &xs) == GC_AT_SPEEDS &&
                   near(xs.seconds, 40.f / 25 + 70.7107f / 300 + 10.f / 60 + 0.2f / 5 + 10.f / 40 + 2.f / 20, 1e-3f),
               "a gc_more cut short at its size: no first-layer speed");
        part.size = (int)sizeof part.size;
        expect(gc_write_more(into, &k, &p, &m, &part, &xs, err, (int)sizeof err) == -1 && strstr(err, "too short"),
               "too short for its options: refused");
        /* The user's own lines sit after the machine's start and before its end. */
        x.first = 0.f;
        x.start = "M117 hello";
        x.end = "M300 S10";
        k.n = 0;
        expect(gc_write_more(into, &k, &p, &m, &x, &xs, err, (int)sizeof err) == 0, "written with the user's own lines");
        {
            const char *hello = strstr(k.buf, "M117 hello\n");
            const char *beep = strstr(k.buf, "M300 S10\n");
            const char *home = strstr(k.buf, "G28");
            const char *end = strstr(k.buf, "M84");
            expect(hello && home && hello > home, "the user's start after the machine's");
            expect(beep && end && beep < end && k.n >= 4 && strcmp(k.buf + k.n - 4, "M84\n") == 0,
                   "the user's end before M84, and M84 last");
            expect(!strstr(k.buf, "\n\n"), "still no empty lines");
        }
        part = x;
        part.size = (int)offsetof(gc_more, start);
        k.n = 0;
        expect(gc_write_more(into, &k, &p, &m, &part, &xs, err, (int)sizeof err) == 0 && !strstr(k.buf, "M117") &&
                   !strstr(k.buf, "M300") && strstr(k.buf, "M84"),
               "a gc_more cut short of start and end: the machine's code only");
    }

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

    {
        /* Acceleration 1000 mm/s2: a 100 mm line at 100 mm/s speeds up for
         * 0.1 s over 5 mm, runs 90 mm, and slows for 0.1 s: 1.1 s. 1 mm
         * never reaches 100: up to sqrt(1000) and down, 2 sqrt(1000)/1000 s. */
        om_machine a = m;
        gt_path q;
        gc_opts qo = o;
        a.accel[OM_X] = a.accel[OM_Y] = a.accel[OM_E] = 1000.f;
        a.accel[OM_Z] = 100.f;
        memset(a.jerk, 0, sizeof a.jerk);
        qo.speed[GT_INFILL] = 100.f;
        qo.retract = 0.f;
        gt_init(&q);
        gt_layer(&q);
        gt_add(&q, GT_TRAVEL, 0, 0, 0.2f, 0);
        gt_add(&q, GT_INFILL, 100, 0, 0.2f, 0.4f);
        gc_estimate(&q, &a, &qo, &est);
        expect(near(est.seconds, 1.1f, 1e-4f), "100 mm at 100 mm/s with 1000 mm/s2: 1.1 s");
        gt_free(&q);
        gt_layer(&q);
        gt_add(&q, GT_TRAVEL, 0, 0, 0.2f, 0);
        gt_add(&q, GT_INFILL, 1, 0, 0.2f, 0.4f);
        gc_estimate(&q, &a, &qo, &est);
        expect(near(est.seconds, 2.f * sqrtf(1000.f) / 1000.f, 1e-5f), "1 mm never reaches 100 mm/s");

        /* A square corner: two 10 mm lines at 50 mm/s. With no jerk it
         * stops there (0.25 s each); with a jerk of 10 it turns at
         * 10 / (2 sin 45) = 7.07 mm/s. */
        qo.speed[GT_INFILL] = 50.f;
        gt_free(&q);
        gt_layer(&q);
        gt_add(&q, GT_TRAVEL, 0, 0, 0.2f, 0);
        gt_add(&q, GT_INFILL, 10, 0, 0.2f, 0.4f);
        gt_add(&q, GT_INFILL, 10, 10, 0.2f, 0.4f);
        gc_estimate(&q, &a, &qo, &est);
        expect(near(est.seconds, 0.5f, 1e-4f), "no jerk: it stops at the corner");
        a.jerk[OM_X] = a.jerk[OM_Y] = 10.f;
        gc_estimate(&q, &a, &qo, &est);
        {
            float vj = 10.f / (2.f * sinf(3.14159265f / 4.f)), one = 0.05f + (50.f - vj) / 1000.f +
                                                                     (10.f - 1.25f - (2500.f - vj * vj) / 2000.f) / 50.f;
            expect(near(est.seconds, 2.f * one, 1e-4f), "a jerk of 10: it turns the corner at 7.07 mm/s");
        }
        gt_add(&q, GT_INFILL, 20, 10, 0.2f, 0.4f);
        gc_estimate(&q, &a, &qo, &st);
        expect(st.seconds > est.seconds + 0.2f, "one more line, more time");
        gt_free(&q);
    }

    /* Grbl laser G-code. */
    {
        static const char *drv =
            "family = laser\n"
            "dialect = grbl\n"
            "name = Test diode\n"
            "send = file\n"
            "features = laser-power homing\n"
            "area = 400 400 50\n"
            "speed = 200 200 20 0\n"
            "accel = 2000 2000 200 0\n"
            "laser = 100\n"
            "start = G21\n"
            "start += G90\n"
            "end = G0 X0 Y0\n";
        om_machine laser;
        gc_laser L;
        gc_stats lst;
        om_job job;
        gt_path lp;
        sink sk;
        char why[128];
        int how;
        om_clear(&laser);
        expect(om_parse(&laser, drv, err, (int)sizeof err) == 0 && laser.family == OM_LASER, "a test Grbl laser");
        gc_laser_defaults(&L);
        L.s100 = 1000;
        L.speed = 20;
        L.travel = 100;
        gt_init(&lp);
        gt_add(&lp, GT_TRAVEL, 0, 0, 0, 0);
        gt_add(&lp, GT_CUT, 10, 0, 0, 50);   /* 50%, 10 mm */
        gt_add(&lp, GT_TRAVEL, 10, 10, 0, 0);
        gt_add(&lp, GT_CUT, 20, 10, 0, 100); /* 100%, 10 mm */
        memset(&sk, 0, sizeof sk);
        sk.room = (long)sizeof sk.buf;
        expect(gc_laser_write_to(into, &sk, &lp, &laser, &L, &lst, err, (int)sizeof err) == 0, "laser G-code written");
        expect(strstr(sk.buf, "; GFOpenMake laser") && strstr(sk.buf, "G21") && strstr(sk.buf, "G90") &&
                   strstr(sk.buf, "G0 X0 Y0"),
               "the start and end code are in the job");
        expect(strstr(sk.buf, "M4") && !strstr(sk.buf, "M3\n"), "M4 is the default");
        expect(strstr(sk.buf, "S500") && strstr(sk.buf, "S1000"), "50% is S500, 100% is S1000, for s100 1000");
        expect(strstr(sk.buf, "G1 X10.000 Y0.000") && strstr(sk.buf, "G1 X20.000 Y10.000"), "the two cuts");
        expect(strstr(sk.buf, "G0 X10.000 Y10.000") != NULL, "the travel between them is G0");
        expect(laser_safe(sk.buf, why, (int)sizeof why), why[0] ? why : "the laser is off on travels and at the end");
        expect(lst.moves == 4 && lst.filament == 0.f && lst.seconds > 0.f, "four moves, no filament, some time");

        L.constant = 1;
        L.s100 = 255;
        memset(&sk, 0, sizeof sk);
        sk.room = (long)sizeof sk.buf;
        expect(gc_laser_write_to(into, &sk, &lp, &laser, &L, NULL, err, (int)sizeof err) == 0 &&
                   strstr(sk.buf, "M3") && !strstr(sk.buf, "M4") && strstr(sk.buf, "S128") &&
                   strstr(sk.buf, "S255") && laser_safe(sk.buf, why, (int)sizeof why),
               "M3 constant, S scaled to 255 (50% is S128)");

        how = gc_laser_estimate_how(&lp, &laser, &L, &lst);
        expect(how == GC_WITH_ACCEL && lst.moves == 4 && lst.seconds > 0.1f && lst.seconds < 2.f,
               "the estimate counts the acceleration");
        L.travel = 500; /* more than the machine's 200 */
        gc_laser_job(&lp, &laser, &L, &job);
        expect(job.min[0] == 0.f && job.max[0] == 20.f && job.max[1] == 10.f && job.laser == 100.f &&
                   job.needs == OM_LASER_POWER && job.speed[OM_X] <= 200.f && job.speed[OM_X] > 0.f,
               "the job's box, 100% power, and travel held to 200 mm/s");

        {
            gc_laser_more more;
            float spd[4] = {0, 10, 0, 10};
            gc_laser_more_defaults(&more, &L);
            more.speed = spd;
            L.constant = 0;
            L.s100 = 1000;
            memset(&sk, 0, sizeof sk);
            sk.room = (long)sizeof sk.buf;
            expect(gc_laser_write_more(into, &sk, &lp, &laser, &more, &lst, err, (int)sizeof err) == 0 &&
                       strstr(sk.buf, " F600\n") && laser_safe(sk.buf, why, (int)sizeof why),
                   "a per-move speed of 10 mm/s is F600");
            more.start = "M117 hello";
            more.end = "M300 S10";
            memset(&sk, 0, sizeof sk);
            sk.room = (long)sizeof sk.buf;
            expect(gc_laser_write_more(into, &sk, &lp, &laser, &more, &lst, err, (int)sizeof err) == 0, "the user's lines");
            {
                const char *hello = strstr(sk.buf, "M117 hello\n");
                const char *beep = strstr(sk.buf, "M300 S10\n");
                const char *g90 = strstr(sk.buf, "G90");
                const char *home = strstr(sk.buf, "G0 X0 Y0");
                expect(hello && g90 && hello > g90, "the user's start after the machine's");
                expect(beep && home && beep < home, "the user's end before the machine's");
                expect(laser_safe(sk.buf, why, (int)sizeof why), why[0] ? why : "still off on travels and at the end");
            }
            {
                gc_laser_more old = more;
                old.size = (int)offsetof(gc_laser_more, start);
                old.start = "M117 hello";
                old.end = "M300 S10";
                memset(&sk, 0, sizeof sk);
                sk.room = (long)sizeof sk.buf;
                expect(gc_laser_write_more(into, &sk, &lp, &laser, &old, &lst, err, (int)sizeof err) == 0 &&
                           !strstr(sk.buf, "M117") && !strstr(sk.buf, "M300") && strstr(sk.buf, "G0 X0 Y0"),
                       "a gc_laser_more cut short of start and end: the machine's code only");
            }
        }

        expect(gc_laser_write_to(into, &sk, &lp, &m, &L, NULL, err, (int)sizeof err) == -1 &&
                   strstr(err, "laser"),
               "a printer is refused for laser G-code");
        gt_free(&lp);
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
