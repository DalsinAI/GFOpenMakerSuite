/* Written by core/library/gen-library.py from openmakevec_lib.sfd: do not edit. */
#ifndef INLINE_OPENMAKEVEC_H
#define INLINE_OPENMAKEVEC_H

#include "omlib.h"
#include <gs_vec.h>

extern struct Library *OpenMakeVecBase;

static __inline__ void GV_DocInit(gv_doc *d)
{
#ifdef OM_LIB_HOST
    ((void (*)(gv_doc *, struct Library *))om_host_vector(OpenMakeVecBase, 30))(d, OpenMakeVecBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeVecBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(d);
    __asm__ volatile("jsr a6@(-30:W)" : "=r"(_d0), "+r"(_a0) : "r"(_a6) : "d1", "a1", "fp0", "fp1", "cc", "memory");
#endif
}

static __inline__ void GV_DocFree(gv_doc *d)
{
#ifdef OM_LIB_HOST
    ((void (*)(gv_doc *, struct Library *))om_host_vector(OpenMakeVecBase, 36))(d, OpenMakeVecBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeVecBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(d);
    __asm__ volatile("jsr a6@(-36:W)" : "=r"(_d0), "+r"(_a0) : "r"(_a6) : "d1", "a1", "fp0", "fp1", "cc", "memory");
#endif
}

static __inline__ int GV_LoadSvg(gv_doc *d, const char *path, char *err, int errlen)
{
#ifdef OM_LIB_HOST
    return ((int (*)(gv_doc *, const char *, char *, int, struct Library *))om_host_vector(OpenMakeVecBase, 42))(d, path, err, errlen, OpenMakeVecBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeVecBase;
    register ULONG _a0 __asm("a0") = (ULONG)(d);
    register ULONG _a1 __asm("a1") = (ULONG)(path);
    register ULONG _a2 __asm("a2") = (ULONG)(err);
    register ULONG _d0 __asm("d0") = (ULONG)(errlen);
    __asm__ volatile("jsr a6@(-42:W)" : "+r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6), "r"(_a2) : "d1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GV_LoadSvgText(gv_doc *d, const char *text, int n, char *err, int errlen)
{
#ifdef OM_LIB_HOST
    return ((int (*)(gv_doc *, const char *, int, char *, int, struct Library *))om_host_vector(OpenMakeVecBase, 48))(d, text, n, err, errlen, OpenMakeVecBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeVecBase;
    register ULONG _a0 __asm("a0") = (ULONG)(d);
    register ULONG _a1 __asm("a1") = (ULONG)(text);
    register ULONG _d0 __asm("d0") = (ULONG)(n);
    register ULONG _a2 __asm("a2") = (ULONG)(err);
    register ULONG _d1 __asm("d1") = (ULONG)(errlen);
    __asm__ volatile("jsr a6@(-48:W)" : "+r"(_d0), "+r"(_d1), "+r"(_a0), "+r"(_a1) : "r"(_a6), "r"(_a2) : "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ unsigned GV_PathColour(const gv_path *p)
{
#ifdef OM_LIB_HOST
    return ((unsigned (*)(const gv_path *, struct Library *))om_host_vector(OpenMakeVecBase, 54))(p, OpenMakeVecBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeVecBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(p);
    __asm__ volatile("jsr a6@(-54:W)" : "=r"(_d0), "+r"(_a0) : "r"(_a6) : "d1", "a1", "fp0", "fp1", "cc", "memory");
    return (unsigned)_d0;
#endif
}

static __inline__ void GV_PathBox(const gv_path *p, float *minx, float *miny, float *maxx, float *maxy)
{
#ifdef OM_LIB_HOST
    ((void (*)(const gv_path *, float *, float *, float *, float *, struct Library *))om_host_vector(OpenMakeVecBase, 60))(p, minx, miny, maxx, maxy, OpenMakeVecBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeVecBase;
    register ULONG _a0 __asm("a0") = (ULONG)(p);
    register ULONG _a1 __asm("a1") = (ULONG)(minx);
    register ULONG _a2 __asm("a2") = (ULONG)(miny);
    register ULONG _a3 __asm("a3") = (ULONG)(maxx);
    register ULONG _d0 __asm("d0") = (ULONG)(maxy);
    __asm__ volatile("jsr a6@(-60:W)" : "+r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6), "r"(_a2), "r"(_a3) : "d1", "fp0", "fp1", "cc", "memory");
#endif
}

static __inline__ int GV_LoadDxf(gv_doc *d, const char *path, char *err, int errlen)
{
#ifdef OM_LIB_HOST
    return ((int (*)(gv_doc *, const char *, char *, int, struct Library *))om_host_vector(OpenMakeVecBase, 66))(d, path, err, errlen, OpenMakeVecBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeVecBase;
    register ULONG _a0 __asm("a0") = (ULONG)(d);
    register ULONG _a1 __asm("a1") = (ULONG)(path);
    register ULONG _a2 __asm("a2") = (ULONG)(err);
    register ULONG _d0 __asm("d0") = (ULONG)(errlen);
    __asm__ volatile("jsr a6@(-66:W)" : "+r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6), "r"(_a2) : "d1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GV_LoadDxfText(gv_doc *d, const char *text, int n, char *err, int errlen)
{
#ifdef OM_LIB_HOST
    return ((int (*)(gv_doc *, const char *, int, char *, int, struct Library *))om_host_vector(OpenMakeVecBase, 72))(d, text, n, err, errlen, OpenMakeVecBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeVecBase;
    register ULONG _a0 __asm("a0") = (ULONG)(d);
    register ULONG _a1 __asm("a1") = (ULONG)(text);
    register ULONG _d0 __asm("d0") = (ULONG)(n);
    register ULONG _a2 __asm("a2") = (ULONG)(err);
    register ULONG _d1 __asm("d1") = (ULONG)(errlen);
    __asm__ volatile("jsr a6@(-72:W)" : "+r"(_d0), "+r"(_d1), "+r"(_a0), "+r"(_a1) : "r"(_a6), "r"(_a2) : "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GV_Load(gv_doc *d, const char *path, char *err, int errlen)
{
#ifdef OM_LIB_HOST
    return ((int (*)(gv_doc *, const char *, char *, int, struct Library *))om_host_vector(OpenMakeVecBase, 78))(d, path, err, errlen, OpenMakeVecBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeVecBase;
    register ULONG _a0 __asm("a0") = (ULONG)(d);
    register ULONG _a1 __asm("a1") = (ULONG)(path);
    register ULONG _a2 __asm("a2") = (ULONG)(err);
    register ULONG _d0 __asm("d0") = (ULONG)(errlen);
    __asm__ volatile("jsr a6@(-78:W)" : "+r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6), "r"(_a2) : "d1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GV_LoadText(gv_doc *d, const char *text, int n, char *err, int errlen)
{
#ifdef OM_LIB_HOST
    return ((int (*)(gv_doc *, const char *, int, char *, int, struct Library *))om_host_vector(OpenMakeVecBase, 84))(d, text, n, err, errlen, OpenMakeVecBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeVecBase;
    register ULONG _a0 __asm("a0") = (ULONG)(d);
    register ULONG _a1 __asm("a1") = (ULONG)(text);
    register ULONG _d0 __asm("d0") = (ULONG)(n);
    register ULONG _a2 __asm("a2") = (ULONG)(err);
    register ULONG _d1 __asm("d1") = (ULONG)(errlen);
    __asm__ volatile("jsr a6@(-84:W)" : "+r"(_d0), "+r"(_d1), "+r"(_a0), "+r"(_a1) : "r"(_a6), "r"(_a2) : "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

#endif
