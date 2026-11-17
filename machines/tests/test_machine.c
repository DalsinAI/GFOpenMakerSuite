/* The machine files, and the checks an app makes before it writes a job.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "om_machine.h"

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

int main(void)
{
    om_machine m;
    om_job job;
    char err[160], why[160], code[OM_CODE];
    int held = 0;

    om_clear(&m);
    expect(om_load(&m, "machines/printer/marlin/driver.om", err, (int)sizeof err) == 0, "the Marlin driver loads");
    expect(om_load(&m, "machines/printer/marlin/profiles/generic-220.om", err, (int)sizeof err) == 0,
           "the generic 220 mm profile loads over it");
    if (err[0]) printf("%s\n", err);
    expect(m.family == OM_PRINTER && strcmp(m.dialect, "marlin") == 0, "a Marlin printer");
    expect(m.area[0] == 220.f && m.area[1] == 220.f && m.area[2] == 250.f, "220 by 220 by 250 mm");
    expect(m.speed[OM_X] == 300.f && m.speed[OM_Z] == 5.f && m.accel[OM_X] == 3000.f, "speeds and accelerations");
    expect(m.jerk[OM_X] == 10.f && m.jerk[OM_Z] == 0.3f && m.jerk[OM_E] == 5.f, "and how sharply it may change speed (jerk)");
    expect(m.hotend == 260.f && m.bed == 100.f && m.extruders == 1, "temperatures and one extruder");
    expect(om_has(&m, OM_HEATED_BED) && om_has(&m, OM_HOMING), "a heated bed, and homing");
    expect(!om_has(&m, OM_AIR_ASSIST) && !om_has(&m, OM_SPINDLE) && !om_has(&m, OM_LASER_POWER),
           "no air assist, spindle or laser: those are greyed out");
    expect(!om_has(&m, OM_BED_LEVELLING), "the generic profile has no levelling probe");
    expect((m.send & OM_SEND_FILE) && (m.send & OM_SEND_SERIAL), "sent as a file or down a serial line");

    /* A 20 mm cube in the middle at 0.2 mm, 60 mm/s: fine. */
    memset(&job, 0, sizeof job);
    job.min[0] = job.min[1] = 100.f;
    job.max[0] = job.max[1] = 120.f;
    job.max[2] = 20.f;
    job.speed[OM_X] = job.speed[OM_Y] = 60.f;
    job.speed[OM_Z] = 5.f;
    job.hotend = 200.f;
    job.bed = 60.f;
    job.extruders = 1;
    job.needs = OM_HEATED_BED;
    expect(om_check(&m, &job, why, (int)sizeof why) == OM_OK, "a 20 mm cube fits");

    job.max[0] = 250.f;
    expect(om_check(&m, &job, why, (int)sizeof why) == OM_TOO_BIG, "a part 150 mm wide from x 100 is too big");
    printf("too big: %s\n", why);
    job.max[0] = 120.f;
    job.min[2] = -1.f;
    expect(om_check(&m, &job, why, (int)sizeof why) == OM_TOO_BIG, "below the plate is outside too");
    job.min[2] = 0.f;

    job.speed[OM_X] = 400.f;
    expect(om_check(&m, &job, why, (int)sizeof why) == OM_TOO_FAST, "400 mm/s on X is over the limit");
    printf("too fast: %s\n", why);
    expect(om_clamp_speed(&m, OM_X, 400.f, &held) == 300.f && held, "and is clamped to 300");
    expect(om_clamp_speed(&m, OM_X, 80.f, &held) == 80.f && !held, "80 mm/s is left alone");
    job.speed[OM_X] = 60.f;

    job.hotend = 300.f;
    expect(om_check(&m, &job, why, (int)sizeof why) == OM_TOO_HOT, "300 C is over the hotend's 260");
    job.hotend = 200.f;

    job.laser = 50.f;
    expect(om_check(&m, &job, why, (int)sizeof why) == OM_TOO_MUCH, "a printer has no laser to power");
    job.laser = 0.f;
    job.extruders = 2;
    job.needs |= OM_BED_LEVELLING;
    expect(om_check(&m, &job, why, (int)sizeof why) == OM_MISSING, "two extruders and levelling are missing");
    printf("missing: %s\n", why);
    job.extruders = 1;
    job.needs = OM_HEATED_BED;
    job.max[0] = 250.f;
    job.speed[OM_Y] = 500.f;
    expect(om_check(&m, &job, why, (int)sizeof why) == (OM_TOO_BIG | OM_TOO_FAST), "two problems at once");

    expect(om_code(m.start, 205.f, 60.f, code, (int)sizeof code) == 0 && strstr(code, "M190 S60") &&
               strstr(code, "M109 S205") && strstr(code, "G28"),
           "the start code heats the bed and the hotend, and homes");
    expect(om_code(m.end, 0.f, 0.f, code, (int)sizeof code) == 0 && strstr(code, "M104 S0") && strstr(code, "M84"),
           "the end code cools and lets the motors go");

    {
        om_machine bad;
        om_clear(&bad);
        expect(om_parse(&bad, "family = toaster\n", err, (int)sizeof err) == -1 && strstr(err, "line 1"),
               "an unknown family is refused, with its line");
        expect(om_parse(&bad, "area = 100 100\n", err, (int)sizeof err) == -1, "an area needs three numbers");
        expect(om_parse(&bad, "features = heated-bed warp-drive\n", err, (int)sizeof err) == -1, "and features are known words");
        expect(om_parse(&bad, "colour = red\n", err, (int)sizeof err) == -1, "an unknown key is refused");
    }

    if (failed) {
        printf("%d failed\n", failed);
        return 1;
    }
    printf("machine ok\n");
    return 0;
}
