/* GFOpenCNC's ARexx commands, the shared set in docs/arexx.md. The window
 * opens the port GFOPENCNC.1 and dispatches through this table. Host
 * tests call the same verbs with omr_do_line.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef CNC_REXX_H
#define CNC_REXX_H

#include "cnc_job.h"
#include "gs_rexx.h"

typedef struct oc_rexx {
    int size;
    gv_doc *doc;
    int *have_doc;
    gt_path *path;
    int *have_path;
    cnc_opts *opts;
    om_machine machine;
    int have_machine;
    char file[256];
    char save_file[256];
    int dirty;
    const char *version;
    const char *port_name; /* "GFOPENCNC.1", or NULL */
    int quit;
    void (*activate)(void *win);
    void (*hide)(void *win);
    void *win;
    void *ui;
    char start[240]; /* the user's own lines; opts->start points here when set */
    char end[240];
    int look; /* -1: leave the camera. 0 top, 1 front, 2 side, 3 frame (the view at open) */
} oc_rexx;

#define OC_REXX_PORT "GFOPENCNC"

void oc_rexx_init(oc_rexx *g);
const omr_verb *oc_rexx_verbs(void);
int oc_rexx_nverbs(void);

#endif
