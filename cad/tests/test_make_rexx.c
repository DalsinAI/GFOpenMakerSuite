/* GFOpenMake's ARexx commands, run on the host through omr_do_line.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "make_rexx.h"

#include "gs_model.h"
#include "om_machine.h"

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

static int run(gm_rexx *g, const char *line, omr_io *io)
{
    return omr_do_line(line, gm_rexx_verbs(), gm_rexx_nverbs(), g, io);
}

int main(void)
{
    gm_rexx g;
    gs_parts *ps;
    ge_steps *steps;
    gs_mesh cube;
    omr_io io;
    char buf[256];
    float pos[3], size[3];

    ps = gs_parts_new();
    steps = ge_steps_new();
    memset(&cube, 0, sizeof cube);
    expect(ps && steps && gm_cube(&cube, 0, 0, 0, 20, 20, 20) == 0 && gs_parts_add(ps, &cube, "Box") == 0,
           "a 20 mm box to start");
    gm_rexx_init(&g);
    g.parts = &ps;
    g.steps = steps;
    g.port_name = "GFOPENMAKE.1";
    {
        char err[160];
        expect(om_load(&g.machine, "machines/printer/marlin/driver.om", err, (int)sizeof err) == 0 &&
                   om_load(&g.machine, "machines/printer/marlin/profiles/generic-220.om", err, (int)sizeof err) == 0,
               "Marlin generic-220 for GCODE");
        g.have_machine = 1;
    }

    io_init(&io, buf, (int)sizeof buf);
    expect(run(&g, "VERSION", &io) == 0 && io.rc == OMR_OK && strstr(buf, "GFOpenMake"), "VERSION");
    expect(run(&g, "HELP", &io) == 0 && io.rc == OMR_OK && strstr(buf, "ADD") && strstr(buf, "OPEN"), "HELP lists ADD");
    expect(run(&g, "STATUS", &io) == 0 && io.rc == OMR_OK && strstr(buf, "GFOPENMAKE.1") && strstr(buf, "0"),
           "STATUS, not dirty");
    expect(run(&g, "GET KEY layer-height", &io) == 0 && io.rc == OMR_OK && strcmp(buf, "0.2") == 0, "GET layer-height");
    expect(run(&g, "SET KEY layer-height VALUE 0.3", &io) == 0 && io.rc == OMR_OK, "SET layer-height");
    expect(run(&g, "GET KEY layer-height", &io) == 0 && io.rc == OMR_OK && strcmp(buf, "0.3") == 0, "GET after SET");
    expect(run(&g, "SET KEY pattern VALUE grid", &io) == 0 && io.rc == OMR_OK && strcmp(buf, "grid") == 0,
           "SET pattern grid");
    expect(run(&g, "SET KEY no-such VALUE 1", &io) == 0 && io.rc == OMR_ERROR, "SET unknown key");
    expect(run(&g, "SET KEY around VALUE none", &io) == 0 && io.rc == OMR_OK, "SET around none");
    expect(run(&g, "GCODE FILE build/rexx-box.gcode", &io) == 0 && io.rc == OMR_OK, "GCODE");
    {
        FILE *f = fopen("build/rexx-box.gcode", "r");
        char line[80];
        int ok = 0;
        if (f) {
            while (fgets(line, (int)sizeof line, f))
                if (line[0] == 'G' || line[0] == 'M') {
                    ok = 1;
                    break;
                }
            fclose(f);
        }
        expect(ok, "GCODE wrote moves");
    }

    expect(run(&g, "ADD box W 10 D 10 H 10", &io) == 0 && io.rc == OMR_OK && strstr(buf, "2 ") &&
               gs_parts_count(ps) == 2,
           "ADD box");
    expect(run(&g, "SELECT PART 1", &io) == 0 && io.rc == OMR_OK && gs_parts_picked(ps) == 0, "SELECT PART 1");
    expect(run(&g, "SELECT PART 9", &io) == 0 && io.rc == OMR_ERROR, "SELECT missing part");
    expect(run(&g, "SELECT NAME Box", &io) == 0 && io.rc == OMR_OK && gs_parts_picked(ps) == 0, "SELECT NAME");

    ge_where(gs_parts_mesh(ps, 0), pos, size);
    expect(run(&g, "MOVE X 5", &io) == 0 && io.rc == OMR_OK, "MOVE X 5");
    {
        float pos2[3], size2[3];
        ge_where(gs_parts_mesh(ps, 0), pos2, size2);
        expect(pos2[0] > pos[0] + 4.9f && pos2[0] < pos[0] + 5.1f, "moved 5 mm in X");
    }
    expect(run(&g, "PLACE Z 0", &io) == 0 && io.rc == OMR_OK, "PLACE Z 0");
    expect(run(&g, "SIZE Z 15", &io) == 0 && io.rc == OMR_OK, "SIZE Z 15");
    ge_where(gs_parts_mesh(ps, 0), pos, size);
    expect(size[2] > 14.5f && size[2] < 15.5f, "box is 15 mm tall");

    expect(run(&g, "UNDO", &io) == 0 && io.rc == OMR_OK, "UNDO");
    ge_where(gs_parts_mesh(ps, 0), pos, size);
    expect(size[2] > 19.5f && size[2] < 20.5f, "undo restored the height");
    expect(run(&g, "REDO", &io) == 0 && io.rc == OMR_OK, "REDO");
    ge_where(gs_parts_mesh(ps, 0), pos, size);
    expect(size[2] > 14.5f && size[2] < 15.5f, "redo sized it again");

    expect(run(&g, "FACTS STEM ST", &io) == 0 && io.rc == OMR_OK && strstr(buf, "triangles") &&
               omr_host_var("ST.TRIANGLES") && atoi(omr_host_var("ST.TRIANGLES")) > 0,
           "FACTS");
    expect(run(&g, "MEASURE X1 0 Y1 0 Z1 0 X2 10 Y2 0 Z2 0 STEM ST", &io) == 0 && io.rc == OMR_OK &&
               strstr(buf, "10.0") && omr_host_var("ST.DISTANCE") && strstr(omr_host_var("ST.DISTANCE"), "10"),
           "MEASURE 10 mm");

    expect(run(&g, "SELECT PART 2", &io) == 0 && run(&g, "DELETE", &io) == 0 && io.rc == OMR_OK &&
               gs_parts_count(ps) == 1,
           "DELETE the added box");
    expect(run(&g, "DELETE", &io) == 0 && io.rc == OMR_ERROR, "DELETE the last part is refused");

    expect(run(&g, "SAVE FILE build/rexx-box.stl", &io) == 0 && io.rc == OMR_OK && g.dirty == 0, "SAVE");
    expect(run(&g, "EXPORT FILE build/rexx-box2.stl", &io) == 0 && io.rc == OMR_OK, "EXPORT");
    expect(run(&g, "OPEN FILE build/rexx-box.stl", &io) == 0 && io.rc == OMR_OK && gs_parts_count(ps) == 2 &&
               g.dirty == 1,
           "OPEN adds a part");

    g.dirty = 1;
    expect(run(&g, "QUIT", &io) == 0 && io.rc == OMR_ERROR && g.quit == 0, "QUIT refuses a dirty document");
    expect(run(&g, "QUIT FORCE", &io) == 0 && io.rc == OMR_OK && g.quit == 1, "QUIT FORCE");

    expect(run(&g, "ACTIVATE", &io) == 0 && io.rc == OMR_OK, "ACTIVATE with no window");
    expect(run(&g, "HIDE", &io) == 0 && io.rc == OMR_OK, "HIDE with no window");
    expect(run(&g, "ADD potato", &io) == 0 && io.rc == OMR_ERROR, "ADD unknown shape");
    expect(run(&g, "PULL MM 2 FACE 0", &io) == 0, "PULL runs (rc depends on the face)");
    expect(run(&g, "HOLE KIND circle D 3 N 4 PCD 10", &io) == 0, "HOLE circle runs");
    expect(run(&g, "HOLE KIND drill D 6", &io) == 0 && io.rc == OMR_OK, "HOLE drill through the box");
    expect(run(&g, "HOLE KIND countersink D 6 CB 12", &io) == 0 && io.rc == OMR_OK, "HOLE countersink");
    {
        int n = gs_parts_count(ps);
        expect(run(&g, "WASHER", &io) == 0 && io.rc == OMR_OK && gs_parts_count(ps) == n + 1 &&
                   strstr(buf, "32 mm washer") && !strcmp(gs_parts_name(ps, gs_parts_picked(ps)), "Washer"),
               "WASHER adds a part");
    }
    {
        int n = gs_parts_count(ps);
        expect(run(&g, "SELECT PART 1", &io) == 0 && run(&g, "SPLIT Z 10", &io) == 0 && io.rc == OMR_OK &&
                   strstr(buf, "10.0") && gs_parts_count(ps) == n + 1 &&
                   !strcmp(gs_parts_name(ps, 0), "Box lower") &&
                   !strcmp(gs_parts_name(ps, gs_parts_picked(ps)), "Box upper"),
               "SPLIT at 10 mm");
    }
    {
        int n = gs_parts_count(ps), i, hidden = 0;
        expect(run(&g, "SELECT PART 1", &io) == 0 && io.rc == OMR_OK, "SELECT the lower half");
        expect(run(&g, "COPY", &io) == 0 && io.rc == OMR_OK && gs_parts_count(ps) == n + 1 && strstr(buf, "copy"),
               "COPY adds one part");
        expect(run(&g, "UNDO", &io) == 0 && io.rc == OMR_OK && gs_parts_count(ps) == n, "UNDO the copy");
        expect(run(&g, "LINE AXIS y COUNT 3 GAP 8", &io) == 0 && io.rc == OMR_OK && gs_parts_count(ps) == n + 2 &&
                   strstr(buf, "along Y"),
               "LINE of 3 along Y");
        n = gs_parts_count(ps);
        expect(run(&g, "LINE AXIS q", &io) == 0 && io.rc == OMR_ERROR, "LINE refuses a bad axis");
        expect(run(&g, "RING COUNT 4", &io) == 0 && io.rc == OMR_OK && gs_parts_count(ps) == n + 3 &&
                   strstr(buf, "Ring of 4"),
               "RING of 4");
        expect(run(&g, "MIRROR", &io) == 0 && io.rc == OMR_OK && strstr(buf, "mirrored"), "MIRROR");
        expect(run(&g, "TURN", &io) == 0 && io.rc == OMR_OK && strstr(buf, "90"), "TURN 90 degrees about Z");
        expect(run(&g, "STACK", &io) == 0 && io.rc == OMR_OK && strstr(buf, "stacked"), "STACK");
        expect(run(&g, "SOLO", &io) == 0 && io.rc == OMR_OK && strstr(buf, "Only"), "SOLO hides the others");
        for (i = 0; i < gs_parts_count(ps); i++)
            if (!gs_parts_shown(ps, i)) hidden++;
        expect(hidden > 0, "a part is hidden");
        expect(run(&g, "SOLO", &io) == 0 && io.rc == OMR_OK && strstr(buf, "Every"), "SOLO again shows them all");
    }
    {
        int n = gs_parts_count(ps);
        expect(run(&g, "SHELF NAME \"Angle bracket\"", &io) == 0 && io.rc == OMR_OK && gs_parts_count(ps) == n + 1 &&
                   strstr(buf, "Angle bracket") &&
                   !strcmp(gs_parts_name(ps, gs_parts_picked(ps)), "Angle bracket"),
               "SHELF puts the angle bracket on the bed");
        expect(run(&g, "UNDO", &io) == 0 && io.rc == OMR_OK && gs_parts_count(ps) == n, "UNDO the shelf part");
        expect(run(&g, "SHELF NAME \"no such thing\"", &io) == 0 && io.rc == OMR_ERROR, "SHELF refuses an unknown name");
    }
    {
        int n = gs_parts_count(ps);
        float pos[3], size[3];
        expect(run(&g, "EXTRUDE KIND block", &io) == 0 && io.rc == OMR_OK && gs_parts_count(ps) == n + 1 &&
                   strstr(buf, "on the bed") && !strcmp(gs_parts_name(ps, gs_parts_picked(ps)), "Block"),
               "EXTRUDE add puts a block on the bed");
        expect(run(&g, "UNDO", &io) == 0 && io.rc == OMR_OK && gs_parts_count(ps) == n, "UNDO the block");
        expect(run(&g, "SELECT PART 1", &io) == 0 && io.rc == OMR_OK, "SELECT a part to stand on");
        ge_where(gs_parts_mesh(ps, gs_parts_picked(ps)), pos, size);
        expect(run(&g, "EXTRUDE KIND round OP on W 8 H 4", &io) == 0 && io.rc == OMR_OK && strstr(buf, "stands") &&
                   gs_parts_count(ps) == n,
               "EXTRUDE on joins a round to the top");
        ge_where(gs_parts_mesh(ps, gs_parts_picked(ps)), pos, size);
        expect(size[2] > 4.f, "the part is still there, and taller than the boss alone");
        expect(run(&g, "EXTRUDE KIND potato", &io) == 0 && io.rc == OMR_ERROR, "EXTRUDE refuses an unknown kind");
    }
    {
        float pos[3], size[3];
        expect(run(&g, "SELECT PART 1", &io) == 0 && io.rc == OMR_OK, "SELECT a part to set on the bed");
        expect(run(&g, "PLACE Z 12", &io) == 0 && io.rc == OMR_OK, "lift it 12 mm");
        expect(run(&g, "DROP", &io) == 0 && io.rc == OMR_OK && strstr(buf, "On the bed"), "DROP");
        ge_where(gs_parts_mesh(ps, gs_parts_picked(ps)), pos, size);
        expect(pos[2] > -0.05f && pos[2] < 0.05f, "DROP sits the part on the bed");
        expect(run(&g, "PLACE X 40 Y 40", &io) == 0 && io.rc == OMR_OK, "move it off the middle");
        expect(run(&g, "CENTRE", &io) == 0 && io.rc == OMR_OK && strstr(buf, "Centred"), "CENTRE");
        ge_where(gs_parts_mesh(ps, gs_parts_picked(ps)), pos, size);
        expect(pos[0] > GS_BED * 0.5f - 0.5f && pos[0] < GS_BED * 0.5f + 0.5f && pos[1] > GS_BED * 0.5f - 0.5f &&
                   pos[1] < GS_BED * 0.5f + 0.5f,
               "CENTRE puts it in the middle of the bed");
        expect(run(&g, "LAY FACE 9999", &io) == 0 && io.rc == OMR_ERROR, "LAY refuses a face that is not there");
        expect(run(&g, "LAY", &io) == 0 && (io.rc == OMR_OK) &&
                   (strstr(buf, "on the bed") || strstr(buf, "already flat")),
               "LAY the largest face");
    }

    gs_parts_free(ps);
    ge_steps_free(steps);
    printf(fails ? "make rexx: %d failed\n" : "make rexx ok\n", fails);
    return fails != 0;
}
