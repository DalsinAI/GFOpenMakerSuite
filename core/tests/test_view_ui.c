/* The suite's shared view UI (core/ui/omview_ui.h) on the host, in a small
 * Clear window laid out by the real GFOpenLayout.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "omview_ui.h"

#include "gs_model.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fails;

static void expect(int ok, const char *what)
{
    if (!ok) {
        printf("fail: %s\n", what);
        fails++;
    }
}

static int measure(void *userdata, const char *utf8, size_t length, const ol_style *style, ol_text_metrics *metrics)
{
    (void)userdata;
    (void)utf8;
    metrics->width = (ol_unit)(length * (size_t)style->font_size * 60 / 100);
    metrics->ascent = style->font_size * 80 / 100;
    metrics->descent = style->font_size * 22 / 100;
    return 1;
}

/* The runs as a window would fill them: into a picture of pens. */
#define W 80
#define H 60
static unsigned char pens[H][W];
static int filled;

static void put(void *ud, int x, int y, int n, int pen)
{
    int i;
    (void)ud;
    for (i = 0; i < n; i++) pens[y][x + i] = (unsigned char)pen;
    filled += n;
}

int main(void)
{
    static const char markup[] =
        "<row><label for=\"lh\">Height</label><spinner id=\"lh\" value=\"0.20\" min=\"0.08\" max=\"0.80\" step=\"0.01\"></row>"
        "<div id=\"view\"></div><div id=\"status\">ready</div>";
    static unsigned char rgb[W * H * 3];
    olui *ui = olui_new();
    char err[160];
    gs_mesh box;
    gs_cam cam;
    int n, l, ok = 1, x, y, runs;

    olui_set_measure(ui, measure, NULL);
    expect(olui_load(ui, markup, "", err, sizeof err), "the window loads");
    omv_say(ui, "Sliced: 130 layers");
    expect(!strcmp(olui_get_text(olui_find(ui, "status")), "Sliced: 130 layers"), "omv_say sets the status line");
    expect(omv_number(ui, "lh") == 0.2f, "omv_number reads the spinner, 0.20");
    expect(omv_number(ui, "nothing") == 0.f, "and 0 for no such field");

    for (n = 1; n <= 300; n++)
        for (l = 1; l <= n; l++) {
            int s = omv_slider_of(l, n);
            if (s < 1 || s > 100 || (n <= 100 && omv_layer_of(s, n) != l)) ok = 0;
        }
    expect(ok, "the slider's 1 to 100 along the layers, and back while each layer has its own place");
    expect(omv_slider_of(1, 130) == 1 && omv_slider_of(130, 130) == 100 && omv_layer_of(100, 130) == 130 &&
               omv_layer_of(1, 1) == 1,
           "the ends are the ends");

    {
        char line[160];
        omv_slice_status(line, (int)sizeof line, 130, 161919, 5160.f, 5610.f, 0);
        expect(!strcmp(line, "Sliced: 130 layers, 161919 moves, 1 h 26 min, 5.61 m filament."),
               "omv_slice_status, all the layers shown");
        omv_slice_status(line, (int)sizeof line, 130, 161919, 5160.f, 5610.f, 40);
        expect(!strcmp(line, "Sliced: layers 1 to 40 of 130, 161919 moves, 1 h 26 min, 5.61 m filament."),
               "and layers 1 to 40");
        omv_slice_status(line, (int)sizeof line, 100, 42090, 1200.f, 1170.f, 50);
        expect(!strcmp(line, "Sliced: layers 1 to 50 of 100, 42090 moves, 20 min, 1.17 m filament."),
               "under an hour, minutes only");
        /* The status line is 624 px (640 less the padding): 78 characters
         * of Topaz's 8 px, 86 at the host's 7.2. A long print still fits. */
        omv_slice_status(line, (int)sizeof line, 1000, 999999, 99.f * 3600.f + 59.f * 60.f, 99990.f, 1000);
        expect(strlen(line) <= 78, "a long print's status fits the line, 78 characters");
        printf("%s\n", line);
    }

    memset(&box, 0, sizeof box);
    expect(gm_cube(&box, 90, 90, 0, 40, 30, 20) == 0, "a box");
    gs_cam_home(&cam);
    expect(gs_view_bed(&box, &cam, rgb, W, H) == 0, "its picture");
    runs = omv_pens(rgb, W, H, put, NULL);
    for (y = 0; y < H; y++)
        for (x = 0; x < W; x++)
            if (pens[y][x] != gs_pen_of(rgb + (y * W + x) * 3)) ok = 0;
    expect(ok && filled == W * H && runs < W * H / 4, "omv_pens: every pixel its pen, in far fewer runs");
    printf("pens: %d runs for %d pixels\n", runs, W * H);

    gs_mesh_free(&box);
    olui_free(ui);
    if (fails) {
        printf("%d failed\n", fails);
        return 1;
    }
    printf("view ui ok\n");
    return 0;
}
