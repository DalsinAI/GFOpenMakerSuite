/* GFOpenMakerWorkshop's ARexx commands, as docs/arexx.md and
 * docs/workshop-plan.md list them. The window opens OPENMAKEWORKSHOP.1
 * and dispatches through this table. Host tests call the same verbs
 * with omr_do_line.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef WORKSHOP_REXX_H
#define WORKSHOP_REXX_H

#include "gs_rexx.h"
#include "workshop_ui.h"

typedef struct gw_rexx {
    int size;
    workshop *ws;
    const char *version;
    const char *port_name; /* "OPENMAKEWORKSHOP.1", or NULL */
    int quit;
    void (*activate)(void *win);
    void (*hide)(void *win);
    void *win;
    void *ui;
    int home; /* 1: the window homes the camera, as a click on the list */
    int look; /* 0 top, 1 front, 2 side, 3 frame; -1: the window leaves the camera */
} gw_rexx;

#define GW_REXX_PORT "OPENMAKEWORKSHOP"

void gw_rexx_init(gw_rexx *g);
const omr_verb *gw_rexx_verbs(void);
int gw_rexx_nverbs(void);

/* Open a 0-based item the way OPENITEM does, and the way the window's
 * Launch button does. On success, used receives the port that took the
 * file, or the program name when that kind has no port. 0, or an RC
 * (10 or 20) with why in err. */
int gw_open_item(workshop *w, int index, char *used, int usedlen, char *err, int errlen);

#endif
