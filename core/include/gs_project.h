/* A GFOpenMakerWorkshop project: one product's drawer and its project.om
 * manifest (the same "key = value" style as machines/ and settings/).
 * The Amiga build is to be openmakeproject.library; this is its C API.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef GS_PROJECT_H
#define GS_PROJECT_H

enum {
    GPR_NAME_MAX = 63,
    GPR_PATH_MAX = 255,
    GPR_NOTES_MAX = 255,
    GPR_KIND_MAX = 31,
    GPR_KEY_MAX = 31,
    GPR_ITEMS_MAX = 128,
    GPR_RECENT_MAX = 8
};

/* An unknown key kept so a later format or kind can add fields without
 * breaking a round trip. */
typedef struct {
    char key[GPR_KEY_MAX + 1];
    char value[GPR_PATH_MAX + 1];
} gpr_extra;

/* One item in the project. kind is the token as stored (open: an unknown
 * kind is valid). from is 1-based, or 0 if the item is not from another.
 * file is as the manifest has it (relative to the drawer, or absolute). */
typedef struct {
    char kind[GPR_KIND_MAX + 1];
    char name[GPR_NAME_MAX + 1];
    char file[GPR_PATH_MAX + 1];
    char settings[GPR_PATH_MAX + 1];
    int from;
    gpr_extra *extra;
    int nextra;
} gpr_item;

/* A project. size is sizeof(gpr_project) as the program was built. A
 * program calls gpr_init before create, open or parse. */
typedef struct {
    int size;
    char path[GPR_PATH_MAX + 1]; /* the drawer, no trailing slash or colon-slash */
    char name[GPR_NAME_MAX + 1];
    char notes[GPR_NOTES_MAX + 1];
    gpr_item *items;
    int n, cap;
    gpr_extra *extra;
    int nextra;
    int dirty;
} gpr_project;

/* Options for gpr_add. A program sets size to sizeof(gpr_add_opts) as it
 * was built; a shorter one is read as the defaults (copy -1, from 0,
 * name and settings unset). copy: -1 the default (copy a known suite
 * kind, reference electronics and an unknown kind), 0 reference, 1 copy. */
typedef struct {
    int size;
    const char *kind;
    const char *file;
    const char *name;
    int from;
    int copy;
    const char *settings;
} gpr_add_opts;

typedef struct {
    int size;
    char path[GPR_RECENT_MAX][GPR_PATH_MAX + 1];
    int n;
} gpr_recent;

void gpr_init(gpr_project *p);
void gpr_free(gpr_project *p);

/* Replaces p with the manifest text, or writes it. format returns the
 * length, or -1 if buf is too short. 0, or -1 with why in err. */
int gpr_parse(gpr_project *p, const char *text, char *err, int errlen);
int gpr_format(const gpr_project *p, char *buf, int len);

/* create makes the drawer, the kind subdrawers and an empty project.om
 * (name may be NULL: the drawer's name is used). open takes the drawer
 * or the project.om path. save writes project.om in p->path. */
int gpr_create(gpr_project *p, const char *drawer, const char *name, char *err, int errlen);
int gpr_open(gpr_project *p, const char *path, char *err, int errlen);
int gpr_save(gpr_project *p, char *err, int errlen);

int gpr_set_name(gpr_project *p, const char *name);
int gpr_set_notes(gpr_project *p, const char *notes);

void gpr_add_defaults(gpr_add_opts *o);
/* Adds an item. Copies or references file as o says. Returns the new
 * item's 0-based index, or -1 with why in err. */
int gpr_add(gpr_project *p, const gpr_add_opts *o, char *err, int errlen);
int gpr_remove(gpr_project *p, int index);
int gpr_rename(gpr_project *p, int index, const char *name);

/* 0-based index of a 1-based number ("3") or an item's name, or -1. */
int gpr_find(const gpr_project *p, const char *item);
/* The item's file as a full path in buf. 0, or -1. */
int gpr_resolve(const gpr_project *p, int index, char *buf, int buflen);

/* Known kinds are cad, print, laser, cnc and electronics. An unknown
 * kind is still valid in the manifest. gpr_kind_group returns the group
 * title (CAD, Print, …) or the token itself. App and port are the
 * program name and the ARexx port stem, or NULL if none is known.
 * gpr_kind_suggest is a default kind from a path's suffix, or NULL
 * (.gcode is not guessed: print and laser both use it). */
int gpr_kind_known(const char *kind);
const char *gpr_kind_group(const char *kind);
const char *gpr_kind_app(const char *kind);
const char *gpr_kind_port(const char *kind);
const char *gpr_kind_suggest(const char *path);

/* Sibling .kicad_pcb and .kicad_sch beside a .kicad_pro, found by name
 * in the same drawer, not by reading the files. Each buffer is filled
 * or emptied. Returns how many exist (0, 1 or 2), or -1 if pro is not a
 * .kicad_pro. buflen is the size of pcb and of sch. */
int gpr_kicad_files(const char *pro, char *pcb, char *sch, int buflen);

void gpr_recent_init(gpr_recent *r);
int gpr_recent_load(gpr_recent *r, const char *path, char *err, int errlen);
int gpr_recent_save(const gpr_recent *r, const char *path, char *err, int errlen);
/* Moves project to the front, unique, capped at GPR_RECENT_MAX. */
int gpr_recent_touch(gpr_recent *r, const char *project);

#endif
