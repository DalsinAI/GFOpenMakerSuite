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

#endif
