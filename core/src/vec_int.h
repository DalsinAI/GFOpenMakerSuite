/* Shared insides of the 2D vector reader, used by vec.c and dxf.c.
 * Not a public API (gs_vec.h is).
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef VEC_INT_H
#define VEC_INT_H

#include "gs_vec.h"

#define GV_FLAT 0.05f
#define GV_PI 3.14159265f
#define GV_MAX_FILE (4 * 1024 * 1024)
#define GV_MAX_PT 1000000
#define GV_MIN 1e-7f

void fail_set(char *err, int errlen, const char *why);
int doc_add_path(gv_doc *d, int closed, unsigned stroke, unsigned fill, float width);
int doc_add_pt(gv_doc *d, float x, float y);
void finish_path(gv_doc *d);
int layers_build(gv_doc *d, char *err, int errlen);
void bounds_of(gv_doc *d);
float gv_atan2(float y, float x);

#endif
