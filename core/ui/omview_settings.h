/* The print settings panel the slicer window and GFOpenMake's Slice group
 * share: laid out from gs_settings' table (gs_settings.h) as a few tabs of
 * a label, a field and its unit a row, so it fits a side column
 * OMV_SETTINGS_W wide in a window of 640 by 480. Each field's id is
 * "set-" and the setting's key. Held to the machine picked: too much
 * brought down, and what it cannot do at all greyed, with a line saying
 * which for the status line. Shared source, as omview_ui.h.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef OMVIEW_SETTINGS_H
#define OMVIEW_SETTINGS_H

#include "gs_settings.h"
#include "om_machine.h"

#include <gfopenlayout_ui.h>

#define OMV_SETTINGS_W 236
#define OMV_SETTINGS_W_CSS "236px"

/* The panel (and its CSS) added into the element parent_id. 0, or -1 with
 * why in err. */
int omv_settings_add(olui *ui, const char *parent_id, char *err, int errlen);

/* Its tabs; the one setting i is on; the tab shown. */
int omv_settings_tabs(void);
int omv_settings_tab_of(int i);
void omv_settings_tab(olui *ui, int tab);

/* The fields set to s, and s read from them (each held to its range). */
void omv_settings_show(olui *ui, const gs_settings *s);
void omv_settings_read(olui *ui, gs_settings *s);

/* The fields read into s, s held to m (gs_settings_fit), the fields shown
 * again and the ones m cannot do greyed. The mask of those changed, and
 * why (78 characters at most; "" when nothing changed). */
unsigned omv_settings_fit(olui *ui, const om_machine *m, gs_settings *s, char *why, int whylen);

#endif
