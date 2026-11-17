/* GFOpenSlicer's settings as the engine's options (slicer_ui.h).
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "slicer_ui.h"

#include "omview_ui.h"

void sl_ui_settings(olui *ui, gs_settings *s)
{
    gs_settings_default(s);
    s->layer_h = omv_number(ui, "layer");
    s->infill = omv_number(ui, "infill");
    s->line_w = omv_number(ui, "line");
    s->walls = (int)(omv_number(ui, "walls") + 0.5f);
    s->loops = (int)(omv_number(ui, "brim") + 0.5f);
    s->around = s->loops > 0 ? GS_BRIM : GS_NONE;
    s->distance = 0.f;
    s->hotend = omv_number(ui, "hotend");
    s->bed = omv_number(ui, "bed");
}
