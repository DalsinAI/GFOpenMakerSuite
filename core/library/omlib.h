/* What every OpenMake library and its callers share: a float carried in a
 * data register as its bits, and REG() for a call's registers.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef OM_LIB_H
#define OM_LIB_H

struct Library;

#ifdef OM_LIB_HOST
#include <stdint.h>
typedef uint32_t ULONG;
/* On the host a slot holds a function: APTR is a function pointer here,
 * so the casts are ISO C's. */
typedef void (*APTR)(void);
#define REG(r, decl) decl
/* The host test's stand-in for the jump table: the call at bias. */
APTR om_host_vector(struct Library *base, int bias);
#else
#include <exec/types.h>
#define REG(r, decl) register decl __asm(#r)
#endif

static __inline__ ULONG om_lib_bits(float f)
{
    union {
        float f;
        ULONG u;
    } b;
    b.f = f;
    return b.u;
}

static __inline__ float om_lib_float(ULONG u)
{
    union {
        float f;
        ULONG u;
    } b;
    b.u = u;
    return b.f;
}

#endif
