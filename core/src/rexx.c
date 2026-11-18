/* ARexx command parser, dispatch, and a host stub for the port and the
 * client (openmakerexx.library, gs_rexx.h). The Amiga port, rexxsyslib
 * and timer.device are in rexx_amiga.c. Written from the public Amiga
 * ARexx and rexxsyslib documentation. MIT, Copyright (c) 2026 AmiCradle Team. */
#if !defined(AMIGA) && !defined(__amigaos__)
#define _POSIX_C_SOURCE 200809L
#endif
#include "gs_rexx.h"
#include "rexx_int.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined(AMIGA) && !defined(__amigaos__)
#include <time.h>
#endif

int omr_eqi(const char *a, const char *b)
{
    if (!a || !b) return 0;
    while (*a && *b) {
        int ca = (unsigned char)*a, cb = (unsigned char)*b;
        if (ca >= 'a' && ca <= 'z') ca -= 32;
        if (cb >= 'a' && cb <= 'z') cb -= 32;
        if (ca != cb) return 0;
        a++;
        b++;
    }
    return *a == 0 && *b == 0;
}

int omr_timeout_ms(int timeout) { return timeout > 0 ? timeout : OMR_DEFAULT_MS; }

int omr_io_ok(const omr_io *io)
{
    return io && io->size >= (int)offsetof(omr_io, resultlen) + (int)sizeof(int);
}

void omr_io_copy(omr_io *io, int rc, const char *result)
{
    if (!omr_io_ok(io)) return;
    io->rc = rc;
    if (io->result && io->resultlen > 0) {
        if (!result) result = "";
        snprintf(io->result, (size_t)io->resultlen, "%s", result);
    }
}

void omr_io_fail(omr_io *io, const char *why)
{
    omr_io_copy(io, OMR_FAIL, why);
}

int omr_ends_index(const char *s)
{
    const char *dot;
    if (!s) return 0;
    dot = strrchr(s, '.');
    if (!dot || dot == s || !dot[1]) return 0;
    for (dot++; *dot; dot++)
        if (*dot < '0' || *dot > '9') return 0;
    return 1;
}

int omr_verb_ok(const omr_verb *v)
{
    int need = (int)offsetof(omr_verb, fn) + (int)sizeof(v->fn);
    if (!v) return 0;
    if (v->size == 0) return 1;
    return v->size >= need;
}

const omr_verb *omr_verb_at(const omr_verb *verbs, int n, int i)
{
    const char *p;
    int size;
    if (!verbs || i < 0 || i >= n) return NULL;
    size = verbs[0].size;
    if (size <= 0) size = (int)sizeof(omr_verb);
    p = (const char *)verbs + (size_t)i * (size_t)size;
    return (const omr_verb *)p;
}

static int is_int(const char *s)
{
    if (!s || !*s) return 0;
    if (*s == '+' || *s == '-') s++;
    if (!*s) return 0;
    for (; *s; s++)
        if (*s < '0' || *s > '9') return 0;
    return 1;
}

static const char *skip_sp(const char *s)
{
    while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n') s++;
    return s;
}

void omr_cmd_wipe(omr_cmd *cmd)
{
    if (!cmd) return;
    memset(cmd, 0, sizeof *cmd);
}

int omr_cmd_load(omr_cmd *cmd, const char *line)
{
    const char *s, *e;
    int n;
    if (!cmd) return -1;
    omr_cmd_wipe(cmd);
    if (!line) {
        snprintf(cmd->why, sizeof cmd->why, "no command");
        return -1;
    }
    s = skip_sp(line);
    if (!*s) {
        snprintf(cmd->why, sizeof cmd->why, "no command");
        return -1;
    }
    snprintf(cmd->text, sizeof cmd->text, "%s", s);
    e = cmd->text;
    while (*e && *e != ' ' && *e != '\t') e++;
    n = (int)(e - cmd->text);
    if (n >= OMR_KEY) n = OMR_KEY - 1;
    memcpy(cmd->verb, cmd->text, (size_t)n);
    cmd->verb[n] = 0;
    return 0;
}

static char *store(omr_cmd *cmd, const char *s, int n)
{
    char *p;
    if (n < 0) n = (int)strlen(s);
    if (cmd->used + n + 1 > (int)sizeof cmd->storage) return NULL;
    p = cmd->storage + cmd->used;
    memcpy(p, s, (size_t)n);
    p[n] = 0;
    cmd->used += n + 1;
    return p;
}

static int add_slot(omr_cmd *cmd, const char *spec)
{
    struct omr_slot *sl;
    const char *slash, *eq;
    int nkey;
    char left[OMR_KEY * 2];
    if (cmd->n >= OMR_ARGS) return -1;
    slash = strchr(spec, '/');
    nkey = slash ? (int)(slash - spec) : (int)strlen(spec);
    if (nkey <= 0 || nkey >= (int)sizeof left) return -1;
    memcpy(left, spec, (size_t)nkey);
    left[nkey] = 0;
    sl = &cmd->a[cmd->n];
    memset(sl, 0, sizeof *sl);
    eq = strchr(left, '=');
    if (eq) {
        int na = (int)(eq - left);
        if (na <= 0 || na >= OMR_KEY || (int)strlen(eq + 1) >= OMR_KEY) return -1;
        memcpy(sl->key, left, (size_t)na);
        sl->key[na] = 0;
        memcpy(sl->alias, eq + 1, strlen(eq + 1) + 1);
    } else {
        if (nkey >= OMR_KEY) return -1;
        memcpy(sl->key, left, (size_t)nkey);
        sl->key[nkey] = 0;
    }
    if (slash) {
        const char *f;
        for (f = slash; *f; f++) {
            if (*f == 'A' || *f == 'a') sl->required = 1;
            else if (*f == 'K' || *f == 'k') sl->is_keyword = 1;
            else if (*f == 'S' || *f == 's') sl->is_switch = 1;
            else if (*f == 'N' || *f == 'n') sl->is_num = 1;
            else if (*f == 'F' || *f == 'f') sl->is_final = 1;
            else if (*f == 'M' || *f == 'm') sl->is_final = 1; /* several words, kept as one string */
            else if (*f == '/') continue;
            else {
                snprintf(cmd->why, sizeof cmd->why, "bad template flag /%c", *f);
                return -1;
            }
        }
    }
    cmd->n++;
    return 0;
}

static int parse_template(omr_cmd *cmd, const char *template)
{
    char spec[64];
    int n;
    const char *s, *e;
    cmd->n = 0;
    if (!template) return 0;
    s = skip_sp(template);
    while (*s) {
        if (*s == ',') {
            s = skip_sp(s + 1);
            continue;
        }
        e = s;
        while (*e && *e != ' ' && *e != '\t' && *e != ',') e++;
        n = (int)(e - s);
        if (n >= (int)sizeof spec) {
            snprintf(cmd->why, sizeof cmd->why, "template too long");
            return -1;
        }
        memcpy(spec, s, (size_t)n);
        spec[n] = 0;
        if (add_slot(cmd, spec) != 0) {
            if (!cmd->why[0]) snprintf(cmd->why, sizeof cmd->why, "bad template");
            return -1;
        }
        s = skip_sp(e);
        if (*s == ',') s = skip_sp(s + 1);
    }
    return 0;
}

static struct omr_slot *slot_named(omr_cmd *cmd, const char *name)
{
    int i;
    for (i = 0; i < cmd->n; i++)
        if (omr_eqi(cmd->a[i].key, name) || (cmd->a[i].alias[0] && omr_eqi(cmd->a[i].alias, name))) return &cmd->a[i];
    return NULL;
}

static struct omr_slot *next_pos(omr_cmd *cmd)
{
    int i;
    for (i = 0; i < cmd->n; i++)
        if (!cmd->a[i].is_keyword && !cmd->a[i].is_switch && !cmd->a[i].present) return &cmd->a[i];
    return NULL;
}

/* Read one token from *ps into buf. Quoted with ". Returns 0, or -1. */
static int next_tok(const char **ps, char *buf, int len, int rest, char *why, int whylen)
{
    const char *s = skip_sp(*ps);
    int n = 0;
    if (!*s) {
        *ps = s;
        return -2; /* no more */
    }
    if (rest) {
        snprintf(buf, (size_t)len, "%s", s);
        *ps = s + strlen(s);
        return 0;
    }
    if (*s == '"') {
        s++;
        while (*s && *s != '"') {
            if (n >= len - 1) {
                snprintf(why, (size_t)whylen, "argument too long");
                return -1;
            }
            buf[n++] = *s++;
        }
        if (*s != '"') {
            snprintf(why, (size_t)whylen, "unclosed quote");
            return -1;
        }
        s++;
        buf[n] = 0;
        *ps = s;
        return 0;
    }
    while (*s && *s != ' ' && *s != '\t') {
        if (n >= len - 1) {
            snprintf(why, (size_t)whylen, "argument too long");
            return -1;
        }
        buf[n++] = *s++;
    }
    buf[n] = 0;
    *ps = s;
    return 0;
}

static int fill_slot(omr_cmd *cmd, struct omr_slot *sl, const char *value)
{
    if (sl->present && !sl->is_final) {
        snprintf(cmd->why, sizeof cmd->why, "%s already set", sl->key);
        return -1;
    }
    if (sl->is_num && !is_int(value)) {
        snprintf(cmd->why, sizeof cmd->why, "%s: not a number", sl->key);
        return -1;
    }
    sl->value = store(cmd, value, -1);
    if (!sl->value) {
        snprintf(cmd->why, sizeof cmd->why, "argument too long");
        return -1;
    }
    sl->present = 1;
    return 0;
}

int omr_parse(omr_cmd *cmd, const char *template)
{
    const char *s;
    char tok[OMR_LINE], val[OMR_LINE], kbuf[OMR_KEY];
    int r;
    if (!cmd) return -1;
    cmd->parsed = 0;
    cmd->n = 0;
    cmd->used = 0;
    cmd->why[0] = 0;
    if (parse_template(cmd, template) != 0) return -1;
    s = cmd->text;
    s = skip_sp(s);
    while (*s && *s != ' ' && *s != '\t') s++;
    s = skip_sp(s);
    while (*s) {
        struct omr_slot *sl, *pos;
        char *cut;
        r = next_tok(&s, tok, (int)sizeof tok, 0, cmd->why, (int)sizeof cmd->why);
        if (r == -2) break;
        if (r != 0) return -1;
        /* KEY=value or KEY/value as one token */
        cut = strchr(tok, '=');
        if (!cut) cut = strchr(tok, '/');
        if (cut && cut != tok && cut[1]) {
            int kn = (int)(cut - tok);
            if (kn > 0 && kn < OMR_KEY) {
                memcpy(kbuf, tok, (size_t)kn);
                kbuf[kn] = 0;
                sl = slot_named(cmd, kbuf);
                if (sl && !sl->is_switch) {
                    if (fill_slot(cmd, sl, cut + 1) != 0) return -1;
                    s = skip_sp(s);
                    continue;
                }
            }
        }
        sl = slot_named(cmd, tok);
        if (sl) {
            if (sl->is_switch) {
                sl->present = 1;
                sl->value = store(cmd, "1", 1);
                s = skip_sp(s);
                continue;
            }
            r = next_tok(&s, val, (int)sizeof val, sl->is_final, cmd->why, (int)sizeof cmd->why);
            if (r == -2) {
                snprintf(cmd->why, sizeof cmd->why, "%s: missing value", sl->key);
                return -1;
            }
            if (r != 0) return -1;
            if (fill_slot(cmd, sl, val) != 0) return -1;
            s = skip_sp(s);
            continue;
        }
        pos = next_pos(cmd);
        if (!pos) {
            snprintf(cmd->why, sizeof cmd->why, "unexpected argument: %.80s", tok);
            return -1;
        }
        if (pos->is_final) {
            char rest[OMR_LINE];
            const char *t = skip_sp(s);
            size_t n = 0;
            snprintf(rest, sizeof rest, "%s", tok);
            n = strlen(rest);
            if (*t && n + 2 < sizeof rest) {
                rest[n++] = ' ';
                rest[n] = 0;
                snprintf(rest + n, sizeof rest - n, "%s", t);
            }
            if (fill_slot(cmd, pos, rest) != 0) return -1;
            s += strlen(s);
            continue;
        }
        if (fill_slot(cmd, pos, tok) != 0) return -1;
        s = skip_sp(s);
    }
    {
        int i;
        for (i = 0; i < cmd->n; i++)
            if (cmd->a[i].required && !cmd->a[i].present) {
                snprintf(cmd->why, sizeof cmd->why, "%s is required", cmd->a[i].key);
                return -1;
            }
    }
    cmd->parsed = 1;
    return 0;
}

const char *omr_why(omr_cmd *cmd) { return cmd ? cmd->why : ""; }

const char *omr_text(omr_cmd *cmd) { return cmd ? cmd->text : NULL; }

const char *omr_arg(omr_cmd *cmd, const char *key)
{
    struct omr_slot *sl;
    if (!cmd || !key) return NULL;
    sl = slot_named(cmd, key);
    return sl && sl->present && !sl->is_switch ? sl->value : NULL;
}

int omr_switch(omr_cmd *cmd, const char *key)
{
    struct omr_slot *sl;
    if (!cmd || !key) return 0;
    sl = slot_named(cmd, key);
    return sl && sl->is_switch && sl->present ? 1 : 0;
}

void omr_set_result(omr_cmd *cmd, const char *result)
{
    if (!cmd) return;
    snprintf(cmd->result, sizeof cmd->result, "%s", result ? result : "");
}

static int add_var(omr_cmd *cmd, const char *name, const char *value)
{
    int i;
    if (!cmd || !name || !value) return -1;
    if ((int)strlen(name) >= 80) return -1;
    for (i = 0; i < cmd->nvar; i++)
        if (omr_eqi(cmd->vars[i].name, name)) {
            snprintf(cmd->vars[i].value, sizeof cmd->vars[i].value, "%s", value);
            return 0;
        }
    if (cmd->nvar >= OMR_VARS) return -1;
    snprintf(cmd->vars[cmd->nvar].name, sizeof cmd->vars[0].name, "%s", name);
    snprintf(cmd->vars[cmd->nvar].value, sizeof cmd->vars[0].value, "%s", value);
    cmd->nvar++;
    return 0;
}

int omr_set_var(omr_cmd *cmd, const char *name, const char *value) { return add_var(cmd, name, value); }

int omr_set_stem(omr_cmd *cmd, const char *stem, const char *tail, const char *value)
{
    char name[80];
    if (!stem || !value) return -1;
    if (!tail || !tail[0])
        snprintf(name, sizeof name, "%s", stem);
    else if (tail[0] == '.')
        snprintf(name, sizeof name, "%s%s", stem, tail);
    else
        snprintf(name, sizeof name, "%s.%s", stem, tail);
    return add_var(cmd, name, value);
}

static struct {
    char name[80];
    char value[OMR_LINE];
} host_vars[OMR_VARS];
static int n_host_vars;

const char *omr_host_var(const char *name)
{
    int i;
    if (!name) return NULL;
    for (i = 0; i < n_host_vars; i++)
        if (omr_eqi(host_vars[i].name, name)) return host_vars[i].value;
    return NULL;
}

void omr_host_vars_from(const omr_cmd *cmd)
{
    int i;
    n_host_vars = 0;
    if (!cmd) return;
    for (i = 0; i < cmd->nvar && n_host_vars < OMR_VARS; i++) {
        snprintf(host_vars[n_host_vars].name, sizeof host_vars[0].name, "%s", cmd->vars[i].name);
        snprintf(host_vars[n_host_vars].value, sizeof host_vars[0].value, "%s", cmd->vars[i].value);
        n_host_vars++;
    }
}

#if !defined(AMIGA) && !defined(__amigaos__)
void omr_reply(omr_cmd *cmd, int rc, const char *result)
{
    if (!cmd || cmd->replied) return;
    /* result aliases cmd->result when dispatch replies with the string the
     * command already wrote; overlapping snprintf would empty it. */
    if (result && result != cmd->result) omr_set_result(cmd, result);
    cmd->rc = rc;
    cmd->replied = 1;
    omr_host_vars_from(cmd);
}
#endif

static const omr_verb *find_verb(const omr_verb *verbs, int n, const char *name)
{
    int i;
    for (i = 0; i < n; i++) {
        const omr_verb *v = omr_verb_at(verbs, n, i);
        if (omr_verb_ok(v) && v->name && omr_eqi(v->name, name)) return v;
    }
    return NULL;
}

static int help_line(omr_cmd *cmd, const omr_verb *verbs, int n, int has_ver)
{
    const char *want = omr_arg(cmd, "COMMAND");
    char buf[OMR_LINE];
    int i, used = 0;
    if (!want) {
        used = snprintf(buf, sizeof buf, "HELP");
        if (has_ver) used += snprintf(buf + used, sizeof buf - (size_t)used, " VERSION");
        for (i = 0; i < n; i++) {
            const omr_verb *v = omr_verb_at(verbs, n, i);
            if (!omr_verb_ok(v) || !v->name) continue;
            if (omr_eqi(v->name, "HELP") || omr_eqi(v->name, "VERSION")) continue;
            used += snprintf(buf + used, sizeof buf - (size_t)used, " %s", v->name);
            if (used >= (int)sizeof buf - 1) break;
        }
        omr_reply(cmd, OMR_OK, buf);
        return OMR_OK;
    }
    if (omr_eqi(want, "HELP")) {
        omr_reply(cmd, OMR_OK, "HELP COMMAND/K: list the commands, or one command's template");
        return OMR_OK;
    }
    if (omr_eqi(want, "VERSION") && has_ver) {
        omr_reply(cmd, OMR_OK, "VERSION: the program name and version");
        return OMR_OK;
    }
    {
        const omr_verb *v = find_verb(verbs, n, want);
        if (!v) {
            snprintf(buf, sizeof buf, "unknown command: %s", want);
            omr_reply(cmd, OMR_ERROR, buf);
            return OMR_ERROR;
        }
        snprintf(buf, sizeof buf, "%s%s%s%s%s", v->name, (v->template && v->template[0]) ? " " : "",
                 v->template ? v->template : "", (v->help && v->help[0]) ? ": " : "", v->help ? v->help : "");
        omr_reply(cmd, OMR_OK, buf);
        return OMR_OK;
    }
}

int omr_dispatch(omr_cmd *cmd, const omr_verb *verbs, int n, void *app)
{
    const omr_verb *v;
    char err[OMR_WHY];
    int has_help, has_ver, rc;
    if (!cmd) return OMR_FAIL;
    n_host_vars = 0;
    if (!cmd->verb[0] && omr_cmd_load(cmd, cmd->text) != 0) {
        omr_reply(cmd, OMR_ERROR, cmd->why[0] ? cmd->why : "no command");
        return OMR_ERROR;
    }
    has_help = find_verb(verbs, n, "HELP") != NULL;
    has_ver = find_verb(verbs, n, "VERSION") != NULL;
    if (!has_help && omr_eqi(cmd->verb, "HELP")) {
        if (omr_parse(cmd, "COMMAND/K") != 0) {
            omr_reply(cmd, OMR_ERROR, cmd->why);
            return OMR_ERROR;
        }
        return help_line(cmd, verbs, n, !has_ver || (cmd->port && cmd->port->version[0]));
    }
    if (!has_ver && omr_eqi(cmd->verb, "VERSION")) {
        if (cmd->port && cmd->port->version[0]) {
            omr_reply(cmd, OMR_OK, cmd->port->version);
            return OMR_OK;
        }
        omr_reply(cmd, OMR_ERROR, "no version");
        return OMR_ERROR;
    }
    v = find_verb(verbs, n, cmd->verb);
    if (!v) {
        snprintf(err, sizeof err, "unknown command: %s", cmd->verb);
        omr_reply(cmd, OMR_ERROR, err);
        return OMR_ERROR;
    }
    if (omr_parse(cmd, v->template) != 0) {
        omr_reply(cmd, OMR_ERROR, cmd->why);
        return OMR_ERROR;
    }
    if (!v->fn) {
        omr_reply(cmd, OMR_ERROR, "no function");
        return OMR_ERROR;
    }
    rc = v->fn(cmd, app);
    if (!cmd->replied) omr_reply(cmd, rc, NULL);
    return cmd->rc;
}

int omr_do_line(const char *line, const omr_verb *verbs, int n, void *app, omr_io *io)
{
    omr_cmd cmd;
    int rc;
    if (!omr_io_ok(io)) return -1;
    if (omr_cmd_load(&cmd, line) != 0) {
        omr_io_copy(io, OMR_ERROR, cmd.why[0] ? cmd.why : "no command");
        return 0;
    }
    rc = omr_dispatch(&cmd, verbs, n, app);
    (void)rc;
    omr_io_copy(io, cmd.rc, cmd.result);
    return 0;
}

const char *omr_name(omr_port *port) { return port ? port->name : NULL; }

int omr_attach(omr_port *port, const omr_verb *verbs, int n, void *app)
{
    if (!port) return -1;
    port->verbs = verbs;
    port->nverbs = n;
    port->app = app;
    return 0;
}

#if !defined(AMIGA) && !defined(__amigaos__)

static omr_port *ports;

static omr_port *host_find(const char *name)
{
    omr_port *p;
    if (!name) return NULL;
    for (p = ports; p; p = p->next)
        if (omr_eqi(p->name, name)) return p;
    return NULL;
}

static int host_taken(const char *name) { return host_find(name) != NULL; }

static int pick_name(const char *base, char *out, int outlen)
{
    int i;
    if (!base || !base[0] || !out || outlen < 4) return -1;
    if (omr_ends_index(base)) {
        if (host_taken(base)) return -1;
        snprintf(out, (size_t)outlen, "%s", base);
        return 0;
    }
    for (i = 1; i <= 99; i++) {
        snprintf(out, (size_t)outlen, "%s.%d", base, i);
        if (!host_taken(out)) return 0;
    }
    return -1;
}

omr_port *omr_open(const char *base_name, const char *version)
{
    omr_port *p;
    char name[OMR_NAME];
    if (!base_name || !base_name[0]) return NULL;
    if (pick_name(base_name, name, (int)sizeof name) != 0) return NULL;
    p = (omr_port *)calloc(1, sizeof *p);
    if (!p) return NULL;
    snprintf(p->name, sizeof p->name, "%s", name);
    if (version) snprintf(p->version, sizeof p->version, "%s", version);
    p->qtail = &p->queue;
    p->next = ports;
    ports = p;
    return p;
}

void omr_close(omr_port *port)
{
    omr_port **pp;
    omr_cmd *c, *n;
    if (!port) return;
    for (pp = &ports; *pp; pp = &(*pp)->next)
        if (*pp == port) {
            *pp = port->next;
            break;
        }
    for (c = port->queue; c; c = n) {
        n = c->next;
        free(c);
    }
    free(port);
}

unsigned long omr_mask(omr_port *port)
{
    (void)port;
    return 0;
}

omr_cmd *omr_get(omr_port *port)
{
    omr_cmd *c;
    if (!port || !port->queue) return NULL;
    c = port->queue;
    port->queue = c->next;
    if (!port->queue) port->qtail = &port->queue;
    c->next = NULL;
    return c;
}

static int find_one(const char *name, char *found, int foundlen)
{
    omr_port *p = host_find(name);
    if (!p) return -1;
    if (!found || foundlen <= 0) return 0;
    if ((int)strlen(p->name) >= foundlen) return -1;
    snprintf(found, (size_t)foundlen, "%s", p->name);
    return 0;
}

int omr_find(const char *name, char *found, int foundlen)
{
    int i;
    char try[OMR_NAME];
    if (!name || !name[0]) return -1;
    if (omr_ends_index(name) || host_find(name)) return find_one(name, found, foundlen);
    for (i = 1; i <= 99; i++) {
        snprintf(try, sizeof try, "%s.%d", name, i);
        if (find_one(try, found, foundlen) == 0) return 0;
    }
    return -1;
}

omr_client *omr_connect(const char *port_base, omr_opt *opt)
{
    omr_client *c;
    char found[OMR_NAME];
    const char *program = NULL;
    const char *args = NULL;
    int timeout, min;
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
        /* found now */
    } else if (program && program[0]) {
        /* Run it and wait. The host test uses a short timeout against a
         * program that never opens a port. */
        char cmd[OMR_LINE];
        int left, slice;
        if (args && args[0])
            snprintf(cmd, sizeof cmd, "%s %s", program, args);
        else
            snprintf(cmd, sizeof cmd, "%s", program);
        /* Best-effort start; a missing program still leads to a timeout. */
        (void)args;
        (void)cmd;
        {
            /* system() would block or want a shell; use a polling wait only
             * if we actually spawned. On the host we spawn with system in
             * the background when the program path is usable. */
            char sh[OMR_LINE + 8];
            int st;
            snprintf(sh, sizeof sh, "%s &", cmd);
            st = system(sh);
            (void)st;
        }
        left = omr_timeout_ms(timeout);
        while (left > 0 && omr_find(port_base, found, (int)sizeof found) != 0) {
            struct timespec ts;
            slice = left > 50 ? 50 : left;
            ts.tv_sec = slice / 1000;
            ts.tv_nsec = (long)(slice % 1000) * 1000000L;
            nanosleep(&ts, NULL);
            left -= slice;
        }
        if (!found[0] && omr_find(port_base, found, (int)sizeof found) != 0) return NULL;
    } else {
        return NULL;
    }
    c = (omr_client *)calloc(1, sizeof *c);
    if (!c) return NULL;
    snprintf(c->name, sizeof c->name, "%s", found);
    c->local = host_find(found);
    if (opt && (opt->size == 0 || opt->size >= (int)sizeof(omr_opt))) snprintf(opt->name, sizeof opt->name, "%s", found);
    return c;
}

void omr_disconnect(omr_client *client) { free(client); }

static void sleep_ms(int ms)
{
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

int omr_send(omr_client *client, const char *command, omr_io *io)
{
    omr_port *p;
    omr_cmd stack, *cmd;
    int left, slice;
    if (!omr_io_ok(io)) return -1;
    if (!client || !command) {
        omr_io_fail(io, "no command");
        return -1;
    }
    p = client->local ? client->local : host_find(client->name);
    if (!p) {
        omr_io_fail(io, "no such port");
        return -1;
    }
    if (p->verbs) {
        if (omr_cmd_load(&stack, command) != 0) {
            omr_io_copy(io, OMR_ERROR, stack.why[0] ? stack.why : "no command");
            return 0;
        }
        stack.port = p;
        omr_dispatch(&stack, p->verbs, p->nverbs, p->app);
        omr_io_copy(io, stack.rc, stack.result);
        return 0;
    }
    /* No table attached: queue and wait for omr_get / omr_reply. */
    cmd = (omr_cmd *)calloc(1, sizeof *cmd);
    if (!cmd) {
        omr_io_fail(io, "out of memory");
        return -1;
    }
    if (omr_cmd_load(cmd, command) != 0) {
        omr_io_copy(io, OMR_ERROR, cmd->why[0] ? cmd->why : "no command");
        free(cmd);
        return 0;
    }
    cmd->port = p;
    *p->qtail = cmd;
    p->qtail = &cmd->next;
    left = omr_timeout_ms(io->timeout);
    while (left > 0 && !cmd->replied) {
        slice = left > 20 ? 20 : left;
        sleep_ms(slice);
        left -= slice;
    }
    if (!cmd->replied) {
        omr_cmd **pp;
        for (pp = &p->queue; *pp; pp = &(*pp)->next)
            if (*pp == cmd) {
                *pp = cmd->next;
                if (!p->queue) p->qtail = &p->queue;
                else {
                    omr_cmd *t = p->queue;
                    while (t->next) t = t->next;
                    p->qtail = &t->next;
                }
                break;
            }
        free(cmd);
        omr_io_fail(io, "timeout");
        return -1;
    }
    omr_io_copy(io, cmd->rc, cmd->result);
    free(cmd);
    return 0;
}

#endif /* host stub */
