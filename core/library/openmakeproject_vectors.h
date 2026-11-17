/* Written by core/library/gen-library.py from openmakeproject_lib.sfd: do not edit.
 * The library's side of each call: its registers to the C function. */
#include "omlib.h"
#include <gs_project.h>

static void LIB_GPR_Init(REG(a0, gpr_project *p), REG(a6, struct Library *base))
{
    (void)base;
    gpr_init(p);
}

static void LIB_GPR_Free(REG(a0, gpr_project *p), REG(a6, struct Library *base))
{
    (void)base;
    gpr_free(p);
}

static int LIB_GPR_Parse(REG(a0, gpr_project *p), REG(a1, const char *text), REG(a2, char *err), REG(d0, int errlen), REG(a6, struct Library *base))
{
    (void)base;
    return gpr_parse(p, text, err, errlen);
}

static int LIB_GPR_Format(REG(a0, const gpr_project *p), REG(a1, char *buf), REG(d0, int len), REG(a6, struct Library *base))
{
    (void)base;
    return gpr_format(p, buf, len);
}

static int LIB_GPR_Create(REG(a0, gpr_project *p), REG(a1, const char *drawer), REG(a2, const char *name), REG(a3, char *err), REG(d0, int errlen), REG(a6, struct Library *base))
{
    (void)base;
    return gpr_create(p, drawer, name, err, errlen);
}

static int LIB_GPR_Open(REG(a0, gpr_project *p), REG(a1, const char *path), REG(a2, char *err), REG(d0, int errlen), REG(a6, struct Library *base))
{
    (void)base;
    return gpr_open(p, path, err, errlen);
}

static int LIB_GPR_Save(REG(a0, gpr_project *p), REG(a1, char *err), REG(d0, int errlen), REG(a6, struct Library *base))
{
    (void)base;
    return gpr_save(p, err, errlen);
}

static int LIB_GPR_SetName(REG(a0, gpr_project *p), REG(a1, const char *name), REG(a6, struct Library *base))
{
    (void)base;
    return gpr_set_name(p, name);
}

static int LIB_GPR_SetNotes(REG(a0, gpr_project *p), REG(a1, const char *notes), REG(a6, struct Library *base))
{
    (void)base;
    return gpr_set_notes(p, notes);
}

static void LIB_GPR_AddDefaults(REG(a0, gpr_add_opts *o), REG(a6, struct Library *base))
{
    (void)base;
    gpr_add_defaults(o);
}

static int LIB_GPR_Add(REG(a0, gpr_project *p), REG(a1, const gpr_add_opts *o), REG(a2, char *err), REG(d0, int errlen), REG(a6, struct Library *base))
{
    (void)base;
    return gpr_add(p, o, err, errlen);
}

static int LIB_GPR_Remove(REG(a0, gpr_project *p), REG(d0, int index), REG(a6, struct Library *base))
{
    (void)base;
    return gpr_remove(p, index);
}

static int LIB_GPR_Rename(REG(a0, gpr_project *p), REG(d0, int index), REG(a1, const char *name), REG(a6, struct Library *base))
{
    (void)base;
    return gpr_rename(p, index, name);
}

static int LIB_GPR_Find(REG(a0, const gpr_project *p), REG(a1, const char *item), REG(a6, struct Library *base))
{
    (void)base;
    return gpr_find(p, item);
}

static int LIB_GPR_Resolve(REG(a0, const gpr_project *p), REG(d0, int index), REG(a1, char *buf), REG(d1, int buflen), REG(a6, struct Library *base))
{
    (void)base;
    return gpr_resolve(p, index, buf, buflen);
}

static int LIB_GPR_KindKnown(REG(a0, const char *kind), REG(a6, struct Library *base))
{
    (void)base;
    return gpr_kind_known(kind);
}

static const char * LIB_GPR_KindGroup(REG(a0, const char *kind), REG(a6, struct Library *base))
{
    (void)base;
    return gpr_kind_group(kind);
}

static const char * LIB_GPR_KindApp(REG(a0, const char *kind), REG(a6, struct Library *base))
{
    (void)base;
    return gpr_kind_app(kind);
}

static const char * LIB_GPR_KindPort(REG(a0, const char *kind), REG(a6, struct Library *base))
{
    (void)base;
    return gpr_kind_port(kind);
}

static const char * LIB_GPR_KindSuggest(REG(a0, const char *path), REG(a6, struct Library *base))
{
    (void)base;
    return gpr_kind_suggest(path);
}

static int LIB_GPR_KicadFiles(REG(a0, const char *pro), REG(a1, char *pcb), REG(a2, char *sch), REG(d0, int buflen), REG(a6, struct Library *base))
{
    (void)base;
    return gpr_kicad_files(pro, pcb, sch, buflen);
}

static void LIB_GPR_RecentInit(REG(a0, gpr_recent *r), REG(a6, struct Library *base))
{
    (void)base;
    gpr_recent_init(r);
}

static int LIB_GPR_RecentLoad(REG(a0, gpr_recent *r), REG(a1, const char *path), REG(a2, char *err), REG(d0, int errlen), REG(a6, struct Library *base))
{
    (void)base;
    return gpr_recent_load(r, path, err, errlen);
}

static int LIB_GPR_RecentSave(REG(a0, const gpr_recent *r), REG(a1, const char *path), REG(a2, char *err), REG(d0, int errlen), REG(a6, struct Library *base))
{
    (void)base;
    return gpr_recent_save(r, path, err, errlen);
}

static int LIB_GPR_RecentTouch(REG(a0, gpr_recent *r), REG(a1, const char *project), REG(a6, struct Library *base))
{
    (void)base;
    return gpr_recent_touch(r, project);
}

#define OPENMAKEPROJECT_LAST_BIAS 174
#define OPENMAKEPROJECT_VECTORS \
    (APTR)LIB_GPR_Init, \
    (APTR)LIB_GPR_Free, \
    (APTR)LIB_GPR_Parse, \
    (APTR)LIB_GPR_Format, \
    (APTR)LIB_GPR_Create, \
    (APTR)LIB_GPR_Open, \
    (APTR)LIB_GPR_Save, \
    (APTR)LIB_GPR_SetName, \
    (APTR)LIB_GPR_SetNotes, \
    (APTR)LIB_GPR_AddDefaults, \
    (APTR)LIB_GPR_Add, \
    (APTR)LIB_GPR_Remove, \
    (APTR)LIB_GPR_Rename, \
    (APTR)LIB_GPR_Find, \
    (APTR)LIB_GPR_Resolve, \
    (APTR)LIB_GPR_KindKnown, \
    (APTR)LIB_GPR_KindGroup, \
    (APTR)LIB_GPR_KindApp, \
    (APTR)LIB_GPR_KindPort, \
    (APTR)LIB_GPR_KindSuggest, \
    (APTR)LIB_GPR_KicadFiles, \
    (APTR)LIB_GPR_RecentInit, \
    (APTR)LIB_GPR_RecentLoad, \
    (APTR)LIB_GPR_RecentSave, \
    (APTR)LIB_GPR_RecentTouch
