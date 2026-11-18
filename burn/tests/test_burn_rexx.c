/* OpenMakeBurn's ARexx commands, run on the host through omr_do_line.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "burn_rexx.h"

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

static int run(gb_rexx *g, const char *line, omr_io *io)
{
    return omr_do_line(line, gb_rexx_verbs(), gb_rexx_nverbs(), g, io);
}

static int load_laser(om_machine *m, char *err, int errlen)
{
    om_clear(m);
    return om_load(m, "machines/laser/grbl/driver.om", err, errlen) ||
           om_load(m, "machines/laser/grbl/profiles/generic-diode-400.om", err, errlen);
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
    gb_rexx g;
    gv_doc doc;
    burn_image pic;
    gt_path path;
    burn_opts opts;
    burn_layer store[16];
    int have_doc = 0, have_image = 0, have_path = 0;
    omr_io io;
    char buf[256], err[160];

    gv_doc_init(&doc);
    burn_image_init(&pic);
    gt_init(&path);
    burn_defaults(&opts);
    memset(store, 0, sizeof store);
    opts.layer = store;
    opts.nlayer = 0;

    gb_rexx_init(&g);
    g.doc = &doc;
    g.pic = &pic;
    g.have_doc = &have_doc;
    g.have_image = &have_image;
    g.path = &path;
    g.have_path = &have_path;
    g.opts = &opts;
    g.store = store;
    g.store_cap = (int)(sizeof store / sizeof store[0]);
    g.port_name = "OPENMAKEBURN.1";
    g.version = "OpenMakeBurn 0.1.0";
    expect(load_laser(&g.machine, err, (int)sizeof err) == 0, "Generic Diode 400");
    g.have_machine = 1;

    io_init(&io, buf, (int)sizeof buf);
    expect(run(&g, "VERSION", &io) == 0 && io.rc == OMR_OK && strstr(buf, "OpenMakeBurn"), "VERSION");
    expect(run(&g, "HELP", &io) == 0 && io.rc == OMR_OK && strstr(buf, "LAYER") && strstr(buf, "OPEN"),
           "HELP lists LAYER");
    expect(run(&g, "STATUS", &io) == 0 && io.rc == OMR_OK && strstr(buf, "OPENMAKEBURN.1") && strstr(buf, "0"),
           "STATUS, not dirty");
    expect(run(&g, "GET KEY power", &io) == 0 && io.rc == OMR_OK && strcmp(buf, "50") == 0, "GET power");
    expect(run(&g, "SET KEY power VALUE 80", &io) == 0 && io.rc == OMR_OK && strstr(buf, "80"), "SET power");
    expect(run(&g, "GET KEY power", &io) == 0 && io.rc == OMR_OK && strcmp(buf, "80") == 0, "GET after SET");
    expect(run(&g, "SET KEY mode VALUE fill", &io) == 0 && io.rc == OMR_OK && strstr(buf, "fill"), "SET mode fill");
    expect(run(&g, "SET KEY mode VALUE line", &io) == 0 && io.rc == OMR_OK, "SET mode line");
    expect(run(&g, "SET KEY no-such VALUE 1", &io) == 0 && io.rc == OMR_ERROR, "SET unknown key");
    expect(run(&g, "LASER POWER 40 SPEED 15 CONSTANT 1", &io) == 0 && io.rc == OMR_OK && opts.power == 40.f &&
               opts.speed == 15.f && opts.constant == 1,
           "LASER");
    expect(run(&g, "SET KEY constant VALUE 0", &io) == 0 && io.rc == OMR_OK && opts.constant == 0, "SET constant 0");

    expect(run(&g, "OPEN FILE core/tests/data/rect-10mm.svg", &io) == 0 && io.rc == OMR_OK && have_doc &&
               g.dirty == 0 && doc.n >= 1,
           "OPEN svg");
    expect(run(&g, "STATUS STEM ST", &io) == 0 && io.rc == OMR_OK && strstr(buf, "rect-10mm.svg") &&
               omr_host_var("ST.KIND") && strstr(omr_host_var("ST.KIND"), "SVG") && omr_host_var("ST.LAYERS") &&
               atoi(omr_host_var("ST.LAYERS")) >= 1,
           "STATUS KIND SVG");
    expect(run(&g, "MATERIAL FILE materials/laser/card.om", &io) == 0 && io.rc == OMR_OK && g.dirty == 1, "MATERIAL");
    expect(run(&g, "MODE NAME line", &io) == 0 && io.rc == OMR_OK, "MODE line");
    expect(run(&g, "LAYER INDEX 1 POWER 90", &io) == 0 && io.rc == OMR_OK && opts.nlayer >= 1 && store[0].power == 90.f,
           "LAYER INDEX 1");
    expect(run(&g, "SET KEY power VALUE 70", &io) == 0 && io.rc == OMR_OK && opts.power == 70.f && store[0].power == 70.f,
           "SET power writes the layer row");
    {
        burn_layer row = store[0];
        int n = opts.nlayer;
        burn_defaults(&opts);
        opts.layer = store;
        opts.nlayer = n;
        store[0] = row;
        burn_job_from_layers(&opts);
        expect(opts.power == 70.f, "a rebuild keeps the port's power from the layer row");
    }
    expect(run(&g, "LAYER INDEX 9 POWER 10", &io) == 0 && io.rc == OMR_ERROR, "LAYER missing");

    expect(run(&g, "PREVIEW", &io) == 0 && io.rc == OMR_OK && have_path && path.n >= 2, "PREVIEW");
    expect(run(&g, "PREVIEW FILE build/rexx-burn.ppm", &io) == 0 && io.rc == OMR_OK, "PREVIEW FILE");
    {
        FILE *f = fopen("build/rexx-burn.ppm", "rb");
        char mag[3] = {0};
        int ok = 0;
        if (f && fread(mag, 1, 2, f) == 2 && mag[0] == 'P' && mag[1] == '6') ok = 1;
        if (f) fclose(f);
        expect(ok, "PREVIEW wrote a PPM");
    }

    expect(run(&g, "SET KEY start VALUE \"M117 hello\"", &io) == 0 && io.rc == OMR_OK && opts.start &&
               !strcmp(opts.start, "M117 hello"),
           "SET start");
    expect(run(&g, "GET KEY start", &io) == 0 && io.rc == OMR_OK && !strcmp(buf, "M117 hello"), "GET start");
    expect(run(&g, "SET KEY end VALUE \"M300 S10\"", &io) == 0 && io.rc == OMR_OK && opts.end &&
               !strcmp(opts.end, "M300 S10"),
           "SET end");
    expect(run(&g, "SAVE FILE build/rexx-burn.gcode", &io) == 0 && io.rc == OMR_OK && g.dirty == 0, "SAVE");
    expect(gcode_has_move("build/rexx-burn.gcode") && file_has("build/rexx-burn.gcode", "M117 hello") &&
               file_has("build/rexx-burn.gcode", "M300 S10"),
           "SAVE wrote the cut and the user's lines");
    expect(run(&g, "FRAME FILE build/rexx-frame.gcode", &io) == 0 && io.rc == OMR_OK, "FRAME");
    expect(gcode_has_move("build/rexx-frame.gcode"), "FRAME wrote moves");
    expect(run(&g, "SET KEY x VALUE 5", &io) == 0 && io.rc == OMR_OK && opts.x == 5.f, "SET x");
    expect(run(&g, "SET KEY y VALUE 5", &io) == 0 && io.rc == OMR_OK && opts.y == 5.f, "SET y");
    expect(run(&g, "SET KEY rot VALUE 90", &io) == 0 && io.rc == OMR_OK && opts.rot == 90.f, "SET rot");
    expect(run(&g, "SET KEY x VALUE no", &io) == 0 && io.rc == OMR_ERROR, "x wants a number");
    expect(run(&g, "SET KEY x VALUE 20000", &io) == 0 && io.rc == OMR_OK && opts.x == 10000.f, "x is held to 10000");
    expect(run(&g, "SET KEY x VALUE 5", &io) == 0 && io.rc == OMR_OK, "x back to 5");
    expect(run(&g, "SET KEY rot VALUE 0", &io) == 0 && io.rc == OMR_OK, "rot back to 0");
    expect(run(&g, "SAVE FILE build/rexx-place.gcode", &io) == 0 && io.rc == OMR_OK, "SAVE the moved square");
    expect(file_has("build/rexx-place.gcode", "X5") && file_has("build/rexx-place.gcode", "X15") &&
               file_has("build/rexx-place.gcode", "Y5") && file_has("build/rexx-place.gcode", "Y15"),
           "the port's X and Y move the cut");

    expect(run(&g, "OPEN FILE burn/tests/data/two-rects.svg", &io) == 0 && io.rc == OMR_OK && doc.nlayer >= 2,
           "OPEN two-rects");
    expect(run(&g, "LAYER COLOUR #ff0000 POWER 70", &io) == 0 && io.rc == OMR_OK, "LAYER by colour");
    expect(run(&g, "OPEN FILE burn/tests/data/bars-8x2.pgm", &io) == 0 && io.rc == OMR_OK && have_image && have_doc,
           "OPEN picture");
    expect(run(&g, "STATUS STEM ST", &io) == 0 && io.rc == OMR_OK && omr_host_var("ST.KIND") &&
               strstr(omr_host_var("ST.KIND"), "Picture"),
           "STATUS KIND Picture");
    expect(run(&g, "SET KEY invert VALUE 1", &io) == 0 && io.rc == OMR_OK && opts.invert == 1 && g.invert == 1,
           "SET invert");
    expect(run(&g, "SET KEY invert VALUE 2", &io) == 0 && io.rc == OMR_ERROR, "invert is 0 or 1");
    expect(opts.invert == 1, "a refused invert left 1 in place");
    expect(run(&g, "SET KEY width VALUE 16", &io) == 0 && io.rc == OMR_OK && opts.width == 16.f && g.width == 16.f,
           "SET width 16 mm");
    expect(run(&g, "SET KEY width VALUE -1", &io) == 0 && io.rc == OMR_ERROR, "a negative width is refused");
    burn_defaults(&opts);
    gb_rexx_keep(&g);
    expect(opts.invert == 1 && opts.width == 16.f && opts.start && !strcmp(opts.start, "M117 hello"),
           "the port keeps invert, width and the lines after the window rebuilds the job");
    expect(run(&g, "SAVE FILE build/rexx-bars.gcode", &io) == 0 && io.rc == OMR_OK, "SAVE the picture");
    expect(file_has("build/rexx-bars.gcode", "X16"), "invert and a 16 mm width reach the far edge");

    expect(run(&g, "OPEN FILE no-such.svg", &io) == 0 && io.rc == OMR_ERROR, "OPEN missing file");
    expect(run(&g, "SET KEY travel VALUE 200", &io) == 0 && io.rc == OMR_OK && opts.travel == 200.f && g.have_travel,
           "SET travel");
    expect(run(&g, "SET KEY s100 VALUE 500", &io) == 0 && io.rc == OMR_OK && opts.s100 == 500.f && g.have_s100,
           "SET s100");
    burn_defaults(&opts);
    opts.layer = store;
    gb_rexx_keep(&g);
    expect(opts.travel == 200.f && opts.s100 == 500.f && opts.start && !strcmp(opts.start, "M117 hello"),
           "a rebuild keeps travel, S at 100%, and the start line");
    g.dirty = 1;
    expect(run(&g, "QUIT", &io) == 0 && io.rc == OMR_ERROR && g.quit == 0, "QUIT refuses a dirty document");
    expect(run(&g, "QUIT FORCE", &io) == 0 && io.rc == OMR_OK && g.quit == 1, "QUIT FORCE");
    expect(run(&g, "ACTIVATE", &io) == 0 && io.rc == OMR_OK, "ACTIVATE with no window");
    expect(run(&g, "HIDE", &io) == 0 && io.rc == OMR_OK, "HIDE with no window");

    burn_image_free(&pic);
    gv_doc_free(&doc);
    gt_free(&path);
    printf(fails ? "burn rexx: %d failed\n" : "burn rexx ok\n", fails);
    return fails != 0;
}
