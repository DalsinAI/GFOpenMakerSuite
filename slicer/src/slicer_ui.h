/* GFOpenSlicer's window's settings (gs_settings.h): core/ui's settings
 * panel (omview_settings.h) in the window, and what it says. Portable; slicer/tests/test_slicer_ui.c runs it on the host.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef SLICER_UI_H
#define SLICER_UI_H

#include "gs_settings.h"

#include <gfopenlayout_ui.h>

/* The settings panel added to the window (into #settings), showing the
 * shared defaults. 0, or -1 with why in err. */
int sl_ui_init(olui *ui, char *err, int errlen);

/* The window's settings, from the panel. */
void sl_ui_settings(olui *ui, gs_settings *s);

/* Support, and one line of the user's own G-code at the start and the end.
 * support, start and end may be NULL. A missing field reads as off, or empty. */
void sl_ui_job_read(olui *ui, int *support, char *start, int start_n, char *end, int end_n);

/* The same fields, written back. NULL is an empty line. */
void sl_ui_job_show(olui *ui, int support, const char *start, const char *end);

#endif
