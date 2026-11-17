/* The slicing commands' settings arguments (sl_args.h): the defaults, the
 * shipped profile, the older names, key=value, and what is refused.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "sl_args.h"

#include <stdio.h>
#include <string.h>

static int fails;

static void expect(int ok, const char *what)
{
    if (!ok) {
        printf("fail: %s\n", what);
        fails++;
    }
}

/* sl_args over words, as a command's argv after in.stl out.gcode. */
static int run(gs_settings *s, char *err, const char *a, const char *b, const char *c, const char *d)
{
    char *argv[7] = {"cmd", "in.stl", "out.gcode", NULL, NULL, NULL, NULL};
    int argc = 3;
    if (a) argv[argc++] = (char *)a;
    if (b) argv[argc++] = (char *)b;
    if (c) argv[argc++] = (char *)c;
    if (d) argv[argc++] = (char *)d;
    err[0] = 0;
    return sl_args(s, argc, argv, 3, err, 160);
}

int main(void)
{
    gs_settings s, d;
    char err[160];
    FILE *f;
    gs_settings_default(&d);

    expect(run(&s, err, NULL, NULL, NULL, NULL) == 0 && !memcmp(&s, &d, sizeof s), "nothing given: the defaults");
    expect(run(&s, err, "DRIVER=x.om", "TASKS=2", "PREVIEW=p.ppm", NULL) == 0 && !memcmp(&s, &d, sizeof s),
           "the command's own words left alone");
    expect(run(&s, err, "SETTINGS=settings/standard", NULL, NULL, NULL) == 0 && !memcmp(&s, &d, sizeof s),
           "the shipped profile is the built-in defaults");
    expect(run(&s, err, "SETTINGS=no/such/file", NULL, NULL, NULL) == -1 && strstr(err, "no/such/file"),
           "a profile that is not there is refused, by name");

    expect(run(&s, err, "LAYER=0.3", "INFILL=30", "SKINS=5", "BRIM=4") == 0 && s.layer_h > 0.29f &&
               s.layer_h < 0.31f && s.infill == 30.f && s.top == 5 && s.bottom == 5 && s.around == GS_BRIM &&
               s.loops == 4,
           "the older names");
    expect(run(&s, err, "BRIM=0", NULL, NULL, NULL) == 0 && s.around == GS_NONE, "BRIM=0: nothing around");
    expect(run(&s, err, "SKIRT=3", NULL, NULL, NULL) == 0 && s.around == GS_SKIRT && s.loops == 3, "SKIRT=3: a skirt");
    expect(run(&s, err, "SKIRT=0", NULL, NULL, NULL) == 0 && s.around == GS_NONE, "SKIRT=0: nothing around");
    expect(run(&s, err, "WALLS=3", "LINE=0.5", "FILAMENT=2.85", "HOTEND=215") == 0 && s.walls == 3 &&
               s.line_w > 0.49f && s.line_w < 0.51f && s.filament > 2.84f && s.hotend == 215.f,
           "and the rest of them");
    expect(run(&s, err, "BED=70", NULL, NULL, NULL) == 0 && s.bed == 70.f, "BED");

    expect(run(&s, err, "pattern=grid", "around=brim", "top-layers=6", "retract-speed=20") == 0 &&
               s.pattern == GS_GRID && s.around == GS_BRIM && s.top == 6 && s.retract_speed == 20.f,
           "key=value, by the profile's keys");
    expect(run(&s, err, "infill=25", "INFILL=30", NULL, NULL) == 0 && s.infill == 25.f,
           "key=value over the older name, wherever it is");
    expect(run(&s, err, "layer-height=5", NULL, NULL, NULL) == 0 && s.layer_h <= 1.f, "held to its range");
    expect(run(&s, err, "spped=60", NULL, NULL, NULL) == -1 && strstr(err, "spped") && !strstr(err, "line "),
           "a key that is not a setting is refused, by name");
    expect(run(&s, err, "pattern=zigzag", NULL, NULL, NULL) == -1 && strstr(err, "zigzag"), "a choice not there");
    expect(run(&s, err, "infill=lots", NULL, NULL, NULL) == -1 && strstr(err, "lots"), "a number that is not");
    expect(run(&s, err, "bed", NULL, NULL, NULL) == -1 && strstr(err, "bed"), "a word with no value");

    f = fopen("build/sl-args.profile", "w");
    expect(f != NULL, "a profile written");
    if (f) {
        fputs("# a test profile\ninfill = 40\npattern = grid\nwalls = 4\n", f);
        fclose(f);
    }
    expect(run(&s, err, "SETTINGS=build/sl-args.profile", NULL, NULL, NULL) == 0 && s.infill == 40.f &&
               s.pattern == GS_GRID && s.walls == 4 && s.layer_h == d.layer_h,
           "a profile over the defaults");
    expect(run(&s, err, "INFILL=30", "SETTINGS=build/sl-args.profile", "walls=1", NULL) == 0 && s.infill == 30.f &&
               s.walls == 1 && s.pattern == GS_GRID,
           "the older names over the profile, key=value over both");
    if (fails) {
        printf("%d failed\n", fails);
        return 1;
    }
    printf("slice args ok\n");
    return 0;
}
