/* Written by core/library/gen-library.py from openmakepath_lib.sfd: do not edit. */
#ifndef INLINE_OPENMAKEPATH_H
#define INLINE_OPENMAKEPATH_H

#include "omlib.h"
#include <gs_layer.h>
#include <gs_toolpath.h>

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

static __inline__ int GL_Inset(const gl_loops *in, float d, gl_loops *out)
{
#ifdef OM_LIB_HOST
    return ((int (*)(const gl_loops *, ULONG, gl_loops *, struct Library *))om_host_vector(OpenMakePathBase, 90))(in, om_lib_bits(d), out, OpenMakePathBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakePathBase;
    register ULONG _a0 __asm("a0") = (ULONG)(in);
    register ULONG _d0 __asm("d0") = om_lib_bits(d);
    register ULONG _a1 __asm("a1") = (ULONG)(out);
    __asm__ volatile("jsr a6@(-90:W)" : "+r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6) : "d1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ void GT_Init(gt_path *p)
{
#ifdef OM_LIB_HOST
    ((void (*)(gt_path *, struct Library *))om_host_vector(OpenMakePathBase, 96))(p, OpenMakePathBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakePathBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(p);
    __asm__ volatile("jsr a6@(-96:W)" : "=r"(_d0), "+r"(_a0) : "r"(_a6) : "d1", "a1", "fp0", "fp1", "cc", "memory");
#endif
}

static __inline__ void GT_Free(gt_path *p)
{
#ifdef OM_LIB_HOST
    ((void (*)(gt_path *, struct Library *))om_host_vector(OpenMakePathBase, 102))(p, OpenMakePathBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakePathBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(p);
    __asm__ volatile("jsr a6@(-102:W)" : "=r"(_d0), "+r"(_a0) : "r"(_a6) : "d1", "a1", "fp0", "fp1", "cc", "memory");
#endif
}

static __inline__ int GT_Layer(gt_path *p)
{
#ifdef OM_LIB_HOST
    return ((int (*)(gt_path *, struct Library *))om_host_vector(OpenMakePathBase, 108))(p, OpenMakePathBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakePathBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(p);
    __asm__ volatile("jsr a6@(-108:W)" : "=r"(_d0), "+r"(_a0) : "r"(_a6) : "d1", "a1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GT_Add(gt_path *p, int kind, float x, float y, float z, float w)
{
#ifdef OM_LIB_HOST
    return ((int (*)(gt_path *, int, ULONG, ULONG, ULONG, ULONG, struct Library *))om_host_vector(OpenMakePathBase, 114))(p, kind, om_lib_bits(x), om_lib_bits(y), om_lib_bits(z), om_lib_bits(w), OpenMakePathBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakePathBase;
    register ULONG _a0 __asm("a0") = (ULONG)(p);
    register ULONG _d0 __asm("d0") = (ULONG)(kind);
    register ULONG _d1 __asm("d1") = om_lib_bits(x);
    register ULONG _d2 __asm("d2") = om_lib_bits(y);
    register ULONG _d3 __asm("d3") = om_lib_bits(z);
    register ULONG _d4 __asm("d4") = om_lib_bits(w);
    __asm__ volatile("jsr a6@(-114:W)" : "+r"(_d0), "+r"(_d1), "+r"(_a0) : "r"(_a6), "r"(_d2), "r"(_d3), "r"(_d4) : "a1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GT_Loop(gt_path *p, int kind, const gl_loop *l, float z, float w)
{
#ifdef OM_LIB_HOST
    return ((int (*)(gt_path *, int, const gl_loop *, ULONG, ULONG, struct Library *))om_host_vector(OpenMakePathBase, 120))(p, kind, l, om_lib_bits(z), om_lib_bits(w), OpenMakePathBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakePathBase;
    register ULONG _a0 __asm("a0") = (ULONG)(p);
    register ULONG _d0 __asm("d0") = (ULONG)(kind);
    register ULONG _a1 __asm("a1") = (ULONG)(l);
    register ULONG _d1 __asm("d1") = om_lib_bits(z);
    register ULONG _d2 __asm("d2") = om_lib_bits(w);
    __asm__ volatile("jsr a6@(-120:W)" : "+r"(_d0), "+r"(_d1), "+r"(_a0), "+r"(_a1) : "r"(_a6), "r"(_d2) : "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GT_Append(gt_path *dst, const gt_path *src)
{
#ifdef OM_LIB_HOST
    return ((int (*)(gt_path *, const gt_path *, struct Library *))om_host_vector(OpenMakePathBase, 126))(dst, src, OpenMakePathBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakePathBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(dst);
    register ULONG _a1 __asm("a1") = (ULONG)(src);
    __asm__ volatile("jsr a6@(-126:W)" : "=r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6) : "d1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ float GT_Length(const gt_path *p, int kind)
{
#ifdef OM_LIB_HOST
    return om_lib_float(((ULONG (*)(const gt_path *, int, struct Library *))om_host_vector(OpenMakePathBase, 132))(p, kind, OpenMakePathBase));
#else
    register struct Library *_a6 __asm("a6") = OpenMakePathBase;
    register ULONG _a0 __asm("a0") = (ULONG)(p);
    register ULONG _d0 __asm("d0") = (ULONG)(kind);
    __asm__ volatile("jsr a6@(-132:W)" : "+r"(_d0), "+r"(_a0) : "r"(_a6) : "d1", "a1", "fp0", "fp1", "cc", "memory");
    return om_lib_float(_d0);
#endif
}

static __inline__ int GT_Bounds(const gt_path *p, float *min, float *max)
{
#ifdef OM_LIB_HOST
    return ((int (*)(const gt_path *, float *, float *, struct Library *))om_host_vector(OpenMakePathBase, 138))(p, min, max, OpenMakePathBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakePathBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(p);
    register ULONG _a1 __asm("a1") = (ULONG)(min);
    register ULONG _a2 __asm("a2") = (ULONG)(max);
    __asm__ volatile("jsr a6@(-138:W)" : "=r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6), "r"(_a2) : "d1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GT_Fill(gt_path *p, const gl_loops *region, const gl_loops *const *solid, int nsolid, float z, float w, int every, int cross)
{
#ifdef OM_LIB_HOST
    return ((int (*)(gt_path *, const gl_loops *, const gl_loops *const *, int, ULONG, ULONG, int, int, struct Library *))om_host_vector(OpenMakePathBase, 144))(p, region, solid, nsolid, om_lib_bits(z), om_lib_bits(w), every, cross, OpenMakePathBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakePathBase;
    register ULONG _a0 __asm("a0") = (ULONG)(p);
    register ULONG _a1 __asm("a1") = (ULONG)(region);
    register ULONG _a2 __asm("a2") = (ULONG)(solid);
    register ULONG _d0 __asm("d0") = (ULONG)(nsolid);
    register ULONG _d1 __asm("d1") = om_lib_bits(z);
    register ULONG _d2 __asm("d2") = om_lib_bits(w);
    register ULONG _d3 __asm("d3") = (ULONG)(every);
    register ULONG _d4 __asm("d4") = (ULONG)(cross);
    __asm__ volatile("jsr a6@(-144:W)" : "+r"(_d0), "+r"(_d1), "+r"(_a0), "+r"(_a1) : "r"(_a6), "r"(_a2), "r"(_d2), "r"(_d3), "r"(_d4) : "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

#endif
