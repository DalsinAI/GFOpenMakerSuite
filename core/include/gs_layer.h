/* GFOpenMake's Layers group: the walls of a part at one height. Walls only,
 * no infill and no G-code. MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef GS_LAYER_H
#define GS_LAYER_H

#include "gs.h"
#include "gs_view.h"

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

/* The pieces of wall welded into loops: ends within tol are one point.
 * Closed loops are anticlockwise round the part and clockwise round a
 * hole (by how many loops hold them, odd or even). A piece that does not
 * close is kept, open, and counted in open. */
typedef struct {
    float *xy;     /* n points, x then y; a closed loop's last joins its first */
    int n, closed;
} gl_loop;

typedef struct {
    gl_loop *l;
    int n, open;
} gl_loops;

/* Replaces out with w's loops. 0, or -1 out of memory. */
int gl_weld(const gl_walls *w, float tol, gl_loops *out);
void gl_loops_free(gl_loops *out);
/* Anticlockwise is positive; an open piece's is as if it were closed. */
float gl_loop_area(const gl_loop *l);
float gl_loop_length(const gl_loop *l);

/* "Layer n of N, X mm of wall. No infill." for the window's status. */
void gl_status(char *buf, int len, const gs_mesh *m, float h, int layer, const gl_walls *w);

/* The walls in Clear's blue (#0067c0), over a gs_view_bed picture of m
 * (the plate's frame). The pen drawer's picture; OpenGPU draws them with
 * gs_gpu_walls_plate. 0, or -1. */
int gl_paint(const gl_walls *w, const gs_mesh *m, const gs_cam *cam, unsigned char *rgb, int width, int height);

#endif
