/* OpenMakeSlicer's job (sl_job.h) on the host: the NASA wrench on the
 * generic Marlin printer, sliced on one task and on four, written, and
 * refused for a bed it does not fit.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "sl_job.h"

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

static char *slurp(const char *path, long *n)
{
    FILE *f = fopen(path, "rb");
    char *s;
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    *n = ftell(f);
    fseek(f, 0, SEEK_SET);
    s = malloc((size_t)*n + 1);
    if (s && fread(s, 1, (size_t)*n, f) != (size_t)*n) {
        free(s);
        s = NULL;
    }
    if (s) s[*n] = 0;
    fclose(f);
    return s;
}

int main(void)
{
    sl_job j;
    om_machine m, tiny;
    gs_settings set;
    char err[160];
    char *one, *four;
    long n1 = 0, n4 = 0;

    sl_init(&j);
    om_clear(&m);
    om_clear(&tiny);
    expect(!om_load(&m, "machines/printer/marlin/driver.om", err, (int)sizeof err) &&
               !om_load(&m, "machines/printer/marlin/profiles/generic-220.om", err, (int)sizeof err) &&
               !om_load(&tiny, "machines/printer/marlin/driver.om", err, (int)sizeof err) &&
               !om_load(&tiny, "slicer/tests/data/tiny-bed.om", err, (int)sizeof err),
           "the generic Marlin and the tiny printer");
    gs_settings_default(&set);

    expect(sl_slice(&j, &set, &m, 1, err, (int)sizeof err) == -1 && err[0] && !j.sliced, "nothing to slice yet");
    expect(sl_load(&j, "core/tests/data/nowhere.stl", 220, 220, err, (int)sizeof err) == -1 && err[0] && !j.have_part,
           "no such STL");
    expect(sl_load(&j, "core/tests/data/nasa-wrench.stl", m.area[0], m.area[1], err, (int)sizeof err) == 0 && j.have_part,
           "the wrench loaded");
    expect(j.part.n > 0, "it has triangles");

    expect(sl_slice(&j, &set, &m, 1, err, (int)sizeof err) == 0 && j.sliced && j.tasks == 1 && j.path.nlayers == 130,
           "130 layers on one task");
    expect(j.st.moves > 1000 && j.st.seconds > 60.f && j.st.filament > 100.f, "and its estimate");
    expect(j.how == GC_WITH_ACCEL, "timed with the printer's acceleration, and said so");
    expect(sl_write(&j, "build/sl-job-1.gcode", &m, err, (int)sizeof err) == 0, "written for the Marlin");
    expect(sl_slice(&j, &set, &m, 4, err, (int)sizeof err) == 0 && j.tasks == 4 && j.path.nlayers == 130,
           "130 layers on four tasks");
    expect(sl_write(&j, "build/sl-job-4.gcode", &m, err, (int)sizeof err) == 0, "written again");
    one = slurp("build/sl-job-1.gcode", &n1);
    four = slurp("build/sl-job-4.gcode", &n4);
    expect(one && four && n1 == n4 && !memcmp(one, four, (size_t)n1), "the same G-code on one task and on four");
    expect(one && strstr(one, "M109 S200") && strstr(one, "; LAYER 130"), "the hotend heated, the last layer there");
    free(one);
    free(four);

    /* The settings it was sliced with are the ones written: a grid, a
     * skirt, the first layer at 15 mm/s, the hotend at 215. Changed after
     * the slice, they are not (the window keeps on editing them). */
    set.pattern = GS_GRID;
    set.around = GS_SKIRT;
    set.first = 15.f;
    set.hotend = 215.f;
    expect(sl_slice(&j, &set, &m, 2, err, (int)sizeof err) == 0 && j.set.hotend == 215.f && j.set.first == 15.f,
           "sliced with its settings, kept");
    set.hotend = 180.f;
    expect(sl_write(&j, "build/sl-job-set.gcode", &m, err, (int)sizeof err) == 0, "written");
    one = slurp("build/sl-job-set.gcode", &n1);
    expect(one && strstr(one, "M109 S215") && !strstr(one, "M109 S180") && strstr(one, " F900\n"),
           "at the hotend it was sliced at, the first layer at 15 mm/s");
    free(one);

    remove("build/sl-job-tiny.gcode");
    expect(sl_write(&j, "build/sl-job-tiny.gcode", &tiny, err, (int)sizeof err) == -1 &&
               !strncmp(err, "Tiny 50 cannot print this: ", 27),
           "refused for a bed it does not fit, and why");
    expect(fopen("build/sl-job-tiny.gcode", "r") == NULL, "nothing written then");

    expect(sl_load(&j, "core/tests/data/nasa-wrench.stl", 220, 220, err, (int)sizeof err) == 0 && !j.sliced,
           "a new part drops the slice");
    expect(sl_write(&j, "build/sl-job-x.gcode", &m, err, (int)sizeof err) == -1, "nothing to write before a slice");
    sl_free(&j);
    printf(fails ? "slicer job: %d failed\n" : "slicer job ok\n", fails);
    return fails != 0;
}
