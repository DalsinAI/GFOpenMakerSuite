/* Written by core/library/gen-library.py from openmakeproject_lib.sfd: do not edit. */
#ifndef INLINE_OPENMAKEPROJECT_H
#define INLINE_OPENMAKEPROJECT_H

#include "omlib.h"
#include <gs_project.h>

extern struct Library *OpenMakeProjectBase;

static __inline__ void GPR_Init(gpr_project *p)
{
#ifdef OM_LIB_HOST
    ((void (*)(gpr_project *, struct Library *))om_host_vector(OpenMakeProjectBase, 30))(p, OpenMakeProjectBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeProjectBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(p);
    __asm__ volatile("jsr a6@(-30:W)" : "=r"(_d0), "+r"(_a0) : "r"(_a6) : "d1", "a1", "fp0", "fp1", "cc", "memory");
#endif
}

static __inline__ void GPR_Free(gpr_project *p)
{
#ifdef OM_LIB_HOST
    ((void (*)(gpr_project *, struct Library *))om_host_vector(OpenMakeProjectBase, 36))(p, OpenMakeProjectBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeProjectBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(p);
    __asm__ volatile("jsr a6@(-36:W)" : "=r"(_d0), "+r"(_a0) : "r"(_a6) : "d1", "a1", "fp0", "fp1", "cc", "memory");
#endif
}

static __inline__ int GPR_Parse(gpr_project *p, const char *text, char *err, int errlen)
{
#ifdef OM_LIB_HOST
    return ((int (*)(gpr_project *, const char *, char *, int, struct Library *))om_host_vector(OpenMakeProjectBase, 42))(p, text, err, errlen, OpenMakeProjectBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeProjectBase;
    register ULONG _a0 __asm("a0") = (ULONG)(p);
    register ULONG _a1 __asm("a1") = (ULONG)(text);
    register ULONG _a2 __asm("a2") = (ULONG)(err);
    register ULONG _d0 __asm("d0") = (ULONG)(errlen);
    __asm__ volatile("jsr a6@(-42:W)" : "+r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6), "r"(_a2) : "d1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GPR_Format(const gpr_project *p, char *buf, int len)
{
#ifdef OM_LIB_HOST
    return ((int (*)(const gpr_project *, char *, int, struct Library *))om_host_vector(OpenMakeProjectBase, 48))(p, buf, len, OpenMakeProjectBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeProjectBase;
    register ULONG _a0 __asm("a0") = (ULONG)(p);
    register ULONG _a1 __asm("a1") = (ULONG)(buf);
    register ULONG _d0 __asm("d0") = (ULONG)(len);
    __asm__ volatile("jsr a6@(-48:W)" : "+r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6) : "d1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GPR_Create(gpr_project *p, const char *drawer, const char *name, char *err, int errlen)
{
#ifdef OM_LIB_HOST
    return ((int (*)(gpr_project *, const char *, const char *, char *, int, struct Library *))om_host_vector(OpenMakeProjectBase, 54))(p, drawer, name, err, errlen, OpenMakeProjectBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeProjectBase;
    register ULONG _a0 __asm("a0") = (ULONG)(p);
    register ULONG _a1 __asm("a1") = (ULONG)(drawer);
    register ULONG _a2 __asm("a2") = (ULONG)(name);
    register ULONG _a3 __asm("a3") = (ULONG)(err);
    register ULONG _d0 __asm("d0") = (ULONG)(errlen);
    __asm__ volatile("jsr a6@(-54:W)" : "+r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6), "r"(_a2), "r"(_a3) : "d1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GPR_Open(gpr_project *p, const char *path, char *err, int errlen)
{
#ifdef OM_LIB_HOST
    return ((int (*)(gpr_project *, const char *, char *, int, struct Library *))om_host_vector(OpenMakeProjectBase, 60))(p, path, err, errlen, OpenMakeProjectBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeProjectBase;
    register ULONG _a0 __asm("a0") = (ULONG)(p);
    register ULONG _a1 __asm("a1") = (ULONG)(path);
    register ULONG _a2 __asm("a2") = (ULONG)(err);
    register ULONG _d0 __asm("d0") = (ULONG)(errlen);
    __asm__ volatile("jsr a6@(-60:W)" : "+r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6), "r"(_a2) : "d1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GPR_Save(gpr_project *p, char *err, int errlen)
{
#ifdef OM_LIB_HOST
    return ((int (*)(gpr_project *, char *, int, struct Library *))om_host_vector(OpenMakeProjectBase, 66))(p, err, errlen, OpenMakeProjectBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeProjectBase;
    register ULONG _a0 __asm("a0") = (ULONG)(p);
    register ULONG _a1 __asm("a1") = (ULONG)(err);
    register ULONG _d0 __asm("d0") = (ULONG)(errlen);
    __asm__ volatile("jsr a6@(-66:W)" : "+r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6) : "d1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GPR_SetName(gpr_project *p, const char *name)
{
#ifdef OM_LIB_HOST
    return ((int (*)(gpr_project *, const char *, struct Library *))om_host_vector(OpenMakeProjectBase, 72))(p, name, OpenMakeProjectBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeProjectBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(p);
    register ULONG _a1 __asm("a1") = (ULONG)(name);
    __asm__ volatile("jsr a6@(-72:W)" : "=r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6) : "d1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GPR_SetNotes(gpr_project *p, const char *notes)
{
#ifdef OM_LIB_HOST
    return ((int (*)(gpr_project *, const char *, struct Library *))om_host_vector(OpenMakeProjectBase, 78))(p, notes, OpenMakeProjectBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeProjectBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(p);
    register ULONG _a1 __asm("a1") = (ULONG)(notes);
    __asm__ volatile("jsr a6@(-78:W)" : "=r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6) : "d1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ void GPR_AddDefaults(gpr_add_opts *o)
{
#ifdef OM_LIB_HOST
    ((void (*)(gpr_add_opts *, struct Library *))om_host_vector(OpenMakeProjectBase, 84))(o, OpenMakeProjectBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeProjectBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(o);
    __asm__ volatile("jsr a6@(-84:W)" : "=r"(_d0), "+r"(_a0) : "r"(_a6) : "d1", "a1", "fp0", "fp1", "cc", "memory");
#endif
}

static __inline__ int GPR_Add(gpr_project *p, const gpr_add_opts *o, char *err, int errlen)
{
#ifdef OM_LIB_HOST
    return ((int (*)(gpr_project *, const gpr_add_opts *, char *, int, struct Library *))om_host_vector(OpenMakeProjectBase, 90))(p, o, err, errlen, OpenMakeProjectBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeProjectBase;
    register ULONG _a0 __asm("a0") = (ULONG)(p);
    register ULONG _a1 __asm("a1") = (ULONG)(o);
    register ULONG _a2 __asm("a2") = (ULONG)(err);
    register ULONG _d0 __asm("d0") = (ULONG)(errlen);
    __asm__ volatile("jsr a6@(-90:W)" : "+r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6), "r"(_a2) : "d1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GPR_Remove(gpr_project *p, int index)
{
#ifdef OM_LIB_HOST
    return ((int (*)(gpr_project *, int, struct Library *))om_host_vector(OpenMakeProjectBase, 96))(p, index, OpenMakeProjectBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeProjectBase;
    register ULONG _a0 __asm("a0") = (ULONG)(p);
    register ULONG _d0 __asm("d0") = (ULONG)(index);
    __asm__ volatile("jsr a6@(-96:W)" : "+r"(_d0), "+r"(_a0) : "r"(_a6) : "d1", "a1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GPR_Rename(gpr_project *p, int index, const char *name)
{
#ifdef OM_LIB_HOST
    return ((int (*)(gpr_project *, int, const char *, struct Library *))om_host_vector(OpenMakeProjectBase, 102))(p, index, name, OpenMakeProjectBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeProjectBase;
    register ULONG _a0 __asm("a0") = (ULONG)(p);
    register ULONG _d0 __asm("d0") = (ULONG)(index);
    register ULONG _a1 __asm("a1") = (ULONG)(name);
    __asm__ volatile("jsr a6@(-102:W)" : "+r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6) : "d1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GPR_Find(const gpr_project *p, const char *item)
{
#ifdef OM_LIB_HOST
    return ((int (*)(const gpr_project *, const char *, struct Library *))om_host_vector(OpenMakeProjectBase, 108))(p, item, OpenMakeProjectBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeProjectBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(p);
    register ULONG _a1 __asm("a1") = (ULONG)(item);
    __asm__ volatile("jsr a6@(-108:W)" : "=r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6) : "d1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GPR_Resolve(const gpr_project *p, int index, char *buf, int buflen)
{
#ifdef OM_LIB_HOST
    return ((int (*)(const gpr_project *, int, char *, int, struct Library *))om_host_vector(OpenMakeProjectBase, 114))(p, index, buf, buflen, OpenMakeProjectBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeProjectBase;
    register ULONG _a0 __asm("a0") = (ULONG)(p);
    register ULONG _d0 __asm("d0") = (ULONG)(index);
    register ULONG _a1 __asm("a1") = (ULONG)(buf);
    register ULONG _d1 __asm("d1") = (ULONG)(buflen);
    __asm__ volatile("jsr a6@(-114:W)" : "+r"(_d0), "+r"(_d1), "+r"(_a0), "+r"(_a1) : "r"(_a6) : "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GPR_KindKnown(const char *kind)
{
#ifdef OM_LIB_HOST
    return ((int (*)(const char *, struct Library *))om_host_vector(OpenMakeProjectBase, 120))(kind, OpenMakeProjectBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeProjectBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(kind);
    __asm__ volatile("jsr a6@(-120:W)" : "=r"(_d0), "+r"(_a0) : "r"(_a6) : "d1", "a1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ const char * GPR_KindGroup(const char *kind)
{
#ifdef OM_LIB_HOST
    return ((const char * (*)(const char *, struct Library *))om_host_vector(OpenMakeProjectBase, 126))(kind, OpenMakeProjectBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeProjectBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(kind);
    __asm__ volatile("jsr a6@(-126:W)" : "=r"(_d0), "+r"(_a0) : "r"(_a6) : "d1", "a1", "fp0", "fp1", "cc", "memory");
    return (const char *)_d0;
#endif
}

static __inline__ const char * GPR_KindApp(const char *kind)
{
#ifdef OM_LIB_HOST
    return ((const char * (*)(const char *, struct Library *))om_host_vector(OpenMakeProjectBase, 132))(kind, OpenMakeProjectBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeProjectBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(kind);
    __asm__ volatile("jsr a6@(-132:W)" : "=r"(_d0), "+r"(_a0) : "r"(_a6) : "d1", "a1", "fp0", "fp1", "cc", "memory");
    return (const char *)_d0;
#endif
}

static __inline__ const char * GPR_KindPort(const char *kind)
{
#ifdef OM_LIB_HOST
    return ((const char * (*)(const char *, struct Library *))om_host_vector(OpenMakeProjectBase, 138))(kind, OpenMakeProjectBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeProjectBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(kind);
    __asm__ volatile("jsr a6@(-138:W)" : "=r"(_d0), "+r"(_a0) : "r"(_a6) : "d1", "a1", "fp0", "fp1", "cc", "memory");
    return (const char *)_d0;
#endif
}

static __inline__ const char * GPR_KindSuggest(const char *path)
{
#ifdef OM_LIB_HOST
    return ((const char * (*)(const char *, struct Library *))om_host_vector(OpenMakeProjectBase, 144))(path, OpenMakeProjectBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeProjectBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(path);
    __asm__ volatile("jsr a6@(-144:W)" : "=r"(_d0), "+r"(_a0) : "r"(_a6) : "d1", "a1", "fp0", "fp1", "cc", "memory");
    return (const char *)_d0;
#endif
}

static __inline__ int GPR_KicadFiles(const char *pro, char *pcb, char *sch, int buflen)
{
#ifdef OM_LIB_HOST
    return ((int (*)(const char *, char *, char *, int, struct Library *))om_host_vector(OpenMakeProjectBase, 150))(pro, pcb, sch, buflen, OpenMakeProjectBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeProjectBase;
    register ULONG _a0 __asm("a0") = (ULONG)(pro);
    register ULONG _a1 __asm("a1") = (ULONG)(pcb);
    register ULONG _a2 __asm("a2") = (ULONG)(sch);
    register ULONG _d0 __asm("d0") = (ULONG)(buflen);
    __asm__ volatile("jsr a6@(-150:W)" : "+r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6), "r"(_a2) : "d1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ void GPR_RecentInit(gpr_recent *r)
{
#ifdef OM_LIB_HOST
    ((void (*)(gpr_recent *, struct Library *))om_host_vector(OpenMakeProjectBase, 156))(r, OpenMakeProjectBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeProjectBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(r);
    __asm__ volatile("jsr a6@(-156:W)" : "=r"(_d0), "+r"(_a0) : "r"(_a6) : "d1", "a1", "fp0", "fp1", "cc", "memory");
#endif
}

static __inline__ int GPR_RecentLoad(gpr_recent *r, const char *path, char *err, int errlen)
{
#ifdef OM_LIB_HOST
    return ((int (*)(gpr_recent *, const char *, char *, int, struct Library *))om_host_vector(OpenMakeProjectBase, 162))(r, path, err, errlen, OpenMakeProjectBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeProjectBase;
    register ULONG _a0 __asm("a0") = (ULONG)(r);
    register ULONG _a1 __asm("a1") = (ULONG)(path);
    register ULONG _a2 __asm("a2") = (ULONG)(err);
    register ULONG _d0 __asm("d0") = (ULONG)(errlen);
    __asm__ volatile("jsr a6@(-162:W)" : "+r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6), "r"(_a2) : "d1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GPR_RecentSave(const gpr_recent *r, const char *path, char *err, int errlen)
{
#ifdef OM_LIB_HOST
    return ((int (*)(const gpr_recent *, const char *, char *, int, struct Library *))om_host_vector(OpenMakeProjectBase, 168))(r, path, err, errlen, OpenMakeProjectBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeProjectBase;
    register ULONG _a0 __asm("a0") = (ULONG)(r);
    register ULONG _a1 __asm("a1") = (ULONG)(path);
    register ULONG _a2 __asm("a2") = (ULONG)(err);
    register ULONG _d0 __asm("d0") = (ULONG)(errlen);
    __asm__ volatile("jsr a6@(-168:W)" : "+r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6), "r"(_a2) : "d1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int GPR_RecentTouch(gpr_recent *r, const char *project)
{
#ifdef OM_LIB_HOST
    return ((int (*)(gpr_recent *, const char *, struct Library *))om_host_vector(OpenMakeProjectBase, 174))(r, project, OpenMakeProjectBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeProjectBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(r);
    register ULONG _a1 __asm("a1") = (ULONG)(project);
    __asm__ volatile("jsr a6@(-174:W)" : "=r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6) : "d1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

#endif
