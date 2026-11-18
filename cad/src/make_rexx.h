/* GFOpenMake's ARexx commands, as docs/arexx.md lists them. The window
 * opens the port GFOPENMAKE.1 and dispatches through this table. Host
 * tests call the same verbs with omr_do_line.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef MAKE_REXX_H
#define MAKE_REXX_H

#include "gs_edit.h"
#include "gs_rexx.h"
#include "gs_settings.h"
#include "om_machine.h"

typedef struct gm_rexx {
    int size;
    gs_parts **parts; /* undo may replace the list */
    ge_steps *steps;
    gs_settings set;
    om_machine machine;
    int have_machine;
    char file[256];
    int dirty;
    int face; /* picked face, or -1 */
    gs_v mark[2];
    int marks;
    const char *version;
    const char *port_name; /* "GFOPENMAKE.1", or NULL */
    int quit;
    void (*activate)(void *win);
    void (*hide)(void *win);
    void (*after)(void *ui); /* refresh the window after a change; may be NULL */
    void *win;
    void *ui;
    int look;  /* -1: leave the camera. 0 top, 1 front, 2 side, 3 frame */
    int shade; /* 0 metal, 1 clay, 2 wire */
    int snap;  /* 0 off, 1 on */
    int section; /* 0 off, 1: hide the picture above section_z */
    float section_z;
    int tool; /* -1: leave it. 0 orbit, 1 move, 2 rot, 3 scale, 4 measure, 5 pull, 6 draw */
    int layers;   /* 0 hide the layer picture, 1 show it */
    int layer_no; /* 1-based layer the slider shows */
} gm_rexx;

#define GM_REXX_PORT "GFOPENMAKE"

void gm_rexx_init(gm_rexx *g);
const omr_verb *gm_rexx_verbs(void);
int gm_rexx_nverbs(void);

#endif
