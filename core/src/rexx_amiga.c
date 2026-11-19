/* The Amiga side of openmakerexx.library: a public message port, replies
 * through rexxsyslib, a client that finds or starts a port and sends a
 * command, and a timeout with timer.device. Wait() on the port and the
 * timer together; there is no busy loop. Written from the public Amiga
 * ARexx and rexxsyslib documentation (the ARexx User's Reference and the
 * rexxsyslib autodocs). MIT, Copyright (c) 2026 AmiCradle Team. */
#if defined(AMIGA) || defined(__amigaos__)

#include "gs_rexx.h"
#include <devices/timer.h>
#include <dos/dostags.h>
#include <exec/memory.h>
#include <exec/ports.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/rexxsyslib.h>
#include <clib/alib_protos.h>
#include <rexx/rxslib.h>
#include <rexx/storage.h>
#include "rexx_int.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct RxsLib *RexxSysBase;

static omr_port *ports;

static int ensure_rexx(void)
{
    if (!RexxSysBase) RexxSysBase = (struct RxsLib *)OpenLibrary((CONST_STRPTR) "rexxsyslib.library", 0);
    return RexxSysBase != NULL;
}

static omr_port *local_find(const char *name)
{
    omr_port *p;
    if (!name) return NULL;
    for (p = ports; p; p = p->next)
        if (omr_eqi(p->name, name)) return p;
    return NULL;
}

static int port_taken(const char *name)
{
    struct MsgPort *mp;
    if (local_find(name)) return 1;
    Forbid();
    mp = FindPort((CONST_STRPTR)name);
    Permit();
    return mp != NULL;
}

static int pick_name(const char *base, char *out, int outlen)
{
    int i;
    if (!base || !base[0] || !out || outlen < 4) return -1;
    if (omr_ends_index(base)) {
        if (port_taken(base)) return -1;
        snprintf(out, (size_t)outlen, "%s", base);
        return 0;
    }
    for (i = 1; i <= 99; i++) {
        snprintf(out, (size_t)outlen, "%s.%d", base, i);
        if (!port_taken(out)) return 0;
    }
    return -1;
}

omr_port *omr_open(const char *base_name, const char *version)
{
    omr_port *p;
    char name[OMR_NAME];
    struct MsgPort *mp;
    if (!base_name || !base_name[0]) return NULL;
    if (!ensure_rexx()) return NULL;
    if (pick_name(base_name, name, (int)sizeof name) != 0) return NULL;
    p = (omr_port *)calloc(1, sizeof *p);
    if (!p) return NULL;
    snprintf(p->name, sizeof p->name, "%s", name);
    if (version) snprintf(p->version, sizeof p->version, "%s", version);
    p->qtail = &p->queue;
    mp = CreateMsgPort();
    if (!mp) {
        free(p);
        return NULL;
    }
    mp->mp_Node.ln_Name = p->name;
    mp->mp_Node.ln_Pri = 0;
    Forbid();
    if (FindPort((CONST_STRPTR)p->name)) {
        Permit();
        DeleteMsgPort(mp);
        free(p);
        return NULL;
    }
    AddPort(mp);
    Permit();
    p->mp = mp;
    p->next = ports;
    ports = p;
    return p;
}

void omr_close(omr_port *port)
{
    omr_port **pp;
    struct Message *msg;
    if (!port) return;
    for (pp = &ports; *pp; pp = &(*pp)->next)
        if (*pp == port) {
            *pp = port->next;
            break;
        }
    if (port->mp) {
        RemPort(port->mp);
        while ((msg = GetMsg(port->mp)) != NULL) {
            if (ensure_rexx() && IsRexxMsg((struct RexxMsg *)msg)) {
                struct RexxMsg *rm = (struct RexxMsg *)msg;
                rm->rm_Result1 = OMR_FAIL;
                rm->rm_Result2 = 0;
                ReplyMsg(msg);
            } else
                ReplyMsg(msg);
        }
        DeleteMsgPort(port->mp);
    }
    free(port);
}

unsigned long omr_mask(omr_port *port)
{
    if (!port || !port->mp) return 0;
    return 1UL << port->mp->mp_SigBit;
}

omr_cmd *omr_get(omr_port *port)
{
    struct Message *msg;
    struct RexxMsg *rm;
    omr_cmd *cmd;
    const char *text;
    if (!port || !port->mp || !ensure_rexx()) return NULL;
    for (;;) {
        msg = GetMsg(port->mp);
        if (!msg) return NULL;
        if (!IsRexxMsg((struct RexxMsg *)msg)) {
            ReplyMsg(msg);
            continue;
        }
        rm = (struct RexxMsg *)msg;
        text = (const char *)rm->rm_Args[0];
        cmd = (omr_cmd *)calloc(1, sizeof *cmd);
        if (!cmd) {
            rm->rm_Result1 = OMR_FAIL;
            rm->rm_Result2 = 0;
            ReplyMsg(msg);
            return NULL;
        }
        if (omr_cmd_load(cmd, text ? text : "") != 0) {
            /* still hand the command to the app: dispatch will reply RC 10 */
        }
        cmd->port = port;
        cmd->msg = rm;
        return cmd;
    }
}

void omr_reply(omr_cmd *cmd, int rc, const char *result)
{
    struct RexxMsg *rm;
    int i;
    if (!cmd || cmd->replied) return;
    if (result && result != cmd->result) omr_set_result(cmd, result);
    cmd->rc = rc;
    cmd->replied = 1;
    omr_host_vars_from(cmd);
    rm = cmd->msg;
    if (!rm || !ensure_rexx()) return;
    rm->rm_Result1 = rc;
    rm->rm_Result2 = 0;
    if ((rm->rm_Action & RXFF_RESULT) && cmd->result[0]) {
        UBYTE *arg = (UBYTE *)CreateArgstring((STRPTR)cmd->result, (ULONG)strlen(cmd->result));
        rm->rm_Result2 = (LONG)arg;
    }
    for (i = 0; i < cmd->nvar; i++)
        SetRexxVar(rm, (UBYTE *)cmd->vars[i].name, (UBYTE *)cmd->vars[i].value, (ULONG)strlen(cmd->vars[i].value));
    ReplyMsg((struct Message *)rm);
    cmd->msg = NULL;
    free(cmd);
}

static int find_public(const char *name, char *found, int foundlen)
{
    struct MsgPort *mp;
    if (!name || !name[0]) return -1;
    Forbid();
    mp = FindPort((CONST_STRPTR)name);
    Permit();
    if (!mp) return -1;
    if (found && foundlen > 0) {
        if ((int)strlen(name) >= foundlen) return -1;
        snprintf(found, (size_t)foundlen, "%s", name);
    }
    return 0;
}

int omr_find(const char *name, char *found, int foundlen)
{
    int i;
    char try[OMR_NAME];
    if (!name || !name[0]) return -1;
    if (omr_ends_index(name)) return find_public(name, found, foundlen);
    if (find_public(name, found, foundlen) == 0) return 0;
    for (i = 1; i <= 99; i++) {
        snprintf(try, sizeof try, "%s.%d", name, i);
        if (find_public(try, found, foundlen) == 0) return 0;
    }
    return -1;
}

/* Wait for mask or timeout_ms. 1 if mask fired, 0 on timeout, -1 on error. */
static int wait_mask(unsigned long mask, int timeout_ms)
{
    struct MsgPort *tp;
    struct timerequest *tr;
    unsigned long got, tbit;
    int ok = -1, ms;
    ms = omr_timeout_ms(timeout_ms);
    if (!mask) {
        /* only a timeout */
    }
    tp = CreateMsgPort();
    if (!tp) return -1;
    tr = (struct timerequest *)CreateIORequest(tp, sizeof *tr);
    if (!tr) {
        DeleteMsgPort(tp);
        return -1;
    }
    if (OpenDevice((CONST_STRPTR) "timer.device", UNIT_MICROHZ, (struct IORequest *)tr, 0) != 0) {
        DeleteIORequest((struct IORequest *)tr);
        DeleteMsgPort(tp);
        return -1;
    }
    tr->tr_node.io_Command = TR_ADDREQUEST;
    tr->tr_time.tv_secs = (ULONG)(ms / 1000);
    tr->tr_time.tv_micro = (ULONG)(ms % 1000) * 1000UL;
    SendIO((struct IORequest *)tr);
    tbit = 1UL << tp->mp_SigBit;
    got = Wait(mask | tbit);
    if (got & mask) {
        if (!CheckIO((struct IORequest *)tr)) {
            AbortIO((struct IORequest *)tr);
            WaitIO((struct IORequest *)tr);
        } else
            WaitIO((struct IORequest *)tr);
        ok = 1;
    } else {
        WaitIO((struct IORequest *)tr);
        ok = 0;
    }
    CloseDevice((struct IORequest *)tr);
    DeleteIORequest((struct IORequest *)tr);
    DeleteMsgPort(tp);
    return ok;
}

static int start_program(const char *program, const char *args)
{
    char cmd[OMR_LINE];
    BPTR in, out;
    LONG rc;
    if (!program || !program[0]) return -1;
    if (args && args[0])
        snprintf(cmd, sizeof cmd, "%s %s", program, args);
    else
        snprintf(cmd, sizeof cmd, "%s", program);
    in = Open((CONST_STRPTR) "NIL:", MODE_NEWFILE);
    out = Open((CONST_STRPTR) "NIL:", MODE_NEWFILE);
    rc = SystemTags((STRPTR)cmd, SYS_Input, in, SYS_Output, out, SYS_Asynch, TRUE, SYS_UserShell, TRUE, TAG_DONE);
    /* SYS_Asynch takes the handles; do not close them. */
    if (rc != 0) {
        if (in) Close(in);
        if (out) Close(out);
        return -1;
    }
    return 0;
}

omr_client *omr_connect(const char *port_base, omr_opt *opt)
{
    omr_client *c;
    char found[OMR_NAME];
    const char *program = NULL, *args = NULL;
    int timeout, min, left, slice;
    if (!port_base || !port_base[0]) return NULL;
    min = (int)offsetof(omr_opt, args) + (int)sizeof(const char *);
    if (opt) {
        if (opt->size > 0 && opt->size < min) return NULL;
        if (opt->size == 0 || opt->size >= min) {
            program = opt->program;
            args = opt->args;
        }
        timeout = opt->timeout;
    } else
        timeout = 0;
    found[0] = 0;
    if (omr_find(port_base, found, (int)sizeof found) == 0) {
        /* found */
    } else if (program && program[0]) {
        if (start_program(program, args) != 0) {
            /* still wait: the program may have started anyway */
        }
        left = omr_timeout_ms(timeout);
        while (left > 0 && omr_find(port_base, found, (int)sizeof found) != 0) {
            slice = left > 50 ? 50 : left;
            if (wait_mask(0, slice) < 0) {
                /* timer failed: do not spin */
                break;
            }
            left -= slice;
        }
        if (!found[0] && omr_find(port_base, found, (int)sizeof found) != 0) return NULL;
    } else
        return NULL;
    c = (omr_client *)calloc(1, sizeof *c);
    if (!c) return NULL;
    snprintf(c->name, sizeof c->name, "%s", found);
    c->local = local_find(found);
    if (opt && (opt->size == 0 || opt->size >= (int)sizeof(omr_opt))) snprintf(opt->name, sizeof opt->name, "%s", found);
    return c;
}

void omr_disconnect(omr_client *client) { free(client); }

int omr_send(omr_client *client, const char *command, omr_io *io)
{
    struct MsgPort *reply;
    struct RexxMsg *rm, *back;
    omr_port *local;
    unsigned long rbit;
    int st;
    if (!omr_io_ok(io)) return -1;
    if (!client || !command) {
        omr_io_fail(io, "no command");
        return -1;
    }
    local = client->local ? client->local : local_find(client->name);
    if (local && local->verbs) {
        omr_cmd cmd;
        if (omr_cmd_load(&cmd, command) != 0) {
            omr_io_copy(io, OMR_ERROR, cmd.why[0] ? cmd.why : "no command");
            return 0;
        }
        cmd.port = local;
        omr_dispatch(&cmd, local->verbs, local->nverbs, local->app);
        omr_io_copy(io, cmd.rc, cmd.result);
        return 0;
    }
    if (!ensure_rexx()) {
        omr_io_fail(io, "rexxsyslib.library is not open");
        return -1;
    }
    reply = CreateMsgPort();
    if (!reply) {
        omr_io_fail(io, "out of memory");
        return -1;
    }
    rm = CreateRexxMsg(reply, NULL, NULL);
    if (!rm) {
        DeleteMsgPort(reply);
        omr_io_fail(io, "out of memory");
        return -1;
    }
    rm->rm_Action = RXCOMM | RXFF_RESULT;
    rm->rm_Args[0] = (STRPTR)CreateArgstring((STRPTR)command, (ULONG)strlen(command));
    if (!rm->rm_Args[0]) {
        DeleteRexxMsg(rm);
        DeleteMsgPort(reply);
        omr_io_fail(io, "out of memory");
        return -1;
    }
    Forbid();
    {
        struct MsgPort *dest = FindPort((CONST_STRPTR)client->name);
        if (dest) PutMsg(dest, (struct Message *)rm);
        Permit();
        if (!dest) {
            DeleteArgstring((UBYTE *)rm->rm_Args[0]);
            DeleteRexxMsg(rm);
            DeleteMsgPort(reply);
            omr_io_fail(io, "no such port");
            return -1;
        }
    }
    rbit = 1UL << reply->mp_SigBit;
    st = wait_mask(rbit, io->timeout);
    if (st != 1) {
        /* Timed out: do not DeleteRexxMsg, the host may still reply. Leak a
         * port and the message rather than reply to a freed one. */
        omr_io_fail(io, "timeout");
        return -1;
    }
    back = (struct RexxMsg *)GetMsg(reply);
    if (!back) {
        DeleteArgstring((UBYTE *)rm->rm_Args[0]);
        DeleteRexxMsg(rm);
        DeleteMsgPort(reply);
        omr_io_fail(io, "timeout");
        return -1;
    }
    omr_io_copy(io, (int)back->rm_Result1, back->rm_Result2 ? (const char *)back->rm_Result2 : "");
    if (back->rm_Result2) DeleteArgstring((UBYTE *)back->rm_Result2);
    DeleteArgstring((UBYTE *)rm->rm_Args[0]);
    DeleteRexxMsg(rm);
    DeleteMsgPort(reply);
    return 0;
}

#endif /* Amiga */
