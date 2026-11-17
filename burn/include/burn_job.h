/* OpenMakeBurn's job: an SVG or DXF to Grbl laser G-code, through openmakevec
 * and openmakegcode. Line mode follows each path; fill mode scans back
 * and forth inside closed shapes (openmakepath's gt_fill); offset
 * fill follows them inwards (gl_inset). A layer can
 * take its own power, speed, passes and mode; the job's values are the
 * default. cuts_last (default on) burns fills first, then lines, with
 * the heavier lines last; the layer list order is used when it is off. MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef BURN_JOB_H
#define BURN_JOB_H

#include "gs_gcode.h"
#include "gs_vec.h"
#include "om_machine.h"

enum {
    BURN_SET_POWER = 1,
    BURN_SET_SPEED = 2,
    BURN_SET_PASSES = 4,
    BURN_SET_ON = 8,
    BURN_SET_MODE = 16,
    BURN_SET_INTERVAL = 32,
    BURN_SET_KERF = 64,
    BURN_SET_TABS = 128,
    BURN_SET_TABW = 256
};

/* How a layer is burned. */
enum { BURN_LINE = 0, BURN_FILL = 1, BURN_OFFSET = 2 };

/* One layer's settings, matching a colour or a 1-based place in the
 * document's layer list. Fields not in set come from the job. */
typedef struct {
    int size;
    unsigned colour; /* 0xRRGGBB; GV_NONE when index is used */
    int index;       /* 1-based; 0 when colour is used */
    float power;     /* 0 to 100 */
    float speed;     /* mm/s for a cut */
    int passes;
    int on;          /* 0: leave the layer out; not 0: include it */
    int mode;        /* BURN_LINE, BURN_FILL or BURN_OFFSET */
    float interval;  /* mm between fill lines; 0: 0.2 */
    float kerf;      /* mm of kerf on a cut layer; 0: none */
    int tabs;        /* holding tabs per closed cut; 0: none */
    float tabw;      /* mm width of each tab; 0: 1 */
    unsigned set;
} burn_layer;

typedef struct {
    int size;
    float power;    /* 0 to 100, every layer; 0: 50 */
    float speed;    /* mm/s for a cut; 0: 20 */
    float travel;   /* mm/s for a travel; 0: the machine's most X, or 150 */
    int passes;     /* 0 or 1: once; more than 1: that many */
    int constant;   /* 0: M4, not 0: M3 */
    float s100;     /* S at 100%; 0: 1000 */
    int mode;       /* BURN_LINE, BURN_FILL or BURN_OFFSET; 0: line */
    float interval; /* mm between fill lines; 0: 0.2 */
    const burn_layer *layer; /* overrides, or NULL */
    int nlayer;
    int cuts_last;     /* 1: fills first, heavier lines last; 0: seq or file order */
    const int *seq;    /* 0-based document layers, nseq long; NULL: file order */
    int nseq;
    int invert;   /* 1: white is more power on a picture */
    float width;  /* picture width mm; 0: interval times the columns */
    float kerf;   /* mm of kerf on cut layers; 0: none */
    int tabs;     /* holding tabs per closed cut; 0: none */
    float tabw;   /* mm width of each tab; 0: 1 */
} burn_opts;

void burn_defaults(burn_opts *o);

/* "SVG", "DXF" or "Picture" from the path's suffix, or a peek at the
 * file when the suffix is not enough. Empty or unknown is "File". */
const char *burn_drawing_kind(const char *path);

/* line, fill, offset, or -1 if s is not a known mode. */
int burn_mode_parse(const char *s);
const char *burn_mode_name(int mode);

/* Parse a layer= value: "#rrggbb,power=80,speed=10,passes=2,on=1" or
 * "1,power=80", and mode= / interval= for a fill, kerf= for a cut,
 * tabs= / tabw= for holding tabs on a closed cut. 0, or -1 with why in err. */
int burn_parse_layer(const char *s, burn_layer *L, char *err, int errlen);

/* Layer L (0-based) of d, with o's defaults and any matching override.
 * power, speed, passes, on, mode, interval, kerf, tabs and tabw are filled in. */
void burn_layer_of(const burn_opts *o, const gv_doc *d, int L, burn_layer *out);

/* 0-based document layer indices into seq[0..d->nlayer-1] as the job will
 * run them. cap must be at least d->nlayer. 0, or -1 if cap is short. */
int burn_seq(const burn_opts *o, const gv_doc *d, int *seq, int cap);

/* How heavy a cut is: power * passes / speed, speed at least 0.001 mm/s. */
float burn_heaviness(const burn_layer *ly);

/* Replaces p with the line or fill paths of d, each layer at its power,
 * its passes times, in cuts-last or list order. If speed is not NULL, *speed is a new
 * array of p->n mm/s values the caller frees (0 on a travel). 0, or -1
 * out of memory. */
int burn_path(gt_path *p, float **speed, const gv_doc *d, const burn_opts *o);

/* Load svg, build the path, check it against m, write gcode. 0, or -1
 * with why in err. st (may be NULL) is filled as the writer left it. */
int burn_write(const char *drawing, const char *gcode, const om_machine *m, const burn_opts *o, gc_stats *st, char *err,
               int errlen);

/* A travel around the job's box (laser off). Replaces out. 0, or -1 if
 * the job has no cuts or memory ran out. */
int burn_frame_path(gt_path *out, const gt_path *job);

/* Write p (a frame, or any path) as Grbl laser G-code for m. 0, or -1. */
int burn_write_path(const gt_path *p, const float *speed, const char *gcode, const om_machine *m, const burn_opts *o,
                    gc_stats *st, char *err, int errlen);

#endif
