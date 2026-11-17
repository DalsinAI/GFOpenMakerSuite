/* The machine picker every app of the suite shares: the machines under
 * machines/<family>/ (each driver with a driver.om, each machine a
 * profiles/ .om file read over it), for a select in the app's window, and the
 * chosen one loaded. Shared source, as omview_ui.h. Portable: the
 * directories are read with opendir (libnix's on the Amiga).
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef OMVIEW_MACHINES_H
#define OMVIEW_MACHINES_H

#include "om_machine.h"

#include <gfopenlayout_ui.h>

typedef struct {
    char name[64];             /* the profile's name, or its file's */
    char driver[256], profile[256];
} omv_machine;

typedef struct {
    omv_machine *at;
    int n, cap;
    int skipped;               /* profiles that would not load */
    char why[160];             /* the first one's reason */
} omv_machines;

/* Every machine of family ("printer", "laser", "cnc") under root (e.g.
 * "PROGDIR:machines"), the generic ones ("Generic ...") first, then
 * sorted by name, in place of what l had
 * (l zeroed the first time). Only profiles that
 * load and say that family are listed. How many, or -1 out of memory. */
int omv_machines_scan(omv_machines *l, const char *root, const char *family);
void omv_machines_free(omv_machines *l);

/* Machine i loaded into m: 0, or -1 with why in err. */
int omv_machines_load(const omv_machines *l, int i, om_machine *m, char *err, int errlen);

/* The select id filled with l's names, the first chosen; with none, one
 * option saying so (none) and the select disabled. How many machines. */
int omv_machines_fill(olui *ui, const char *id, const omv_machines *l, const char *none);

/* The machine the select id has chosen, or -1. */
int omv_machines_chosen(olui *ui, const char *id, const omv_machines *l);

#endif
