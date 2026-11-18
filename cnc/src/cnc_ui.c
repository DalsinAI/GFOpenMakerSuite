/* The mill window's fields (cnc_ui.h). MIT, Copyright (c) 2026 AmiCradle Team. */
#include "cnc_ui.h"
#include "omview_ui.h"

#include <stdio.h>
#include <string.h>

static const char *mode_id(int mode)
{
    if (mode == CNC_POCKET) return "pocket";
    if (mode == CNC_DRILL) return "drill";
    return "profile";
}

void cnc_ui_mode(olui *ui, int mode)
{
    olui_set_selected(ui, olui_find(ui, "profile"), mode == CNC_PROFILE);
    olui_set_selected(ui, olui_find(ui, "pocket"), mode == CNC_POCKET);
    olui_set_selected(ui, olui_find(ui, "drill"), mode == CNC_DRILL);
}

int cnc_ui_mode_get(olui *ui)
{
    if (olui_is_selected(olui_find(ui, "pocket"))) return CNC_POCKET;
    if (olui_is_selected(olui_find(ui, "drill"))) return CNC_DRILL;
    return CNC_PROFILE;
}

void cnc_ui_read(olui *ui, cnc_opts *o)
{
    if (!o) return;
    cnc_defaults(o);
    o->mode = cnc_ui_mode_get(ui);
    o->tool = omv_number(ui, "tool");
    o->depth = omv_number(ui, "depth");
    o->stepdown = omv_number(ui, "stepdown");
    o->stepover = omv_number(ui, "stepover");
    o->feed = omv_number(ui, "feed");
    o->plunge = omv_number(ui, "plunge");
    o->safe = omv_number(ui, "safe");
    o->rpm = omv_number(ui, "rpm");
    o->x = omv_number(ui, "x");
    o->y = omv_number(ui, "y");
}

void cnc_ui_kind(olui *ui, const char *path)
{
    const char *name = "File";
    const char *dot;
    if (path) {
        dot = strrchr(path, '.');
        if (dot && (!strcmp(dot, ".svg") || !strcmp(dot, ".SVG"))) name = "SVG";
        else if (dot && (!strcmp(dot, ".dxf") || !strcmp(dot, ".DXF"))) name = "DXF";
    }
    olui_set_text(ui, olui_find(ui, "kind"), name);
}

void cnc_ui_status(char *buf, int len, const char *machine, int mode, int moves, float seconds)
{
    if (!buf || len < 1) return;
    if (moves <= 0) {
        snprintf(buf, (size_t)len, "Open a drawing.");
        return;
    }
    snprintf(buf, (size_t)len, "%s, %s. %d moves, %.0f s.", machine && machine[0] ? machine : "No mill", mode_id(mode),
             moves, (double)seconds);
}
