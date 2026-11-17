/* Raster engraving: a Netpbm PBM/PGM/PPM (plain or raw, 8-bit) to
 * scan-line cuts. Black is more power; travels stay at 0. Written from
 * the public Netpbm format, never from another application's source.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef BURN_IMAGE_H
#define BURN_IMAGE_H

#include "burn_job.h"
#include "gs_toolpath.h"

#define BURN_IMAGE_MAX 2048

typedef struct {
    int size;
    int cols, rows;
    float *grey; /* 0 white, 1 black, row 0 is the top of the picture */
} burn_image;

void burn_image_init(burn_image *im);
void burn_image_free(burn_image *im);

/* 1 if path looks like a Netpbm bitmap (suffix or the P1..P6 magic). */
int burn_is_image(const char *path);

/* Replaces im. maxval must be 1..255. 0, or -1 with why in err. */
int burn_image_load(burn_image *im, const char *path, char *err, int errlen);

/* Pixel size in mm: o->width / cols when width is set, else interval
 * (0: 0.2 mm, at least 0.01). */
float burn_image_px(const burn_opts *o, int cols);

/* Replaces p with left-to-right then right-to-left scan cuts. A pixel's
 * power is its grey times the job's power (invert swaps black and white).
 * Travels are at power 0. 0, or -1. */
int burn_image_path(gt_path *p, float **speed, const burn_image *im, const burn_opts *o);

/* A one-layer document for the window, bounds the picture, no paths. */
int burn_image_doc(gv_doc *d, const burn_image *im, const burn_opts *o);

#endif
