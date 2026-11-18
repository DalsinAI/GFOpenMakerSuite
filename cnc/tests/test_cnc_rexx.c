/* GFOpenCNC's ARexx commands, run on the host through omr_do_line.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "cnc_rexx.h"

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

static int run(oc_rexx *g, const char *line, omr_io *io)
{
    return omr_do_line(line, oc_rexx_verbs(), oc_rexx_nverbs(), g, io);
}

static int gcode_has(const char *path, const char *what)
{
    FILE *f = fopen(path, "r");
    char line[120];
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

int main(void)
{
    oc_rexx g;
    gv_doc doc;
    gt_path path;
    cnc_opts opts;
    omr_io io;
    char buf[256];
    int have_doc = 0, have_path = 0;

    gv_doc_init(&doc);
    gt_init(&path);
    cnc_defaults(&opts);
    oc_rexx_init(&g);
    g.doc = &doc;
    g.have_doc = &have_doc;
    g.path = &path;
    g.have_path = &have_path;
    g.opts = &opts;
    g.version = "GFOpenCNC 0.1.0";
    g.port_name = "GFOPENCNC.1";
    io_init(&io, buf, (int)sizeof buf);
    expect(run(&g, "VERSION", &io) == 0 && io.rc == OMR_OK && strstr(buf, "GFOpenCNC 0.1.0"), "VERSION");
    expect(run(&g, "ACTIVATE", &io) == 0 && io.rc == OMR_OK, "ACTIVATE with no window");
    expect(run(&g, "HIDE", &io) == 0 && io.rc == OMR_OK, "HIDE with no window");
    expect(run(&g, "OPEN", &io) == 0 && io.rc == OMR_ERROR, "OPEN with no FILE is refused");
    expect(run(&g, "PREVIEW", &io) == 0 && io.rc == OMR_ERROR && strstr(buf, "drawing"),
           "PREVIEW with no drawing is refused");
    expect(run(&g, "OPEN FILE work:missing.svg", &io) == 0 && io.rc == OMR_ERROR && buf[0],
           "a missing drawing says why");
    expect(have_doc == 0, "a failed open leaves the document closed");
    expect(run(&g, "OPEN FILE core/tests/data/rect-10mm.svg", &io) == 0 && io.rc == OMR_OK &&
               strstr(buf, "rect-10mm.svg") && have_doc == 1,
           "OPEN loads the square");
    expect(run(&g, "GET KEY tool", &io) == 0 && io.rc == OMR_OK && !strcmp(buf, "0"), "tool starts at 0");
    expect(run(&g, "SET KEY tool VALUE 2", &io) == 0 && io.rc == OMR_OK && !strcmp(buf, "2") && g.dirty,
           "SET tool 2");
    expect(run(&g, "SET KEY tool VALUE 100", &io) == 0 && io.rc == OMR_OK && !strcmp(buf, "40"),
           "tool is held to 40");
    expect(run(&g, "SET KEY depth VALUE 0", &io) == 0 && io.rc == OMR_OK && !strcmp(buf, "0.1"),
           "depth is held to 0.1");
    expect(run(&g, "SET KEY mode VALUE Pocket", &io) == 0 && io.rc == OMR_OK && !strcmp(buf, "pocket"),
           "mode pocket, any case");
    expect(run(&g, "SET KEY mode VALUE fil", &io) == 0 && io.rc == OMR_ERROR && strstr(buf, "profile"),
           "a bad mode is refused");
    expect(opts.mode == CNC_POCKET, "the refused mode left pocket in place");
    expect(run(&g, "SET KEY feed VALUE fast", &io) == 0 && io.rc == OMR_ERROR, "a word is not a feed");
    expect(run(&g, "GET KEY no-such", &io) == 0 && io.rc == OMR_ERROR, "an unknown key is refused");
    expect(run(&g, "GET STEM ST", &io) == 0 && io.rc == OMR_OK && !strcmp(buf, "13") && omr_host_var("ST.0") &&
               !strcmp(omr_host_var("ST.0"), "13") && omr_host_var("ST.1") && !strcmp(omr_host_var("ST.1"), "tool") &&
               omr_host_var("ST.13") && !strcmp(omr_host_var("ST.13"), "end"),
           "GET with no key lists the settings");
    expect(run(&g, "STATUS STEM ST", &io) == 0 && io.rc == OMR_OK && strstr(buf, "GFOPENCNC.1") &&
               strstr(buf, "rect-10mm.svg") && strstr(buf, "pocket") && omr_host_var("ST.DIRTY") &&
               !strcmp(omr_host_var("ST.DIRTY"), "1") && omr_host_var("ST.MODE") &&
               !strcmp(omr_host_var("ST.MODE"), "pocket"),
           "STATUS is the port, the file, dirty, and the mode");
    expect(run(&g, "SET KEY tool VALUE 3", &io) == 0 && io.rc == OMR_OK && !strcmp(buf, "3"), "tool 3 for a preview");
    expect(run(&g, "SET KEY mode VALUE profile", &io) == 0 && io.rc == OMR_OK, "profile for a preview");
    expect(run(&g, "PREVIEW", &io) == 0 && io.rc == OMR_OK && have_path && path.n >= 2, "PREVIEW plans the cut");
    expect(run(&g, "PREVIEW FILE build/cnc-rexx.ppm", &io) == 0 && io.rc == OMR_OK && strstr(buf, "cnc-rexx.ppm"),
           "PREVIEW FILE");
    {
        FILE *f = fopen("build/cnc-rexx.ppm", "rb");
        char mag[3] = {0};
        int ok = f && fread(mag, 1, 2, f) == 2 && mag[0] == 'P' && mag[1] == '6';
        if (f) fclose(f);
        expect(ok, "PREVIEW wrote a PPM");
    }
    expect(run(&g, "QUIT", &io) == 0 && io.rc == OMR_ERROR && !g.quit, "QUIT refuses a dirty document");
    expect(run(&g, "SAVE FILE build/cnc-rexx-none.nc", &io) == 0 && io.rc == OMR_ERROR && strstr(buf, "no mill"),
           "SAVE with no mill is refused");
    expect(run(&g, "MILL", &io) == 0 && io.rc == OMR_ERROR, "MILL needs a file");
    expect(run(&g, "MILL DRIVER machines/printer/marlin/driver.om PROFILE machines/printer/marlin/profiles/generic-220.om",
               &io) == 0 &&
               io.rc == OMR_ERROR && !g.have_machine,
           "MILL refuses a printer");
    expect(run(&g, "MILL DRIVER machines/cnc/grbl/driver.om PROFILE machines/cnc/grbl/profiles/generic-300.om", &io) == 0 &&
               io.rc == OMR_OK && g.have_machine && g.machine.family == OM_CNC && buf[0],
           "MILL the generic mill");
    {
        opts.mode = CNC_PROFILE;
        opts.tool = 0.f;
        g.dirty = 1;
        expect(run(&g, "SAVE FILE build/cnc-rexx.nc", &io) == 0 && io.rc == OMR_OK && !g.dirty &&
                   gcode_has("build/cnc-rexx.nc", "G1"),
               "SAVE writes a cut");
        expect(run(&g, "SET KEY start VALUE \"M117 hello\"", &io) == 0 && io.rc == OMR_OK && strstr(buf, "M117 hello"),
               "SET start");
        expect(run(&g, "SET KEY end VALUE \"M300 S10\"", &io) == 0 && io.rc == OMR_OK && strstr(buf, "M300"),
               "SET end");
        expect(run(&g, "GET KEY start", &io) == 0 && io.rc == OMR_OK && strstr(buf, "M117 hello"), "GET start");
        g.dirty = 1;
        expect(run(&g, "SAVE FILE build/cnc-rexx-lines.nc", &io) == 0 && io.rc == OMR_OK, "SAVE with the user's lines");
        {
            FILE *f = fopen("build/cnc-rexx-lines.nc", "r");
            char text[8000];
            char *hello, *spindle, *beep, *home;
            size_t n = f ? fread(text, 1, sizeof text - 1, f) : 0;
            if (f) fclose(f);
            text[n] = 0;
            hello = strstr(text, "M117 hello");
            spindle = strstr(text, "M3 S");
            beep = strstr(text, "M300 S10");
            home = strstr(text, "G0 X0 Y0");
            expect(hello && spindle && beep && home && hello < spindle && beep < home,
                   "the port's lines sit after the machine's start and before its end");
        }
    }
    expect(run(&g, "QUIT", &io) == 0 && io.rc == OMR_OK && g.quit, "QUIT after a save");
    g.quit = 0;
    g.dirty = 1;
    expect(run(&g, "QUIT FORCE", &io) == 0 && io.rc == OMR_OK && g.quit, "QUIT FORCE discards a dirty document");
    expect(run(&g, "HELP", &io) == 0 && io.rc == OMR_OK && strstr(buf, "OPEN") && strstr(buf, "SAVE") &&
               strstr(buf, "PREVIEW"),
           "HELP lists the commands");
    gv_doc_free(&doc);
    gt_free(&path);
    if (fails) {
        printf("%d failed\n", fails);
        return 1;
    }
    printf("cnc rexx ok\n");
    return 0;
}
