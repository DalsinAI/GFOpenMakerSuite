/* OpenMakeBurn's window's layer list, laid out in #layers. Portable;
 * burn/tests/test_burn_ui.c runs it on the host.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef BURN_UI_H
#define BURN_UI_H

#include "burn_job.h"
#include "burn_material.h"

#include <gfopenlayout_ui.h>

/* Fill #layers from d, each row the colour's power, speed and passes
 * (o's defaults, and any override), in o->seq order when that is set.
 * Cuts last is shown from o. 0, or -1 with why in err. */
int burn_ui_layers(olui *ui, const gv_doc *d, const burn_opts *o, char *err, int errlen);

/* How many layer rows are in the window. */
int burn_ui_count(olui *ui);

/* 0-based document layer indices as the list is shown, into seq[0..n). */
int burn_ui_order(olui *ui, int *seq, int cap);

/* Move document layer L one step in the list (dir < 0 up, dir > 0 down).
 * Reads the current settings into store, writes o->seq from seq, and
 * rebuilds the rows. 0, or -1 with why in err. A move off the end is 0. */
int burn_ui_move(olui *ui, const gv_doc *d, burn_opts *o, burn_layer *store, int cap, int *seq, int seqcap, int L,
                 int dir, char *err, int errlen);

/* The window's layer settings, into store (up to cap) and o->layer.
 * constant, cuts_last, X, Y and rotation come from their fields. The
 * first row's power, speed, passes, mode, gap, kerf and tabs become the job's. */
void burn_ui_read(olui *ui, burn_opts *o, burn_layer *store, int cap);

/* Write o's X, Y and rotation into the place fields, so a port SET
 * survives the next read. */
void burn_ui_place(olui *ui, const burn_opts *o);

/* The materials picker: "None" first, then the library, sorted as scanned. */
int burn_ui_materials_fill(olui *ui, const char *id, const burn_materials *l);

/* The chosen profile, or -1 when "None" (or there is no list). */
int burn_ui_materials_chosen(olui *ui, const char *id, const burn_materials *l);

/* Put a profile's settings into every layer row that is on the list, and
 * into o's job-wide defaults. */
void burn_ui_material(olui *ui, burn_opts *o, const burn_material *m);

/* Set the Drawing field's label to SVG, DXF, Picture, or File. */
void burn_ui_kind(olui *ui, const char *path);

/* A short status line: moves, the machine's name, and how long. */
void burn_ui_status(char *buf, int len, const char *name, int moves, float seconds);

#endif
