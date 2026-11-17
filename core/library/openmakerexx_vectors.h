/* Written by core/library/gen-library.py from openmakerexx_lib.sfd: do not edit.
 * The library's side of each call: its registers to the C function. */
#include "omlib.h"
#include <gs_rexx.h>

static omr_port * LIB_OMR_Open(REG(a0, const char *base_name), REG(a1, const char *version), REG(a6, struct Library *base))
{
    (void)base;
    return omr_open(base_name, version);
}

static void LIB_OMR_Close(REG(a0, omr_port *port), REG(a6, struct Library *base))
{
    (void)base;
    omr_close(port);
}

static unsigned long LIB_OMR_Mask(REG(a0, omr_port *port), REG(a6, struct Library *base))
{
    (void)base;
    return omr_mask(port);
}

static const char * LIB_OMR_Name(REG(a0, omr_port *port), REG(a6, struct Library *base))
{
    (void)base;
    return omr_name(port);
}

static omr_cmd * LIB_OMR_Get(REG(a0, omr_port *port), REG(a6, struct Library *base))
{
    (void)base;
    return omr_get(port);
}

static const char * LIB_OMR_Text(REG(a0, omr_cmd *cmd), REG(a6, struct Library *base))
{
    (void)base;
    return omr_text(cmd);
}

static int LIB_OMR_Parse(REG(a0, omr_cmd *cmd), REG(a1, const char *template), REG(a6, struct Library *base))
{
    (void)base;
    return omr_parse(cmd, template);
}

static const char * LIB_OMR_Why(REG(a0, omr_cmd *cmd), REG(a6, struct Library *base))
{
    (void)base;
    return omr_why(cmd);
}

static const char * LIB_OMR_Arg(REG(a0, omr_cmd *cmd), REG(a1, const char *key), REG(a6, struct Library *base))
{
    (void)base;
    return omr_arg(cmd, key);
}

static int LIB_OMR_Switch(REG(a0, omr_cmd *cmd), REG(a1, const char *key), REG(a6, struct Library *base))
{
    (void)base;
    return omr_switch(cmd, key);
}

static void LIB_OMR_SetResult(REG(a0, omr_cmd *cmd), REG(a1, const char *result), REG(a6, struct Library *base))
{
    (void)base;
    omr_set_result(cmd, result);
}

static int LIB_OMR_SetVar(REG(a0, omr_cmd *cmd), REG(a1, const char *name), REG(a2, const char *value), REG(a6, struct Library *base))
{
    (void)base;
    return omr_set_var(cmd, name, value);
}

static int LIB_OMR_SetStem(REG(a0, omr_cmd *cmd), REG(a1, const char *stem), REG(a2, const char *tail), REG(a3, const char *value), REG(a6, struct Library *base))
{
    (void)base;
    return omr_set_stem(cmd, stem, tail, value);
}

static void LIB_OMR_Reply(REG(a0, omr_cmd *cmd), REG(d0, int rc), REG(a1, const char *result), REG(a6, struct Library *base))
{
    (void)base;
    omr_reply(cmd, rc, result);
}

static int LIB_OMR_Dispatch(REG(a0, omr_cmd *cmd), REG(a1, const omr_verb *verbs), REG(d0, int n), REG(a2, void *app), REG(a6, struct Library *base))
{
    (void)base;
    return omr_dispatch(cmd, verbs, n, app);
}

static int LIB_OMR_DoLine(REG(a0, const char *line), REG(a1, const omr_verb *verbs), REG(d0, int n), REG(a2, void *app), REG(a3, omr_io *io), REG(a6, struct Library *base))
{
    (void)base;
    return omr_do_line(line, verbs, n, app, io);
}

static int LIB_OMR_Attach(REG(a0, omr_port *port), REG(a1, const omr_verb *verbs), REG(d0, int n), REG(a2, void *app), REG(a6, struct Library *base))
{
    (void)base;
    return omr_attach(port, verbs, n, app);
}

static int LIB_OMR_Find(REG(a0, const char *name), REG(a1, char *found), REG(d0, int foundlen), REG(a6, struct Library *base))
{
    (void)base;
    return omr_find(name, found, foundlen);
}

static omr_client * LIB_OMR_Connect(REG(a0, const char *port_base), REG(a1, omr_opt *opt), REG(a6, struct Library *base))
{
    (void)base;
    return omr_connect(port_base, opt);
}

static void LIB_OMR_Disconnect(REG(a0, omr_client *client), REG(a6, struct Library *base))
{
    (void)base;
    omr_disconnect(client);
}

static int LIB_OMR_Send(REG(a0, omr_client *client), REG(a1, const char *command), REG(a2, omr_io *io), REG(a6, struct Library *base))
{
    (void)base;
    return omr_send(client, command, io);
}

#define OPENMAKEREXX_LAST_BIAS 150
#define OPENMAKEREXX_VECTORS \
    (APTR)LIB_OMR_Open, \
    (APTR)LIB_OMR_Close, \
    (APTR)LIB_OMR_Mask, \
    (APTR)LIB_OMR_Name, \
    (APTR)LIB_OMR_Get, \
    (APTR)LIB_OMR_Text, \
    (APTR)LIB_OMR_Parse, \
    (APTR)LIB_OMR_Why, \
    (APTR)LIB_OMR_Arg, \
    (APTR)LIB_OMR_Switch, \
    (APTR)LIB_OMR_SetResult, \
    (APTR)LIB_OMR_SetVar, \
    (APTR)LIB_OMR_SetStem, \
    (APTR)LIB_OMR_Reply, \
    (APTR)LIB_OMR_Dispatch, \
    (APTR)LIB_OMR_DoLine, \
    (APTR)LIB_OMR_Attach, \
    (APTR)LIB_OMR_Find, \
    (APTR)LIB_OMR_Connect, \
    (APTR)LIB_OMR_Disconnect, \
    (APTR)LIB_OMR_Send
