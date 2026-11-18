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
} gw_rexx;

#define GW_REXX_PORT "OPENMAKEWORKSHOP"

void gw_rexx_init(gw_rexx *g);
const omr_verb *gw_rexx_verbs(void);
int gw_rexx_nverbs(void);

#endif
