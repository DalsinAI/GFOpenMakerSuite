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

    io_init(&io, buf, (int)sizeof buf);
    expect(run(&g, "VERSION", &io) == 0 && io.rc == OMR_OK && strstr(buf, "GFOpenMake"), "VERSION");
    expect(run(&g, "HELP", &io) == 0 && io.rc == OMR_OK && strstr(buf, "ADD") && strstr(buf, "OPEN"), "HELP lists ADD");
    expect(run(&g, "STATUS", &io) == 0 && io.rc == OMR_OK && strstr(buf, "GFOPENMAKE.1") && strstr(buf, "0"),
           "STATUS, not dirty");
    expect(run(&g, "STATUS STEM ST", &io) == 0 && io.rc == OMR_OK && omr_host_var("ST.SHOWN") &&
               !strcmp(omr_host_var("ST.SHOWN"), "1") && omr_host_var("ST.COLOUR") &&
               !strcmp(omr_host_var("ST.COLOUR"), "brass") && omr_host_var("ST.NAME") &&
               !strcmp(omr_host_var("ST.NAME"), "Box"),
           "STATUS says the part is shown, in brass, named Box");
    expect(run(&g, "SHOW ON 0", &io) == 0 && io.rc == OMR_OK && strstr(buf, "Hidden"), "SHOW hides it");
    expect(run(&g, "STATUS STEM ST", &io) == 0 && io.rc == OMR_OK && omr_host_var("ST.SHOWN") &&
               !strcmp(omr_host_var("ST.SHOWN"), "0"),
           "STATUS says it is hidden");
    expect(run(&g, "UNDO", &io) == 0 && io.rc == OMR_OK, "UNDO the hide");
    expect(run(&g, "COLOUR NAME steel", &io) == 0 && io.rc == OMR_OK, "COLOUR steel");
    expect(run(&g, "STATUS STEM ST", &io) == 0 && io.rc == OMR_OK && omr_host_var("ST.COLOUR") &&
               !strcmp(omr_host_var("ST.COLOUR"), "steel") && omr_host_var("ST.SHOWN") &&
               !strcmp(omr_host_var("ST.SHOWN"), "1"),
           "STATUS says the metal");
    expect(run(&g, "UNDO", &io) == 0 && io.rc == OMR_OK, "UNDO the metal");
    expect(run(&g, "GET KEY layer-height", &io) == 0 && io.rc == OMR_OK && strcmp(buf, "0.2") == 0, "GET layer-height");
    expect(run(&g, "SET KEY layer-height VALUE 0.3", &io) == 0 && io.rc == OMR_OK, "SET layer-height");
    expect(run(&g, "GET KEY layer-height", &io) == 0 && io.rc == OMR_OK && strcmp(buf, "0.3") == 0, "GET after SET");
    expect(run(&g, "SET KEY pattern VALUE grid", &io) == 0 && io.rc == OMR_OK && strcmp(buf, "grid") == 0,
           "SET pattern grid");
    expect(run(&g, "SET KEY no-such VALUE 1", &io) == 0 && io.rc == OMR_ERROR, "SET unknown key");
    expect(run(&g, "SET KEY around VALUE none", &io) == 0 && io.rc == OMR_OK, "SET around none");
    expect(run(&g, "PRINTER", &io) == 0 && io.rc == OMR_ERROR, "PRINTER needs a file");
    expect(run(&g, "PRINTER DRIVER machines/laser/grbl/driver.om PROFILE machines/laser/grbl/profiles/generic-diode-400.om",
               &io) == 0 &&
               io.rc == OMR_ERROR && !g.have_machine,
           "PRINTER refuses a laser");
    expect(run(&g, "PRINTER DRIVER machines/printer/marlin/driver.om PROFILE machines/printer/marlin/profiles/generic-220.om",
               &io) == 0 &&
               io.rc == OMR_OK && g.have_machine && g.machine.family == OM_PRINTER && buf[0],
           "PRINTER Marlin");
    expect(run(&g, "GCODE FILE build/rexx-box.gcode", &io) == 0 && io.rc == OMR_OK, "GCODE");
    {
        int dirty = g.dirty, back = ge_steps_back(steps);
        expect(run(&g, "SLICE", &io) == 0 && io.rc == OMR_OK && g.sliced == 1 && g.slice_layers > 0 &&
                   g.slice_moves > 0 && g.slice_tasks >= 1 && g.slice_tasks <= 64 && strstr(buf, "layers"),
               "SLICE");
        expect(g.dirty == dirty && ge_steps_back(steps) == back, "SLICE is not an undo step");
    }
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
    {
        int dirty = g.dirty, back = ge_steps_back(steps);
        expect(run(&g, "WHERE STEM ST", &io) == 0 && io.rc == OMR_OK && strstr(buf, "20.0 x 20.0 x 20.0 mm at 10.0, 10.0, 0.0"),
               "WHERE the starting box");
        expect(omr_host_var("ST.SX") && !strcmp(omr_host_var("ST.SX"), "20.0") && omr_host_var("ST.X") &&
                   !strcmp(omr_host_var("ST.X"), "10.0") && omr_host_var("ST.Z") && !strcmp(omr_host_var("ST.Z"), "0.0"),
               "WHERE stem is the Place fields");
        expect(g.dirty == dirty && ge_steps_back(steps) == back, "WHERE is not an undo step");
    }

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
    {
        int dirty = g.dirty, back = ge_steps_back(steps);
        const char *off;
        expect(run(&g, "MASS STEM ST", &io) == 0 && io.rc == OMR_OK && strstr(buf, " g") && !strstr(buf, "off the plate"),
               "MASS on the plate");
        off = omr_host_var("ST.OFF");
        expect(off && !strcmp(off, "0") && omr_host_var("ST.MASS") && atof(omr_host_var("ST.MASS")) > 1.0,
               "MASS stem, on the plate");
        expect(g.dirty == dirty && ge_steps_back(steps) == back && g.face == -1, "MASS is not an undo step");
    }
    expect(run(&g, "PLACE X 400", &io) == 0 && io.rc == OMR_OK, "PLACE the part off the plate");
    expect(run(&g, "MASS STEM ST", &io) == 0 && io.rc == OMR_OK && strstr(buf, "off the plate") && omr_host_var("ST.OFF") &&
               !strcmp(omr_host_var("ST.OFF"), "1"),
           "MASS marks a part off the plate");
    expect(run(&g, "UNDO", &io) == 0 && io.rc == OMR_OK, "UNDO the place");
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
        expect(run(&g, "DRAW KIND block", &io) == 0 && io.rc == OMR_ERROR, "DRAW needs two points");
        expect(run(&g, "DRAW X0 10 Y0 20 X1 40 Y1 28 KIND block H 6", &io) == 0 && io.rc == OMR_OK &&
                   gs_parts_count(ps) == n + 1 && strstr(buf, "on the bed"),
               "DRAW adds the box between the points");
        ge_where(gs_parts_mesh(ps, gs_parts_picked(ps)), pos, size);
        expect(size[0] > 29.f && size[0] < 31.f && size[1] > 7.f && size[1] < 9.f && size[2] > 5.f && size[2] < 7.f,
               "DRAW's block is 30 by 8 by 6");
        expect(run(&g, "UNDO", &io) == 0 && io.rc == OMR_OK && gs_parts_count(ps) == n, "UNDO the drawn block");
        expect(run(&g, "SNAP", &io) == 0 && io.rc == OMR_OK && g.snap == 1, "SNAP for a draw");
        expect(run(&g, "DRAW X0 10.4 Y0 20.4 X1 30.6 Y1 24.2 KIND block H 4", &io) == 0 && io.rc == OMR_OK,
               "DRAW with snap");
        ge_where(gs_parts_mesh(ps, gs_parts_picked(ps)), pos, size);
        expect(size[0] > 20.f && size[0] < 22.f && size[1] > 3.f && size[1] < 5.f, "snap holds the corners to 1 mm");
        expect(run(&g, "SNAP ON 0", &io) == 0 && io.rc == OMR_OK && g.snap == 0, "SNAP off after the draw");
        expect(run(&g, "UNDO", &io) == 0 && io.rc == OMR_OK && gs_parts_count(ps) == n, "UNDO the snapped block");
        expect(run(&g, "DRAW X0 0 Y0 0 X1 0.2 Y1 0.2 KIND block", &io) == 0 && io.rc == OMR_ERROR,
               "DRAW refuses a footprint under 1 mm");
        expect(run(&g, "DRAW X0 10 Y0 20 X1 30 Y1 28 H 5", &io) == 0 && io.rc == OMR_OK &&
                   !strcmp(gs_parts_name(ps, gs_parts_picked(ps)), "Block") && strstr(buf, "on the bed"),
               "DRAW without KIND is a block");
        expect(run(&g, "UNDO", &io) == 0 && io.rc == OMR_OK && gs_parts_count(ps) == n, "UNDO the default block");
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
    {
        int at;
        expect(run(&g, "SELECT PART 1", &io) == 0 && io.rc == OMR_OK, "SELECT a part to rename");
        at = gs_parts_picked(ps);
        expect(run(&g, "RENAME NAME Bracket", &io) == 0 && io.rc == OMR_OK && !strcmp(gs_parts_name(ps, at), "Bracket"),
               "RENAME");
        expect(run(&g, "STATUS STEM ST", &io) == 0 && io.rc == OMR_OK && omr_host_var("ST.NAME") &&
                   !strcmp(omr_host_var("ST.NAME"), "Bracket"),
               "STATUS says the new name");
        expect(run(&g, "UNDO", &io) == 0 && io.rc == OMR_OK && strcmp(gs_parts_name(ps, at), "Bracket") != 0,
               "UNDO the rename");
        expect(run(&g, "SHOW ON 0", &io) == 0 && io.rc == OMR_OK && !gs_parts_shown(ps, at), "SHOW hides it");
        expect(run(&g, "SHOW", &io) == 0 && io.rc == OMR_OK && gs_parts_shown(ps, at), "SHOW again toggles it on");
        expect(run(&g, "COLOUR NAME teal", &io) == 0 && io.rc == OMR_OK && gs_parts_colour(ps, at) == 4, "COLOUR teal");
        expect(run(&g, "COLOUR NAME plum", &io) == 0 && io.rc == OMR_ERROR, "COLOUR refuses an unknown metal");
    }
    expect(run(&g, "LOOK NAME top", &io) == 0 && io.rc == OMR_OK && g.look == 0 && strstr(buf, "Top"), "LOOK top");
    expect(run(&g, "LOOK NAME frame", &io) == 0 && io.rc == OMR_OK && g.look == 3 && strstr(buf, "Framed"), "LOOK frame");
    expect(run(&g, "LOOK NAME under", &io) == 0 && io.rc == OMR_ERROR && g.look == 3, "LOOK refuses an unknown view");
    expect(run(&g, "SHADE NAME clay", &io) == 0 && io.rc == OMR_OK && g.shade == 1 && strstr(buf, "Clay"), "SHADE clay");
    expect(run(&g, "SHADE NAME plum", &io) == 0 && io.rc == OMR_ERROR && g.shade == 1, "SHADE refuses an unknown picture");
    expect(run(&g, "SNAP", &io) == 0 && io.rc == OMR_OK && g.snap == 1 && strstr(buf, "1 mm"), "SNAP turns on");
    expect(run(&g, "SNAP ON 0", &io) == 0 && io.rc == OMR_OK && g.snap == 0 && strstr(buf, "off"), "SNAP off");
    expect(run(&g, "SNAP ON no", &io) == 0 && io.rc == OMR_ERROR && g.snap == 0, "SNAP wants a number");
    expect(run(&g, "SECTION", &io) == 0 && io.rc == OMR_OK && g.section == 1 && strstr(buf, "Section at 0.0"),
           "SECTION turns on");
    expect(run(&g, "SECTION Z 12.34", &io) == 0 && io.rc == OMR_OK && g.section == 1 && g.section_z > 12.2f &&
               g.section_z < 12.4f && strstr(buf, "12.3"),
           "SECTION at 12.3 mm");
    expect(run(&g, "SECTION Z 400", &io) == 0 && io.rc == OMR_OK && g.section_z == 220.f, "SECTION holds at 220 mm");
    expect(run(&g, "SECTION Z -4", &io) == 0 && io.rc == OMR_OK && g.section_z == 0.f && g.section == 1,
           "SECTION holds at 0 mm");
    expect(run(&g, "SECTION ON 0", &io) == 0 && io.rc == OMR_OK && g.section == 0 && strstr(buf, "off"), "SECTION off");
    expect(run(&g, "SECTION Z no", &io) == 0 && io.rc == OMR_ERROR && g.section == 0, "SECTION wants a number");
    expect(run(&g, "TOOL NAME orbit", &io) == 0 && io.rc == OMR_OK && g.tool == 0 && strstr(buf, "Orbit"), "TOOL orbit");
    expect(run(&g, "TOOL NAME move", &io) == 0 && io.rc == OMR_OK && g.tool == 1 && strstr(buf, "Move"), "TOOL move");
    expect(run(&g, "TOOL NAME rotate", &io) == 0 && io.rc == OMR_OK && g.tool == 2, "TOOL rotate");
    expect(run(&g, "TOOL NAME scale", &io) == 0 && io.rc == OMR_OK && g.tool == 3, "TOOL scale");
    expect(run(&g, "TOOL NAME measure", &io) == 0 && io.rc == OMR_OK && g.tool == 4, "TOOL measure");
    expect(run(&g, "TOOL NAME pull", &io) == 0 && io.rc == OMR_ERROR && g.tool == 4 && strstr(buf, "face"),
           "TOOL pull wants a face");
    g.face = 0;
    expect(run(&g, "TOOL NAME pull", &io) == 0 && io.rc == OMR_OK && g.tool == 5 && strstr(buf, "Pull"), "TOOL pull");
    expect(run(&g, "TOOL NAME draw", &io) == 0 && io.rc == OMR_OK && g.tool == 6 && strstr(buf, "Draw"), "TOOL draw");
    expect(run(&g, "TOOL NAME sketch", &io) == 0 && io.rc == OMR_OK && g.tool == 6, "TOOL sketch is draw");
    expect(run(&g, "TOOL NAME lathe", &io) == 0 && io.rc == OMR_ERROR && g.tool == 6, "TOOL refuses an unknown tool");
    {
        int back = ge_steps_back(steps), dirty = g.dirty;
        expect(g.layers == 0 && g.layer_no == 1, "layers start hidden, on layer 1");
        expect(run(&g, "LAYERS", &io) == 0 && io.rc == OMR_OK && g.layers == 1 && g.layer_no == 1 && strstr(buf, "Layer 1"),
               "LAYERS turns the picture on");
        expect(run(&g, "LAYERS INDEX 12", &io) == 0 && io.rc == OMR_OK && g.layers == 1 && g.layer_no == 12 &&
                   strstr(buf, "Layer 12"),
               "LAYERS index");
        expect(run(&g, "LAYERS STEP 1", &io) == 0 && io.rc == OMR_OK && g.layer_no == 13 && g.layers == 1,
               "LAYERS next");
        expect(run(&g, "LAYERS STEP -1", &io) == 0 && io.rc == OMR_OK && g.layer_no == 12, "LAYERS prev");
        expect(run(&g, "LAYERS INDEX 1", &io) == 0 && io.rc == OMR_OK && g.layer_no == 1, "LAYERS back to 1");
        expect(run(&g, "LAYERS STEP -5", &io) == 0 && io.rc == OMR_OK && g.layer_no == 1 && g.layers == 1,
               "LAYERS step holds at 1");
        expect(run(&g, "LAYERS STEP no", &io) == 0 && io.rc == OMR_ERROR && g.layer_no == 1, "LAYERS step wants a number");
        expect(run(&g, "LAYERS INDEX 0", &io) == 0 && io.rc == OMR_OK && g.layer_no == 1 && g.layers == 1,
               "LAYERS index 0 is 1");
        expect(run(&g, "LAYERS INDEX -4", &io) == 0 && io.rc == OMR_OK && g.layer_no == 1, "LAYERS holds at 1");
        expect(run(&g, "LAYERS ON 0", &io) == 0 && io.rc == OMR_OK && g.layers == 0 && strstr(buf, "hidden"),
               "LAYERS hides the picture");
        expect(run(&g, "LAYERS INDEX 4", &io) == 0 && io.rc == OMR_OK && g.layers == 1 && g.layer_no == 4,
               "an index shows that layer");
        expect(run(&g, "LAYERS ON no", &io) == 0 && io.rc == OMR_ERROR && g.layers == 1 && g.layer_no == 4,
               "LAYERS wants a number");
        expect(run(&g, "LAYERS INDEX no", &io) == 0 && io.rc == OMR_ERROR && g.layer_no == 4, "LAYERS index wants a number");
        expect(g.dirty == dirty && ge_steps_back(steps) == back, "LAYERS is not an undo step");
    }

    gs_parts_free(ps);
    ge_steps_free(steps);
    printf(fails ? "make rexx: %d failed\n" : "make rexx ok\n", fails);
    return fails != 0;
}
