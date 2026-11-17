/* GFOpenSlicer, from the shell: the window's job (sl_job.h, the toolpath
 * engine on one task a core) for a printer, checked against it first.
 *   gfopenslicer in.stl out.gcode [DRIVER=driver.om] [PROFILE=profile.om]
 *       [LAYER=0.2] [INFILL=20] [LINE=0.4] [WALLS=2] [SKINS=3] [BRIM=3]
 *       [FILAMENT=1.75] [HOTEND=200] [BED=60] [TASKS=cores]
 * The printer is the generic Marlin one (machines/printer/marlin) unless
 * DRIVER or PROFILE say otherwise. SKIRT is no more: the engine makes a
 * brim. openmake-slice is the same with a picture as well.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gs.h"
#include "gs_layer.h"
#include "gs_par.h"
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

static void num(int argc, char **argv, const char *name, float *out)
{
    const char *v = arg(argc, argv, name);
    if (v) *out = (float)atof(v);
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
    gp_opts po;
    gc_opts go;
    char err[160];
    int rc = 1, tasks = gs_par_cores();
    (void)version;
    if (argc < 3) {
        fprintf(stderr, "GFOpenSlicer %s (%s)\n", GS_VERSION, GS_DATE);
        fprintf(stderr, "usage: gfopenslicer in.stl out.gcode [DRIVER=driver.om] [PROFILE=profile.om] [LAYER=0.2]\n"
                        "       [INFILL=20] [LINE=0.4] [WALLS=2] [SKINS=3] [BRIM=3] [FILAMENT=1.75] [HOTEND=200]\n"
                        "       [BED=60] [TASKS=cores]\n");
        return 1;
    }
    if (!driver) driver = "machines/printer/marlin/driver.om";
    if (!profile && !arg(argc, argv, "DRIVER")) profile = "machines/printer/marlin/profiles/generic-220.om";
    if (arg(argc, argv, "SKIRT")) fprintf(stderr, "GFOpenSlicer: SKIRT is gone: the slicer makes a brim (BRIM=3)\n");
    gp_defaults(&po);
    gc_defaults(&go);
    num(argc, argv, "LAYER", &po.layer_h);
    num(argc, argv, "INFILL", &po.infill);
    num(argc, argv, "LINE", &po.line_w);
    whole(argc, argv, "WALLS", &po.walls);
    whole(argc, argv, "SKINS", &po.skins);
    whole(argc, argv, "BRIM", &po.brim);
    num(argc, argv, "FILAMENT", &go.filament);
    num(argc, argv, "HOTEND", &go.hotend);
    num(argc, argv, "BED", &go.bed);
    whole(argc, argv, "TASKS", &tasks);
    if (tasks < 1) tasks = 1;
    if (tasks > 64) tasks = 64;
    go.layer_h = gl_height(po.layer_h);
    om_clear(&m);
    if (om_load(&m, driver, err, (int)sizeof err) || (profile && om_load(&m, profile, err, (int)sizeof err))) {
        fprintf(stderr, "GFOpenSlicer: %s\n", err);
        return 1;
    }
    sl_init(&j);
    if (sl_load(&j, argv[1], m.area[0], m.area[1], err, (int)sizeof err) ||
        sl_slice(&j, &po, &go, &m, tasks, err, (int)sizeof err) || sl_write(&j, argv[2], &m, &go, err, (int)sizeof err))
        fprintf(stderr, "GFOpenSlicer: %s\n", err);
    else {
        printf("GFOpenSlicer: wrote %s: %d layers, %d moves for %s\n", argv[2], j.path.nlayers, j.st.moves, m.name);
        rc = 0;
    }
    sl_free(&j);
    return rc;
}
