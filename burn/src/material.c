/* Materials-library profiles (burn_material.h): key = value, '#' a comment,
 * as the machines/ and settings/ files. Starting points, not a measured cut.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "burn_material.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *trim(char *s)
{
    char *e;
    while (*s == ' ' || *s == '\t' || *s == '\r') s++;
    e = s + strlen(s);
    while (e > s && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\r' || e[-1] == '\n')) *--e = 0;
    return s;
}

static int parse_f(const char *s, float *out)
{
    float v = 0.f, d = 1.f;
    int seen = 0, neg = 0;
    if (!s) return -1;
    while (*s == ' ' || *s == '\t') s++;
    if (*s == '-') {
        neg = 1;
        s++;
    }
    while (*s >= '0' && *s <= '9') {
        v = v * 10.f + (float)(*s++ - '0');
        seen = 1;
    }
    if (*s == '.') {
        s++;
        while (*s >= '0' && *s <= '9') {
            d *= 10.f;
            v = v + (float)(*s++ - '0') / d;
            seen = 1;
        }
    }
    while (*s == ' ' || *s == '\t') s++;
    if (!seen || *s) return -1;
    *out = neg ? -v : v;
    return 0;
}

static int parse_i(const char *s, int *out)
{
    float v;
    if (parse_f(s, &v)) return -1;
    if (v != (float)(int)v) return -1;
    *out = (int)v;
    return 0;
}

static int one(burn_material *m, const char *key, const char *val, int line, char *err, int errlen)
{
    if (!strcmp(key, "name")) {
        snprintf(m->name, sizeof m->name, "%s", val);
        return 0;
    }
    if (!strcmp(key, "power")) {
        if (parse_f(val, &m->power)) {
            snprintf(err, (size_t)errlen, "line %d: power wants a number", line);
            return -1;
        }
        m->set |= BURN_SET_POWER;
        return 0;
    }
    if (!strcmp(key, "speed")) {
        if (parse_f(val, &m->speed)) {
            snprintf(err, (size_t)errlen, "line %d: speed wants a number", line);
            return -1;
        }
        m->set |= BURN_SET_SPEED;
        return 0;
    }
    if (!strcmp(key, "interval")) {
        if (parse_f(val, &m->interval)) {
            snprintf(err, (size_t)errlen, "line %d: interval wants a number", line);
            return -1;
        }
        m->set |= BURN_SET_INTERVAL;
        return 0;
    }
    if (!strcmp(key, "passes")) {
        if (parse_i(val, &m->passes) || m->passes < 1) {
            snprintf(err, (size_t)errlen, "line %d: passes wants an integer from 1", line);
            return -1;
        }
        m->set |= BURN_SET_PASSES;
        return 0;
    }
    if (!strcmp(key, "mode")) {
        int md = burn_mode_parse(val);
        if (md < 0) {
            snprintf(err, (size_t)errlen, "line %d: mode wants line, fill or offset", line);
            return -1;
        }
        m->mode = md;
        m->set |= BURN_SET_MODE;
        return 0;
    }
    if (!strcmp(key, "constant")) {
        if (parse_i(val, &m->constant)) {
            snprintf(err, (size_t)errlen, "line %d: constant wants 0 or 1", line);
            return -1;
        }
        m->constant = m->constant ? 1 : 0;
        m->set |= BURN_SET_CONSTANT;
        return 0;
    }
    snprintf(err, (size_t)errlen, "line %d: %s: not a key", line, key);
    return -1;
}

int burn_material_load(burn_material *m, const char *path, char *err, int errlen)
{
    FILE *f;
    char buf[256];
    int line = 0;
    if (!m || !path || !path[0]) {
        snprintf(err, (size_t)errlen, "no material file");
        return -1;
    }
    memset(m, 0, sizeof *m);
    m->size = (int)sizeof *m;
    snprintf(m->path, sizeof m->path, "%s", path);
    f = fopen(path, "r");
    if (!f) {
        snprintf(err, (size_t)errlen, "%s: cannot open", path);
        return -1;
    }
    while (fgets(buf, (int)sizeof buf, f)) {
        char *key, *val, *hash, *eq;
        line++;
        hash = strchr(buf, '#');
        if (hash) *hash = 0;
        key = trim(buf);
        if (!*key) continue;
        eq = strchr(key, '=');
        if (!eq) {
            snprintf(err, (size_t)errlen, "line %d: wants key = value", line);
            fclose(f);
            return -1;
        }
        *eq++ = 0;
        key = trim(key);
        val = trim(eq);
        if (!*key) {
            snprintf(err, (size_t)errlen, "line %d: wants key = value", line);
            fclose(f);
            return -1;
        }
        if (one(m, key, val, line, err, errlen)) {
            fclose(f);
            return -1;
        }
    }
    fclose(f);
    if (!m->name[0]) {
        snprintf(err, (size_t)errlen, "%s: wants a name", path);
        return -1;
    }
    if (!m->set) {
        snprintf(err, (size_t)errlen, "%s: wants a setting", path);
        return -1;
    }
    return 0;
}

void burn_material_apply(const burn_material *m, burn_opts *o)
{
    if (!m || !o) return;
    if (m->set & BURN_SET_POWER) o->power = m->power;
    if (m->set & BURN_SET_SPEED) o->speed = m->speed;
    if (m->set & BURN_SET_INTERVAL) o->interval = m->interval;
    if (m->set & BURN_SET_PASSES) o->passes = m->passes;
    if (m->set & BURN_SET_MODE) o->mode = m->mode;
    if (m->set & BURN_SET_CONSTANT) o->constant = m->constant;
}

void burn_materials_init(burn_materials *l)
{
    if (!l) return;
    memset(l, 0, sizeof *l);
}

void burn_materials_free(burn_materials *l)
{
    if (!l) return;
    free(l->at);
    memset(l, 0, sizeof *l);
}

static int by_name(const void *a, const void *b)
{
    const burn_material *x = a, *y = b;
    int c = strcmp(x->name, y->name);
    return c ? c : strcmp(x->path, y->path);
}

int burn_materials_scan(burn_materials *l, const char *dir)
{
    DIR *d;
    struct dirent *e;
    if (!l) return -1;
    if (!dir || !dir[0]) return 0;
    d = opendir(dir);
    if (!d) return 0;
    while ((e = readdir(d)) != NULL) {
        size_t n = strlen(e->d_name);
        char path[256], err[160];
        burn_material m, *at;
        if (n < 4 || strcmp(e->d_name + n - 3, ".om")) continue;
        {
            size_t dn = strlen(dir), sn;
            const char *sep = (dn && (dir[dn - 1] == '/' || dir[dn - 1] == ':')) ? "" : "/";
            sn = strlen(sep);
            if (dn + sn + n + 1 > sizeof path) continue;
            memcpy(path, dir, dn);
            memcpy(path + dn, sep, sn);
            memcpy(path + dn + sn, e->d_name, n + 1);
        }
        if (burn_material_load(&m, path, err, (int)sizeof err)) {
            if (!l->skipped++) snprintf(l->why, sizeof l->why, "%.60s: %.90s", e->d_name, err);
            continue;
        }
        if (l->n == l->cap) {
            int cap = l->cap ? l->cap * 2 : 8;
            burn_material *more = realloc(l->at, (size_t)cap * sizeof *more);
            if (!more) {
                closedir(d);
                return -1;
            }
            l->at = more;
            l->cap = cap;
        }
        at = &l->at[l->n++];
        *at = m;
    }
    closedir(d);
    if (l->n > 1) qsort(l->at, (size_t)l->n, sizeof *l->at, by_name);
    return 0;
}
