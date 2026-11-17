/* Written by core/library/gen-library.py from openmakevec_lib.sfd: do not edit.
 * The C API through openmakevec.library: a program links this instead of
 * the sources, and opens OpenMakeVecBase itself. */
#include <proto/openmakevec.h>

void gv_doc_init(gv_doc *d)
{
    GV_DocInit(d);
}

void gv_doc_free(gv_doc *d)
{
    GV_DocFree(d);
}

int gv_load_svg(gv_doc *d, const char *path, char *err, int errlen)
{
    return GV_LoadSvg(d, path, err, errlen);
}

int gv_load_svg_text(gv_doc *d, const char *text, int n, char *err, int errlen)
{
    return GV_LoadSvgText(d, text, n, err, errlen);
}

unsigned gv_path_colour(const gv_path *p)
{
    return GV_PathColour(p);
}

void gv_path_box(const gv_path *p, float *minx, float *miny, float *maxx, float *maxy)
{
    GV_PathBox(p, minx, miny, maxx, maxy);
}

int gv_load_dxf(gv_doc *d, const char *path, char *err, int errlen)
{
    return GV_LoadDxf(d, path, err, errlen);
}

int gv_load_dxf_text(gv_doc *d, const char *text, int n, char *err, int errlen)
{
    return GV_LoadDxfText(d, text, n, err, errlen);
}

int gv_load(gv_doc *d, const char *path, char *err, int errlen)
{
    return GV_Load(d, path, err, errlen);
}

int gv_load_text(gv_doc *d, const char *text, int n, char *err, int errlen)
{
    return GV_LoadText(d, text, n, err, errlen);
}
