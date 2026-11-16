/* GFOpenMake's Layers group: the walls of a part at one height. Walls only,
 * no infill and no G-code. MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef GS_LAYER_H
#define GS_LAYER_H

#include "gs.h"

#define GL_MIN_H 0.08f
#define GL_MAX_H 0.8f

typedef struct {
    float x0, y0, x1, y1;
} gl_seg;

typedef struct {
    gl_seg *s;
    int n, cap;
    float z;       /* the height the plane cuts at */
    float length;  /* mm of wall at that height */
} gl_walls;

/* A layer height in 0.08 to 0.8 mm. Anything not a number is 0.2. */
float gl_height(float h);
/* Layers in the part at height h, at least 1 for a part with any height. */
int gl_layers(const gs_mesh *m, float h);
/* Layer 1 is the first. Layer n cuts mid-layer, (n - 0.5) heights above
 * the floor, as the bench does. */
float gl_layer_z(const gs_mesh *m, float h, int layer);
/* Replaces w with the walls of that layer. 0, or -1. */
int gl_cut(const gs_mesh *m, float h, int layer, gl_walls *w);
void gl_walls_free(gl_walls *w);

#endif
