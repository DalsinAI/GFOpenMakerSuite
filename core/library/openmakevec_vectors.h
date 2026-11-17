/* Written by core/library/gen-library.py from openmakevec_lib.sfd: do not edit.
 * The library's side of each call: its registers to the C function. */
#include "omlib.h"
#include <gs_vec.h>

static void LIB_GV_DocInit(REG(a0, gv_doc *d), REG(a6, struct Library *base))
{
    (void)base;
    gv_doc_init(d);
}

static void LIB_GV_DocFree(REG(a0, gv_doc *d), REG(a6, struct Library *base))
{
    (void)base;
    gv_doc_free(d);
}

static int LIB_GV_LoadSvg(REG(a0, gv_doc *d), REG(a1, const char *path), REG(a2, char *err), REG(d0, int errlen), REG(a6, struct Library *base))
{
    (void)base;
    return gv_load_svg(d, path, err, errlen);
}

static int LIB_GV_LoadSvgText(REG(a0, gv_doc *d), REG(a1, const char *text), REG(d0, int n), REG(a2, char *err), REG(d1, int errlen), REG(a6, struct Library *base))
{
    (void)base;
    return gv_load_svg_text(d, text, n, err, errlen);
}

static unsigned LIB_GV_PathColour(REG(a0, const gv_path *p), REG(a6, struct Library *base))
{
    (void)base;
    return gv_path_colour(p);
}

static void LIB_GV_PathBox(REG(a0, const gv_path *p), REG(a1, float *minx), REG(a2, float *miny), REG(a3, float *maxx), REG(d0, float *maxy), REG(a6, struct Library *base))
{
    (void)base;
    gv_path_box(p, minx, miny, maxx, maxy);
}

static int LIB_GV_LoadDxf(REG(a0, gv_doc *d), REG(a1, const char *path), REG(a2, char *err), REG(d0, int errlen), REG(a6, struct Library *base))
{
    (void)base;
    return gv_load_dxf(d, path, err, errlen);
}

static int LIB_GV_LoadDxfText(REG(a0, gv_doc *d), REG(a1, const char *text), REG(d0, int n), REG(a2, char *err), REG(d1, int errlen), REG(a6, struct Library *base))
{
    (void)base;
    return gv_load_dxf_text(d, text, n, err, errlen);
}

static int LIB_GV_Load(REG(a0, gv_doc *d), REG(a1, const char *path), REG(a2, char *err), REG(d0, int errlen), REG(a6, struct Library *base))
{
    (void)base;
    return gv_load(d, path, err, errlen);
}

static int LIB_GV_LoadText(REG(a0, gv_doc *d), REG(a1, const char *text), REG(d0, int n), REG(a2, char *err), REG(d1, int errlen), REG(a6, struct Library *base))
{
    (void)base;
    return gv_load_text(d, text, n, err, errlen);
}

#define OPENMAKEVEC_LAST_BIAS 84
#define OPENMAKEVEC_VECTORS \
    (APTR)LIB_GV_DocInit, \
    (APTR)LIB_GV_DocFree, \
    (APTR)LIB_GV_LoadSvg, \
    (APTR)LIB_GV_LoadSvgText, \
    (APTR)LIB_GV_PathColour, \
    (APTR)LIB_GV_PathBox, \
    (APTR)LIB_GV_LoadDxf, \
    (APTR)LIB_GV_LoadDxfText, \
    (APTR)LIB_GV_Load, \
    (APTR)LIB_GV_LoadText
