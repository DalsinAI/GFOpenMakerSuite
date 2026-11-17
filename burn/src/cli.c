/* OpenMakeBurn, from the shell: an SVG to Grbl laser G-code.
 *   openmake-burn in.svg out.gcode [DRIVER=driver.om] [PROFILE=profile.om]
 *       [power=50] [speed=20] [passes=1] [constant=0] [s100=1000]
 *       [layer=#rrggbb,power=80,speed=10,passes=2]
 * The machine is the generic 400 mm diode (machines/laser/grbl) unless
 * DRIVER or PROFILE say otherwise. Power is a percentage, speed is mm/s.
 * A layer= sets power, speed, passes or on for one colour (or a 1-based
 * index). Line mode only in this cut. MIT, Copyright (c) 2026 AmiCradle Team. */
#include "burn_job.h"

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
                                       "mode", "MODE", "layer", "LAYER", NULL};
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
    fprintf(stderr, "usage: openmake-burn in.svg out.gcode [DRIVER=driver.om] [PROFILE=profile.om]\n"
                    "       [power=50] [speed=20] [passes=1] [constant=0] [s100=1000]\n"
                    "       [layer=#rrggbb,power=80,speed=10,passes=2]\n"
                    "power is a percentage, speed is mm/s, constant=1 uses M3 instead of M4.\n"
                    "layer= sets one colour (or a 1-based index). on=0 leaves that layer out.\n"
                    "The machine is Generic Diode 400 unless DRIVER or PROFILE say otherwise.\n");
}

int main(int argc, char **argv)
{
    const char *driver = arg(argc, argv, "DRIVER"), *profile = arg(argc, argv, "PROFILE");
    const char *v;
    burn_opts o;
    burn_layer layers[32];
    int nlayer = 0;
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
    if (v && strcmp(v, "line") != 0) {
        fprintf(stderr, "openmake-burn: mode=%s: only line in this cut\n", v);
        return 1;
    }
    if (!driver) driver = "machines/laser/grbl/driver.om";
    if (!profile && !arg(argc, argv, "DRIVER"))
        profile = "machines/laser/grbl/profiles/generic-diode-400.om";
    burn_defaults(&o);
    if ((v = arg2(argc, argv, "power", "POWER")) != NULL) o.power = (float)atof(v);
    if ((v = arg2(argc, argv, "speed", "SPEED")) != NULL) o.speed = (float)atof(v);
    if ((v = arg2(argc, argv, "passes", "PASSES")) != NULL) o.passes = atoi(v);
    if ((v = arg2(argc, argv, "constant", "CONSTANT")) != NULL) o.constant = atoi(v);
    if ((v = arg2(argc, argv, "s100", "S100")) != NULL) o.s100 = (float)atof(v);
    if (o.passes < 1) o.passes = 1;
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
    om_clear(&m);
    if (om_load(&m, driver, err, (int)sizeof err) || (profile && om_load(&m, profile, err, (int)sizeof err))) {
        fprintf(stderr, "openmake-burn: %s\n", err);
        return 1;
    }
    memset(&st, 0, sizeof st);
    if (burn_write(argv[1], argv[2], &m, &o, &st, err, (int)sizeof err)) {
        fprintf(stderr, "openmake-burn: %s\n", err);
        return 1;
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
