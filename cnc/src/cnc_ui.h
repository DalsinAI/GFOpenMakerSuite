/* The mill window's fields (cnc/src/markup.h). Portable; the host test
 * lays the window out. MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef CNC_UI_H
#define CNC_UI_H

#include "cnc_job.h"

#include <gfopenlayout_ui.h>

/* Profile, pocket or drill lit. */
void cnc_ui_mode(olui *ui, int mode);
int cnc_ui_mode_get(olui *ui);

/* The cut, from the fields. mode is whichever button is lit. */
void cnc_ui_read(olui *ui, cnc_opts *o);

/* The Drawing label: SVG, DXF, or File. */
void cnc_ui_kind(olui *ui, const char *path);

/* One status line. */
void cnc_ui_status(char *buf, int len, const char *machine, int mode, int moves, float seconds);

#endif
