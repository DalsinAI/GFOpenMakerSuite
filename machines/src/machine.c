/* Machine files and the checks on a job. Plain C99, no floating point
 * beyond reading numbers and comparing them.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "om_machine.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const fnames[] = {
    "heated-bed", "bed-levelling", "rotary", "air-assist", "tool-changer",
    "spindle", "homing", "probing", "laser-power"
};
static const unsigned fbits[] = {
    OM_HEATED_BED, OM_BED_LEVELLING, OM_ROTARY, OM_AIR_ASSIST, OM_TOOL_CHANGER,
    OM_SPINDLE, OM_HOMING, OM_PROBING, OM_LASER_POWER
};
#define NF ((int)(sizeof fbits / sizeof fbits[0]))

void om_clear(om_machine *m)
{
    memset(m, 0, sizeof *m);
}

int om_has(const om_machine *m, unsigned feature)
{
    return (m->features & feature) == feature;
}

static void say(char *err, int errlen, int line, const char *what, const char *word)
{
    if (err && errlen > 0) snprintf(err, (size_t)errlen, "line %d: %s%s%s", line, what, word ? ": " : "", word ? word : "");
}

/* Up to n numbers from s; how many were read. */
static int numbers(const char *s, float *v, int n)
{
    int k = 0;
    char *end;
    while (k < n) {
        double d = strtod(s, &end);
        if (end == s) break;
        v[k++] = (float)d;
        s = end;
    }
    while (*s == ' ' || *s == '\t') s++;
    return *s ? -1 : k;
}

static int words(const char *s, int send, unsigned *mask, char *bad, int badlen)
{
    while (*s) {
        char w[32];
        int n = 0, i;
        unsigned bit = 0;
        while (*s == ' ' || *s == '\t') s++;
        if (!*s) break;
        while (*s && *s != ' ' && *s != '\t' && n < (int)sizeof w - 1) w[n++] = *s++;
        w[n] = 0;
        if (send) {
            if (!strcmp(w, "file")) bit = OM_SEND_FILE;
            else if (!strcmp(w, "serial")) bit = OM_SEND_SERIAL;
            else if (!strcmp(w, "network")) bit = OM_SEND_NETWORK;
        } else {
            for (i = 0; i < NF; i++)
                if (!strcmp(w, fnames[i])) bit = fbits[i];
        }
        if (!bit) {
            snprintf(bad, (size_t)badlen, "%s", w);
            return -1;
        }
        *mask |= bit;
    }
    return 0;
}

static int add_line(char *code, const char *line, int more)
{
    size_t have = more ? strlen(code) : 0, add = strlen(line);
    if (have + add + 2 > OM_CODE) return -1;
    memcpy(code + have, line, add);
    code[have + add] = '\n';
    code[have + add + 1] = 0;
    return 0;
}

static int one(om_machine *m, const char *key, const char *val, int more, int line, char *err, int errlen)
{
    float v[4];
    char bad[32];
    if (!strcmp(key, "start") || !strcmp(key, "end")) {
        if (add_line(!strcmp(key, "start") ? m->start : m->end, val, more)) {
            say(err, errlen, line, "too much G-code", key);
            return -1;
        }
        return 0;
    }
    if (!strcmp(key, "features") || !strcmp(key, "send")) {
        unsigned *mask = !strcmp(key, "send") ? &m->send : &m->features;
        if (!more) *mask = 0;
        if (words(val, mask == &m->send, mask, bad, (int)sizeof bad)) {
            say(err, errlen, line, mask == &m->send ? "not a way to send" : "not a feature", bad);
            return -1;
        }
        return 0;
    }
    if (more) {
        say(err, errlen, line, "+= is for start, end, features and send", key);
        return -1;
    }
    if (!strcmp(key, "name") || !strcmp(key, "dialect")) {
        char *to = !strcmp(key, "name") ? m->name : m->dialect;
        size_t cap = !strcmp(key, "name") ? sizeof m->name : sizeof m->dialect;
        snprintf(to, cap, "%s", val);
        return 0;
    }
    if (!strcmp(key, "family")) {
        if (!strcmp(val, "printer")) m->family = OM_PRINTER;
        else if (!strcmp(val, "laser")) m->family = OM_LASER;
        else if (!strcmp(val, "cnc")) m->family = OM_CNC;
        else {
            say(err, errlen, line, "not a family", val);
            return -1;
        }
        return 0;
    }
    if (!strcmp(key, "area") || !strcmp(key, "speed") || !strcmp(key, "accel") || !strcmp(key, "jerk")) {
        int want = !strcmp(key, "area") ? 3 : 4, i;
        float *to = !strcmp(key, "area") ? m->area : !strcmp(key, "speed") ? m->speed : !strcmp(key, "accel") ? m->accel : m->jerk;
        if (numbers(val, v, want) != want) {
            say(err, errlen, line, want == 3 ? "wants X Y Z" : "wants X Y Z E", key);
            return -1;
        }
        for (i = 0; i < want; i++) to[i] = v[i];
        return 0;
    }
    if (!strcmp(key, "hotend") || !strcmp(key, "bed") || !strcmp(key, "laser") || !strcmp(key, "rpm") ||
        !strcmp(key, "extruders")) {
        if (numbers(val, v, 1) != 1 || v[0] < 0.f) {
            say(err, errlen, line, "wants a number", key);
            return -1;
        }
        if (!strcmp(key, "hotend")) m->hotend = v[0];
        else if (!strcmp(key, "bed")) m->bed = v[0];
        else if (!strcmp(key, "laser")) m->laser = v[0];
        else if (!strcmp(key, "rpm")) m->rpm = v[0];
        else m->extruders = (int)v[0];
        return 0;
    }
    say(err, errlen, line, "not a key", key);
    return -1;
}

static char *trim(char *s)
{
    char *e;
    while (*s == ' ' || *s == '\t') s++;
    e = s + strlen(s);
    while (e > s && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\r')) *--e = 0;
    return s;
}

int om_parse(om_machine *m, const char *text, char *err, int errlen)
{
    int line = 0;
    if (err && errlen > 0) err[0] = 0;
    while (*text) {
        char buf[256], *key, *val, *eq, *hash;
        size_t n = strcspn(text, "\n");
        int more = 0;
        line++;
        if (n >= sizeof buf) {
            say(err, errlen, line, "too long", NULL);
            return -1;
        }
        memcpy(buf, text, n);
        buf[n] = 0;
        text += n + (text[n] == '\n');
        /* A comment is a whole line from '#'; in G-code ';' is kept. */
        hash = strchr(buf, '#');
        if (hash) *hash = 0;
        key = trim(buf);
        if (!*key) continue;
        eq = strchr(key, '=');
        if (!eq) {
            say(err, errlen, line, "wants key = value", NULL);
            return -1;
        }
        if (eq > key && eq[-1] == '+') {
            more = 1;
            eq[-1] = 0;
        }
        *eq = 0;
        val = trim(eq + 1);
        key = trim(key);
        if (one(m, key, val, more, line, err, errlen)) return -1;
    }
    return 0;
}

int om_load(om_machine *m, const char *path, char *err, int errlen)
{
    FILE *f = fopen(path, "rb");
    char *text;
    long len;
    int rc;
    if (!f) {
        if (err && errlen > 0) snprintf(err, (size_t)errlen, "%s: cannot open", path);
        return -1;
    }
    fseek(f, 0, SEEK_END);
    len = ftell(f);
    fseek(f, 0, SEEK_SET);
    text = (char *)malloc((size_t)(len > 0 ? len : 0) + 1);
    if (!text || len < 0 || fread(text, 1, (size_t)len, f) != (size_t)len) {
        fclose(f);
        free(text);
        if (err && errlen > 0) snprintf(err, (size_t)errlen, "%s: cannot read", path);
        return -1;
    }
    fclose(f);
    text[len] = 0;
    rc = om_parse(m, text, err, errlen);
    free(text);
    return rc;
}

float om_clamp_speed(const om_machine *m, int axis, float v, int *held)
{
    float most = axis >= 0 && axis < OM_AXES ? m->speed[axis] : 0.f;
    int h = most > 0.f && v > most;
    if (held) *held = h;
    return h ? most : v;
}

/* The first reason, in words, goes in why. */
static void tell(char *why, int whylen, int *said, const char *msg)
{
    if (*said || !why || whylen <= 0) return;
    snprintf(why, (size_t)whylen, "%s", msg);
    *said = 1;
}

int om_check(const om_machine *m, const om_job *job, char *why, int whylen)
{
    static const char axis[] = "XYZE";
    char msg[96];
    int rc = OM_OK, said = 0, i;
    if (why && whylen > 0) why[0] = 0;
    for (i = 0; i < 3; i++) {
        if (m->area[i] <= 0.f) continue;
        if (job->min[i] < 0.f || job->max[i] > m->area[i]) {
            snprintf(msg, sizeof msg, "%c runs to %.1f mm; the machine has %.1f", axis[i],
                     (double)(job->min[i] < 0.f ? job->min[i] : job->max[i]), (double)m->area[i]);
            tell(why, whylen, &said, msg);
            rc |= OM_TOO_BIG;
        }
    }
    for (i = 0; i < OM_AXES; i++)
        if (m->speed[i] > 0.f && job->speed[i] > m->speed[i]) {
            snprintf(msg, sizeof msg, "%c at %.0f mm/s; the most is %.0f", axis[i], (double)job->speed[i],
                     (double)m->speed[i]);
            tell(why, whylen, &said, msg);
            rc |= OM_TOO_FAST;
        }
    if (job->hotend > m->hotend || job->bed > m->bed) {
        if (job->hotend > m->hotend)
            snprintf(msg, sizeof msg, "hotend %.0f C; the most is %.0f", (double)job->hotend, (double)m->hotend);
        else
            snprintf(msg, sizeof msg, "bed %.0f C; the most is %.0f", (double)job->bed, (double)m->bed);
        tell(why, whylen, &said, msg);
        rc |= OM_TOO_HOT;
    }
    if (job->laser > m->laser || job->rpm > m->rpm) {
        if (job->laser > m->laser)
            snprintf(msg, sizeof msg, "laser at %.0f%%; the most is %.0f", (double)job->laser, (double)m->laser);
        else
            snprintf(msg, sizeof msg, "spindle at %.0f rpm; the most is %.0f", (double)job->rpm, (double)m->rpm);
        tell(why, whylen, &said, msg);
        rc |= OM_TOO_MUCH;
    }
    if (job->extruders > m->extruders || (job->needs & ~m->features)) {
        if (job->extruders > m->extruders)
            snprintf(msg, sizeof msg, "%d extruders; the machine has %d", job->extruders, m->extruders);
        else
            for (i = 0; i < NF; i++)
                if (job->needs & ~m->features & fbits[i]) {
                    snprintf(msg, sizeof msg, "the job needs %s", fnames[i]);
                    break;
                }
        tell(why, whylen, &said, msg);
        rc |= OM_MISSING;
    }
    return rc;
}

int om_code(const char *tmpl, float hotend, float bed, char *out, int outlen)
{
    int n = 0;
    while (*tmpl) {
        const char *name = NULL;
        float v = 0.f;
        if (!strncmp(tmpl, "{hotend}", 8)) {
            name = "{hotend}";
            v = hotend;
        } else if (!strncmp(tmpl, "{bed}", 5)) {
            name = "{bed}";
            v = bed;
        }
        if (name) {
            int k = snprintf(out + n, (size_t)(outlen - n), "%.0f", (double)v);
            if (k < 0 || n + k >= outlen) return -1;
            n += k;
            tmpl += strlen(name);
            continue;
        }
        if (n + 1 >= outlen) return -1;
        out[n++] = *tmpl++;
    }
    out[n] = 0;
    return 0;
}
