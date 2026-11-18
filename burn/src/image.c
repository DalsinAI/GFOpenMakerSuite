/* Netpbm scan path (burn_image.h). The file is read in image_load.c.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "burn_image.h"

#include <stddef.h>
#include <stdlib.h>

static int opts_have(const burn_opts *o, size_t off, size_t sz)
{
    return o && o->size >= (int)(off + sz);
}

float burn_image_px(const burn_opts *o, int cols)
{
    float w = 0.f, gap;
    if (cols < 1) return 0.2f;
    if (opts_have(o, offsetof(burn_opts, width), sizeof o->width)) w = o->width;
    if (w > 0.f) return w / (float)cols;
    gap = o ? o->interval : 0.f;
    if (gap <= 0.f) gap = 0.2f;
    if (gap < 0.01f) gap = 0.01f;
    return gap;
}

static int grow_speed(float **spd, int *cap, int need)
{
    int ncap;
    float *n;
    if (need <= *cap) return 0;
    ncap = *cap ? *cap : 64;
    while (ncap < need) ncap = ncap * 2 + 32;
    n = (float *)realloc(*spd, (size_t)ncap * sizeof *n);
    if (!n) return -1;
    *spd = n;
    *cap = ncap;
    return 0;
}

static int tag_speeds(float *spd, int *nspd, const gt_path *p, int start, float vcut)
{
    int i;
    for (i = start; i < p->n; i++) spd[(*nspd)++] = (p->m[i].kind == GT_CUT) ? vcut : 0.f;
    return *nspd == p->n ? 0 : -1;
}

static int add_move(gt_path *p, float **spd, int *nspd, int *cap, int kind, float x, float y, float power, float vcut)
{
    int start = p->n;
    if (gt_add(p, kind, x, y, 0.f, kind == GT_CUT ? power : 0.f)) return -1;
    if (spd && *spd) {
        if (grow_speed(spd, cap, p->n)) return -1;
        if (tag_speeds(*spd, nspd, p, start, vcut)) return -1;
    }
    return 0;
}

int burn_image_path(gt_path *p, float **speed, const burn_image *im, const burn_opts *o)
{
    float *spd = NULL;
    int cap = 0, nspd = 0, pass, row, col, passes, invert = 0;
    float px, power0, vcut;
    if (speed) *speed = NULL;
    gt_free(p);
    gt_init(p);
    if (!im || !im->grey || im->cols < 1 || im->rows < 1) return -1;
    px = burn_image_px(o, im->cols);
    power0 = (o && o->power > 0.f) ? o->power : 50.f;
    vcut = (o && o->speed > 0.f) ? o->speed : 20.f;
    passes = (o && o->passes > 1) ? o->passes : 1;
    if (opts_have(o, offsetof(burn_opts, invert), sizeof o->invert)) invert = o->invert ? 1 : 0;
    if (speed) {
        cap = 64;
        spd = (float *)malloc((size_t)cap * sizeof *spd);
        if (!spd) return -1;
        *speed = spd;
    }
    for (pass = 0; pass < passes; pass++) {
        for (row = 0; row < im->rows; row++) {
            int rtl = row & 1;
            float y = (float)(im->rows - 1 - row) * px;
            int start_c = rtl ? im->cols - 1 : 0;
            int step = rtl ? -1 : 1;
            int last_p = -1;
            for (col = start_c; (step > 0) ? (col < im->cols) : (col >= 0); col += step) {
                float g = im->grey[row * im->cols + col];
                int pw;
                float x0, x1;
                if (invert) g = 1.f - g;
                if (g < 0.f) g = 0.f;
                if (g > 1.f) g = 1.f;
                pw = (int)(g * power0 + 0.5f);
                if (pw < 1) pw = 0;
                if (pw > 100) pw = 100;
                x0 = (float)col * px;
                x1 = (float)(col + 1) * px;
                if (rtl) {
                    float t = x0;
                    x0 = x1;
                    x1 = t;
                }
                if (!pw) {
                    last_p = -1;
                    continue;
                }
                if (pw != last_p) {
                    if (add_move(p, speed ? &spd : NULL, &nspd, &cap, GT_TRAVEL, x0, y, 0.f, vcut)) goto fail;
                    if (add_move(p, speed ? &spd : NULL, &nspd, &cap, GT_CUT, x1, y, (float)pw, vcut)) goto fail;
                    last_p = pw;
                } else {
                    /* Extend the last cut to this pixel's far edge. */
                    if (p->n > 0 && p->m[p->n - 1].kind == GT_CUT) {
                        p->m[p->n - 1].x = x1;
                    } else {
                        if (add_move(p, speed ? &spd : NULL, &nspd, &cap, GT_TRAVEL, x0, y, 0.f, vcut)) goto fail;
                        if (add_move(p, speed ? &spd : NULL, &nspd, &cap, GT_CUT, x1, y, (float)pw, vcut)) goto fail;
                    }
                }
            }
        }
    }
    if (speed) {
        *speed = spd;
        if (p->n == 0) {
            free(spd);
            *speed = NULL;
        }
    }
    if (o && burn_place_path(p, o->x, o->y, o->rot)) goto fail;
    return 0;
fail:
    free(spd);
    if (speed) *speed = NULL;
    gt_free(p);
    return -1;
}

int burn_image_doc(gv_doc *d, const burn_image *im, const burn_opts *o)
{
    float px, w, h;
    gv_layer *ly;
    if (!d || !im || im->cols < 1 || im->rows < 1) return -1;
    gv_doc_free(d);
    gv_doc_init(d);
    px = burn_image_px(o, im->cols);
    w = px * (float)im->cols;
    h = px * (float)im->rows;
    ly = (gv_layer *)calloc(1, sizeof *ly);
    if (!ly) return -1;
    ly->colour = 0;
    ly->n = 0;
    d->layer = ly;
    d->nlayer = 1;
    d->width = w;
    d->height = h;
    d->minx = 0.f;
    d->miny = 0.f;
    d->maxx = w;
    d->maxy = h;
    return 0;
}

