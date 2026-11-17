/* Written by core/library/gen-library.py from openmakemesh_lib.sfd: do not edit. */
#ifndef INLINE_OPENMAKEMESH_H
#define INLINE_OPENMAKEMESH_H

#include "omlib.h"
#include <gs_model.h>
#include <gs_parts.h>

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

static __inline__ int GM_Sphere(gs_mesh *m, float x, float y, float z, float r, int seg, int rings)
{
#ifdef OM_LIB_HOST
    return ((int (*)(gs_mesh *, ULONG, ULONG, ULONG, ULONG, int, int, struct Library *))om_host_vector(OpenMakeMeshBase, 90))(m, om_lib_bits(x), om_lib_bits(y), om_lib_bits(z), om_lib_bits(r), seg, rings, OpenMakeMeshBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeMeshBase;
    register ULONG _a0 __asm("a0") = (ULONG)(m);
    register ULONG _d0 __asm("d0") = om_lib_bits(x);
    register ULONG _d1 __asm("d1") = om_lib_bits(y);
    register ULONG _d2 __asm("d2") = om_lib_bits(z);
    register ULONG _d3 __asm("d3") = om_lib_bits(r);
    register ULONG _d4 __asm("d4") = (ULONG)(seg);
    register ULONG _d5 __asm("d5") = (ULONG)(rings);
    __asm__ volatile("jsr a6@(-90:W)" : "+r"(_d0), "+r"(_d1), "+r"(_a0) : "r"(_a6), "r"(_d2), "r"(_d3), "r"(_d4), "r"(_d5) : "a1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GM_Cone(gs_mesh *m, float x, float y, float z, float r, float h, int seg)
{
#ifdef OM_LIB_HOST
    return ((int (*)(gs_mesh *, ULONG, ULONG, ULONG, ULONG, ULONG, int, struct Library *))om_host_vector(OpenMakeMeshBase, 96))(m, om_lib_bits(x), om_lib_bits(y), om_lib_bits(z), om_lib_bits(r), om_lib_bits(h), seg, OpenMakeMeshBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeMeshBase;
    register ULONG _a0 __asm("a0") = (ULONG)(m);
    register ULONG _d0 __asm("d0") = om_lib_bits(x);
    register ULONG _d1 __asm("d1") = om_lib_bits(y);
    register ULONG _d2 __asm("d2") = om_lib_bits(z);
    register ULONG _d3 __asm("d3") = om_lib_bits(r);
    register ULONG _d4 __asm("d4") = om_lib_bits(h);
    register ULONG _d5 __asm("d5") = (ULONG)(seg);
    __asm__ volatile("jsr a6@(-96:W)" : "+r"(_d0), "+r"(_d1), "+r"(_a0) : "r"(_a6), "r"(_d2), "r"(_d3), "r"(_d4), "r"(_d5) : "a1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GM_Torus(gs_mesh *m, float x, float y, float z, float ring, float tube, int seg, int tseg)
{
#ifdef OM_LIB_HOST
    return ((int (*)(gs_mesh *, ULONG, ULONG, ULONG, ULONG, ULONG, int, int, struct Library *))om_host_vector(OpenMakeMeshBase, 102))(m, om_lib_bits(x), om_lib_bits(y), om_lib_bits(z), om_lib_bits(ring), om_lib_bits(tube), seg, tseg, OpenMakeMeshBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeMeshBase;
    register ULONG _a0 __asm("a0") = (ULONG)(m);
    register ULONG _d0 __asm("d0") = om_lib_bits(x);
    register ULONG _d1 __asm("d1") = om_lib_bits(y);
    register ULONG _d2 __asm("d2") = om_lib_bits(z);
    register ULONG _d3 __asm("d3") = om_lib_bits(ring);
    register ULONG _d4 __asm("d4") = om_lib_bits(tube);
    register ULONG _d5 __asm("d5") = (ULONG)(seg);
    register ULONG _d6 __asm("d6") = (ULONG)(tseg);
    __asm__ volatile("jsr a6@(-102:W)" : "+r"(_d0), "+r"(_d1), "+r"(_a0) : "r"(_a6), "r"(_d2), "r"(_d3), "r"(_d4), "r"(_d5), "r"(_d6) : "a1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GM_Wedge(gs_mesh *m, float x, float y, float z, float sx, float sy, float sz)
{
#ifdef OM_LIB_HOST
    return ((int (*)(gs_mesh *, ULONG, ULONG, ULONG, ULONG, ULONG, ULONG, struct Library *))om_host_vector(OpenMakeMeshBase, 108))(m, om_lib_bits(x), om_lib_bits(y), om_lib_bits(z), om_lib_bits(sx), om_lib_bits(sy), om_lib_bits(sz), OpenMakeMeshBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeMeshBase;
    register ULONG _a0 __asm("a0") = (ULONG)(m);
    register ULONG _d0 __asm("d0") = om_lib_bits(x);
    register ULONG _d1 __asm("d1") = om_lib_bits(y);
    register ULONG _d2 __asm("d2") = om_lib_bits(z);
    register ULONG _d3 __asm("d3") = om_lib_bits(sx);
    register ULONG _d4 __asm("d4") = om_lib_bits(sy);
    register ULONG _d5 __asm("d5") = om_lib_bits(sz);
    __asm__ volatile("jsr a6@(-108:W)" : "+r"(_d0), "+r"(_d1), "+r"(_a0) : "r"(_a6), "r"(_d2), "r"(_d3), "r"(_d4), "r"(_d5) : "a1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GM_Slot(gs_mesh *m, float x, float y, float z, float sx, float sy, float sz, int seg)
{
#ifdef OM_LIB_HOST
    return ((int (*)(gs_mesh *, ULONG, ULONG, ULONG, ULONG, ULONG, ULONG, int, struct Library *))om_host_vector(OpenMakeMeshBase, 114))(m, om_lib_bits(x), om_lib_bits(y), om_lib_bits(z), om_lib_bits(sx), om_lib_bits(sy), om_lib_bits(sz), seg, OpenMakeMeshBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeMeshBase;
    register ULONG _a0 __asm("a0") = (ULONG)(m);
    register ULONG _d0 __asm("d0") = om_lib_bits(x);
    register ULONG _d1 __asm("d1") = om_lib_bits(y);
    register ULONG _d2 __asm("d2") = om_lib_bits(z);
    register ULONG _d3 __asm("d3") = om_lib_bits(sx);
    register ULONG _d4 __asm("d4") = om_lib_bits(sy);
    register ULONG _d5 __asm("d5") = om_lib_bits(sz);
    register ULONG _d6 __asm("d6") = (ULONG)(seg);
    __asm__ volatile("jsr a6@(-114:W)" : "+r"(_d0), "+r"(_d1), "+r"(_a0) : "r"(_a6), "r"(_d2), "r"(_d3), "r"(_d4), "r"(_d5), "r"(_d6) : "a1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GM_BoltCircle(gs_mesh *m, float x, float y, float z, float h, float d, float pcd, int n)
{
#ifdef OM_LIB_HOST
    return ((int (*)(gs_mesh *, ULONG, ULONG, ULONG, ULONG, ULONG, ULONG, int, struct Library *))om_host_vector(OpenMakeMeshBase, 120))(m, om_lib_bits(x), om_lib_bits(y), om_lib_bits(z), om_lib_bits(h), om_lib_bits(d), om_lib_bits(pcd), n, OpenMakeMeshBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeMeshBase;
    register ULONG _a0 __asm("a0") = (ULONG)(m);
    register ULONG _d0 __asm("d0") = om_lib_bits(x);
    register ULONG _d1 __asm("d1") = om_lib_bits(y);
    register ULONG _d2 __asm("d2") = om_lib_bits(z);
    register ULONG _d3 __asm("d3") = om_lib_bits(h);
    register ULONG _d4 __asm("d4") = om_lib_bits(d);
    register ULONG _d5 __asm("d5") = om_lib_bits(pcd);
    register ULONG _d6 __asm("d6") = (ULONG)(n);
    __asm__ volatile("jsr a6@(-120:W)" : "+r"(_d0), "+r"(_d1), "+r"(_a0) : "r"(_a6), "r"(_d2), "r"(_d3), "r"(_d4), "r"(_d5), "r"(_d6) : "a1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GM_HoleRow(gs_mesh *m, float x, float y, float z, float h, float d, float pitch, int n)
{
#ifdef OM_LIB_HOST
    return ((int (*)(gs_mesh *, ULONG, ULONG, ULONG, ULONG, ULONG, ULONG, int, struct Library *))om_host_vector(OpenMakeMeshBase, 126))(m, om_lib_bits(x), om_lib_bits(y), om_lib_bits(z), om_lib_bits(h), om_lib_bits(d), om_lib_bits(pitch), n, OpenMakeMeshBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeMeshBase;
    register ULONG _a0 __asm("a0") = (ULONG)(m);
    register ULONG _d0 __asm("d0") = om_lib_bits(x);
    register ULONG _d1 __asm("d1") = om_lib_bits(y);
    register ULONG _d2 __asm("d2") = om_lib_bits(z);
    register ULONG _d3 __asm("d3") = om_lib_bits(h);
    register ULONG _d4 __asm("d4") = om_lib_bits(d);
    register ULONG _d5 __asm("d5") = om_lib_bits(pitch);
    register ULONG _d6 __asm("d6") = (ULONG)(n);
    __asm__ volatile("jsr a6@(-126:W)" : "+r"(_d0), "+r"(_d1), "+r"(_a0) : "r"(_a6), "r"(_d2), "r"(_d3), "r"(_d4), "r"(_d5), "r"(_d6) : "a1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GM_Counterbore(gs_mesh *m, float x, float y, float z, float h, float d, float cb, float deep)
{
#ifdef OM_LIB_HOST
    return ((int (*)(gs_mesh *, ULONG, ULONG, ULONG, ULONG, ULONG, ULONG, ULONG, struct Library *))om_host_vector(OpenMakeMeshBase, 132))(m, om_lib_bits(x), om_lib_bits(y), om_lib_bits(z), om_lib_bits(h), om_lib_bits(d), om_lib_bits(cb), om_lib_bits(deep), OpenMakeMeshBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeMeshBase;
    register ULONG _a0 __asm("a0") = (ULONG)(m);
    register ULONG _d0 __asm("d0") = om_lib_bits(x);
    register ULONG _d1 __asm("d1") = om_lib_bits(y);
    register ULONG _d2 __asm("d2") = om_lib_bits(z);
    register ULONG _d3 __asm("d3") = om_lib_bits(h);
    register ULONG _d4 __asm("d4") = om_lib_bits(d);
    register ULONG _d5 __asm("d5") = om_lib_bits(cb);
    register ULONG _d6 __asm("d6") = om_lib_bits(deep);
    __asm__ volatile("jsr a6@(-132:W)" : "+r"(_d0), "+r"(_d1), "+r"(_a0) : "r"(_a6), "r"(_d2), "r"(_d3), "r"(_d4), "r"(_d5), "r"(_d6) : "a1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GM_Facts(const gs_mesh *m, gs_facts *f)
{
#ifdef OM_LIB_HOST
    return ((int (*)(const gs_mesh *, gs_facts *, struct Library *))om_host_vector(OpenMakeMeshBase, 138))(m, f, OpenMakeMeshBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeMeshBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(m);
    register ULONG _a1 __asm("a1") = (ULONG)(f);
    __asm__ volatile("jsr a6@(-138:W)" : "=r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6) : "d1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GM_Extrude(gs_mesh *m, const gs_mesh *part, int kind, int op, float x, float y, float w, float d, float h)
{
#ifdef OM_LIB_HOST
    return ((int (*)(gs_mesh *, const gs_mesh *, int, int, ULONG, ULONG, ULONG, ULONG, ULONG, struct Library *))om_host_vector(OpenMakeMeshBase, 144))(m, part, kind, op, om_lib_bits(x), om_lib_bits(y), om_lib_bits(w), om_lib_bits(d), om_lib_bits(h), OpenMakeMeshBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeMeshBase;
    register ULONG _a0 __asm("a0") = (ULONG)(m);
    register ULONG _a1 __asm("a1") = (ULONG)(part);
    register ULONG _d0 __asm("d0") = (ULONG)(kind);
    register ULONG _d1 __asm("d1") = (ULONG)(op);
    register ULONG _d2 __asm("d2") = om_lib_bits(x);
    register ULONG _d3 __asm("d3") = om_lib_bits(y);
    register ULONG _d4 __asm("d4") = om_lib_bits(w);
    register ULONG _d5 __asm("d5") = om_lib_bits(d);
    register ULONG _d6 __asm("d6") = om_lib_bits(h);
    __asm__ volatile("jsr a6@(-144:W)" : "+r"(_d0), "+r"(_d1), "+r"(_a0), "+r"(_a1) : "r"(_a6), "r"(_d2), "r"(_d3), "r"(_d4), "r"(_d5), "r"(_d6) : "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ gs_parts * GS_PartsNew(void)
{
#ifdef OM_LIB_HOST
    return ((gs_parts * (*)(struct Library *))om_host_vector(OpenMakeMeshBase, 150))(OpenMakeMeshBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeMeshBase;
    register ULONG _d0 __asm("d0");
    __asm__ volatile("jsr a6@(-150:W)" : "=r"(_d0) : "r"(_a6) : "d1", "a0", "a1", "fp0", "fp1", "cc", "memory");
    return (gs_parts *)_d0;
#endif
}

static __inline__ void GS_PartsFree(gs_parts *ps)
{
#ifdef OM_LIB_HOST
    ((void (*)(gs_parts *, struct Library *))om_host_vector(OpenMakeMeshBase, 156))(ps, OpenMakeMeshBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeMeshBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(ps);
    __asm__ volatile("jsr a6@(-156:W)" : "=r"(_d0), "+r"(_a0) : "r"(_a6) : "d1", "a1", "fp0", "fp1", "cc", "memory");
#endif
}

static __inline__ int GS_PartsCount(const gs_parts *ps)
{
#ifdef OM_LIB_HOST
    return ((int (*)(const gs_parts *, struct Library *))om_host_vector(OpenMakeMeshBase, 162))(ps, OpenMakeMeshBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeMeshBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(ps);
    __asm__ volatile("jsr a6@(-162:W)" : "=r"(_d0), "+r"(_a0) : "r"(_a6) : "d1", "a1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GS_PartsAdd(gs_parts *ps, gs_mesh *m, const char *name)
{
#ifdef OM_LIB_HOST
    return ((int (*)(gs_parts *, gs_mesh *, const char *, struct Library *))om_host_vector(OpenMakeMeshBase, 168))(ps, m, name, OpenMakeMeshBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeMeshBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(ps);
    register ULONG _a1 __asm("a1") = (ULONG)(m);
    register ULONG _a2 __asm("a2") = (ULONG)(name);
    __asm__ volatile("jsr a6@(-168:W)" : "=r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6), "r"(_a2) : "d1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GS_PartsRemove(gs_parts *ps, int i)
{
#ifdef OM_LIB_HOST
    return ((int (*)(gs_parts *, int, struct Library *))om_host_vector(OpenMakeMeshBase, 174))(ps, i, OpenMakeMeshBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeMeshBase;
    register ULONG _a0 __asm("a0") = (ULONG)(ps);
    register ULONG _d0 __asm("d0") = (ULONG)(i);
    __asm__ volatile("jsr a6@(-174:W)" : "+r"(_d0), "+r"(_a0) : "r"(_a6) : "d1", "a1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ gs_mesh * GS_PartsMesh(gs_parts *ps, int i)
{
#ifdef OM_LIB_HOST
    return ((gs_mesh * (*)(gs_parts *, int, struct Library *))om_host_vector(OpenMakeMeshBase, 180))(ps, i, OpenMakeMeshBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeMeshBase;
    register ULONG _a0 __asm("a0") = (ULONG)(ps);
    register ULONG _d0 __asm("d0") = (ULONG)(i);
    __asm__ volatile("jsr a6@(-180:W)" : "+r"(_d0), "+r"(_a0) : "r"(_a6) : "d1", "a1", "fp0", "fp1", "cc", "memory");
    return (gs_mesh *)_d0;
#endif
}

static __inline__ const char * GS_PartsName(const gs_parts *ps, int i)
{
#ifdef OM_LIB_HOST
    return ((const char * (*)(const gs_parts *, int, struct Library *))om_host_vector(OpenMakeMeshBase, 186))(ps, i, OpenMakeMeshBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeMeshBase;
    register ULONG _a0 __asm("a0") = (ULONG)(ps);
    register ULONG _d0 __asm("d0") = (ULONG)(i);
    __asm__ volatile("jsr a6@(-186:W)" : "+r"(_d0), "+r"(_a0) : "r"(_a6) : "d1", "a1", "fp0", "fp1", "cc", "memory");
    return (const char *)_d0;
#endif
}

static __inline__ int GS_PartsRename(gs_parts *ps, int i, const char *name)
{
#ifdef OM_LIB_HOST
    return ((int (*)(gs_parts *, int, const char *, struct Library *))om_host_vector(OpenMakeMeshBase, 192))(ps, i, name, OpenMakeMeshBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeMeshBase;
    register ULONG _a0 __asm("a0") = (ULONG)(ps);
    register ULONG _d0 __asm("d0") = (ULONG)(i);
    register ULONG _a1 __asm("a1") = (ULONG)(name);
    __asm__ volatile("jsr a6@(-192:W)" : "+r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6) : "d1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GS_PartsColour(const gs_parts *ps, int i)
{
#ifdef OM_LIB_HOST
    return ((int (*)(const gs_parts *, int, struct Library *))om_host_vector(OpenMakeMeshBase, 198))(ps, i, OpenMakeMeshBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeMeshBase;
    register ULONG _a0 __asm("a0") = (ULONG)(ps);
    register ULONG _d0 __asm("d0") = (ULONG)(i);
    __asm__ volatile("jsr a6@(-198:W)" : "+r"(_d0), "+r"(_a0) : "r"(_a6) : "d1", "a1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GS_PartsSetColour(gs_parts *ps, int i, int colour)
{
#ifdef OM_LIB_HOST
    return ((int (*)(gs_parts *, int, int, struct Library *))om_host_vector(OpenMakeMeshBase, 204))(ps, i, colour, OpenMakeMeshBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeMeshBase;
    register ULONG _a0 __asm("a0") = (ULONG)(ps);
    register ULONG _d0 __asm("d0") = (ULONG)(i);
    register ULONG _d1 __asm("d1") = (ULONG)(colour);
    __asm__ volatile("jsr a6@(-204:W)" : "+r"(_d0), "+r"(_d1), "+r"(_a0) : "r"(_a6) : "a1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ unsigned long GS_PartsRgb(int colour)
{
#ifdef OM_LIB_HOST
    return ((unsigned long (*)(int, struct Library *))om_host_vector(OpenMakeMeshBase, 210))(colour, OpenMakeMeshBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeMeshBase;
    register ULONG _d0 __asm("d0") = (ULONG)(colour);
    __asm__ volatile("jsr a6@(-210:W)" : "+r"(_d0) : "r"(_a6) : "d1", "a0", "a1", "fp0", "fp1", "cc", "memory");
    return (unsigned long)_d0;
#endif
}

static __inline__ int GS_PartsShown(const gs_parts *ps, int i)
{
#ifdef OM_LIB_HOST
    return ((int (*)(const gs_parts *, int, struct Library *))om_host_vector(OpenMakeMeshBase, 216))(ps, i, OpenMakeMeshBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeMeshBase;
    register ULONG _a0 __asm("a0") = (ULONG)(ps);
    register ULONG _d0 __asm("d0") = (ULONG)(i);
    __asm__ volatile("jsr a6@(-216:W)" : "+r"(_d0), "+r"(_a0) : "r"(_a6) : "d1", "a1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GS_PartsShow(gs_parts *ps, int i, int on)
{
#ifdef OM_LIB_HOST
    return ((int (*)(gs_parts *, int, int, struct Library *))om_host_vector(OpenMakeMeshBase, 222))(ps, i, on, OpenMakeMeshBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeMeshBase;
    register ULONG _a0 __asm("a0") = (ULONG)(ps);
    register ULONG _d0 __asm("d0") = (ULONG)(i);
    register ULONG _d1 __asm("d1") = (ULONG)(on);
    __asm__ volatile("jsr a6@(-222:W)" : "+r"(_d0), "+r"(_d1), "+r"(_a0) : "r"(_a6) : "a1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GS_PartsPicked(const gs_parts *ps)
{
#ifdef OM_LIB_HOST
    return ((int (*)(const gs_parts *, struct Library *))om_host_vector(OpenMakeMeshBase, 228))(ps, OpenMakeMeshBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeMeshBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(ps);
    __asm__ volatile("jsr a6@(-228:W)" : "=r"(_d0), "+r"(_a0) : "r"(_a6) : "d1", "a1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GS_PartsPick(gs_parts *ps, int i)
{
#ifdef OM_LIB_HOST
    return ((int (*)(gs_parts *, int, struct Library *))om_host_vector(OpenMakeMeshBase, 234))(ps, i, OpenMakeMeshBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeMeshBase;
    register ULONG _a0 __asm("a0") = (ULONG)(ps);
    register ULONG _d0 __asm("d0") = (ULONG)(i);
    __asm__ volatile("jsr a6@(-234:W)" : "+r"(_d0), "+r"(_a0) : "r"(_a6) : "d1", "a1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GS_PartsScene(const gs_parts *ps, gs_mesh *out)
{
#ifdef OM_LIB_HOST
    return ((int (*)(const gs_parts *, gs_mesh *, struct Library *))om_host_vector(OpenMakeMeshBase, 240))(ps, out, OpenMakeMeshBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeMeshBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(ps);
    register ULONG _a1 __asm("a1") = (ULONG)(out);
    __asm__ volatile("jsr a6@(-240:W)" : "=r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6) : "d1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ gs_parts * GS_PartsCopy(const gs_parts *ps)
{
#ifdef OM_LIB_HOST
    return ((gs_parts * (*)(const gs_parts *, struct Library *))om_host_vector(OpenMakeMeshBase, 246))(ps, OpenMakeMeshBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeMeshBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(ps);
    __asm__ volatile("jsr a6@(-246:W)" : "=r"(_d0), "+r"(_a0) : "r"(_a6) : "d1", "a1", "fp0", "fp1", "cc", "memory");
    return (gs_parts *)_d0;
#endif
}

#endif
