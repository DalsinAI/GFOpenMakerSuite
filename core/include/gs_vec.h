/* 2D vector drawings, an SVG reader (W3C SVG 1.1: paths, basic shapes,
 * paint, coordinates and the path grammar), and an ASCII DXF reader
 * (Autodesk's published DXF reference: lines, arcs, circles, polylines
 * with bulge, blocks, colours and units). Written from those public
 * specifications, never from another application's source.
 * Coordinates are millimetres, origin at the bottom-left of the view box,
 * Y up. The Amiga build is to be openmakevec.library; this is its C API.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef GS_VEC_H
#define GS_VEC_H

/* No paint (stroke or fill set to none). 0 is black, a real colour. */
#define GV_NONE 0xFFFFFFFFu

/* A path: n points, x then y, in millimetres. A closed path's last joins
 * its first. stroke and fill are 0x00RRGGBB, or GV_NONE if none. width is
 * the stroke in millimetres, 0 if there is no stroke. */
typedef struct {
    float *xy;
    int n, closed;
    unsigned stroke, fill;
    float width;
} gv_path;

/* The paths of one colour, as they first appear in the drawing. A path
 * belongs to the layer of its stroke, or of its fill if it has no stroke
 * (gv_path_colour). */
typedef struct {
    unsigned colour;
    int n;
} gv_layer;

/* A drawing. size is sizeof(gv_doc) as the program was built. A program
 * calls gv_doc_init before a load; a load replaces what was there. */
typedef struct {
    int size;
    gv_path *p;
    int n, cap;
    gv_layer *layer;
    int nlayer;
    float width, height; /* the view box, millimetres */
    float minx, miny, maxx, maxy;
} gv_doc;

void gv_doc_init(gv_doc *d);
void gv_doc_free(gv_doc *d);

/* Replaces d with the SVG at path, or the n bytes at text (n may be -1
 * when text has a 0 at the end). 0, or -1 with why in err. A CSS style
 * element, text that is still a font, pictures, and a <use> of another
 * element are not read. */
int gv_load_svg(gv_doc *d, const char *path, char *err, int errlen);
int gv_load_svg_text(gv_doc *d, const char *text, int n, char *err, int errlen);

/* Replaces d with the ASCII DXF at path, or the n bytes at text (n may
 * be -1 when text has a 0 at the end). 0, or -1 with why in err. Units
 * follow HEADER $INSUNITS (unitless is millimetres). Binary DXF, font
 * text, hatches, splines and ellipses are not read. */
int gv_load_dxf(gv_doc *d, const char *path, char *err, int errlen);
int gv_load_dxf_text(gv_doc *d, const char *text, int n, char *err, int errlen);

/* SVG or ASCII DXF, chosen from the contents (and a .dxf suffix). */
int gv_load(gv_doc *d, const char *path, char *err, int errlen);
int gv_load_text(gv_doc *d, const char *text, int n, char *err, int errlen);

/* The colour that puts p on a layer: its stroke, or its fill if it has
 * no stroke. GV_NONE if it has neither. */
unsigned gv_path_colour(const gv_path *p);

/* The bounds of p's points. A path with no points is all 0. */
void gv_path_box(const gv_path *p, float *minx, float *miny, float *maxx, float *maxy);

#endif
