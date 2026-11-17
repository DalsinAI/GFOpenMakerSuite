/* The shared machine picker (omview_machines.h).
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE /* opendir under -std=c99 on the host */
#endif
#include "omview_machines.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* a and b as one path: no '/' after a volume's ':' or one already there. */
static void join(char *out, int len, const char *a, const char *b)
{
    size_t n = strlen(a);
    snprintf(out, (size_t)len, "%s%s%s", a, n && (a[n - 1] == '/' || a[n - 1] == ':') ? "" : "/", b);
}

static int lower(int c) { return c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c; }

static int by_name(const void *pa, const void *pb)
{
    const char *a = ((const omv_machine *)pa)->name, *b = ((const omv_machine *)pb)->name;
    while (*a && lower((unsigned char)*a) == lower((unsigned char)*b)) a++, b++;
    return lower((unsigned char)*a) - lower((unsigned char)*b);
}

static int family_of(const char *name)
{
    if (!strcmp(name, "printer")) return OM_PRINTER;
    if (!strcmp(name, "laser")) return OM_LASER;
    if (!strcmp(name, "cnc")) return OM_CNC;
    return 0;
}

static int readable(const char *path)
{
    FILE *f = fopen(path, "r");
    if (f) fclose(f);
    return f != NULL;
}

/* Every .om file in dir/profiles, read over dir/driver.om. 0, or -1 out of memory. */
static int add_driver(omv_machines *l, const char *dir, int family)
{
    char driver[256], profiles[256], err[128];
    struct dirent *e;
    DIR *d;
    join(driver, (int)sizeof driver, dir, "driver.om");
    join(profiles, (int)sizeof profiles, dir, "profiles");
    if (!readable(driver) || !(d = opendir(profiles))) return 0;
    while ((e = readdir(d)) != NULL) {
        size_t n = strlen(e->d_name);
        omv_machine *at;
        om_machine m;
        if (n < 4 || strcmp(e->d_name + n - 3, ".om")) continue;
        if (l->n == l->cap) {
            int cap = l->cap ? l->cap * 2 : 8;
            omv_machine *more = realloc(l->at, (size_t)cap * sizeof *more);
            if (!more) {
                closedir(d);
                return -1;
            }
            l->at = more;
            l->cap = cap;
        }
        at = &l->at[l->n];
        strcpy(at->driver, driver);
        join(at->profile, (int)sizeof at->profile, profiles, e->d_name);
        om_clear(&m);
        if (om_load(&m, driver, err, (int)sizeof err) || om_load(&m, at->profile, err, (int)sizeof err)) {
            if (!l->skipped++) snprintf(l->why, sizeof l->why, "%.60s: %.90s", e->d_name, err);
            continue;
        }
        if (m.family != family) continue;
        if (m.name[0]) snprintf(at->name, sizeof at->name, "%s", m.name);
        else snprintf(at->name, sizeof at->name, "%.*s", (int)(n - 3), e->d_name);
        l->n++;
    }
    closedir(d);
    return 0;
}

int omv_machines_scan(omv_machines *l, const char *root, const char *family)
{
    char dir[256], one[256];
    struct dirent *e;
    DIR *d;
    int want = family_of(family);
    l->n = 0;
    l->skipped = 0;
    l->why[0] = 0;
    join(dir, (int)sizeof dir, root, family);
    if (!want || !(d = opendir(dir))) return 0;
    while ((e = readdir(d)) != NULL) {
        if (e->d_name[0] == '.') continue;
        join(one, (int)sizeof one, dir, e->d_name);
        if (add_driver(l, one, want)) {
            closedir(d);
            return -1;
        }
    }
    closedir(d);
    if (l->n > 1) qsort(l->at, (size_t)l->n, sizeof *l->at, by_name);
    return l->n;
}

void omv_machines_free(omv_machines *l)
{
    free(l->at);
    l->at = NULL;
    l->n = l->cap = 0;
}

int omv_machines_load(const omv_machines *l, int i, om_machine *m, char *err, int errlen)
{
    if (i < 0 || i >= l->n) {
        snprintf(err, (size_t)errlen, "No machine chosen");
        return -1;
    }
    om_clear(m);
    if (om_load(m, l->at[i].driver, err, errlen) || om_load(m, l->at[i].profile, err, errlen)) return -1;
    return 0;
}

int omv_machines_fill(olui *ui, const char *id, const omv_machines *l, const char *none)
{
    olui_el *sel = olui_find(ui, id);
    int i;
    if (!sel) return 0;
    olui_select_clear(ui, sel);
    for (i = 0; i < l->n; i++) olui_select_add(ui, sel, l->at[i].name, NULL);
    if (!l->n) olui_select_add(ui, sel, none, NULL);
    olui_select_set(ui, sel, 0);
    olui_set_disabled(ui, sel, !l->n);
    return l->n;
}

int omv_machines_chosen(olui *ui, const char *id, const omv_machines *l)
{
    olui_el *sel = olui_find(ui, id);
    int i = sel ? olui_select_index(sel) : -1;
    return l->n && i >= 0 && i < l->n ? i : -1;
}
