/* The view's UI every app of the suite shares (Cad, Slicer, Burn, CNC): the
 * status line, a number field, the layer slider's 1 to 100, and the pen
 * drawer, the window's fallback when there is no OpenGPU. Shared source,
 * not a library: GFOpenLayout's UI is a link library, so these work on its
 * olui as the app's own code does. view_ui.c is the portable part.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef OMVIEW_UI_H
#define OMVIEW_UI_H

#include "gs_view.h"

#include <gfopenlayout_ui.h>

/* The #status line says text. */
void omv_say(olui *ui, const char *text);

/* A spinner's number (id), or 0 when there is none. */
float omv_number(olui *ui, const char *id);

/* Layer l of n (1 to n) as the slider's 1 to 100, and back. */
int omv_slider_of(int l, int n);
int omv_layer_of(int slider, int n);

/* What a slice came to, for the status line: its layers and moves, how
 * long (seconds) and how much filament (mm), and which layers are shown
 * (upto; 0: all). */
void omv_slice_status(char *buf, int len, int layers, int moves, float seconds, float filament, int upto);

/* The pen drawer: a w by h picture (rgb, as gs_view_* draw it) as runs of
 * one pen (gs_pen_of), each put(ud, x, y, n, pen) once: on the Amiga one
 * SetAPen and RectFill. Returns how many runs, or -1 out of memory. */
typedef void (*omv_put_fn)(void *ud, int x, int y, int n, int pen);
int omv_pens(const unsigned char *rgb, int w, int h, omv_put_fn put, void *ud);

#endif
