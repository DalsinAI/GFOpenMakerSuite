/* GFOpenSlicer's ARexx commands, run on the host through omr_do_line.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "slicer_rexx.h"

#include "gs_gcode.h"

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

static void io_init(omr_io *io, char *buf, int len)
{
    memset(io, 0, sizeof *io);
    io->size = (int)sizeof *io;
    io->result = buf;
    io->resultlen = len;
}

static int run(os_rexx *g, const char *line, omr_io *io)
{
    return omr_do_line(line, os_rexx_verbs(), os_rexx_nverbs(), g, io);
}

/* A 20 mm cube, centred later by sl_load, so the host test stays short. */
static int write_cube(const char *path)
{
    static const float v[8][3] = {{0, 0, 0}, {20, 0, 0}, {20, 20, 0}, {0, 20, 0},
                                  {0, 0, 20}, {20, 0, 20}, {20, 20, 20}, {0, 20, 20}};
    static const int f[12][3] = {{0, 1, 2}, {0, 2, 3}, {4, 6, 5}, {4, 7, 6}, {0, 4, 5}, {0, 5, 1},
                                 {1, 5, 6}, {1, 6, 2}, {2, 6, 7}, {2, 7, 3}, {3, 7, 4}, {3, 4, 0}};
    FILE *fp;
    int i;
    fp = fopen(path, "w");
    if (!fp) return -1;
    fprintf(fp, "solid cube\n");
    for (i = 0; i < 12; i++)
        fprintf(fp,
                "  facet normal 0 0 0\n    outer loop\n"
                "      vertex %g %g %g\n      vertex %g %g %g\n      vertex %g %g %g\n"
                "    endloop\n  endfacet\n",
                (double)v[f[i][0]][0], (double)v[f[i][0]][1], (double)v[f[i][0]][2],
                (double)v[f[i][1]][0], (double)v[f[i][1]][1], (double)v[f[i][1]][2],
                (double)v[f[i][2]][0], (double)v[f[i][2]][1], (double)v[f[i][2]][2]);
    fprintf(fp, "endsolid cube\n");
    fclose(fp);
    return 0;
}

static int file_has(const char *path, const char *what)
{
    FILE *f = fopen(path, "r");
    char line[240];
    int ok = 0;
    if (!f) return 0;
    while (fgets(line, (int)sizeof line, f))
        if (strstr(line, what)) {
            ok = 1;
            break;
        }
    fclose(f);
    return ok;
}

static int gcode_has_move(const char *path)
{
    FILE *f = fopen(path, "r");
    char line[80];
    int ok = 0;
    if (!f) return 0;
    while (fgets(line, (int)sizeof line, f))
        if (line[0] == 'G' || line[0] == 'M') {
            ok = 1;
            break;
        }
    fclose(f);
    return ok;
}

int main(void)
{
    os_rexx g;
    sl_job job;
    omr_io io;
    char buf[256];

    sl_init(&job);
    os_rexx_init(&g);
    g.job = &job;
    g.port_name = "OPENMAKESLICER.1";
    g.version = "GFOpenSlicer 0.1.0";
    g.tasks = 1;

    io_init(&io, buf, (int)sizeof buf);
    expect(run(&g, "VERSION", &io) == 0 && io.rc == OMR_OK && strstr(buf, "GFOpenSlicer"), "VERSION");
    expect(run(&g, "HELP", &io) == 0 && io.rc == OMR_OK && strstr(buf, "SLICE") && strstr(buf, "LOAD"),
           "HELP lists SLICE");
    expect(run(&g, "STATUS", &io) == 0 && io.rc == OMR_OK && strstr(buf, "OPENMAKESLICER.1") && strstr(buf, "0"),
           "STATUS, not dirty");
    expect(run(&g, "GET KEY tasks", &io) == 0 && io.rc == OMR_OK && strcmp(buf, "1") == 0, "GET tasks with no part");
    expect(run(&g, "SET KEY tasks VALUE 4", &io) == 0 && io.rc == OMR_OK && g.tasks == 4 && g.dirty == 0 &&
               strcmp(buf, "4") == 0,
           "SET tasks does not dirty");
    expect(run(&g, "GET KEY layer-height", &io) == 0 && io.rc == OMR_OK && strcmp(buf, "0.2") == 0, "GET layer-height");
    expect(run(&g, "SET KEY layer-height VALUE 0.3", &io) == 0 && io.rc == OMR_OK && g.dirty == 1, "SET layer-height");
    expect(run(&g, "GET KEY layer-height", &io) == 0 && io.rc == OMR_OK && strcmp(buf, "0.3") == 0, "GET after SET");
    expect(run(&g, "SET KEY around VALUE none", &io) == 0 && io.rc == OMR_OK, "SET around none");
    expect(run(&g, "SET KEY no-such VALUE 1", &io) == 0 && io.rc == OMR_ERROR, "SET unknown key");
    expect(run(&g, "PROFILE FILE settings/standard", &io) == 0 && io.rc == OMR_OK, "PROFILE");
    expect(run(&g, "SET KEY around VALUE none", &io) == 0 && io.rc == OMR_OK, "SET around none after profile");
    expect(run(&g, "STATS", &io) == 0 && io.rc == OMR_ERROR, "STATS before a slice");
    expect(run(&g, "SLICE", &io) == 0 && io.rc == OMR_ERROR, "SLICE with no part");

    expect(write_cube("build/rexx-slicer.stl") == 0, "wrote a 20 mm cube");
    expect(run(&g, "OPEN FILE build/rexx-slicer.stl", &io) == 0 && io.rc == OMR_OK && job.have_part && g.dirty == 0,
           "OPEN stl");
    expect(run(&g, "STATUS STEM ST", &io) == 0 && io.rc == OMR_OK && omr_host_var("ST.LOADED") &&
               strcmp(omr_host_var("ST.LOADED"), "1") == 0 && omr_host_var("ST.SLICED") &&
               strcmp(omr_host_var("ST.SLICED"), "0") == 0 && omr_host_var("ST.MOVES") &&
               !strcmp(omr_host_var("ST.MOVES"), "0") && omr_host_var("ST.ROUGH") &&
               !strcmp(omr_host_var("ST.ROUGH"), "0") && omr_host_var("ST.PICTURE") &&
               !strcmp(omr_host_var("ST.PICTURE"), "0") && omr_host_var("ST.LAYER") &&
               !strcmp(omr_host_var("ST.LAYER"), "1"),
           "STATUS LOADED");
    expect(run(&g, "LOAD FILE build/rexx-slicer.stl", &io) == 0 && io.rc == OMR_OK && job.have_part, "LOAD stl");

    expect(run(&g, "PRINTER", &io) == 0 && io.rc == OMR_ERROR, "PRINTER needs a file");
    expect(run(&g, "PRINTER DRIVER machines/printer/marlin/driver.om PROFILE machines/printer/marlin/profiles/generic-220.om",
                &io) == 0 &&
               io.rc == OMR_OK && g.have_machine && g.machine.family == OM_PRINTER,
           "PRINTER Marlin");
    expect(run(&g, "PRINTER DRIVER machines/laser/grbl/driver.om PROFILE machines/laser/grbl/profiles/generic-diode-400.om",
                &io) == 0 &&
               io.rc == OMR_ERROR && !g.have_machine,
           "PRINTER refuses a laser");
    expect(run(&g, "PRINTER DRIVER machines/printer/marlin/driver.om PROFILE machines/printer/marlin/profiles/generic-220.om",
                &io) == 0 &&
               io.rc == OMR_OK,
           "PRINTER Marlin again");

    expect(run(&g, "GET KEY support", &io) == 0 && io.rc == OMR_OK && strcmp(buf, "0") == 0, "GET support");
    expect(run(&g, "SET KEY support VALUE 1", &io) == 0 && io.rc == OMR_OK && job.support == 1, "SET support");
    expect(run(&g, "SET KEY support VALUE maybe", &io) == 0 && io.rc == OMR_ERROR && job.support == 1,
           "support is 0 or 1");
    expect(run(&g, "SET KEY start VALUE \"M117 hello\"", &io) == 0 && io.rc == OMR_OK && !strcmp(job.start, "M117 hello"),
           "SET start");
    expect(run(&g, "SET KEY end VALUE \"M117 done\"", &io) == 0 && io.rc == OMR_OK && !strcmp(job.end, "M117 done"),
           "SET end");
    expect(run(&g, "GET KEY start", &io) == 0 && io.rc == OMR_OK && !strcmp(buf, "M117 hello"), "GET start");
    expect(run(&g, "SET KEY start VALUE \"\"", &io) == 0 && io.rc == OMR_OK && job.start[0] == 0, "an empty start clears it");
    expect(run(&g, "SET KEY start VALUE \"M117 hello\"", &io) == 0 && io.rc == OMR_OK, "start set again");

    expect(run(&g, "SET KEY tasks VALUE maybe", &io) == 0 && io.rc == OMR_ERROR && g.tasks == 4, "tasks is a number");
    expect(run(&g, "SET KEY tasks VALUE 0", &io) == 0 && io.rc == OMR_OK && g.tasks == 1 && strcmp(buf, "1") == 0,
           "tasks 0 is 1");
    expect(run(&g, "SET KEY tasks VALUE 99", &io) == 0 && io.rc == OMR_OK && g.tasks == 64 && strcmp(buf, "64") == 0,
           "tasks 99 is 64");
    expect(run(&g, "SET KEY tasks VALUE 2", &io) == 0 && io.rc == OMR_OK && g.tasks == 2, "tasks set to 2");

    expect(run(&g, "SLICE STEM ST", &io) == 0 && io.rc == OMR_OK && job.sliced && job.path.nlayers > 0 &&
               job.tasks == 2 && g.fresh == 1 && strstr(buf, "layers") && omr_host_var("ST.LAYERS") &&
               atoi(omr_host_var("ST.LAYERS")) > 0,
           "SLICE");
    expect(omr_host_var("ST.ROUGH") && !strcmp(omr_host_var("ST.ROUGH"), (job.how & GC_FELL_BACK) ? "1" : "0") &&
               omr_host_var("ST.MOVES") && atoi(omr_host_var("ST.MOVES")) == job.st.moves && job.st.moves > 0,
           "SLICE says when the time is short");
    expect(run(&g, "STATUS STEM ST", &io) == 0 && io.rc == OMR_OK && strstr(buf, "OPENMAKESLICER.1") &&
               omr_host_var("ST.MOVES") && atoi(omr_host_var("ST.MOVES")) == job.st.moves &&
               omr_host_var("ST.ROUGH") &&
               !strcmp(omr_host_var("ST.ROUGH"), (job.how & GC_FELL_BACK) ? "1" : "0"),
           "STATUS says the moves, and when the time is short");
    {
        int dirty = g.dirty, n = job.path.nlayers;
        expect(n > 4, "the cube has more than four layers");
        expect(run(&g, "LAYERS", &io) == 0 && io.rc == OMR_OK && g.layers == 1 && g.layer_no == 1 && g.fresh == 0 &&
                   strstr(buf, "Layers 1 to 1"),
               "LAYERS turns the picture on");
        expect(run(&g, "LAYERS INDEX 4", &io) == 0 && io.rc == OMR_OK && g.layers == 1 && g.layer_no == 4 &&
                   strstr(buf, "Layers 1 to 4"),
               "LAYERS index");
        expect(run(&g, "STATUS STEM ST", &io) == 0 && io.rc == OMR_OK && omr_host_var("ST.PICTURE") &&
                   !strcmp(omr_host_var("ST.PICTURE"), "1") && omr_host_var("ST.LAYER") &&
                   !strcmp(omr_host_var("ST.LAYER"), "4"),
               "STATUS says layers 1 to 4");
        expect(run(&g, "LAYERS STEP 1", &io) == 0 && io.rc == OMR_OK && g.layer_no == 5 && g.layers == 1, "LAYERS next");
        expect(run(&g, "LAYERS STEP -1", &io) == 0 && io.rc == OMR_OK && g.layer_no == 4, "LAYERS prev");
        expect(run(&g, "LAYERS INDEX 1000000", &io) == 0 && io.rc == OMR_OK && g.layer_no == n && g.layers == 1,
               "LAYERS holds at the last layer");
        expect(run(&g, "LAYERS STEP 1", &io) == 0 && io.rc == OMR_OK && g.layer_no == n, "LAYERS step holds at the last");
        expect(run(&g, "LAYERS INDEX 0", &io) == 0 && io.rc == OMR_OK && g.layer_no == 1 && g.layers == 1,
               "LAYERS index 0 is 1");
        expect(run(&g, "LAYERS STEP -3", &io) == 0 && io.rc == OMR_OK && g.layer_no == 1, "LAYERS step holds at 1");
        expect(run(&g, "LAYERS ON 0", &io) == 0 && io.rc == OMR_OK && g.layers == 0 && strstr(buf, "hidden"),
               "LAYERS hides the picture");
        expect(run(&g, "LAYERS ON no", &io) == 0 && io.rc == OMR_ERROR && g.layers == 0, "LAYERS wants a number");
        expect(run(&g, "LAYERS INDEX no", &io) == 0 && io.rc == OMR_ERROR && g.layer_no == 1, "LAYERS index wants a number");
        expect(run(&g, "LAYERS STEP no", &io) == 0 && io.rc == OMR_ERROR && g.layer_no == 1, "LAYERS step wants a number");
        expect(g.dirty == dirty, "LAYERS does not dirty");
    }
    {
        int dirty = g.dirty;
        expect(run(&g, "LOOK NAME top", &io) == 0 && io.rc == OMR_OK && g.look == 0 && strstr(buf, "Top"), "LOOK top");
        expect(run(&g, "LOOK NAME front", &io) == 0 && io.rc == OMR_OK && g.look == 1 && strstr(buf, "Front"),
               "LOOK front");
        expect(run(&g, "LOOK NAME side", &io) == 0 && io.rc == OMR_OK && g.look == 2, "LOOK side");
        expect(run(&g, "LOOK NAME frame", &io) == 0 && io.rc == OMR_OK && g.look == 3 && strstr(buf, "Framed"),
               "LOOK frame");
        expect(run(&g, "LOOK NAME under", &io) == 0 && io.rc == OMR_ERROR && g.look == 3, "LOOK refuses an unknown view");
        expect(g.dirty == dirty, "LOOK does not dirty");
    }
    expect(run(&g, "STATS STEM ST", &io) == 0 && io.rc == OMR_OK && omr_host_var("ST.TIME") &&
               omr_host_var("ST.FILAMENT") && omr_host_var("ST.MOVES") &&
               atoi(omr_host_var("ST.MOVES")) == job.st.moves,
           "STATS");
    expect(run(&g, "SAVE FILE build/rexx-slicer.gcode", &io) == 0 && io.rc == OMR_OK && g.dirty == 0, "SAVE");
    expect(gcode_has_move("build/rexx-slicer.gcode") && file_has("build/rexx-slicer.gcode", "M117 hello") &&
               file_has("build/rexx-slicer.gcode", "M117 done"),
           "SAVE wrote the cut and the user's lines");

    expect(run(&g, "OPEN FILE no-such.stl", &io) == 0 && io.rc == OMR_ERROR, "OPEN missing file");
    g.dirty = 1;
    expect(run(&g, "QUIT", &io) == 0 && io.rc == OMR_ERROR && g.quit == 0, "QUIT refuses a dirty document");
    expect(run(&g, "QUIT FORCE", &io) == 0 && io.rc == OMR_OK && g.quit == 1, "QUIT FORCE");
    expect(run(&g, "ACTIVATE", &io) == 0 && io.rc == OMR_OK, "ACTIVATE with no window");
    expect(run(&g, "HIDE", &io) == 0 && io.rc == OMR_OK, "HIDE with no window");

    sl_free(&job);
    printf(fails ? "slicer rexx: %d failed\n" : "slicer rexx ok\n", fails);
    return fails != 0;
}
