/* GFOpenSlicer's settings as the engine's options (slicer_ui.h).
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "slicer_ui.h"

#include "omview_settings.h"

int sl_ui_init(olui *ui, char *err, int errlen)
{
    gs_settings s;
    if (omv_settings_add(ui, "settings", err, errlen)) return -1;
    gs_settings_default(&s);
    omv_settings_show(ui, &s);
    return 0;
}

void sl_ui_settings(olui *ui, gs_settings *s)
{
    gs_settings_default(s);
    omv_settings_read(ui, s);
}
