/* GFOpenMakerWorkshop's window helpers: load the markup, fill the item
 * list and Recent, preview the picked item, and the Open in app hook
 * (starts the kind's program when its port is not already up).
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef WORKSHOP_UI_H
#define WORKSHOP_UI_H

#include "gs_project.h"
#include "gs_view.h"

#include <gfopenlayout_ui.h>

typedef struct workshop {
    gpr_project project;
    gpr_recent recent;
    int have;   /* 1 when a project is open */
    int picked; /* 0-based item, or -1 */
    char recent_path[GPR_PATH_MAX + 1];
    char status[160];
    char last_launch[512];
} workshop;

void workshop_init(workshop *w);
void workshop_free(workshop *w);

int workshop_ui_load(olui *ui, char *err, int errlen);
/* Fills the product name, notes, item list, Recent, buttons and status. */
void workshop_ui_refresh(olui *ui, workshop *w);

/* The picked item into rgb. cam may be NULL (a home camera). 0 painted,
 * or -1 when there is no picture (rgb is still a dark view). */
int workshop_ui_preview(const workshop *w, const gs_cam *cam, unsigned char *rgb, int width, int height, char *err,
                        int errlen);

/* Starts the kind's app with the picked item's file (PROGDIR: first, then
 * the name). On the host, records the command in last_launch. 0, or -1. */
int workshop_launch(workshop *w, char *err, int errlen);
const char *workshop_launch_last(const workshop *w);

#endif
