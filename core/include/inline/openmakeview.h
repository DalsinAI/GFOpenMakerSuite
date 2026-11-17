/* Written by core/library/gen-library.py from openmakeview_lib.sfd: do not edit. */
#ifndef INLINE_OPENMAKEVIEW_H
#define INLINE_OPENMAKEVIEW_H

#include "omlib.h"
#include <gs_view.h>
#include <gs_layer.h>
#include <gs_toolpath.h>

extern struct Library *OpenMakeViewBase;

static __inline__ void GS_CamHome(gs_cam *c)
{
#ifdef OM_LIB_HOST
    ((void (*)(gs_cam *, struct Library *))om_host_vector(OpenMakeViewBase, 30))(c, OpenMakeViewBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeViewBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(c);
    __asm__ volatile("jsr a6@(-30:W)" : "=r"(_d0), "+r"(_a0) : "r"(_a6) : "d1", "a1", "fp0", "fp1", "cc", "memory");
#endif
}

static __inline__ void GS_CamOrbit(gs_cam *c, float dx, float dy)
{
#ifdef OM_LIB_HOST
    ((void (*)(gs_cam *, ULONG, ULONG, struct Library *))om_host_vector(OpenMakeViewBase, 36))(c, om_lib_bits(dx), om_lib_bits(dy), OpenMakeViewBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeViewBase;
    register ULONG _a0 __asm("a0") = (ULONG)(c);
    register ULONG _d0 __asm("d0") = om_lib_bits(dx);
    register ULONG _d1 __asm("d1") = om_lib_bits(dy);
    __asm__ volatile("jsr a6@(-36:W)" : "+r"(_d0), "+r"(_d1), "+r"(_a0) : "r"(_a6) : "a1", "fp0", "fp1", "cc", "memory");
#endif
}

static __inline__ void GS_CamPan(gs_cam *c, float dx, float dy)
{
#ifdef OM_LIB_HOST
    ((void (*)(gs_cam *, ULONG, ULONG, struct Library *))om_host_vector(OpenMakeViewBase, 42))(c, om_lib_bits(dx), om_lib_bits(dy), OpenMakeViewBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeViewBase;
    register ULONG _a0 __asm("a0") = (ULONG)(c);
    register ULONG _d0 __asm("d0") = om_lib_bits(dx);
    register ULONG _d1 __asm("d1") = om_lib_bits(dy);
    __asm__ volatile("jsr a6@(-42:W)" : "+r"(_d0), "+r"(_d1), "+r"(_a0) : "r"(_a6) : "a1", "fp0", "fp1", "cc", "memory");
#endif
}

static __inline__ void GS_CamZoom(gs_cam *c, float factor)
{
#ifdef OM_LIB_HOST
    ((void (*)(gs_cam *, ULONG, struct Library *))om_host_vector(OpenMakeViewBase, 48))(c, om_lib_bits(factor), OpenMakeViewBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeViewBase;
    register ULONG _a0 __asm("a0") = (ULONG)(c);
    register ULONG _d0 __asm("d0") = om_lib_bits(factor);
    __asm__ volatile("jsr a6@(-48:W)" : "+r"(_d0), "+r"(_a0) : "r"(_a6) : "d1", "a1", "fp0", "fp1", "cc", "memory");
#endif
}

static __inline__ int GS_ViewFrame(const gs_mesh *m, const gs_cam *cam, int w, int h, gs_frame *f)
{
#ifdef OM_LIB_HOST
    return ((int (*)(const gs_mesh *, const gs_cam *, int, int, gs_frame *, struct Library *))om_host_vector(OpenMakeViewBase, 54))(m, cam, w, h, f, OpenMakeViewBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeViewBase;
    register ULONG _a0 __asm("a0") = (ULONG)(m);
    register ULONG _a1 __asm("a1") = (ULONG)(cam);
    register ULONG _d0 __asm("d0") = (ULONG)(w);
    register ULONG _d1 __asm("d1") = (ULONG)(h);
    register ULONG _a2 __asm("a2") = (ULONG)(f);
    __asm__ volatile("jsr a6@(-54:W)" : "+r"(_d0), "+r"(_d1), "+r"(_a0), "+r"(_a1) : "r"(_a6), "r"(_a2) : "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GS_ViewFrameBed(const gs_mesh *m, const gs_cam *cam, int w, int h, float x0, float y0, float x1, float y1, gs_frame *f)
{
#ifdef OM_LIB_HOST
    return ((int (*)(const gs_mesh *, const gs_cam *, int, int, ULONG, ULONG, ULONG, ULONG, gs_frame *, struct Library *))om_host_vector(OpenMakeViewBase, 60))(m, cam, w, h, om_lib_bits(x0), om_lib_bits(y0), om_lib_bits(x1), om_lib_bits(y1), f, OpenMakeViewBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeViewBase;
    register ULONG _a0 __asm("a0") = (ULONG)(m);
    register ULONG _a1 __asm("a1") = (ULONG)(cam);
    register ULONG _d0 __asm("d0") = (ULONG)(w);
    register ULONG _d1 __asm("d1") = (ULONG)(h);
    register ULONG _d2 __asm("d2") = om_lib_bits(x0);
    register ULONG _d3 __asm("d3") = om_lib_bits(y0);
    register ULONG _d4 __asm("d4") = om_lib_bits(x1);
    register ULONG _d5 __asm("d5") = om_lib_bits(y1);
    register ULONG _a2 __asm("a2") = (ULONG)(f);
    __asm__ volatile("jsr a6@(-60:W)" : "+r"(_d0), "+r"(_d1), "+r"(_a0), "+r"(_a1) : "r"(_a6), "r"(_d2), "r"(_d3), "r"(_d4), "r"(_d5), "r"(_a2) : "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GS_ViewSolid(const gs_mesh *m, const gs_cam *cam, unsigned char *rgb, int w, int h)
{
#ifdef OM_LIB_HOST
    return ((int (*)(const gs_mesh *, const gs_cam *, unsigned char *, int, int, struct Library *))om_host_vector(OpenMakeViewBase, 66))(m, cam, rgb, w, h, OpenMakeViewBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeViewBase;
    register ULONG _a0 __asm("a0") = (ULONG)(m);
    register ULONG _a1 __asm("a1") = (ULONG)(cam);
    register ULONG _a2 __asm("a2") = (ULONG)(rgb);
    register ULONG _d0 __asm("d0") = (ULONG)(w);
    register ULONG _d1 __asm("d1") = (ULONG)(h);
    __asm__ volatile("jsr a6@(-66:W)" : "+r"(_d0), "+r"(_d1), "+r"(_a0), "+r"(_a1) : "r"(_a6), "r"(_a2) : "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GS_ViewBed(const gs_mesh *m, const gs_cam *cam, unsigned char *rgb, int w, int h)
{
#ifdef OM_LIB_HOST
    return ((int (*)(const gs_mesh *, const gs_cam *, unsigned char *, int, int, struct Library *))om_host_vector(OpenMakeViewBase, 72))(m, cam, rgb, w, h, OpenMakeViewBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeViewBase;
    register ULONG _a0 __asm("a0") = (ULONG)(m);
    register ULONG _a1 __asm("a1") = (ULONG)(cam);
    register ULONG _a2 __asm("a2") = (ULONG)(rgb);
    register ULONG _d0 __asm("d0") = (ULONG)(w);
    register ULONG _d1 __asm("d1") = (ULONG)(h);
    __asm__ volatile("jsr a6@(-72:W)" : "+r"(_d0), "+r"(_d1), "+r"(_a0), "+r"(_a1) : "r"(_a6), "r"(_a2) : "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GS_PenOf(const unsigned char *rgb)
{
#ifdef OM_LIB_HOST
    return ((int (*)(const unsigned char *, struct Library *))om_host_vector(OpenMakeViewBase, 78))(rgb, OpenMakeViewBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeViewBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(rgb);
    __asm__ volatile("jsr a6@(-78:W)" : "=r"(_d0), "+r"(_a0) : "r"(_a6) : "d1", "a1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GS_PenRow(const unsigned char *rgb, int w, gs_run *runs)
{
#ifdef OM_LIB_HOST
    return ((int (*)(const unsigned char *, int, gs_run *, struct Library *))om_host_vector(OpenMakeViewBase, 84))(rgb, w, runs, OpenMakeViewBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeViewBase;
    register ULONG _a0 __asm("a0") = (ULONG)(rgb);
    register ULONG _d0 __asm("d0") = (ULONG)(w);
    register ULONG _a1 __asm("a1") = (ULONG)(runs);
    __asm__ volatile("jsr a6@(-84:W)" : "+r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6) : "d1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GS_ViewOnGpu(void)
{
#ifdef OM_LIB_HOST
    return ((int (*)(struct Library *))om_host_vector(OpenMakeViewBase, 90))(OpenMakeViewBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeViewBase;
    register ULONG _d0 __asm("d0");
    __asm__ volatile("jsr a6@(-90:W)" : "=r"(_d0) : "r"(_a6) : "d1", "a0", "a1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GL_Paint(const gl_walls *w, const gs_mesh *m, const gs_cam *cam, unsigned char *rgb, int width, int height)
{
#ifdef OM_LIB_HOST
    return ((int (*)(const gl_walls *, const gs_mesh *, const gs_cam *, unsigned char *, int, int, struct Library *))om_host_vector(OpenMakeViewBase, 96))(w, m, cam, rgb, width, height, OpenMakeViewBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeViewBase;
    register ULONG _a0 __asm("a0") = (ULONG)(w);
    register ULONG _a1 __asm("a1") = (ULONG)(m);
    register ULONG _a2 __asm("a2") = (ULONG)(cam);
    register ULONG _a3 __asm("a3") = (ULONG)(rgb);
    register ULONG _d0 __asm("d0") = (ULONG)(width);
    register ULONG _d1 __asm("d1") = (ULONG)(height);
    __asm__ volatile("jsr a6@(-96:W)" : "+r"(_d0), "+r"(_d1), "+r"(_a0), "+r"(_a1) : "r"(_a6), "r"(_a2), "r"(_a3) : "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ unsigned long GT_Colour(int kind)
{
#ifdef OM_LIB_HOST
    return ((unsigned long (*)(int, struct Library *))om_host_vector(OpenMakeViewBase, 102))(kind, OpenMakeViewBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeViewBase;
    register ULONG _d0 __asm("d0") = (ULONG)(kind);
    __asm__ volatile("jsr a6@(-102:W)" : "+r"(_d0) : "r"(_a6) : "d1", "a0", "a1", "fp0", "fp1", "cc", "memory");
    return (unsigned long)_d0;
#endif
}

static __inline__ unsigned long GT_Shade(int kind, float z, float top)
{
#ifdef OM_LIB_HOST
    return ((unsigned long (*)(int, ULONG, ULONG, struct Library *))om_host_vector(OpenMakeViewBase, 108))(kind, om_lib_bits(z), om_lib_bits(top), OpenMakeViewBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeViewBase;
    register ULONG _d0 __asm("d0") = (ULONG)(kind);
    register ULONG _d1 __asm("d1") = om_lib_bits(z);
    register ULONG _d2 __asm("d2") = om_lib_bits(top);
    __asm__ volatile("jsr a6@(-108:W)" : "+r"(_d0), "+r"(_d1) : "r"(_a6), "r"(_d2) : "a0", "a1", "fp0", "fp1", "cc", "memory");
    return (unsigned long)_d0;
#endif
}

static __inline__ int GT_End(const gt_path *p, int upto)
{
#ifdef OM_LIB_HOST
    return ((int (*)(const gt_path *, int, struct Library *))om_host_vector(OpenMakeViewBase, 114))(p, upto, OpenMakeViewBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeViewBase;
    register ULONG _a0 __asm("a0") = (ULONG)(p);
    register ULONG _d0 __asm("d0") = (ULONG)(upto);
    __asm__ volatile("jsr a6@(-114:W)" : "+r"(_d0), "+r"(_a0) : "r"(_a6) : "d1", "a1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GT_Frame(const gt_path *p, const gs_cam *cam, int part, int w, int h, gs_frame *f)
{
#ifdef OM_LIB_HOST
    return ((int (*)(const gt_path *, const gs_cam *, int, int, int, gs_frame *, struct Library *))om_host_vector(OpenMakeViewBase, 120))(p, cam, part, w, h, f, OpenMakeViewBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeViewBase;
    register ULONG _a0 __asm("a0") = (ULONG)(p);
    register ULONG _a1 __asm("a1") = (ULONG)(cam);
    register ULONG _d0 __asm("d0") = (ULONG)(part);
    register ULONG _d1 __asm("d1") = (ULONG)(w);
    register ULONG _d2 __asm("d2") = (ULONG)(h);
    register ULONG _a2 __asm("a2") = (ULONG)(f);
    __asm__ volatile("jsr a6@(-120:W)" : "+r"(_d0), "+r"(_d1), "+r"(_a0), "+r"(_a1) : "r"(_a6), "r"(_d2), "r"(_a2) : "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GT_Paint(const gt_path *p, const gs_cam *cam, int upto, unsigned char *rgb, int w, int h)
{
#ifdef OM_LIB_HOST
    return ((int (*)(const gt_path *, const gs_cam *, int, unsigned char *, int, int, struct Library *))om_host_vector(OpenMakeViewBase, 126))(p, cam, upto, rgb, w, h, OpenMakeViewBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeViewBase;
    register ULONG _a0 __asm("a0") = (ULONG)(p);
    register ULONG _a1 __asm("a1") = (ULONG)(cam);
    register ULONG _d0 __asm("d0") = (ULONG)(upto);
    register ULONG _a2 __asm("a2") = (ULONG)(rgb);
    register ULONG _d1 __asm("d1") = (ULONG)(w);
    register ULONG _d2 __asm("d2") = (ULONG)(h);
    __asm__ volatile("jsr a6@(-126:W)" : "+r"(_d0), "+r"(_d1), "+r"(_a0), "+r"(_a1) : "r"(_a6), "r"(_a2), "r"(_d2) : "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GT_PaintPart(const gt_path *p, const gs_cam *cam, int upto, unsigned char *rgb, int w, int h)
{
#ifdef OM_LIB_HOST
    return ((int (*)(const gt_path *, const gs_cam *, int, unsigned char *, int, int, struct Library *))om_host_vector(OpenMakeViewBase, 132))(p, cam, upto, rgb, w, h, OpenMakeViewBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeViewBase;
    register ULONG _a0 __asm("a0") = (ULONG)(p);
    register ULONG _a1 __asm("a1") = (ULONG)(cam);
    register ULONG _d0 __asm("d0") = (ULONG)(upto);
    register ULONG _a2 __asm("a2") = (ULONG)(rgb);
    register ULONG _d1 __asm("d1") = (ULONG)(w);
    register ULONG _d2 __asm("d2") = (ULONG)(h);
    __asm__ volatile("jsr a6@(-132:W)" : "+r"(_d0), "+r"(_d1), "+r"(_a0), "+r"(_a1) : "r"(_a6), "r"(_a2), "r"(_d2) : "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

#endif
