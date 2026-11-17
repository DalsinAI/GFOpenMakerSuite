/* GFOpenSlicer's window's settings as the toolpath engine's options: the
 * engine's defaults (gp_defaults, gc_defaults), the window's numbers over
 * them. Portable; slicer/tests/test_slicer_ui.c runs it on the host.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef SLICER_UI_H
#define SLICER_UI_H

#include "gs_gcode.h"
#include "gs_plan.h"

#include <gfopenlayout_ui.h>

void sl_ui_opts(olui *ui, gp_opts *po, gc_opts *go);

#endif
