/* The suite's shared view UI: see omview_ui.h.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "omview_ui.h"

#include <stdio.h>
#include <stdlib.h>

void omv_say(olui *ui, const char *text)
{
    olui_set_text(ui, olui_find(ui, "status"), text);
}

float omv_number(olui *ui, const char *id)
{
    olui_el *el = olui_find(ui, id);
    int dec, v;
    float d = 1.f;
    if (!el) return 0.f;
    dec = olui_number_decimals(el);
    v = olui_number_value(el);
    while (dec-- > 0) d *= 10.f;
    return (float)v / d;
}

int omv_slider_of(int l, int n)
{
    if (n <= 1) return 1;
    if (l < 1) l = 1;
    if (l > n) l = n;
    return 1 + ((l - 1) * 99 + (n - 1) / 2) / (n - 1);
}

int omv_layer_of(int slider, int n)
{
    if (n <= 1) return 1;
    if (slider < 1) slider = 1;
    if (slider > 100) slider = 100;
    return 1 + ((slider - 1) * (n - 1) + 49) / 99;
}

int omv_pens(const unsigned char *rgb, int w, int h, omv_put_fn put, void *ud)
{
    gs_run *runs = (gs_run *)malloc((size_t)w * sizeof *runs);
    int row, k, n, total = 0;
    if (!runs) return -1;
    for (row = 0; row < h; row++) {
        n = gs_pen_row(rgb + (size_t)row * (size_t)w * 3, w, runs);
        for (k = 0; k < n; k++) put(ud, runs[k].x, row, runs[k].n, runs[k].pen);
        total += n;
    }
    free(runs);
    return total;
}

void omv_slice_status(char *buf, int len, int layers, int moves, float seconds, float filament, int upto)
{
    long min = (long)(seconds / 60.f + 0.5f);
    int n = snprintf(buf, (size_t)len, "Sliced: %d layers, %d moves, about %ld h %02ld min, %.2f m of filament.", layers,
                     moves, min / 60, min % 60, (double)(filament / 1000.f));
    if (upto > 0 && n > 0 && n < len) snprintf(buf + n, (size_t)(len - n), " Layers 1 to %d.", upto);
}
