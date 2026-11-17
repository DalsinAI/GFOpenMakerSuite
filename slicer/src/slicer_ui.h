/* GFOpenSlicer's window's settings (gs_settings.h): the shared defaults,
 * the window's numbers over them. Portable; slicer/tests/test_slicer_ui.c runs it on the host.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef SLICER_UI_H
#define SLICER_UI_H

#include "gs_settings.h"

#include <gfopenlayout_ui.h>

void sl_ui_settings(olui *ui, gs_settings *s);

#endif
