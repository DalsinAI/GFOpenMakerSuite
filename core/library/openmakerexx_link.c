/* Written by core/library/gen-library.py from openmakerexx_lib.sfd: do not edit.
 * The C API through openmakerexx.library: a program links this instead of
 * the sources, and opens OpenMakeRexxBase itself. */
#include <proto/openmakerexx.h>

omr_port * omr_open(const char *base_name, const char *version)
{
    return OMR_Open(base_name, version);
}

void omr_close(omr_port *port)
{
    OMR_Close(port);
}

unsigned long omr_mask(omr_port *port)
{
    return OMR_Mask(port);
}

const char * omr_name(omr_port *port)
{
    return OMR_Name(port);
}

omr_cmd * omr_get(omr_port *port)
{
    return OMR_Get(port);
}

const char * omr_text(omr_cmd *cmd)
{
    return OMR_Text(cmd);
}

int omr_parse(omr_cmd *cmd, const char *template)
{
    return OMR_Parse(cmd, template);
}

const char * omr_why(omr_cmd *cmd)
{
    return OMR_Why(cmd);
}

const char * omr_arg(omr_cmd *cmd, const char *key)
{
    return OMR_Arg(cmd, key);
}

int omr_switch(omr_cmd *cmd, const char *key)
{
    return OMR_Switch(cmd, key);
}

void omr_set_result(omr_cmd *cmd, const char *result)
{
    OMR_SetResult(cmd, result);
}

int omr_set_var(omr_cmd *cmd, const char *name, const char *value)
{
    return OMR_SetVar(cmd, name, value);
}

int omr_set_stem(omr_cmd *cmd, const char *stem, const char *tail, const char *value)
{
    return OMR_SetStem(cmd, stem, tail, value);
}

void omr_reply(omr_cmd *cmd, int rc, const char *result)
{
    OMR_Reply(cmd, rc, result);
}

int omr_dispatch(omr_cmd *cmd, const omr_verb *verbs, int n, void *app)
{
    return OMR_Dispatch(cmd, verbs, n, app);
}

int omr_do_line(const char *line, const omr_verb *verbs, int n, void *app, omr_io *io)
{
    return OMR_DoLine(line, verbs, n, app, io);
}

int omr_attach(omr_port *port, const omr_verb *verbs, int n, void *app)
{
    return OMR_Attach(port, verbs, n, app);
}

int omr_find(const char *name, char *found, int foundlen)
{
    return OMR_Find(name, found, foundlen);
}

omr_client * omr_connect(const char *port_base, omr_opt *opt)
{
    return OMR_Connect(port_base, opt);
}

void omr_disconnect(omr_client *client)
{
    OMR_Disconnect(client);
}

int omr_send(omr_client *client, const char *command, omr_io *io)
{
    return OMR_Send(client, command, io);
}
