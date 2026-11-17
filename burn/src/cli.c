/* OpenMakeBurn, from the shell: an SVG or DXF to Grbl laser G-code.
 *   openmake-burn in.svg|in.dxf|in.pgm out.gcode [DRIVER=driver.om] [PROFILE=profile.om]
 *       [power=50] [speed=20] [passes=1] [constant=0] [s100=1000]
 *       [layer=#rrggbb,power=80,speed=10,passes=2] [mode=line] [interval=0.2]
 *       [cuts_last=1] [order=1,2] [material=materials/laser/card.om]
 * The machine is the generic 400 mm diode (machines/laser/grbl) unless
 * DRIVER or PROFILE say otherwise. Power is a percentage, speed is mm/s.
 * A layer= sets power, speed, passes or on for one colour (or a 1-based
 * index). mode=fill scans inside closed shapes, mode=offset follows them inwards, interval the gap in mm.
 * cuts_last=1 (the default) burns fills first, then lines, heavier last;
 * cuts_last=0 uses the file order, or order= of 1-based layers. MIT, Copyright (c) 2026 AmiCradle Team. */
#include "burn_job.h"
#include "burn_material.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *arg(int argc, char **argv, const char *name)
{
    size_t n = strlen(name);
    int i;
    for (i = 3; i < argc; i++)
        if (strncmp(argv[i], name, n) == 0 && argv[i][n] == '=') return argv[i] + n + 1;
    return NULL;
}

static const char *arg2(int argc, char **argv, const char *a, const char *b)
{
    const char *v = arg(argc, argv, a);
    return v ? v : arg(argc, argv, b);
}

static int known_key(const char *s)
{
    static const char *const keys[] = {"DRIVER", "PROFILE", "power", "POWER", "speed", "SPEED",
                                       "passes", "PASSES", "constant", "CONSTANT", "s100", "S100",
                                       "mode", "MODE", "interval", "INTERVAL", "layer", "LAYER",
                                       "cuts_last", "CUTS_LAST", "order", "ORDER", "material", "MATERIAL",
                                       "invert", "INVERT", "width", "WIDTH", "kerf", "KERF", "tabs", "TABS", "tabw", "TABW", "frame", "FRAME", "x", "X", "y", "Y", "rot", "ROT", NULL};
    int i;
    const char *eq = strchr(s, '=');
    size_t n;
    if (!eq) return 0;
    n = (size_t)(eq - s);
    for (i = 0; keys[i]; i++)
        if (strlen(keys[i]) == n && strncmp(s, keys[i], n) == 0) return 1;
    return 0;
}

static void usage(void)
{
    fprintf(stderr, "usage: openmake-burn in.svg|in.dxf|in.pgm out.gcode [DRIVER=driver.om] [PROFILE=profile.om]\n"
                    "       [power=50] [speed=20] [passes=1] [constant=0] [s100=1000]\n"
                    "       [layer=#rrggbb,power=80,speed=10,passes=2]\n"
                    "       [mode=line] [interval=0.2] [kerf=0] [tabs=0] [tabw=1] [frame=0] [x=0] [y=0] [rot=0]\n"
                    "       [cuts_last=1] [order=1,2]\n"
                    "       [material=materials/laser/card.om]\n"
                    "power is a percentage, speed is mm/s, constant=1 uses M3 instead of M4.\n"
                    "mode=fill scans inside closed shapes, mode=offset follows them inwards; interval is the gap in mm.\n"
                    "cuts_last=1 (the default) burns fills first, then lines, with the heavier lines last.\n"
                    "cuts_last=0 uses the file order, or order= of 1-based layers.\n"
                    "material= is a starting-point profile; power, speed and the other keys after it still win.\n"
                    "kerf= (mm) grows closed cuts by half the kerf and shrinks holes; 0 leaves them.\n"
                    "tabs= is how many holding tabs to leave on each closed cut, after the kerf; 0 leaves none.\n"
                    "tabw= is each tab's width in mm (default 1). Fill, offset, open paths and pictures are unchanged.\n"
                    "frame=1 writes a travel around the job's box with the laser off, never a cut.\n"
                    "x= and y= (mm) move the drawing on the bed; rot= is degrees CCW around its box centre.\n"
                    "A Netpbm PBM, PGM or PPM is engraved as scan lines (black is more power).\n"
                    "width= is the picture width in mm; invert=1 burns white.\n"
                    "layer= sets one colour (or a 1-based index). on=0 leaves that layer out.\n"
                    "The machine is Generic Diode 400 unless DRIVER or PROFILE say otherwise.\n");
}

int main(int argc, char **argv)
{
    const char *driver = arg(argc, argv, "DRIVER"), *profile = arg(argc, argv, "PROFILE");
    const char *v;
    burn_opts o;
    burn_layer layers[32];
    int nlayer = 0, seq[32], nseq = 0;
    om_machine m;
    gc_stats st;
    char err[160];
    int i;
    if (argc < 3) {
        usage();
        return 1;
    }
    for (i = 3; i < argc; i++) {
        if (!strchr(argv[i], '=')) {
            fprintf(stderr, "openmake-burn: %s: not key=value\n", argv[i]);
            return 1;
        }
        if (!known_key(argv[i])) {
            fprintf(stderr, "openmake-burn: %s: no such setting\n", argv[i]);
            return 1;
        }
    }
    v = arg2(argc, argv, "mode", "MODE");
    if (v) {
        int m = burn_mode_parse(v);
        if (m < 0) {
            fprintf(stderr, "openmake-burn: mode=%s: line, fill or offset\n", v);
            return 1;
        }
        /* stored after burn_defaults */
    }
    if (!driver) driver = "machines/laser/grbl/driver.om";
    if (!profile && !arg(argc, argv, "DRIVER"))
        profile = "machines/laser/grbl/profiles/generic-diode-400.om";
    burn_defaults(&o);
    if ((v = arg2(argc, argv, "material", "MATERIAL")) != NULL) {
        burn_material mat;
        if (burn_material_load(&mat, v, err, (int)sizeof err)) {
            fprintf(stderr, "openmake-burn: %s\n", err);
            return 1;
        }
        burn_material_apply(&mat, &o);
    }
    if ((v = arg2(argc, argv, "power", "POWER")) != NULL) o.power = (float)atof(v);
    if ((v = arg2(argc, argv, "speed", "SPEED")) != NULL) o.speed = (float)atof(v);
    if ((v = arg2(argc, argv, "passes", "PASSES")) != NULL) o.passes = atoi(v);
    if ((v = arg2(argc, argv, "constant", "CONSTANT")) != NULL) o.constant = atoi(v);
    if ((v = arg2(argc, argv, "s100", "S100")) != NULL) o.s100 = (float)atof(v);
    if ((v = arg2(argc, argv, "mode", "MODE")) != NULL) o.mode = burn_mode_parse(v);
    if ((v = arg2(argc, argv, "interval", "INTERVAL")) != NULL) o.interval = (float)atof(v);
    if ((v = arg2(argc, argv, "kerf", "KERF")) != NULL) o.kerf = (float)atof(v);
    if ((v = arg2(argc, argv, "tabs", "TABS")) != NULL) o.tabs = atoi(v);
    if ((v = arg2(argc, argv, "tabw", "TABW")) != NULL) o.tabw = (float)atof(v);
    if ((v = arg2(argc, argv, "invert", "INVERT")) != NULL) o.invert = atoi(v) ? 1 : 0;
    if ((v = arg2(argc, argv, "width", "WIDTH")) != NULL) o.width = (float)atof(v);
    if ((v = arg2(argc, argv, "cuts_last", "CUTS_LAST")) != NULL) o.cuts_last = atoi(v) ? 1 : 0;
    if ((v = arg2(argc, argv, "x", "X")) != NULL) o.x = (float)atof(v);
    if ((v = arg2(argc, argv, "y", "Y")) != NULL) o.y = (float)atof(v);
    if ((v = arg2(argc, argv, "rot", "ROT")) != NULL) o.rot = (float)atof(v);
    if (o.passes < 1) o.passes = 1;
    if (o.tabs < 0) {
        fprintf(stderr, "openmake-burn: tabs is a whole number, 0 to 32\n");
        return 1;
    }
    if (o.tabs > 32) o.tabs = 32;
    if (o.tabw < 0.f) {
        fprintf(stderr, "openmake-burn: tabw is a width in mm, 0 or more\n");
        return 1;
    }
    for (i = 3; i < argc; i++) {
        const char *eq = strchr(argv[i], '=');
        size_t kn = eq ? (size_t)(eq - argv[i]) : 0;
        if (kn == 5 && (strncmp(argv[i], "layer", 5) == 0 || strncmp(argv[i], "LAYER", 5) == 0)) {
            if (nlayer >= 32) {
                fprintf(stderr, "openmake-burn: too many layer= settings\n");
                return 1;
            }
            if (burn_parse_layer(eq + 1, &layers[nlayer], err, (int)sizeof err)) {
                fprintf(stderr, "openmake-burn: %s\n", err);
                return 1;
            }
            nlayer++;
        }
    }
    if (nlayer) {
        o.layer = layers;
        o.nlayer = nlayer;
    }
    if ((v = arg2(argc, argv, "order", "ORDER")) != NULL) {
        const char *s = v;
        while (*s) {
            int n = 0;
            if (*s == ',') s++;
            if (*s < '0' || *s > '9') {
                fprintf(stderr, "openmake-burn: order=%s: 1-based layers, comma separated\n", v);
                return 1;
            }
            while (*s >= '0' && *s <= '9') n = n * 10 + (*s++ - '0');
            if (n < 1 || nseq >= 32) {
                fprintf(stderr, "openmake-burn: order=%s: 1-based layers, comma separated\n", v);
                return 1;
            }
            seq[nseq++] = n - 1;
            if (*s && *s != ',') {
                fprintf(stderr, "openmake-burn: order=%s: 1-based layers, comma separated\n", v);
                return 1;
            }
        }
        o.seq = seq;
        o.nseq = nseq;
    }
    om_clear(&m);
    if (om_load(&m, driver, err, (int)sizeof err) || (profile && om_load(&m, profile, err, (int)sizeof err))) {
        fprintf(stderr, "openmake-burn: %s\n", err);
        return 1;
    }
    {
        int frame = 0;
        if ((v = arg2(argc, argv, "frame", "FRAME")) != NULL) frame = atoi(v) ? 1 : 0;
        memset(&st, 0, sizeof st);
        if ((frame ? burn_write_frame : burn_write)(argv[1], argv[2], &m, &o, &st, err, (int)sizeof err)) {
            fprintf(stderr, "openmake-burn: %s\n", err);
            return 1;
        }
    }
    printf("openmake-burn: wrote %s: %d moves for %s", argv[2], st.moves, m.name);
    if (st.seconds > 0.f && st.seconds < 60.f)
        printf(", about %.1f s", (double)st.seconds);
    else if (st.seconds >= 60.f) {
        int s = (int)(st.seconds + 0.5f);
        printf(", about %d min %d s", s / 60, s % 60);
    }
    printf("\n");
    return 0;
}
