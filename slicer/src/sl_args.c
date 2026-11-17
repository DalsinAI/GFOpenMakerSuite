/* The print settings from a slicing command's arguments (sl_args.h).
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "sl_args.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *arg(int argc, char **argv, int first, const char *name)
{
    size_t n = strlen(name);
    int i;
    for (i = first; i < argc; i++)
        if (strncmp(argv[i], name, n) == 0 && argv[i][n] == '=') return argv[i] + n + 1;
    return NULL;
}

static void num(int argc, char **argv, int first, const char *name, float *out)
{
    const char *v = arg(argc, argv, first, name);
    if (v) *out = (float)atof(v);
}

static void whole(int argc, char **argv, int first, const char *name, int *out)
{
    const char *v = arg(argc, argv, first, name);
    if (v) *out = atoi(v);
}

int sl_args(gs_settings *s, int argc, char **argv, int first, char *err, int errlen)
{
    const char *file = arg(argc, argv, first, "SETTINGS"), *v;
    char why[160];
    int i;
    gs_settings_default(s);
    if (file && gs_settings_load(s, file, why, (int)sizeof why)) {
        snprintf(err, (size_t)errlen, "%s%s%s", strstr(why, file) ? "" : file, strstr(why, file) ? "" : ": ", why);
        return -1;
    }
    num(argc, argv, first, "LAYER", &s->layer_h);
    num(argc, argv, first, "INFILL", &s->infill);
    num(argc, argv, first, "LINE", &s->line_w);
    whole(argc, argv, first, "WALLS", &s->walls);
    if (arg(argc, argv, first, "SKINS")) whole(argc, argv, first, "SKINS", &s->top), s->bottom = s->top;
    if ((v = arg(argc, argv, first, "BRIM")) != NULL) s->loops = atoi(v), s->around = s->loops > 0 ? GS_BRIM : GS_NONE;
    if ((v = arg(argc, argv, first, "SKIRT")) != NULL)
        s->loops = atoi(v), s->around = s->loops > 0 ? GS_SKIRT : GS_NONE;
    num(argc, argv, first, "FILAMENT", &s->filament);
    num(argc, argv, first, "HOTEND", &s->hotend);
    num(argc, argv, first, "BED", &s->bed);
    /* Then each lower-case key=value, as a line of a profile. */
    for (i = first; i < argc; i++) {
        if (!argv[i] || argv[i][0] < 'a' || argv[i][0] > 'z') continue;
        if (gs_settings_parse(s, argv[i], why, (int)sizeof why) || !strchr(argv[i], '=')) {
            const char *w = strncmp(why, "line 1: ", 8) ? why : why + 8;
            if (!strchr(argv[i], '=')) w = "not key=value";
            snprintf(err, (size_t)errlen, "%s: %s", argv[i], w);
            return -1;
        }
    }
    return 0;
}
