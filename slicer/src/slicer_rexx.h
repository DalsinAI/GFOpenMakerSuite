/* GFOpenSlicer's ARexx commands, as docs/arexx.md lists them. The window
 * opens the port OPENMAKESLICER.1 and dispatches through this table. Host
 * tests call the same verbs with omr_do_line.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef SLICER_REXX_H
#define SLICER_REXX_H

#include "gs_rexx.h"
#include "gs_settings.h"
#include "om_machine.h"
#include "sl_job.h"

typedef struct os_rexx {
    int size;
    sl_job *job;
    gs_settings set;
    om_machine machine;
    int have_machine;
    int tasks; /* slice tasks; 0: one */
    char file[256];
    char save_file[256];
    int dirty;
    const char *version;
    const char *port_name; /* "OPENMAKESLICER.1", or NULL */
    int quit;
    void (*activate)(void *win);
    void (*hide)(void *win);
    void *win;
    void *ui;
} os_rexx;

#define OS_REXX_PORT "OPENMAKESLICER"

void os_rexx_init(os_rexx *g);
const omr_verb *os_rexx_verbs(void);
int os_rexx_nverbs(void);

#endif
