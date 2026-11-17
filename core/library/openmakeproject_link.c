/* Written by core/library/gen-library.py from openmakeproject_lib.sfd: do not edit.
 * The C API through openmakeproject.library: a program links this instead of
 * the sources, and opens OpenMakeProjectBase itself. */
#include <proto/openmakeproject.h>

void gpr_init(gpr_project *p)
{
    GPR_Init(p);
}

void gpr_free(gpr_project *p)
{
    GPR_Free(p);
}

int gpr_parse(gpr_project *p, const char *text, char *err, int errlen)
{
    return GPR_Parse(p, text, err, errlen);
}

int gpr_format(const gpr_project *p, char *buf, int len)
{
    return GPR_Format(p, buf, len);
}

int gpr_create(gpr_project *p, const char *drawer, const char *name, char *err, int errlen)
{
    return GPR_Create(p, drawer, name, err, errlen);
}

int gpr_open(gpr_project *p, const char *path, char *err, int errlen)
{
    return GPR_Open(p, path, err, errlen);
}

int gpr_save(gpr_project *p, char *err, int errlen)
{
    return GPR_Save(p, err, errlen);
}

int gpr_set_name(gpr_project *p, const char *name)
{
    return GPR_SetName(p, name);
}

int gpr_set_notes(gpr_project *p, const char *notes)
{
    return GPR_SetNotes(p, notes);
}

void gpr_add_defaults(gpr_add_opts *o)
{
    GPR_AddDefaults(o);
}

int gpr_add(gpr_project *p, const gpr_add_opts *o, char *err, int errlen)
{
    return GPR_Add(p, o, err, errlen);
}

int gpr_remove(gpr_project *p, int index)
{
    return GPR_Remove(p, index);
}

int gpr_rename(gpr_project *p, int index, const char *name)
{
    return GPR_Rename(p, index, name);
}

int gpr_find(const gpr_project *p, const char *item)
{
    return GPR_Find(p, item);
}

int gpr_resolve(const gpr_project *p, int index, char *buf, int buflen)
{
    return GPR_Resolve(p, index, buf, buflen);
}

int gpr_kind_known(const char *kind)
{
    return GPR_KindKnown(kind);
}

const char * gpr_kind_group(const char *kind)
{
    return GPR_KindGroup(kind);
}

const char * gpr_kind_app(const char *kind)
{
    return GPR_KindApp(kind);
}

const char * gpr_kind_port(const char *kind)
{
    return GPR_KindPort(kind);
}

const char * gpr_kind_suggest(const char *path)
{
    return GPR_KindSuggest(path);
}

int gpr_kicad_files(const char *pro, char *pcb, char *sch, int buflen)
{
    return GPR_KicadFiles(pro, pcb, sch, buflen);
}

void gpr_recent_init(gpr_recent *r)
{
    GPR_RecentInit(r);
}

int gpr_recent_load(gpr_recent *r, const char *path, char *err, int errlen)
{
    return GPR_RecentLoad(r, path, err, errlen);
}

int gpr_recent_save(const gpr_recent *r, const char *path, char *err, int errlen)
{
    return GPR_RecentSave(r, path, err, errlen);
}

int gpr_recent_touch(gpr_recent *r, const char *project)
{
    return GPR_RecentTouch(r, project);
}
