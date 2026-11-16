/* OpenMakeSlicer's slice from the shell: a part to G-code for a machine,
 * checked against the machine before it is written.
 *   openmake-slice in.stl out.gcode DRIVER=driver.om PROFILE=profile.om
 *       [LAYER=0.2] [LINE=0.4] [WALLS=2] [SKINS=3] [INFILL=20] [BRIM=3]
 *       [HOTEND=200] [BED=60] [PREVIEW=picture.ppm] [SIZE=800x600] [ZOOM=1] [UPTO=0] [VIEW=part]
 * PREVIEW also draws the toolpath (gt_paint) to a PPM picture: ZOOM times
 * nearer than the home view, layers 1 to UPTO (0: all), framed to the part
 * (VIEW=part) or to the plate (VIEW=plate).
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gs_gcode.h"
#include "gs_plan.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *arg(int argc, char **argv, const char *name)
{
    size_t n = strlen(name);
    int i;
    for (i = 3; i < argc; i++)
        if (strncmp(argv[i], name, n) == 0 && argv[i][n] == '=') return argv[i] + n + 1;
    return NULL;
}

static void num(int argc, char **argv, const char *name, float *out)
{
    const char *v = arg(argc, argv, name);
    if (v) *out = (float)atof(v);
}

static void whole(int argc, char **argv, const char *name, int *out)
{
    const char *v = arg(argc, argv, name);
    if (v) *out = atoi(v);
}

/* The toolpath drawn to a PPM picture. 0, or -1. */
static int preview(const gt_path *p, const char *path, const char *size, float zoom, int upto, int part)
{
    gs_cam cam;
    int w = 800, h = 600, rc = -1;
    unsigned char *rgb;
    FILE *f;
    if (size && (sscanf(size, "%dx%d", &w, &h) != 2 || w < 16 || h < 16 || w > 4096 || h > 4096)) {
        fprintf(stderr, "openmake-slice: SIZE is WIDTHxHEIGHT, 16 to 4096\n");
        return -1;
    }
    gs_cam_home(&cam);
    if (zoom > 0.f) gs_cam_zoom(&cam, zoom);
    rgb = (unsigned char *)malloc((size_t)w * (size_t)h * 3);
    if (rgb && (part ? gt_paint_part : gt_paint)(p, &cam, upto, rgb, w, h) == 0 && (f = fopen(path, "wb")) != NULL) {
        fprintf(f, "P6\n%d %d\n255\n", w, h);
        rc = fwrite(rgb, 3, (size_t)w * (size_t)h, f) == (size_t)w * (size_t)h ? 0 : -1;
        if (fclose(f)) rc = -1;
    }
    if (rc) fprintf(stderr, "openmake-slice: %s: the preview could not be drawn or written\n", path);
    else printf("%s: the toolpath, %dx%d\n", path, w, h);
    free(rgb);
    return rc;
}

int main(int argc, char **argv)
{
    gs_mesh part;
    om_machine m;
    gp_opts po;
    gc_opts go;
    gc_stats st;
    om_job job;
    gt_path path;
    char err[160];
    const char *driver = arg(argc, argv, "DRIVER"), *profile = arg(argc, argv, "PROFILE");
    FILE *out;
    int check, rc = 1;
    if (argc < 3 || !driver) {
        fprintf(stderr, "usage: openmake-slice in.stl out.gcode DRIVER=driver.om [PROFILE=profile.om] [LAYER=0.2] [LINE=0.4]\n"
                        "       [WALLS=2] [SKINS=3] [INFILL=20] [BRIM=3] [HOTEND=200] [BED=60] [PREVIEW=out.ppm] [SIZE=800x600]\n"
                        "       [ZOOM=1] [UPTO=0] [VIEW=part|plate]\n");
        return 2;
    }
    gp_defaults(&po);
    gc_defaults(&go);
    num(argc, argv, "LAYER", &po.layer_h);
    num(argc, argv, "LINE", &po.line_w);
    num(argc, argv, "INFILL", &po.infill);
    whole(argc, argv, "WALLS", &po.walls);
    whole(argc, argv, "SKINS", &po.skins);
    whole(argc, argv, "BRIM", &po.brim);
    num(argc, argv, "HOTEND", &go.hotend);
    num(argc, argv, "BED", &go.bed);
    po.layer_h = gl_height(po.layer_h);
    go.layer_h = po.layer_h;

    om_clear(&m);
    if (om_load(&m, driver, err, (int)sizeof err) || (profile && om_load(&m, profile, err, (int)sizeof err))) {
        fprintf(stderr, "openmake-slice: %s\n", err);
        return 1;
    }
    if (gs_stl_load(&part, argv[1], err, (int)sizeof err)) {
        fprintf(stderr, "openmake-slice: %s: %s\n", argv[1], err);
        return 1;
    }
    gs_mesh_repair(&part);
    gp_place(&part, m.area[0], m.area[1]);
    gt_init(&path);
    if (gp_part(&path, &part, &po)) {
        fprintf(stderr, "openmake-slice: out of memory\n");
        goto done;
    }
    /* The machine first: a job it cannot do is not written. Speeds are
     * held to the machine's as they are written, so never too fast. */
    gc_job(&path, &m, &go, &job);
    check = om_check(&m, &job, err, (int)sizeof err);
    if (check & ~OM_TOO_FAST) {
        fprintf(stderr, "openmake-slice: %s cannot print this: %s\n", m.name, err);
        goto done;
    }
    out = fopen(argv[2], "w");
    if (!out) {
        fprintf(stderr, "openmake-slice: %s: cannot write\n", argv[2]);
        goto done;
    }
    rc = gc_write(out, &path, &m, &go, &st, err, (int)sizeof err);
    if (fclose(out) || rc) {
        fprintf(stderr, "openmake-slice: %s\n", rc ? err : "cannot write");
        rc = 1;
        goto done;
    }
    printf("%s: %d layers, %d moves for %s\n", argv[2], path.nlayers, st.moves, m.name);
    printf("about %d h %02d min at the set speeds (no acceleration counted), %.2f m of %.2f mm filament (%.0f g of PLA)\n",
           (int)(st.seconds / 3600.f), (int)(st.seconds / 60.f) % 60, (double)(st.filament / 1000.f), (double)go.filament,
           (double)(st.filament * 3.14159265f * go.filament * go.filament * 0.25f * 1.24e-3f));
    if (arg(argc, argv, "PREVIEW")) {
        float zoom = 0.f;
        int upto = 0;
        num(argc, argv, "ZOOM", &zoom);
        whole(argc, argv, "UPTO", &upto);
        const char *view = arg(argc, argv, "VIEW");
        if (preview(&path, arg(argc, argv, "PREVIEW"), arg(argc, argv, "SIZE"), zoom, upto, !view || strcmp(view, "plate")))
            rc = 1;
    }
done:
    gt_free(&path);
    gs_mesh_free(&part);
    return rc;
}
