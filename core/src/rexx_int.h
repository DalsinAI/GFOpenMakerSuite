/* Internal to openmakerexx (rexx.c, rexx_amiga.c). Not a public header.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef REXX_INT_H
#define REXX_INT_H

#include "gs_rexx.h"

#define OMR_NAME 40
#define OMR_VER 80
#define OMR_LINE 1024
#define OMR_WHY 160
#define OMR_KEY 32
#define OMR_ARGS 16
#define OMR_VARS 32
#define OMR_DEFAULT_MS 30000

struct omr_slot {
    char key[OMR_KEY];
    char alias[OMR_KEY];
    char *value;
    int is_switch, is_keyword, is_num, is_final, required, present;
};

struct omr_var {
    char name[80];
    char value[OMR_LINE];
};

struct omr_cmd {
    struct omr_cmd *next;
    char text[OMR_LINE];
    char verb[OMR_KEY];
    char why[OMR_WHY];
    char result[OMR_LINE];
    struct omr_slot a[OMR_ARGS];
    int n, used, rc, replied, parsed;
    char storage[OMR_LINE * 2];
    omr_port *port;
    struct omr_var vars[OMR_VARS];
    int nvar;
#if defined(AMIGA) || defined(__amigaos__)
    struct RexxMsg *msg;
#endif
};

struct omr_port {
    struct omr_port *next;
    char name[OMR_NAME];
    char version[OMR_VER];
    const omr_verb *verbs;
    int nverbs;
    void *app;
    struct omr_cmd *queue;
    struct omr_cmd **qtail;
#if defined(AMIGA) || defined(__amigaos__)
    struct MsgPort *mp;
#endif
};

struct omr_client {
    char name[OMR_NAME];
    omr_port *local;
};

int omr_eqi(const char *a, const char *b);
int omr_timeout_ms(int timeout);
int omr_io_ok(const omr_io *io);
void omr_io_fail(omr_io *io, const char *why);
void omr_io_copy(omr_io *io, int rc, const char *result);
int omr_cmd_load(omr_cmd *cmd, const char *line);
void omr_cmd_wipe(omr_cmd *cmd);
int omr_ends_index(const char *s);
int omr_verb_ok(const omr_verb *v);
const omr_verb *omr_verb_at(const omr_verb *verbs, int n, int i);
void omr_host_vars_from(const omr_cmd *cmd);

#endif
