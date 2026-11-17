/* GFOpenSlicer: a mesh, and the settings a slice is made with.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef GS_H
#define GS_H

#include <stdio.h>

#define GS_VERSION "0.1.0"
#define GS_DATE "8.11.2026"
#define GS_VERSTR "$VER: GFOpenSlicer 0.1.0 (8.11.2026) AmiCradle Team"

typedef struct {
    float x, y, z;
} gs_v;

typedef struct {
    gs_v a, b, c;
} gs_tri;

typedef struct {
    gs_tri *t;
    int n, cap;
    gs_v min, max;
} gs_mesh;

/* 0, or -1 with why in err. */
int gs_stl_load(gs_mesh *m, const char *path, char *err, int errlen);
void gs_mesh_free(gs_mesh *m);
/* Welds vertices, drops empty triangles, and turns the outside out. */
void gs_mesh_repair(gs_mesh *m);

#endif
