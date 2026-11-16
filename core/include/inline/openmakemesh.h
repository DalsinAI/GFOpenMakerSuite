/* Written by core/library/gen-library.py from openmakemesh_lib.sfd: do not edit. */
#ifndef INLINE_OPENMAKEMESH_H
#define INLINE_OPENMAKEMESH_H

#include "omlib.h"
#include <gs_model.h>

extern struct Library *OpenMakeMeshBase;

static __inline__ int GS_StlLoad(gs_mesh *m, const char *path, char *err, int errlen)
{
#ifdef OM_LIB_HOST
    return ((int (*)(gs_mesh *, const char *, char *, int, struct Library *))om_host_vector(OpenMakeMeshBase, 30))(m, path, err, errlen, OpenMakeMeshBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeMeshBase;
    register ULONG _a0 __asm("a0") = (ULONG)(m);
    register ULONG _a1 __asm("a1") = (ULONG)(path);
    register ULONG _a2 __asm("a2") = (ULONG)(err);
    register ULONG _d0 __asm("d0") = (ULONG)(errlen);
    __asm__ volatile("jsr a6@(-30:W)" : "+r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6), "r"(_a2) : "d1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ void GS_MeshFree(gs_mesh *m)
{
#ifdef OM_LIB_HOST
    ((void (*)(gs_mesh *, struct Library *))om_host_vector(OpenMakeMeshBase, 36))(m, OpenMakeMeshBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeMeshBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(m);
    __asm__ volatile("jsr a6@(-36:W)" : "=r"(_d0), "+r"(_a0) : "r"(_a6) : "d1", "a1", "fp0", "fp1", "cc", "memory");
#endif
}

static __inline__ void GS_MeshRepair(gs_mesh *m)
{
#ifdef OM_LIB_HOST
    ((void (*)(gs_mesh *, struct Library *))om_host_vector(OpenMakeMeshBase, 42))(m, OpenMakeMeshBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeMeshBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(m);
    __asm__ volatile("jsr a6@(-42:W)" : "=r"(_d0), "+r"(_a0) : "r"(_a6) : "d1", "a1", "fp0", "fp1", "cc", "memory");
#endif
}

static __inline__ int GM_Cube(gs_mesh *m, float x, float y, float z, float sx, float sy, float sz)
{
#ifdef OM_LIB_HOST
    return ((int (*)(gs_mesh *, ULONG, ULONG, ULONG, ULONG, ULONG, ULONG, struct Library *))om_host_vector(OpenMakeMeshBase, 48))(m, om_lib_bits(x), om_lib_bits(y), om_lib_bits(z), om_lib_bits(sx), om_lib_bits(sy), om_lib_bits(sz), OpenMakeMeshBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeMeshBase;
    register ULONG _a0 __asm("a0") = (ULONG)(m);
    register ULONG _d0 __asm("d0") = om_lib_bits(x);
    register ULONG _d1 __asm("d1") = om_lib_bits(y);
    register ULONG _d2 __asm("d2") = om_lib_bits(z);
    register ULONG _d3 __asm("d3") = om_lib_bits(sx);
    register ULONG _d4 __asm("d4") = om_lib_bits(sy);
    register ULONG _d5 __asm("d5") = om_lib_bits(sz);
    __asm__ volatile("jsr a6@(-48:W)" : "+r"(_d0), "+r"(_d1), "+r"(_a0) : "r"(_a6), "r"(_d2), "r"(_d3), "r"(_d4), "r"(_d5) : "a1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GM_Cylinder(gs_mesh *m, float x, float y, float z, float r, float h, int seg)
{
#ifdef OM_LIB_HOST
    return ((int (*)(gs_mesh *, ULONG, ULONG, ULONG, ULONG, ULONG, int, struct Library *))om_host_vector(OpenMakeMeshBase, 54))(m, om_lib_bits(x), om_lib_bits(y), om_lib_bits(z), om_lib_bits(r), om_lib_bits(h), seg, OpenMakeMeshBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeMeshBase;
    register ULONG _a0 __asm("a0") = (ULONG)(m);
    register ULONG _d0 __asm("d0") = om_lib_bits(x);
    register ULONG _d1 __asm("d1") = om_lib_bits(y);
    register ULONG _d2 __asm("d2") = om_lib_bits(z);
    register ULONG _d3 __asm("d3") = om_lib_bits(r);
    register ULONG _d4 __asm("d4") = om_lib_bits(h);
    register ULONG _d5 __asm("d5") = (ULONG)(seg);
    __asm__ volatile("jsr a6@(-54:W)" : "+r"(_d0), "+r"(_d1), "+r"(_a0) : "r"(_a6), "r"(_d2), "r"(_d3), "r"(_d4), "r"(_d5) : "a1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GM_Union(gs_mesh *dst, const gs_mesh *a, const gs_mesh *b)
{
#ifdef OM_LIB_HOST
    return ((int (*)(gs_mesh *, const gs_mesh *, const gs_mesh *, struct Library *))om_host_vector(OpenMakeMeshBase, 60))(dst, a, b, OpenMakeMeshBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeMeshBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(dst);
    register ULONG _a1 __asm("a1") = (ULONG)(a);
    register ULONG _a2 __asm("a2") = (ULONG)(b);
    __asm__ volatile("jsr a6@(-60:W)" : "=r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6), "r"(_a2) : "d1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GM_Subtract(gs_mesh *dst, const gs_mesh *a, const gs_mesh *b)
{
#ifdef OM_LIB_HOST
    return ((int (*)(gs_mesh *, const gs_mesh *, const gs_mesh *, struct Library *))om_host_vector(OpenMakeMeshBase, 66))(dst, a, b, OpenMakeMeshBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeMeshBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(dst);
    register ULONG _a1 __asm("a1") = (ULONG)(a);
    register ULONG _a2 __asm("a2") = (ULONG)(b);
    __asm__ volatile("jsr a6@(-66:W)" : "=r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6), "r"(_a2) : "d1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GM_Intersect(gs_mesh *dst, const gs_mesh *a, const gs_mesh *b)
{
#ifdef OM_LIB_HOST
    return ((int (*)(gs_mesh *, const gs_mesh *, const gs_mesh *, struct Library *))om_host_vector(OpenMakeMeshBase, 72))(dst, a, b, OpenMakeMeshBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeMeshBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(dst);
    register ULONG _a1 __asm("a1") = (ULONG)(a);
    register ULONG _a2 __asm("a2") = (ULONG)(b);
    __asm__ volatile("jsr a6@(-72:W)" : "=r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6), "r"(_a2) : "d1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GM_OpenEdges(const gs_mesh *m)
{
#ifdef OM_LIB_HOST
    return ((int (*)(const gs_mesh *, struct Library *))om_host_vector(OpenMakeMeshBase, 78))(m, OpenMakeMeshBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeMeshBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(m);
    __asm__ volatile("jsr a6@(-78:W)" : "=r"(_d0), "+r"(_a0) : "r"(_a6) : "d1", "a1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GM_StlWrite(const gs_mesh *m, const char *path)
{
#ifdef OM_LIB_HOST
    return ((int (*)(const gs_mesh *, const char *, struct Library *))om_host_vector(OpenMakeMeshBase, 84))(m, path, OpenMakeMeshBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeMeshBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(m);
    register ULONG _a1 __asm("a1") = (ULONG)(path);
    __asm__ volatile("jsr a6@(-84:W)" : "=r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6) : "d1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

#endif
