/* Written by core/library/gen-library.py from openmakerexx_lib.sfd: do not edit. */
#ifndef INLINE_OPENMAKEREXX_H
#define INLINE_OPENMAKEREXX_H

#include "omlib.h"
#include <gs_rexx.h>

extern struct Library *OpenMakeRexxBase;

static __inline__ omr_port * OMR_Open(const char *base_name, const char *version)
{
#ifdef OM_LIB_HOST
    return ((omr_port * (*)(const char *, const char *, struct Library *))om_host_vector(OpenMakeRexxBase, 30))(base_name, version, OpenMakeRexxBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeRexxBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(base_name);
    register ULONG _a1 __asm("a1") = (ULONG)(version);
    __asm__ volatile("jsr a6@(-30:W)" : "=r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6) : "d1", "fp0", "fp1", "cc", "memory");
    return (omr_port *)_d0;
#endif
}

static __inline__ void OMR_Close(omr_port *port)
{
#ifdef OM_LIB_HOST
    ((void (*)(omr_port *, struct Library *))om_host_vector(OpenMakeRexxBase, 36))(port, OpenMakeRexxBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeRexxBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(port);
    __asm__ volatile("jsr a6@(-36:W)" : "=r"(_d0), "+r"(_a0) : "r"(_a6) : "d1", "a1", "fp0", "fp1", "cc", "memory");
#endif
}

static __inline__ unsigned long OMR_Mask(omr_port *port)
{
#ifdef OM_LIB_HOST
    return ((unsigned long (*)(omr_port *, struct Library *))om_host_vector(OpenMakeRexxBase, 42))(port, OpenMakeRexxBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeRexxBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(port);
    __asm__ volatile("jsr a6@(-42:W)" : "=r"(_d0), "+r"(_a0) : "r"(_a6) : "d1", "a1", "fp0", "fp1", "cc", "memory");
    return (unsigned long)_d0;
#endif
}

static __inline__ const char * OMR_Name(omr_port *port)
{
#ifdef OM_LIB_HOST
    return ((const char * (*)(omr_port *, struct Library *))om_host_vector(OpenMakeRexxBase, 48))(port, OpenMakeRexxBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeRexxBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(port);
    __asm__ volatile("jsr a6@(-48:W)" : "=r"(_d0), "+r"(_a0) : "r"(_a6) : "d1", "a1", "fp0", "fp1", "cc", "memory");
    return (const char *)_d0;
#endif
}

static __inline__ omr_cmd * OMR_Get(omr_port *port)
{
#ifdef OM_LIB_HOST
    return ((omr_cmd * (*)(omr_port *, struct Library *))om_host_vector(OpenMakeRexxBase, 54))(port, OpenMakeRexxBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeRexxBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(port);
    __asm__ volatile("jsr a6@(-54:W)" : "=r"(_d0), "+r"(_a0) : "r"(_a6) : "d1", "a1", "fp0", "fp1", "cc", "memory");
    return (omr_cmd *)_d0;
#endif
}

static __inline__ const char * OMR_Text(omr_cmd *cmd)
{
#ifdef OM_LIB_HOST
    return ((const char * (*)(omr_cmd *, struct Library *))om_host_vector(OpenMakeRexxBase, 60))(cmd, OpenMakeRexxBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeRexxBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(cmd);
    __asm__ volatile("jsr a6@(-60:W)" : "=r"(_d0), "+r"(_a0) : "r"(_a6) : "d1", "a1", "fp0", "fp1", "cc", "memory");
    return (const char *)_d0;
#endif
}

static __inline__ int OMR_Parse(omr_cmd *cmd, const char *template)
{
#ifdef OM_LIB_HOST
    return ((int (*)(omr_cmd *, const char *, struct Library *))om_host_vector(OpenMakeRexxBase, 66))(cmd, template, OpenMakeRexxBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeRexxBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(cmd);
    register ULONG _a1 __asm("a1") = (ULONG)(template);
    __asm__ volatile("jsr a6@(-66:W)" : "=r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6) : "d1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ const char * OMR_Why(omr_cmd *cmd)
{
#ifdef OM_LIB_HOST
    return ((const char * (*)(omr_cmd *, struct Library *))om_host_vector(OpenMakeRexxBase, 72))(cmd, OpenMakeRexxBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeRexxBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(cmd);
    __asm__ volatile("jsr a6@(-72:W)" : "=r"(_d0), "+r"(_a0) : "r"(_a6) : "d1", "a1", "fp0", "fp1", "cc", "memory");
    return (const char *)_d0;
#endif
}

static __inline__ const char * OMR_Arg(omr_cmd *cmd, const char *key)
{
#ifdef OM_LIB_HOST
    return ((const char * (*)(omr_cmd *, const char *, struct Library *))om_host_vector(OpenMakeRexxBase, 78))(cmd, key, OpenMakeRexxBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeRexxBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(cmd);
    register ULONG _a1 __asm("a1") = (ULONG)(key);
    __asm__ volatile("jsr a6@(-78:W)" : "=r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6) : "d1", "fp0", "fp1", "cc", "memory");
    return (const char *)_d0;
#endif
}

static __inline__ int OMR_Switch(omr_cmd *cmd, const char *key)
{
#ifdef OM_LIB_HOST
    return ((int (*)(omr_cmd *, const char *, struct Library *))om_host_vector(OpenMakeRexxBase, 84))(cmd, key, OpenMakeRexxBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeRexxBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(cmd);
    register ULONG _a1 __asm("a1") = (ULONG)(key);
    __asm__ volatile("jsr a6@(-84:W)" : "=r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6) : "d1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ void OMR_SetResult(omr_cmd *cmd, const char *result)
{
#ifdef OM_LIB_HOST
    ((void (*)(omr_cmd *, const char *, struct Library *))om_host_vector(OpenMakeRexxBase, 90))(cmd, result, OpenMakeRexxBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeRexxBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(cmd);
    register ULONG _a1 __asm("a1") = (ULONG)(result);
    __asm__ volatile("jsr a6@(-90:W)" : "=r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6) : "d1", "fp0", "fp1", "cc", "memory");
#endif
}

static __inline__ int OMR_SetVar(omr_cmd *cmd, const char *name, const char *value)
{
#ifdef OM_LIB_HOST
    return ((int (*)(omr_cmd *, const char *, const char *, struct Library *))om_host_vector(OpenMakeRexxBase, 96))(cmd, name, value, OpenMakeRexxBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeRexxBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(cmd);
    register ULONG _a1 __asm("a1") = (ULONG)(name);
    register ULONG _a2 __asm("a2") = (ULONG)(value);
    __asm__ volatile("jsr a6@(-96:W)" : "=r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6), "r"(_a2) : "d1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int OMR_SetStem(omr_cmd *cmd, const char *stem, const char *tail, const char *value)
{
#ifdef OM_LIB_HOST
    return ((int (*)(omr_cmd *, const char *, const char *, const char *, struct Library *))om_host_vector(OpenMakeRexxBase, 102))(cmd, stem, tail, value, OpenMakeRexxBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeRexxBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(cmd);
    register ULONG _a1 __asm("a1") = (ULONG)(stem);
    register ULONG _a2 __asm("a2") = (ULONG)(tail);
    register ULONG _a3 __asm("a3") = (ULONG)(value);
    __asm__ volatile("jsr a6@(-102:W)" : "=r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6), "r"(_a2), "r"(_a3) : "d1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ void OMR_Reply(omr_cmd *cmd, int rc, const char *result)
{
#ifdef OM_LIB_HOST
    ((void (*)(omr_cmd *, int, const char *, struct Library *))om_host_vector(OpenMakeRexxBase, 108))(cmd, rc, result, OpenMakeRexxBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeRexxBase;
    register ULONG _a0 __asm("a0") = (ULONG)(cmd);
    register ULONG _d0 __asm("d0") = (ULONG)(rc);
    register ULONG _a1 __asm("a1") = (ULONG)(result);
    __asm__ volatile("jsr a6@(-108:W)" : "+r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6) : "d1", "fp0", "fp1", "cc", "memory");
#endif
}

static __inline__ int OMR_Dispatch(omr_cmd *cmd, const omr_verb *verbs, int n, void *app)
{
#ifdef OM_LIB_HOST
    return ((int (*)(omr_cmd *, const omr_verb *, int, void *, struct Library *))om_host_vector(OpenMakeRexxBase, 114))(cmd, verbs, n, app, OpenMakeRexxBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeRexxBase;
    register ULONG _a0 __asm("a0") = (ULONG)(cmd);
    register ULONG _a1 __asm("a1") = (ULONG)(verbs);
    register ULONG _d0 __asm("d0") = (ULONG)(n);
    register ULONG _a2 __asm("a2") = (ULONG)(app);
    __asm__ volatile("jsr a6@(-114:W)" : "+r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6), "r"(_a2) : "d1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int OMR_DoLine(const char *line, const omr_verb *verbs, int n, void *app, omr_io *io)
{
#ifdef OM_LIB_HOST
    return ((int (*)(const char *, const omr_verb *, int, void *, omr_io *, struct Library *))om_host_vector(OpenMakeRexxBase, 120))(line, verbs, n, app, io, OpenMakeRexxBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeRexxBase;
    register ULONG _a0 __asm("a0") = (ULONG)(line);
    register ULONG _a1 __asm("a1") = (ULONG)(verbs);
    register ULONG _d0 __asm("d0") = (ULONG)(n);
    register ULONG _a2 __asm("a2") = (ULONG)(app);
    register ULONG _a3 __asm("a3") = (ULONG)(io);
    __asm__ volatile("jsr a6@(-120:W)" : "+r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6), "r"(_a2), "r"(_a3) : "d1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int OMR_Attach(omr_port *port, const omr_verb *verbs, int n, void *app)
{
#ifdef OM_LIB_HOST
    return ((int (*)(omr_port *, const omr_verb *, int, void *, struct Library *))om_host_vector(OpenMakeRexxBase, 126))(port, verbs, n, app, OpenMakeRexxBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeRexxBase;
    register ULONG _a0 __asm("a0") = (ULONG)(port);
    register ULONG _a1 __asm("a1") = (ULONG)(verbs);
    register ULONG _d0 __asm("d0") = (ULONG)(n);
    register ULONG _a2 __asm("a2") = (ULONG)(app);
    __asm__ volatile("jsr a6@(-126:W)" : "+r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6), "r"(_a2) : "d1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ int OMR_Find(const char *name, char *found, int foundlen)
{
#ifdef OM_LIB_HOST
    return ((int (*)(const char *, char *, int, struct Library *))om_host_vector(OpenMakeRexxBase, 132))(name, found, foundlen, OpenMakeRexxBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeRexxBase;
    register ULONG _a0 __asm("a0") = (ULONG)(name);
    register ULONG _a1 __asm("a1") = (ULONG)(found);
    register ULONG _d0 __asm("d0") = (ULONG)(foundlen);
    __asm__ volatile("jsr a6@(-132:W)" : "+r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6) : "d1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

static __inline__ omr_client * OMR_Connect(const char *port_base, omr_opt *opt)
{
#ifdef OM_LIB_HOST
    return ((omr_client * (*)(const char *, omr_opt *, struct Library *))om_host_vector(OpenMakeRexxBase, 138))(port_base, opt, OpenMakeRexxBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeRexxBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(port_base);
    register ULONG _a1 __asm("a1") = (ULONG)(opt);
    __asm__ volatile("jsr a6@(-138:W)" : "=r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6) : "d1", "fp0", "fp1", "cc", "memory");
    return (omr_client *)_d0;
#endif
}

static __inline__ void OMR_Disconnect(omr_client *client)
{
#ifdef OM_LIB_HOST
    ((void (*)(omr_client *, struct Library *))om_host_vector(OpenMakeRexxBase, 144))(client, OpenMakeRexxBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeRexxBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(client);
    __asm__ volatile("jsr a6@(-144:W)" : "=r"(_d0), "+r"(_a0) : "r"(_a6) : "d1", "a1", "fp0", "fp1", "cc", "memory");
#endif
}

static __inline__ int OMR_Send(omr_client *client, const char *command, omr_io *io)
{
#ifdef OM_LIB_HOST
    return ((int (*)(omr_client *, const char *, omr_io *, struct Library *))om_host_vector(OpenMakeRexxBase, 150))(client, command, io, OpenMakeRexxBase);
#else
    register struct Library *_a6 __asm("a6") = OpenMakeRexxBase;
    register ULONG _d0 __asm("d0");
    register ULONG _a0 __asm("a0") = (ULONG)(client);
    register ULONG _a1 __asm("a1") = (ULONG)(command);
    register ULONG _a2 __asm("a2") = (ULONG)(io);
    __asm__ volatile("jsr a6@(-150:W)" : "=r"(_d0), "+r"(_a0), "+r"(_a1) : "r"(_a6), "r"(_a2) : "d1", "fp0", "fp1", "cc", "memory");
    return (int)_d0;
#endif
}

#endif
