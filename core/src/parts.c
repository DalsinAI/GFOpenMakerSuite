/* gs_parts: the bench's parts list (cad/bench/modeller/bench.ts's parts,
 * names, metals and the 24 limit), written again in C. Only malloc, free,
 * memcpy and memset, so openmakemesh.library can carry it.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gs_parts.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
    gs_mesh m;
    char name[GS_PARTS_NAME + 1];
    int colour, shown;
} part;

struct gs_parts {
    part p[GS_PARTS_MAX];
    int n, picked, next_colour, made;
};

static const unsigned long metals[GS_PARTS_COLOURS] = {0xc4a15aUL, 0xd7c4a2UL, 0xc47a5aUL,
                                                       0x8aa0a8UL, 0x3e8f96UL, 0xa3b18aUL};

static void bound(gs_mesh *m)
{
    int i, k;
    m->min.x = m->min.y = m->min.z = 1e30f;
    m->max.x = m->max.y = m->max.z = -1e30f;
    for (i = 0; i < m->n; i++)
        for (k = 0; k < 3; k++) {
            const gs_v *v = k == 0 ? &m->t[i].a : k == 1 ? &m->t[i].b : &m->t[i].c;
            if (v->x < m->min.x) m->min.x = v->x;
            if (v->y < m->min.y) m->min.y = v->y;
            if (v->z < m->min.z) m->min.z = v->z;
            if (v->x > m->max.x) m->max.x = v->x;
            if (v->y > m->max.y) m->max.y = v->y;
            if (v->z > m->max.z) m->max.z = v->z;
        }
    if (!m->n) memset(&m->min, 0, sizeof m->min), memset(&m->max, 0, sizeof m->max);
}

static int ok(const gs_parts *ps, int i) { return ps && i >= 0 && i < ps->n; }

/* Trimmed into to, at most 48; 0, or -1 when blank. */
static int set_name(char *to, const char *from)
{
    const char *e;
    int n;
    if (!from) return -1;
    while (*from == ' ' || *from == '\t') from++;
    e = from + strlen(from);
    while (e > from && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\n')) e--;
    n = (int)(e - from);
    if (n <= 0) return -1;
    if (n > GS_PARTS_NAME) n = GS_PARTS_NAME;
    memcpy(to, from, (size_t)n);
    to[n] = 0;
    return 0;
}

gs_parts *gs_parts_new(void)
{
    gs_parts *ps = (gs_parts *)malloc(sizeof *ps);
    if (ps) {
        memset(ps, 0, sizeof *ps);
        ps->picked = -1;
    }
    return ps;
}

void gs_parts_free(gs_parts *ps)
{
    int i;
    if (!ps) return;
    for (i = 0; i < ps->n; i++) free(ps->p[i].m.t);
    free(ps);
}

int gs_parts_count(const gs_parts *ps) { return ps ? ps->n : 0; }

int gs_parts_add(gs_parts *ps, gs_mesh *m, const char *name)
{
    part *p;
    if (!ps || !m || !m->t || m->n <= 0 || ps->n >= GS_PARTS_MAX) return -1;
    p = &ps->p[ps->n];
    memset(p, 0, sizeof *p);
    ps->made++;
    if (set_name(p->name, name)) { /* "Part n", n the parts made so far */
        char d[8];
        int k = 0, v = ps->made;
        do d[k++] = (char)('0' + v % 10), v /= 10;
        while (v && k < 7);
        memcpy(p->name, "Part ", 5);
        for (v = 0; v < k; v++) p->name[5 + v] = d[k - 1 - v];
        p->name[5 + k] = 0;
    }
    p->m = *m;
    bound(&p->m);
    memset(m, 0, sizeof *m);
    p->colour = ps->next_colour;
    ps->next_colour = (ps->next_colour + 1) % GS_PARTS_COLOURS;
    p->shown = 1;
    ps->picked = ps->n;
    return ps->n++;
}

int gs_parts_remove(gs_parts *ps, int i)
{
    if (!ok(ps, i)) return -1;
    free(ps->p[i].m.t);
    for (; i + 1 < ps->n; i++) ps->p[i] = ps->p[i + 1];
    ps->n--;
    ps->picked = ps->n - 1;
    return 0;
}

gs_mesh *gs_parts_mesh(gs_parts *ps, int i) { return ok(ps, i) ? &ps->p[i].m : NULL; }
const char *gs_parts_name(const gs_parts *ps, int i) { return ok(ps, i) ? ps->p[i].name : NULL; }
int gs_parts_rename(gs_parts *ps, int i, const char *name) { return ok(ps, i) ? set_name(ps->p[i].name, name) : -1; }
int gs_parts_colour(const gs_parts *ps, int i) { return ok(ps, i) ? ps->p[i].colour : -1; }

int gs_parts_set_colour(gs_parts *ps, int i, int colour)
{
    if (!ok(ps, i) || colour < 0 || colour >= GS_PARTS_COLOURS) return -1;
    ps->p[i].colour = colour;
    return 0;
}

unsigned long gs_parts_rgb(int colour) { return colour >= 0 && colour < GS_PARTS_COLOURS ? metals[colour] : 0UL; }
int gs_parts_shown(const gs_parts *ps, int i) { return ok(ps, i) ? ps->p[i].shown : -1; }

int gs_parts_show(gs_parts *ps, int i, int on)
{
    if (!ok(ps, i)) return -1;
    ps->p[i].shown = on != 0;
    return 0;
}

int gs_parts_picked(const gs_parts *ps) { return ps ? ps->picked : -1; }

int gs_parts_pick(gs_parts *ps, int i)
{
    if (!ps || (i != -1 && !ok(ps, i))) return -1;
    ps->picked = i;
    return 0;
}

int gs_parts_scene(const gs_parts *ps, gs_mesh *out)
{
    int i, n = 0;
    gs_tri *t;
    if (!ps || !out) return -1;
    for (i = 0; i < ps->n; i++)
        if (ps->p[i].shown) n += ps->p[i].m.n;
    t = n ? (gs_tri *)malloc((size_t)n * sizeof *t) : NULL;
    if (n && !t) return -1;
    free(out->t);
    memset(out, 0, sizeof *out);
    out->t = t;
    for (i = 0; i < ps->n; i++)
        if (ps->p[i].shown && ps->p[i].m.n) {
            memcpy(t + out->n, ps->p[i].m.t, (size_t)ps->p[i].m.n * sizeof *t);
            out->n += ps->p[i].m.n;
        }
    out->cap = out->n;
    bound(out);
    return 0;
}

gs_parts *gs_parts_copy(const gs_parts *ps)
{
    gs_parts *c;
    int i;
    if (!ps || !(c = (gs_parts *)malloc(sizeof *c))) return NULL;
    memcpy(c, ps, sizeof *c);
    for (i = 0; i < c->n; i++) {
        size_t sz = (size_t)ps->p[i].m.n * sizeof(gs_tri);
        c->p[i].m.t = sz ? (gs_tri *)malloc(sz) : NULL;
        if (sz && !c->p[i].m.t) {
            c->n = i;
            gs_parts_free(c);
            return NULL;
        }
        if (sz) memcpy(c->p[i].m.t, ps->p[i].m.t, sz);
        c->p[i].m.cap = c->p[i].m.n;
    }
    return c;
}
