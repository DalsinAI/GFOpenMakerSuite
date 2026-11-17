/* An OpenMake window on the Amiga, as every app opens it: on the default
 * public screen, filling it below its title bar (omv_window_fit: 640 by
 * 480 inside on a PAL screen, the whole of a bigger RTG one), resizable
 * from OMV_MIN_W by OMV_MIN_H inside to the screen's size. GFOpenLayout lays the window out again at each
 * IDCMP_NEWSIZE (olui_amiga_event), and the app's view, drawn at the size
 * it is given, grows with it. window_amiga.c.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef OMVIEW_WINDOW_H
#define OMVIEW_WINDOW_H

#include "omview_ui.h"

#include <intuition/intuition.h>

/* The window titled title, with idcmp (olui_amiga_idcmp()), or NULL. */
struct Window *omv_window_open(const char *title, ULONG idcmp);

#endif
