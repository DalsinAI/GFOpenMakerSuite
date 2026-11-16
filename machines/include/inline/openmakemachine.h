/* Written by core/library/gen-library.py from openmakemachine_lib.sfd: do not edit. */
#ifndef INLINE_OPENMAKEMACHINE_H
#define INLINE_OPENMAKEMACHINE_H

#include "omlib.h"
#include <om_machine.h>

extern struct Library *OpenMakeMachineBase;

static __inline__ int OM_Parse(om_machine *m, const char *text, char *err, int errlen)
{
#ifdef OM_LIB_HOST
    return ((int (*)(om_machine *, const char *, char *, int, struct Library *))om_host_vector(OpenMakeMachineBase, 30))(m, text, err, errlen, OpenMakeMachineBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeMachineBase;
    register ULONG _a0 __asm("a0") = (ULONG)(m);
    register ULONG _a1 __asm("a1") = (ULONG)(text);
    register ULONG _a2 __asm("a2") = (ULONG)(err);
    register ULONG _d0 __asm("d0") = (ULONG)(errlen);
    __asm__ volatile("jsr a6@(-30:W)" : "+r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6), "r"(_a2) : "d1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int OM_Load(om_machine *m, const char *path, char *err, int errlen)
{
#ifdef OM_LIB_HOST
    return ((int (*)(om_machine *, const char *, char *, int, struct Library *))om_host_vector(OpenMakeMachineBase, 36))(m, path, err, errlen, OpenMakeMachineBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeMachineBase;
    register ULONG _a0 __asm("a0") = (ULONG)(m);
    register ULONG _a1 __asm("a1") = (ULONG)(path);
    register ULONG _a2 __asm("a2") = (ULONG)(err);
    register ULONG _d0 __asm("d0") = (ULONG)(errlen);
    __asm__ volatile("jsr a6@(-36:W)" : "+r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6), "r"(_a2) : "d1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ void OM_Clear(om_machine *m)
{
#ifdef OM_LIB_HOST
    ((void (*)(om_machine *, struct Library *))om_host_vector(OpenMakeMachineBase, 42))(m, OpenMakeMachineBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeMachineBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(m);
    __asm__ volatile("jsr a6@(-42:W)" : "=r"(_d0), "+r"(_a0) : "r"(_a6) : "d1", "a1", "fp0", "fp1", "cc", "memory");
#endif
}

static __inline__ int OM_Has(const om_machine *m, unsigned feature)
{
#ifdef OM_LIB_HOST
    return ((int (*)(const om_machine *, unsigned, struct Library *))om_host_vector(OpenMakeMachineBase, 48))(m, feature, OpenMakeMachineBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeMachineBase;
    register ULONG _a0 __asm("a0") = (ULONG)(m);
    register ULONG _d0 __asm("d0") = (ULONG)(feature);
    __asm__ volatile("jsr a6@(-48:W)" : "+r"(_d0), "+r"(_a0) : "r"(_a6) : "d1", "a1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int OM_Check(const om_machine *m, const om_job *job, char *why, int whylen)
{
#ifdef OM_LIB_HOST
    return ((int (*)(const om_machine *, const om_job *, char *, int, struct Library *))om_host_vector(OpenMakeMachineBase, 54))(m, job, why, whylen, OpenMakeMachineBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeMachineBase;
    register ULONG _a0 __asm("a0") = (ULONG)(m);
    register ULONG _a1 __asm("a1") = (ULONG)(job);
    register ULONG _a2 __asm("a2") = (ULONG)(why);
    register ULONG _d0 __asm("d0") = (ULONG)(whylen);
    __asm__ volatile("jsr a6@(-54:W)" : "+r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6), "r"(_a2) : "d1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ float OM_ClampSpeed(const om_machine *m, int axis, float v, int *held)
{
#ifdef OM_LIB_HOST
    return om_lib_float(((ULONG (*)(const om_machine *, int, ULONG, int *, struct Library *))om_host_vector(OpenMakeMachineBase, 60))(m, axis, om_lib_bits(v), held, OpenMakeMachineBase));
#else
    register struct Library *_a6 __asm("a6") = OpenMakeMachineBase;
    register ULONG _a0 __asm("a0") = (ULONG)(m);
    register ULONG _d0 __asm("d0") = (ULONG)(axis);
    register ULONG _d1 __asm("d1") = om_lib_bits(v);
    register ULONG _a1 __asm("a1") = (ULONG)(held);
    __asm__ volatile("jsr a6@(-60:W)" : "+r"(_d0), "+r"(_d1), "+r"(_a0), "+r"(_a1) : "r"(_a6) : "fp0", "fp1", "cc", "memory");
    return om_lib_float(_d0);
#endif
}

static __inline__ int OM_Code(const char *tmpl, float hotend, float bed, char *out, int outlen)
{
#ifdef OM_LIB_HOST
    return ((int (*)(const char *, ULONG, ULONG, char *, int, struct Library *))om_host_vector(OpenMakeMachineBase, 66))(tmpl, om_lib_bits(hotend), om_lib_bits(bed), out, outlen, OpenMakeMachineBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeMachineBase;
    register ULONG _a0 __asm("a0") = (ULONG)(tmpl);
    register ULONG _d0 __asm("d0") = om_lib_bits(hotend);
    register ULONG _d1 __asm("d1") = om_lib_bits(bed);
    register ULONG _a1 __asm("a1") = (ULONG)(out);
    register ULONG _d2 __asm("d2") = (ULONG)(outlen);
    __asm__ volatile("jsr a6@(-66:W)" : "+r"(_d0), "+r"(_d1), "+r"(_a0), "+r"(_a1) : "r"(_a6), "r"(_d2) : "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

#endif
