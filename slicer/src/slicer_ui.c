/* GFOpenSlicer's settings as the engine's options (slicer_ui.h).
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "slicer_ui.h"

#include "omview_settings.h"

#include <stdio.h>
#include <string.h>

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

static const char *field_text(olui *ui, const char *id)
{
    const char *s = olui_get_text(olui_find(ui, id));
    return s ? s : "";
}

void sl_ui_job_read(olui *ui, int *support, char *start, int start_n, char *end, int end_n)
{
    olui_el *el;
    if (support) {
        el = olui_find(ui, "support");
        *support = (el && olui_get_checked(el)) ? 1 : 0;
    }
    if (start && start_n > 0) snprintf(start, (size_t)start_n, "%s", field_text(ui, "start"));
    if (end && end_n > 0) snprintf(end, (size_t)end_n, "%s", field_text(ui, "end"));
}

void sl_ui_job_show(olui *ui, int support, const char *start, const char *end)
{
    olui_el *el = olui_find(ui, "support");
    if (el) olui_set_checked(ui, el, support ? 1 : 0);
    el = olui_find(ui, "start");
    if (el) olui_set_text(ui, el, start ? start : "");
    el = olui_find(ui, "end");
    if (el) olui_set_text(ui, el, end ? end : "");
}
