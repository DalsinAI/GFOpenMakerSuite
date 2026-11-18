/* GFOpenSlicer, from the shell: the window's job (sl_job.h, the toolpath
 * engine on one task a core) for a printer, checked against it first.
 *   gfopenslicer in.stl out.gcode [DRIVER=driver.om] [PROFILE=profile.om]
 *       [SETTINGS=settings/standard] [key=value ...] [TASKS=cores]
 *       [LAYER=0.2] [INFILL=20] [LINE=0.4] [WALLS=2] [SKINS=3] [BRIM=n]
 *       [SKIRT=n] [FILAMENT=1.75] [HOTEND=200] [BED=60]
 *       [START=lines] [END=lines] [SUPPORT=0]
 * The printer is the generic Marlin one (machines/printer/marlin) unless
 * DRIVER or PROFILE say otherwise. The print settings: a profile, the
 * older names over it, key=value over those (sl_args.h). openmake-slice
 * is the same with a picture as well. START and END are the user's own
 * lines, after the machine's start and before its end. They do not
 * replace the heat, the home, or M84. SUPPORT=1 adds one wall under an
 * overhang steeper than 45 degrees, from the bed up to it.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gs.h"
#include "gs_layer.h"
#include "gs_par.h"
#include "sl_args.h"
#include "sl_job.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char version[] = GS_VERSTR;

static const char *arg(int argc, char **argv, const char *name)
{
    size_t n = strlen(name);
    int i;
    for (i = 3; i < argc; i++)
        if (strncmp(argv[i], name, n) == 0 && argv[i][n] == '=') return argv[i] + n + 1;
    return NULL;
}

static void whole(int argc, char **argv, const char *name, int *out)
{
    const char *v = arg(argc, argv, name);
    if (v) *out = atoi(v);
}

int main(int argc, char **argv)
{
    const char *driver = arg(argc, argv, "DRIVER"), *profile = arg(argc, argv, "PROFILE");
    sl_job j;
    om_machine m;
    gs_settings set;
    char err[160];
    int rc = 1, tasks = gs_par_cores();
    (void)version;
    if (argc < 3) {
        fprintf(stderr, "GFOpenSlicer %s (%s)\n", GS_VERSION, GS_DATE);
        fprintf(stderr, "usage: gfopenslicer in.stl out.gcode [DRIVER=driver.om] [PROFILE=profile.om]\n"
                        "       [SETTINGS=settings/standard] [key=value ...] [TASKS=cores]\n"
                        "       [LAYER=0.2] [INFILL=20] [LINE=0.4] [WALLS=2] [SKINS=3] [BRIM=n] [SKIRT=n]\n"
                        "       [FILAMENT=1.75] [HOTEND=200] [BED=60] [START=lines] [END=lines] [SUPPORT=0]\n"
                        "key: a settings profile's, as layer-height=0.16 pattern=grid around=brim\n"
                        "START and END: your own lines, after the machine's start and before its end\n"
                        "SUPPORT=1: one wall under an overhang steeper than 45 degrees, from the bed up\n");
        return 1;
    }
    if (!driver) driver = "machines/printer/marlin/driver.om";
    if (!profile && !arg(argc, argv, "DRIVER")) profile = "machines/printer/marlin/profiles/generic-220.om";
    if (sl_args(&set, argc, argv, 3, err, (int)sizeof err)) {
        fprintf(stderr, "GFOpenSlicer: %s\n", err);
        return 1;
    }
    whole(argc, argv, "TASKS", &tasks);
    if (tasks < 1) tasks = 1;
    if (tasks > 64) tasks = 64;
    om_clear(&m);
    if (om_load(&m, driver, err, (int)sizeof err) || (profile && om_load(&m, profile, err, (int)sizeof err))) {
        fprintf(stderr, "GFOpenSlicer: %s\n", err);
        return 1;
    }
    sl_init(&j);
    sl_own(&j, arg(argc, argv, "START"), arg(argc, argv, "END"));
    whole(argc, argv, "SUPPORT", &j.support);
    if (j.support < 0) j.support = 0;
    if (j.support > 1) j.support = 1;
    if (sl_load(&j, argv[1], m.area[0], m.area[1], err, (int)sizeof err) ||
        sl_slice(&j, &set, &m, tasks, err, (int)sizeof err) || sl_write(&j, argv[2], &m, err, (int)sizeof err))
        fprintf(stderr, "GFOpenSlicer: %s\n", err);
    else {
        printf("GFOpenSlicer: wrote %s: %d layers, %d moves for %s\n", argv[2], j.path.nlayers, j.st.moves, m.name);
        rc = 0;
    }
    sl_free(&j);
    return rc;
}
