/* openmakerexx.library: a public ARexx port and a client, for every
 * OpenMake program. Written from the public Amiga ARexx and rexxsyslib
 * documentation (the ARexx User's Reference and the rexxsyslib autodocs).
 * It is not written from any other application's source.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef GS_REXX_H
#define GS_REXX_H

enum { OMR_OK = 0, OMR_WARN = 5, OMR_ERROR = 10, OMR_FAIL = 20 };

/* A command in an app's table. size is sizeof(omr_verb). */
typedef struct omr_verb {
    int size;
    const char *name;    /* "OPEN", "SLICE", ... */
    const char *template; /* ReadArgs-style, or empty */
    const char *help;    /* one short line */
    int (*fn)(void *cmd, void *app); /* returns RC; set RESULT with omr_set_result */
} omr_verb;

/* A send's options and its reply. size is sizeof(omr_io). */
typedef struct omr_io {
    int size;
    int timeout; /* milliseconds; 0 means 30000 */
    int rc;      /* out: the command's RC, once a reply arrives */
    char *result;
    int resultlen;
} omr_io;

/* How to find or start a port. size is sizeof(omr_opt). */
typedef struct omr_opt {
    int size;
    int timeout;         /* milliseconds to wait after a start; 0 means 30000 */
    const char *program; /* run this if the port is not found; NULL: find only */
    const char *args;    /* extra arguments for the program, or NULL */
    char name[40];       /* out: the public port name found */
} omr_opt;

typedef struct omr_port omr_port;
typedef struct omr_cmd omr_cmd;
typedef struct omr_client omr_client;

/* Open a public port. base_name is "GFOPENMAKE" (the first copy is
 * GFOPENMAKE.1; if that is taken, .2, and so on). A name that already
 * ends in .digits is used as it is. version is the VERSION string, or
 * NULL. NULL if the port could not be opened. */
omr_port *omr_open(const char *base_name, const char *version);
void omr_close(omr_port *port);

/* The Exec signal bit to OR into Wait(), or 0. */
unsigned long omr_mask(omr_port *port);
const char *omr_name(omr_port *port); /* "GFOPENMAKE.1", or NULL */

/* The next pending command, or NULL. */
omr_cmd *omr_get(omr_port *port);
const char *omr_text(omr_cmd *cmd); /* the command line, or NULL */

/* Parse the line against a ReadArgs-style template (the arguments after
 * the verb). 0, or -1 with why in omr_why. */
int omr_parse(omr_cmd *cmd, const char *template);
const char *omr_why(omr_cmd *cmd);
const char *omr_arg(omr_cmd *cmd, const char *key); /* NULL if absent */
int omr_switch(omr_cmd *cmd, const char *key);      /* 1 if the switch was given */

void omr_set_result(omr_cmd *cmd, const char *result);
/* Write an ARexx variable or a stem tail in the program that sent the
 * command (SetRexxVar on Amiga; held for omr_host_var on the host). 0, or
 * -1. tail may be empty or start with a '.'. */
int omr_set_var(omr_cmd *cmd, const char *name, const char *value);
int omr_set_stem(omr_cmd *cmd, const char *stem, const char *tail, const char *value);
void omr_reply(omr_cmd *cmd, int rc, const char *result);

/* Look the first word up in verbs[0..n-1], parse, call fn, and reply.
 * HELP and VERSION are handled here when the table does not define them
 * (VERSION uses the string given to omr_open). Returns the RC replied. */
int omr_dispatch(omr_cmd *cmd, const omr_verb *verbs, int n, void *app);

/* Parse and dispatch a line in this process. Fills io->rc and io->result.
 * 0, or -1 if io is short or line is NULL. */
int omr_do_line(const char *line, const omr_verb *verbs, int n, void *app, omr_io *io);

/* Serve a client in this process from this table, with no second thread.
 * A later omr_send to this port runs the table and returns the reply. */
int omr_attach(omr_port *port, const omr_verb *verbs, int n, void *app);

/* Find a public port by its base name ("GFOPENMAKE") or a full name
 * ("GFOPENMAKE.1"). Writes the name that was found. 0, or -1. */
int omr_find(const char *name, char *found, int foundlen);

/* Find the port, or start opt->program and wait until it appears or the
 * timeout. opt may be NULL (find only, no wait). NULL if it did not. */
omr_client *omr_connect(const char *port_base, omr_opt *opt);
void omr_disconnect(omr_client *client);

/* Send a command and wait for the reply, or until the timeout. 0 if a
 * reply arrived (io->rc is the command's RC); -1 on a timeout or a
 * transport failure, with why in io->result. */
int omr_send(omr_client *client, const char *command, omr_io *io);

/* Host tests: a variable or stem written by the last command, or NULL.
 * Not a library call; on Amiga the values go to the ARexx program. */
const char *omr_host_var(const char *name);

#endif
