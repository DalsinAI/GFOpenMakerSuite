/* OpenMakeCNC, from the shell: a drawing to Grbl mill G-code.
 *   openmake-cnc in.svg|in.dxf out.gcode [DRIVER=] [PROFILE=]
 *       [mode=profile|pocket|drill] [tool=0] [depth=1] [stepdown=]
 *       [stepover=] [feed=8] [plunge=3] [safe=5] [rpm=10000] [x=0] [y=0]
 * The machine is the generic 300 mm Grbl mill unless DRIVER or PROFILE
 * say otherwise. Lengths are millimetres, feed and plunge are mm/s.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "cnc_job.h"

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

static int known_key(const char *s)
{
    static const char *const keys[] = {"DRIVER", "PROFILE", "mode", "MODE", "tool", "TOOL", "depth", "DEPTH",
                                       "stepdown", "STEPDOWN", "stepover", "STEPOVER", "feed", "FEED", "plunge",
                                       "PLUNGE", "safe", "SAFE", "rpm", "RPM", "x", "X", "y", "Y", NULL};
    int i;
    const char *eq = strchr(s, '=');
    size_t n;
    if (!eq) return 0;
    n = (size_t)(eq - s);
    for (i = 0; keys[i]; i++)
        if (strlen(keys[i]) == n && strncmp(s, keys[i], n) == 0) return 1;
    return 0;
}

static int num(const char *s, float *v)
{
    char *end;
    if (!s || !*s) return -1;
    *v = strtof(s, &end);
    return end != s && !*end ? 0 : -1;
}

static void usage(void)
{
    fprintf(stderr, "usage: openmake-cnc in.svg|in.dxf out.gcode [DRIVER=driver.om] [PROFILE=profile.om]\n"
                    "       [mode=profile|pocket|drill] [tool=0] [depth=1] [stepdown=1] [stepover=]\n"
                    "       [feed=8] [plunge=3] [safe=5] [rpm=10000] [x=0] [y=0]\n"
                    "Profile follows the drawn edge, the tool outside it by its radius.\n"
                    "Pocket clears the inside. Drill plunges at each shape's centre.\n"
                    "Lengths are mm. Feed and plunge are mm/s. The machine is Generic Grbl Mill 300\n"
                    "unless DRIVER or PROFILE say otherwise.\n");
}

int main(int argc, char **argv)
{
    const char *driver, *profile, *v;
    cnc_opts o;
    om_machine m;
    gc_stats st;
    char err[200];
    int i;
    if (argc < 3) {
        usage();
        return 1;
    }
    for (i = 3; i < argc; i++)
        if (!known_key(argv[i])) {
            fprintf(stderr, "%s: no such setting\n", argv[i]);
            return 1;
        }
    driver = arg(argc, argv, "DRIVER");
    profile = arg(argc, argv, "PROFILE");
    if (!driver) driver = "machines/cnc/grbl/driver.om";
    if (!profile) profile = "machines/cnc/grbl/profiles/generic-300.om";
    cnc_defaults(&o);
    om_clear(&m);
    err[0] = 0;
    if (om_load(&m, driver, err, (int)sizeof err) || om_load(&m, profile, err, (int)sizeof err)) {
        fprintf(stderr, "%s\n", err[0] ? err : "Could not read that machine.");
        return 1;
    }
    v = arg(argc, argv, "mode");
    if (!v) v = arg(argc, argv, "MODE");
    if (v) {
        int mode = cnc_mode_parse(v);
        if (mode < 0) {
            fprintf(stderr, "mode=%s: not profile, pocket or drill\n", v);
            return 1;
        }
        o.mode = mode;
    }
    v = arg(argc, argv, "tool");
    if (!v) v = arg(argc, argv, "TOOL");
    if (v && num(v, &o.tool)) {
        fprintf(stderr, "tool=%s: not a number\n", v);
        return 1;
    }
    v = arg(argc, argv, "depth");
    if (!v) v = arg(argc, argv, "DEPTH");
    if (v && num(v, &o.depth)) {
        fprintf(stderr, "depth=%s: not a number\n", v);
        return 1;
    }
    v = arg(argc, argv, "stepdown");
    if (!v) v = arg(argc, argv, "STEPDOWN");
    if (v && num(v, &o.stepdown)) {
        fprintf(stderr, "stepdown=%s: not a number\n", v);
        return 1;
    }
    v = arg(argc, argv, "stepover");
    if (!v) v = arg(argc, argv, "STEPOVER");
    if (v && num(v, &o.stepover)) {
        fprintf(stderr, "stepover=%s: not a number\n", v);
        return 1;
    }
    v = arg(argc, argv, "feed");
    if (!v) v = arg(argc, argv, "FEED");
    if (v && num(v, &o.feed)) {
        fprintf(stderr, "feed=%s: not a number\n", v);
        return 1;
    }
    v = arg(argc, argv, "plunge");
    if (!v) v = arg(argc, argv, "PLUNGE");
    if (v && num(v, &o.plunge)) {
        fprintf(stderr, "plunge=%s: not a number\n", v);
        return 1;
    }
    v = arg(argc, argv, "safe");
    if (!v) v = arg(argc, argv, "SAFE");
    if (v && num(v, &o.safe)) {
        fprintf(stderr, "safe=%s: not a number\n", v);
        return 1;
    }
    v = arg(argc, argv, "rpm");
    if (!v) v = arg(argc, argv, "RPM");
    if (v && num(v, &o.rpm)) {
        fprintf(stderr, "rpm=%s: not a number\n", v);
        return 1;
    }
    v = arg(argc, argv, "x");
    if (!v) v = arg(argc, argv, "X");
    if (v && num(v, &o.x)) {
        fprintf(stderr, "x=%s: not a number\n", v);
        return 1;
    }
    v = arg(argc, argv, "y");
    if (!v) v = arg(argc, argv, "Y");
    if (v && num(v, &o.y)) {
        fprintf(stderr, "y=%s: not a number\n", v);
        return 1;
    }
    err[0] = 0;
    if (cnc_write(argv[1], argv[2], &m, &o, &st, err, (int)sizeof err)) {
        fprintf(stderr, "%s\n", err[0] ? err : "Could not write that job.");
        return 1;
    }
    printf("wrote %s (%s, %.0f moves, %.0f s) for %s\n", argv[2], cnc_mode_name(o.mode), (double)st.moves,
           (double)st.seconds, m.name);
    return 0;
}
