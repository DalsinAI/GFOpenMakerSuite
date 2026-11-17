/* A print's settings, as the slicer window, GFOpenMake and openmake-slice
 * share them: kept as a simple text profile of "key = value" lines, each
 * setting described once (gs_setting) so a window can lay them out, and
 * held to what the machine can do (greyed when it cannot do one at all).
 * The first layer's speed is kept here ahead of the G-code taking it.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef GS_SETTINGS_H
#define GS_SETTINGS_H

#include "gs_gcode.h"
#include "gs_plan.h"
#include "om_machine.h"

enum { GS_LINES, GS_GRID };              /* infill patterns */
enum { GS_NONE, GS_SKIRT, GS_BRIM };     /* round the part on the first layer */

typedef struct {
    float layer_h, line_w;      /* mm */
    int walls;
    float infill;               /* % */
    int pattern;                /* GS_LINES ... */
    int top, bottom;            /* solid layers */
    float speed, travel, first; /* mm/s: printing, travelling, the first layer */
    float hotend, bed;          /* degrees C; a bed of 0 is not heated */
    float filament;             /* mm across */
    int around;                 /* GS_NONE, GS_SKIRT, GS_BRIM */
    int loops;                  /* of the skirt or brim */
    float distance;             /* mm out from the part */
    float retract, retract_speed; /* mm of filament pulled back over a travel, mm/s */
} gs_settings;

enum { GS_INT, GS_FLOAT, GS_CHOICE };

/* One setting, for the file and a window. */
typedef struct {
    const char *key;            /* in the file */
    const char *label;          /* in a window */
    const char *unit;           /* "mm", "%" ... or "" */
    int type;                   /* GS_INT ... */
    float min, max, step;
    const char *const *choices; /* GS_CHOICE: the words, NULL after the last */
} gs_setting;

void gs_settings_default(gs_settings *s);

/* The settings, 0 to gs_settings_count() - 1; NULL past the last. A
 * setting's bit in a mask is 1 << its number. */
int gs_settings_count(void);
const gs_setting *gs_setting_at(int i);
int gs_setting_find(const char *key);   /* -1: none */
#define GS_BIT(key) (1u << gs_setting_find(key))
float gs_setting_get(const gs_settings *s, int i);
void gs_setting_set(gs_settings *s, int i, float v);    /* held to its range */
const char *gs_setting_choice(int i, int v);            /* a choice's word; NULL: none */

/* "key = value" lines read over s ('#' starts a comment). 0, or -1 with
 * the line and why in err (s as far as it got). */
int gs_settings_parse(gs_settings *s, const char *text, char *err, int errlen);
int gs_settings_load(gs_settings *s, const char *path, char *err, int errlen);

/* s as text: how many characters, or -1 when len is too little. */
int gs_settings_format(const gs_settings *s, char *buf, int len);
int gs_settings_save(const gs_settings *s, const char *path, char *err, int errlen);

/* The settings m cannot do at all (a mask): not a printer, all; no
 * heated bed, the bed. */
unsigned gs_settings_greyed(const om_machine *m);

/* The most m allows of setting i (its hotend, its X speed ...), or the
 * setting's own most. */
float gs_setting_most(const om_machine *m, int i);

/* s held to m: too much brought down to m's most, greyed ones to 0. The
 * mask of those changed. */
unsigned gs_settings_fit(gs_settings *s, const om_machine *m);

/* s as the planner's and the G-code's options. */
void gs_settings_opts(const gs_settings *s, gp_opts *po, gc_opts *go);

/* And the planner's more: the pattern, top and bottom, the skirt, the gap. */
void gs_settings_more(const gs_settings *s, gp_more *x);

#endif
