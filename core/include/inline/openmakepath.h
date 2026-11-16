/* Written by core/library/gen-library.py from openmakepath_lib.sfd: do not edit. */
#ifndef INLINE_OPENMAKEPATH_H
#define INLINE_OPENMAKEPATH_H

#include "omlib.h"
#include <gs_layer.h>

extern struct Library *OpenMakePathBase;

static __inline__ float GL_Height(float h)
{
#ifdef OM_LIB_HOST
    return om_lib_float(((ULONG (*)(ULONG, struct Library *))om_host_vector(OpenMakePathBase, 30))(om_lib_bits(h), OpenMakePathBase));
#else
    register struct Library *_a6 __asm("a6") = OpenMakePathBase;
    register ULONG _d0 __asm("d0") = om_lib_bits(h);
    __asm__ volatile("jsr a6@(-30:W)" : "+r"(_d0) : "r"(_a6) : "d1", "a0", "a1", "fp0", "fp1", "cc", "memory");
    return om_lib_float(_d0);
#endif
}

static __inline__ int GL_Layers(const gs_mesh *m, float h)
{
#ifdef OM_LIB_HOST
    return ((int (*)(const gs_mesh *, ULONG, struct Library *))om_host_vector(OpenMakePathBase, 36))(m, om_lib_bits(h), OpenMakePathBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakePathBase;
    register ULONG _a0 __asm("a0") = (ULONG)(m);
    register ULONG _d0 __asm("d0") = om_lib_bits(h);
    __asm__ volatile("jsr a6@(-36:W)" : "+r"(_d0), "+r"(_a0) : "r"(_a6) : "d1", "a1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ float GL_LayerZ(const gs_mesh *m, float h, int layer)
{
#ifdef OM_LIB_HOST
    return om_lib_float(((ULONG (*)(const gs_mesh *, ULONG, int, struct Library *))om_host_vector(OpenMakePathBase, 42))(m, om_lib_bits(h), layer, OpenMakePathBase));
#else
    register struct Library *_a6 __asm("a6") = OpenMakePathBase;
    register ULONG _a0 __asm("a0") = (ULONG)(m);
    register ULONG _d0 __asm("d0") = om_lib_bits(h);
    register ULONG _d1 __asm("d1") = (ULONG)(layer);
    __asm__ volatile("jsr a6@(-42:W)" : "+r"(_d0), "+r"(_d1), "+r"(_a0) : "r"(_a6) : "a1", "fp0", "fp1", "cc", "memory");
    return om_lib_float(_d0);
#endif
}

static __inline__ int GL_Cut(const gs_mesh *m, float h, int layer, gl_walls *w)
{
#ifdef OM_LIB_HOST
    return ((int (*)(const gs_mesh *, ULONG, int, gl_walls *, struct Library *))om_host_vector(OpenMakePathBase, 48))(m, om_lib_bits(h), layer, w, OpenMakePathBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakePathBase;
    register ULONG _a0 __asm("a0") = (ULONG)(m);
    register ULONG _d0 __asm("d0") = om_lib_bits(h);
    register ULONG _d1 __asm("d1") = (ULONG)(layer);
    register ULONG _a1 __asm("a1") = (ULONG)(w);
    __asm__ volatile("jsr a6@(-48:W)" : "+r"(_d0), "+r"(_d1), "+r"(_a0), "+r"(_a1) : "r"(_a6) : "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ void GL_WallsFree(gl_walls *w)
{
#ifdef OM_LIB_HOST
    ((void (*)(gl_walls *, struct Library *))om_host_vector(OpenMakePathBase, 54))(w, OpenMakePathBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakePathBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(w);
    __asm__ volatile("jsr a6@(-54:W)" : "=r"(_d0), "+r"(_a0) : "r"(_a6) : "d1", "a1", "fp0", "fp1", "cc", "memory");
#endif
}

static __inline__ void GL_Status(char *buf, int len, const gs_mesh *m, float h, int layer, const gl_walls *w)
{
#ifdef OM_LIB_HOST
    ((void (*)(char *, int, const gs_mesh *, ULONG, int, const gl_walls *, struct Library *))om_host_vector(OpenMakePathBase, 60))(buf, len, m, om_lib_bits(h), layer, w, OpenMakePathBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakePathBase;
    register ULONG _a0 __asm("a0") = (ULONG)(buf);
    register ULONG _d0 __asm("d0") = (ULONG)(len);
    register ULONG _a1 __asm("a1") = (ULONG)(m);
    register ULONG _d1 __asm("d1") = om_lib_bits(h);
    register ULONG _d2 __asm("d2") = (ULONG)(layer);
    register ULONG _a2 __asm("a2") = (ULONG)(w);
    __asm__ volatile("jsr a6@(-60:W)" : "+r"(_d0), "+r"(_d1), "+r"(_a0), "+r"(_a1) : "r"(_a6), "r"(_d2), "r"(_a2) : "fp0", "fp1", "cc", "memory");
#endif
}

static __inline__ int GL_Weld(const gl_walls *w, float tol, gl_loops *out)
{
#ifdef OM_LIB_HOST
    return ((int (*)(const gl_walls *, ULONG, gl_loops *, struct Library *))om_host_vector(OpenMakePathBase, 66))(w, om_lib_bits(tol), out, OpenMakePathBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakePathBase;
    register ULONG _a0 __asm("a0") = (ULONG)(w);
    register ULONG _d0 __asm("d0") = om_lib_bits(tol);
    register ULONG _a1 __asm("a1") = (ULONG)(out);
    __asm__ volatile("jsr a6@(-66:W)" : "+r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6) : "d1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ void GL_LoopsFree(gl_loops *out)
{
#ifdef OM_LIB_HOST
    ((void (*)(gl_loops *, struct Library *))om_host_vector(OpenMakePathBase, 72))(out, OpenMakePathBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakePathBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(out);
    __asm__ volatile("jsr a6@(-72:W)" : "=r"(_d0), "+r"(_a0) : "r"(_a6) : "d1", "a1", "fp0", "fp1", "cc", "memory");
#endif
}

static __inline__ float GL_LoopArea(const gl_loop *l)
{
#ifdef OM_LIB_HOST
    return om_lib_float(((ULONG (*)(const gl_loop *, struct Library *))om_host_vector(OpenMakePathBase, 78))(l, OpenMakePathBase));
#else
    register struct Library *_a6 __asm("a6") = OpenMakePathBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(l);
    __asm__ volatile("jsr a6@(-78:W)" : "=r"(_d0), "+r"(_a0) : "r"(_a6) : "d1", "a1", "fp0", "fp1", "cc", "memory");
    return om_lib_float(_d0);
#endif
}

static __inline__ float GL_LoopLength(const gl_loop *l)
{
#ifdef OM_LIB_HOST
    return om_lib_float(((ULONG (*)(const gl_loop *, struct Library *))om_host_vector(OpenMakePathBase, 84))(l, OpenMakePathBase));
#else
    register struct Library *_a6 __asm("a6") = OpenMakePathBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(l);
    __asm__ volatile("jsr a6@(-84:W)" : "=r"(_d0), "+r"(_a0) : "r"(_a6) : "d1", "a1", "fp0", "fp1", "cc", "memory");
    return om_lib_float(_d0);
#endif
}

#endif
