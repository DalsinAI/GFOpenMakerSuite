/* GFOpenSlicer's settings as the engine's options (slicer_ui.h).
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "slicer_ui.h"

#include "omview_ui.h"

void sl_ui_opts(olui *ui, gp_opts *po, gc_opts *go)
{
    gp_defaults(po);
    gc_defaults(go);
    po->layer_h = omv_number(ui, "layer");
    po->infill = omv_number(ui, "infill");
    po->line_w = omv_number(ui, "line");
    po->walls = (int)(omv_number(ui, "walls") + 0.5f);
    po->brim = (int)(omv_number(ui, "brim") + 0.5f);
    go->hotend = omv_number(ui, "hotend");
    go->bed = omv_number(ui, "bed");
    go->layer_h = po->layer_h;
}
