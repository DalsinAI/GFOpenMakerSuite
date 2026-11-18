/* GFOpenMakerWorkshop's ARexx commands, run on the host through omr_do_line.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#include "workshop_rexx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

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

static int join2(char *dst, int n, const char *a, const char *b)
{
    size_t na, nb;
    if (!dst || n < 1 || !a || !b) return -1;
    na = strlen(a);
    nb = strlen(b);
    if (na + nb + 1 > (size_t)n) return -1;
    memcpy(dst, a, na);
    memcpy(dst + na, b, nb + 1);
    return 0;
}

static int add_line(char *dst, int n, const char *kind, const char *file, const char *name)
{
    char mid[640];
    if (join2(mid, (int)sizeof mid, "ADD KIND ", kind)) return -1;
    if (join2(dst, n, mid, " FILE ")) return -1;
    if (join2(mid, (int)sizeof mid, dst, file)) return -1;
    if (join2(dst, n, mid, " NAME ")) return -1;
    return join2(mid, (int)sizeof mid, dst, name) || join2(dst, n, mid, "");
}

static int run(gw_rexx *g, const char *line, omr_io *io)
{
    return omr_do_line(line, gw_rexx_verbs(), gw_rexx_nverbs(), g, io);
}

static char opened[512];

static int fake_open(void *c, void *g0)
{
    char *dst = g0;
    const char *file = omr_arg(c, "FILE");
    if (dst) snprintf(dst, sizeof opened, "%s", file ? file : "");
    omr_set_result(c, file ? file : "");
    return OMR_OK;
}

static const omr_verb fake_verbs[] = {
    {sizeof(omr_verb), "OPEN", "FILE/A", "Open the file", fake_open},
};

int main(void)
{
    workshop w;
    gw_rexx g;
    omr_io io;
    omr_port *burn = NULL;
    char buf[640], dir[640], svg[640], kicad[640], rm[700];
    char *cwd;
    const char *launch;

    workshop_init(&w);
    gw_rexx_init(&g);
    g.ws = &w;
    g.port_name = "OPENMAKEWORKSHOP.1";
    g.version = "GFOpenMakerWorkshop 0.1.0";

    io_init(&io, buf, (int)sizeof buf);
    expect(run(&g, "VERSION", &io) == 0 && io.rc == OMR_OK && strstr(buf, "GFOpenMakerWorkshop"), "VERSION");
    expect(run(&g, "HELP", &io) == 0 && io.rc == OMR_OK && strstr(buf, "OPENITEM") && strstr(buf, "SEND"),
           "HELP lists OPENITEM and SEND");
    expect(run(&g, "STATUS", &io) == 0 && io.rc == OMR_OK && strstr(buf, "OPENMAKEWORKSHOP.1"), "STATUS with no project");
    expect(run(&g, "STATUS STEM ST", &io) == 0 && io.rc == OMR_OK && omr_host_var("ST.NAME") &&
               !strcmp(omr_host_var("ST.NAME"), "") && omr_host_var("ST.NOTES") &&
               !strcmp(omr_host_var("ST.NOTES"), "") && omr_host_var("ST.PICKED") &&
               !strcmp(omr_host_var("ST.PICKED"), "0") && omr_host_var("ST.ITEM") &&
               !strcmp(omr_host_var("ST.ITEM"), "") && omr_host_var("ST.PATH") &&
               !strcmp(omr_host_var("ST.PATH"), ""),
           "STATUS has no project to name");
    expect(run(&g, "ADD KIND laser FILE x NAME Plate", &io) == 0 && io.rc == OMR_ERROR, "ADD with no project");
    expect(run(&g, "KINDS STEM ST", &io) == 0 && io.rc == OMR_OK && !strcmp(buf, "5") && !w.have && !w.project.dirty &&
               omr_host_var("ST.0") && !strcmp(omr_host_var("ST.0"), "5") && omr_host_var("ST.1") &&
               !strcmp(omr_host_var("ST.1"), "CAD") && omr_host_var("ST.1.KIND") &&
               !strcmp(omr_host_var("ST.1.KIND"), "cad") && omr_host_var("ST.1.APP") &&
               !strcmp(omr_host_var("ST.1.APP"), "GFOpenMake") && omr_host_var("ST.1.PORT") &&
               !strcmp(omr_host_var("ST.1.PORT"), "GFOPENMAKE") && omr_host_var("ST.2") &&
               !strcmp(omr_host_var("ST.2"), "Print") && omr_host_var("ST.5") &&
               !strcmp(omr_host_var("ST.5"), "Electronics") && omr_host_var("ST.5.KIND") &&
               !strcmp(omr_host_var("ST.5.KIND"), "electronics") && omr_host_var("ST.5.APP") &&
               !strcmp(omr_host_var("ST.5.APP"), "gfamigakicad") && omr_host_var("ST.5.PORT") &&
               !strcmp(omr_host_var("ST.5.PORT"), ""),
           "KINDS names the Add list");
    expect(run(&g, "PICK ITEM 1", &io) == 0 && io.rc == OMR_ERROR, "PICK with no project");
    expect(run(&g, "LOOK NAME side", &io) == 0 && io.rc == OMR_OK && g.look == 2 && !strcmp(buf, "Side."),
           "LOOK side");
    expect(run(&g, "LOOK NAME frame", &io) == 0 && io.rc == OMR_OK && g.look == 3 && !strcmp(buf, "Framed."),
           "LOOK frame");
    expect(run(&g, "LOOK NAME under", &io) == 0 && io.rc == OMR_ERROR && g.look == 3, "LOOK wants a view");

    cwd = getcwd(NULL, 0);
    expect(cwd != NULL && join2(dir, (int)sizeof dir, cwd ? cwd : ".", "/build/ws-rexx-proj") == 0, "the working directory");
    expect(join2(svg, (int)sizeof svg, cwd ? cwd : ".", "/core/tests/data/rect-10mm.svg") == 0, "the drawing path");
    expect(join2(kicad, (int)sizeof kicad, cwd ? cwd : ".", "/build/ws-rexx-board.kicad_pro") == 0, "the board path");
    mkdir("build", 0755);
    {
        FILE *f = fopen(kicad, "w");
        expect(f != NULL, "a board file");
        if (f) {
            fputs("(kicad_pro (version 20240101))\n", f);
            fclose(f);
        }
    }
    expect(join2(rm, (int)sizeof rm, "rm -rf ", dir) == 0 && system(rm) == 0, "clear the fixture drawer");

    expect(run(&g, "NEW PATH build/ws-rexx-proj NAME Widget", &io) == 0 && io.rc == OMR_OK && w.have &&
               strstr(buf, "ws-rexx-proj"),
           "NEW");
    expect(run(&g, "SET KEY name VALUE Shelf", &io) == 0 && io.rc == OMR_OK && !strcmp(w.project.name, "Shelf"),
           "SET name");
    expect(run(&g, "SET KEY notes VALUE \"A shelf bracket\"", &io) == 0 && io.rc == OMR_OK &&
               !strcmp(w.project.notes, "A shelf bracket"),
           "SET notes");
    expect(run(&g, "GET KEY notes", &io) == 0 && io.rc == OMR_OK && !strcmp(buf, "A shelf bracket"), "GET notes");
    expect(run(&g, "SET KEY power VALUE 1", &io) == 0 && io.rc == OMR_ERROR, "SET unknown key");
    expect(w.project.dirty, "SET dirties the project");
    expect(run(&g, "QUIT", &io) == 0 && io.rc == OMR_ERROR && !g.quit, "QUIT refuses a dirty project");
    expect(run(&g, "SAVE", &io) == 0 && io.rc == OMR_OK && !w.project.dirty, "SAVE");
    expect(run(&g, "QUIT", &io) == 0 && io.rc == OMR_OK && g.quit, "QUIT");
    g.quit = 0;

    expect(add_line(buf, (int)sizeof buf, "laser", svg, "Plate") == 0 && run(&g, buf, &io) == 0 && io.rc == OMR_OK &&
               w.project.n == 1 && strstr(io.result, "1 Plate"),
           "ADD laser");
    expect(add_line(buf, (int)sizeof buf, "electronics", kicad, "Board") == 0 && run(&g, buf, &io) == 0 && io.rc == OMR_OK &&
               w.project.n == 2,
           "ADD electronics");
    expect(run(&g, "ITEMS STEM IT", &io) == 0 && io.rc == OMR_OK && !strcmp(buf, "2") && omr_host_var("IT.1") &&
               !strcmp(omr_host_var("IT.1"), "Plate") && omr_host_var("IT.2.KIND") &&
               strstr(omr_host_var("IT.2.KIND"), "electronics"),
           "ITEMS");
    expect(run(&g, "ITEMS KIND laser", &io) == 0 && io.rc == OMR_OK && !strcmp(buf, "1"), "ITEMS laser");
    expect(run(&g, "RENAME ITEM Plate NAME Front", &io) == 0 && io.rc == OMR_OK && !strcmp(w.project.items[0].name, "Front"),
           "RENAME");
    expect(run(&g, "STATUS STEM ST", &io) == 0 && io.rc == OMR_OK && omr_host_var("ST.ITEMS") &&
               !strcmp(omr_host_var("ST.ITEMS"), "2") && omr_host_var("ST.DIRTY") && !strcmp(omr_host_var("ST.DIRTY"), "1") &&
               omr_host_var("ST.NAME") && !strcmp(omr_host_var("ST.NAME"), "Shelf") && omr_host_var("ST.NOTES") &&
               !strcmp(omr_host_var("ST.NOTES"), "A shelf bracket") && omr_host_var("ST.PICKED") &&
               !strcmp(omr_host_var("ST.PICKED"), "2") && omr_host_var("ST.ITEM") &&
               !strcmp(omr_host_var("ST.ITEM"), "Board") && omr_host_var("ST.KIND") &&
               !strcmp(omr_host_var("ST.KIND"), "electronics") && omr_host_var("ST.PATH") &&
               strstr(omr_host_var("ST.PATH"), "kicad_pro"),
           "STATUS ITEMS");
    expect(w.picked == 1 && g.home == 1, "ADD shows the new item, as the button");
    g.home = 0;
    expect(run(&g, "PICK ITEM 9", &io) == 0 && io.rc == OMR_ERROR && w.picked == 1 && strstr(buf, "no such"),
           "PICK missing");
    {
        int dirty = w.project.dirty;
        expect(run(&g, "PICK ITEM Front", &io) == 0 && io.rc == OMR_OK && w.picked == 0 && g.home == 1 &&
                   !strcmp(buf, "Front") && w.project.dirty == dirty,
               "PICK shows the row");
        expect(run(&g, "STATUS STEM ST", &io) == 0 && io.rc == OMR_OK && omr_host_var("ST.PICKED") &&
                   !strcmp(omr_host_var("ST.PICKED"), "1") && omr_host_var("ST.ITEM") &&
                   !strcmp(omr_host_var("ST.ITEM"), "Front"),
               "STATUS follows the pick");
        expect(run(&g, "PICK ITEM 2", &io) == 0 && io.rc == OMR_OK && w.picked == 1 && !strcmp(buf, "Board"),
               "PICK by number");
        w.picked = -1;
        g.home = 0;
    }

    expect(run(&g, "OPENITEM ITEM Front", &io) == 0 && io.rc == OMR_OK && strstr(buf, "OPENMAKEBURN"), "OPENITEM launches");
    expect(run(&g, "STATUS STEM ST", &io) == 0 && io.rc == OMR_OK && omr_host_var("ST.PICKED") &&
               !strcmp(omr_host_var("ST.PICKED"), "1") && omr_host_var("ST.ITEM") &&
               !strcmp(omr_host_var("ST.ITEM"), "Front") && omr_host_var("ST.KIND") &&
               !strcmp(omr_host_var("ST.KIND"), "laser") && omr_host_var("ST.FROM") &&
               !strcmp(omr_host_var("ST.FROM"), "") && omr_host_var("ST.PATH") &&
               strstr(omr_host_var("ST.PATH"), "rect-10mm.svg"),
           "STATUS names the item the window is showing");
    launch = workshop_launch_last(&w);
    expect(launch && strstr(launch, "GFOpenBurn") && strstr(launch, "rect-10mm.svg"), "the launch hook has the file");

    burn = omr_open("OPENMAKEBURN", "OpenMakeBurn 0.1.0");
    expect(burn != NULL, "a burn port");
    if (burn) omr_attach(burn, fake_verbs, 1, opened);
    opened[0] = 0;
    expect(run(&g, "OPENITEM ITEM 1", &io) == 0 && io.rc == OMR_OK && strstr(buf, "OPENMAKEBURN.1") &&
               strstr(opened, "rect-10mm.svg"),
           "OPENITEM sends OPEN when the port is up");
    {
        char used[64], why[160];
        opened[0] = 0;
        expect(gw_open_item(&w, 0, used, (int)sizeof used, why, (int)sizeof why) == 0 && !strcmp(used, "OPENMAKEBURN.1") &&
                   strstr(opened, "rect-10mm.svg"),
               "Launch uses the port when it is up");
    }
    expect(run(&g, "SEND APP BURN CMD VERSION", &io) == 0 && io.rc == OMR_OK && strstr(buf, "OpenMakeBurn"), "SEND");
    expect(run(&g, "SEND APP KICAD CMD VERSION", &io) == 0 && io.rc == OMR_ERROR, "SEND to an app with no port");
    if (burn) omr_close(burn);
    expect(run(&g, "SEND APP BURN CMD VERSION", &io) == 0 && io.rc == OMR_ERROR && strstr(buf, "not up"),
           "SEND when the port is down");

    expect(run(&g, "OPENITEM ITEM Board", &io) == 0 && io.rc == OMR_OK && strstr(buf, "gfamigakicad"),
           "OPENITEM of a board names the program");
    expect(run(&g, "REMOVE ITEM Board", &io) == 0 && io.rc == OMR_OK && w.project.n == 1, "REMOVE");
    expect(run(&g, "REMOVE ITEM 9", &io) == 0 && io.rc == OMR_ERROR, "REMOVE missing");

    gpr_recent_touch(&w.recent, w.project.path);
    expect(run(&g, "RECENT STEM RC", &io) == 0 && io.rc == OMR_OK && strstr(buf, "ws-rexx-proj") && omr_host_var("RC.0") &&
               !strcmp(omr_host_var("RC.0"), "1"),
           "RECENT");

    expect(run(&g, "SAVE", &io) == 0 && io.rc == OMR_OK && !w.project.dirty, "SAVE after edits");
    w.have = 0;
    g.home = 0;
    expect(run(&g, "OPEN FILE build/ws-rexx-proj", &io) == 0 && io.rc == OMR_OK && w.have && w.project.n == 1 &&
               !strcmp(w.project.name, "Shelf") && w.picked == 0 && g.home == 1,
           "OPEN the project again, and shows the first item");
    {
        FILE *f = fopen("build/ws-rexx-copy.svg", "w");
        char line[700];
        expect(f != NULL, "a second drawing");
        if (f) {
            fputs("<svg></svg>\n", f);
            fclose(f);
        }
        snprintf(line, sizeof line, "ADD KIND laser FILE %s/build/ws-rexx-copy.svg NAME Copy FROM 1", cwd ? cwd : ".");
        expect(run(&g, line, &io) == 0 && io.rc == OMR_OK && w.project.n == 2 && w.project.items[1].from == 1 &&
                   w.picked == 1 && g.home == 1,
               "ADD a copy from the first item");
    }
    w.picked = 1;
    expect(run(&g, "STATUS STEM ST", &io) == 0 && io.rc == OMR_OK && omr_host_var("ST.PICKED") &&
               !strcmp(omr_host_var("ST.PICKED"), "2") && omr_host_var("ST.ITEM") &&
               !strcmp(omr_host_var("ST.ITEM"), "Copy") && omr_host_var("ST.FROM") &&
               !strcmp(omr_host_var("ST.FROM"), "1") && omr_host_var("ST.PATH") &&
               strstr(omr_host_var("ST.PATH"), "ws-rexx-copy.svg"),
           "STATUS says the row was made from another");

    workshop_free(&w);
    free(cwd);
    if (fails) {
        printf("workshop rexx: %d failed\n", fails);
        return 1;
    }
    printf("workshop rexx ok\n");
    return 0;
}
