/* Move, turn, scale, and lay a face on the bed. The copies for undo are
 * ours. No other editor's source is in here.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gs_edit.h"
#include "gs_model.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HIST 32

struct ge_hist {
    ge_scene slot[HIST];
    int n;
};

static void bound(gs_mesh *m)
{
    int i, k;
    if (!m->n || !m->t) {
        m->min.x = m->min.y = m->min.z = 0.f;
        m->max = m->min;
        return;
    }
    m->min = m->max = m->t[0].a;
    for (i = 0; i < m->n; i++) {
        gs_v p[3];
        p[0] = m->t[i].a;
        p[1] = m->t[i].b;
        p[2] = m->t[i].c;
        for (k = 0; k < 3; k++) {
            if (p[k].x < m->min.x) m->min.x = p[k].x;
            if (p[k].y < m->min.y) m->min.y = p[k].y;
            if (p[k].z < m->min.z) m->min.z = p[k].z;
            if (p[k].x > m->max.x) m->max.x = p[k].x;
            if (p[k].y > m->max.y) m->max.y = p[k].y;
            if (p[k].z > m->max.z) m->max.z = p[k].z;
        }
    }
}

static gs_v mid(const gs_mesh *m)
{
    gs_v c;
    c.x = (m->min.x + m->max.x) * 0.5f;
    c.y = (m->min.y + m->max.y) * 0.5f;
    c.z = (m->min.z + m->max.z) * 0.5f;
    return c;
}

static void spin_about(gs_mesh *m, gs_v c, float ax, float ay, float az, float ang)
{
    float co, si, len;
    int i, k;
    len = sqrtf(ax * ax + ay * ay + az * az);
    if (len < 1e-8f || (ang > -1e-8f && ang < 1e-8f)) return;
    ax /= len;
    ay /= len;
    az /= len;
    co = cosf(ang);
    si = sinf(ang);
    for (i = 0; i < m->n; i++) {
        gs_v *p[3];
        p[0] = &m->t[i].a;
        p[1] = &m->t[i].b;
        p[2] = &m->t[i].c;
        for (k = 0; k < 3; k++) {
            float vx = p[k]->x - c.x, vy = p[k]->y - c.y, vz = p[k]->z - c.z;
            float cx = ay * vz - az * vy;
            float cy = az * vx - ax * vz;
            float cz = ax * vy - ay * vx;
            float d = ax * vx + ay * vy + az * vz;
            p[k]->x = c.x + vx * co + cx * si + ax * d * (1.f - co);
            p[k]->y = c.y + vy * co + cy * si + ay * d * (1.f - co);
            p[k]->z = c.z + vz * co + cz * si + az * d * (1.f - co);
        }
    }
    bound(m);
}

static int face_nrm(const gs_tri *t, gs_v *n)
{
    float ux = t->b.x - t->a.x, uy = t->b.y - t->a.y, uz = t->b.z - t->a.z;
    float vx = t->c.x - t->a.x, vy = t->c.y - t->a.y, vz = t->c.z - t->a.z;
    float nx = uy * vz - uz * vy;
    float ny = uz * vx - ux * vz;
    float nz = ux * vy - uy * vx;
    float L = sqrtf(nx * nx + ny * ny + nz * nz);
    if (L < 1e-12f) return -1;
    n->x = nx / L;
    n->y = ny / L;
    n->z = nz / L;
    return 0;
}

int ge_copy(gs_mesh *dst, const gs_mesh *src)
{
    gs_tri *t = NULL;
    int n = 0;
    if (!dst) return -1;
    if (src && src->n > 0 && src->t) {
        n = src->n;
        t = (gs_tri *)malloc((size_t)n * sizeof *t);
        if (!t) return -1;
        memcpy(t, src->t, (size_t)n * sizeof *t);
    }
    free(dst->t);
    memset(dst, 0, sizeof *dst);
    dst->t = t;
    dst->n = dst->cap = n;
    bound(dst);
    return 0;
}

void ge_move(gs_mesh *m, float dx, float dy, float dz)
{
    int i;
    if (!m || !m->t) return;
    if (dx == 0.f && dy == 0.f && dz == 0.f) return;
    for (i = 0; i < m->n; i++) {
        m->t[i].a.x += dx;
        m->t[i].a.y += dy;
        m->t[i].a.z += dz;
        m->t[i].b.x += dx;
        m->t[i].b.y += dy;
        m->t[i].b.z += dz;
        m->t[i].c.x += dx;
        m->t[i].c.y += dy;
        m->t[i].c.z += dz;
    }
    m->min.x += dx;
    m->min.y += dy;
    m->min.z += dz;
    m->max.x += dx;
    m->max.y += dy;
    m->max.z += dz;
}

void ge_rotate(gs_mesh *m, float rx, float ry, float rz)
{
    gs_v c;
    if (!m || !m->n) return;
    c = mid(m);
    if (rx) spin_about(m, c, 1.f, 0.f, 0.f, rx);
    if (ry) {
        c = mid(m);
        spin_about(m, c, 0.f, 1.f, 0.f, ry);
    }
    if (rz) {
        c = mid(m);
        spin_about(m, c, 0.f, 0.f, 1.f, rz);
    }
}

void ge_scale(gs_mesh *m, float sx, float sy, float sz)
{
    gs_v c;
    int i, k;
    if (!m || !m->t || !m->n) return;
    c = mid(m);
    for (i = 0; i < m->n; i++) {
        gs_v *p[3];
        p[0] = &m->t[i].a;
        p[1] = &m->t[i].b;
        p[2] = &m->t[i].c;
        for (k = 0; k < 3; k++) {
            p[k]->x = c.x + (p[k]->x - c.x) * sx;
            p[k]->y = c.y + (p[k]->y - c.y) * sy;
            p[k]->z = c.z + (p[k]->z - c.z) * sz;
        }
    }
    bound(m);
}

void ge_drop(gs_mesh *m)
{
    if (!m || !m->n) return;
    ge_move(m, 0.f, 0.f, -m->min.z);
}

void ge_centre(gs_mesh *m)
{
    float cx, cy;
    if (!m || !m->n) return;
    cx = (m->min.x + m->max.x) * 0.5f;
    cy = (m->min.y + m->max.y) * 0.5f;
    ge_move(m, GS_BED * 0.5f - cx, GS_BED * 0.5f - cy, 0.f);
}

int ge_largest_face(const gs_mesh *m)
{
    int i, best = -1;
    float besta = 0.f;
    if (!m) return -1;
    for (i = 0; i < m->n; i++) {
        gs_v n;
        float ux, uy, uz, vx, vy, vz, area;
        if (face_nrm(&m->t[i], &n)) continue;
        ux = m->t[i].b.x - m->t[i].a.x;
        uy = m->t[i].b.y - m->t[i].a.y;
        uz = m->t[i].b.z - m->t[i].a.z;
        vx = m->t[i].c.x - m->t[i].a.x;
        vy = m->t[i].c.y - m->t[i].a.y;
        vz = m->t[i].c.z - m->t[i].a.z;
        area = sqrtf((uy * vz - uz * vy) * (uy * vz - uz * vy) +
                     (uz * vx - ux * vz) * (uz * vx - ux * vz) +
                     (ux * vy - uy * vx) * (ux * vy - uy * vx));
        if (area > besta) {
            besta = area;
            best = i;
        }
    }
    return best;
}

int ge_lay_face(gs_mesh *m, int face)
{
    gs_v n, c;
    float d, ang, fz;
    if (!m || face < 0 || face >= m->n) return -1;
    if (face_nrm(&m->t[face], &n)) return -1;
    d = -n.z;
    c = mid(m);
    if (d < 0.9999f) {
        if (d <= -0.9999f) {
            ang = 3.14159265f;
            spin_about(m, c, 1.f, 0.f, 0.f, ang);
        } else {
            float ax = n.y * -1.f - n.z * 0.f;
            float ay = n.z * 0.f - n.x * -1.f;
            float az = n.x * 0.f - n.y * 0.f;
            if (d < -1.f) d = -1.f;
            if (d > 1.f) d = 1.f;
            ang = acosf(d);
            spin_about(m, c, ax, ay, az, ang);
        }
    }
    fz = (m->t[face].a.z + m->t[face].b.z + m->t[face].c.z) / 3.f;
    ge_move(m, 0.f, 0.f, -fz);
    return 0;
}

int ge_pick_at(const gs_mesh *m, const gs_cam *cam, int w, int h, int x, int y, gs_v *at)
{
    gs_frame f;
    int i, best = -1;
    float bestz = -1e30f, b0 = 0.f, b1 = 0.f, b2 = 0.f;
    float px, py;
    if (!m || !cam || gs_view_frame_bed(m, cam, w, h, 0.f, 0.f, GS_BED, GS_BED, &f)) return -1;
    px = (float)x + 0.5f;
    py = (float)y + 0.5f;
    for (i = 0; i < m->n; i++) {
        float ax, ay, az, bx, by, bz, cx, cy, cz;
        float area, inv, w0, w1, w2, z;
        gs_view_map(&f, cam, m->t[i].a, &ax, &ay, &az);
        gs_view_map(&f, cam, m->t[i].b, &bx, &by, &bz);
        gs_view_map(&f, cam, m->t[i].c, &cx, &cy, &cz);
        area = (bx - ax) * (cy - ay) - (cx - ax) * (by - ay);
        if (area > -0.5f && area < 0.5f) continue;
        inv = 1.f / area;
        w0 = ((bx - px) * (cy - py) - (cx - px) * (by - py)) * inv;
        w1 = ((cx - px) * (ay - py) - (ax - px) * (cy - py)) * inv;
        w2 = 1.f - w0 - w1;
        if (w0 < 0.f || w1 < 0.f || w2 < 0.f) continue;
        z = w0 * az + w1 * bz + w2 * cz;
        if (z <= bestz) continue;
        bestz = z;
        best = i;
        b0 = w0, b1 = w1, b2 = w2;
    }
    /* The view is a parallel one, so the screen's weights are the face's. */
    if (best >= 0 && at) {
        const gs_tri *t = &m->t[best];
        at->x = b0 * t->a.x + b1 * t->b.x + b2 * t->c.x;
        at->y = b0 * t->a.y + b1 * t->b.y + b2 * t->c.y;
        at->z = b0 * t->a.z + b1 * t->b.z + b2 * t->c.z;
    }
    return best;
}

int ge_pick(const gs_mesh *m, const gs_cam *cam, int w, int h, int x, int y)
{
    return ge_pick_at(m, cam, w, h, x, y, NULL);
}

int ge_pick_bed(const gs_mesh *m, const gs_cam *cam, int w, int h, int x, int y, gs_v *at)
{
    static const gs_mesh none;
    static const gs_v o = {0.f, 0.f, 0.f}, ex = {GS_BED, 0.f, 0.f}, ey = {0.f, GS_BED, 0.f};
    gs_frame f;
    float ox, oy, ax, ay, bx, by, z, det, u, v, px, py;
    if (!cam || gs_view_frame_bed(m ? m : &none, cam, w, h, 0.f, 0.f, GS_BED, GS_BED, &f)) return -1;
    /* The view is a parallel one: the bed's corners give its two edges on
     * the screen, and the pointer is u of one and v of the other. */
    gs_view_map(&f, cam, o, &ox, &oy, &z);
    gs_view_map(&f, cam, ex, &ax, &ay, &z);
    gs_view_map(&f, cam, ey, &bx, &by, &z);
    ax -= ox, ay -= oy, bx -= ox, by -= oy;
    det = ax * by - ay * bx;
    if (det > -1.f && det < 1.f) return -1; /* the bed seen edge on */
    px = (float)x + 0.5f - ox;
    py = (float)y + 0.5f - oy;
    u = (px * by - py * bx) / det;
    v = (ax * py - ay * px) / det;
    if (u < 0.f || u > 1.f || v < 0.f || v > 1.f) return -1;
    if (at) {
        at->x = u * GS_BED;
        at->y = v * GS_BED;
        at->z = 0.f;
    }
    return 0;
}

int ge_project(const gs_mesh *m, const gs_cam *cam, int w, int h, gs_v p, float *x, float *y)
{
    gs_frame f;
    float z;
    if (!m || !cam || gs_view_frame_bed(m, cam, w, h, 0.f, 0.f, GS_BED, GS_BED, &f)) return -1;
    gs_view_map(&f, cam, p, x, y, &z);
    return 0;
}

void ge_measure_line(char *buf, int len, gs_v a, gs_v b)
{
    float dx = b.x - a.x, dy = b.y - a.y, dz = b.z - a.z;
    snprintf(buf, (size_t)len, "%.1f mm between the two points (X %.1f, Y %.1f, Z %.1f).",
             (double)sqrtf(dx * dx + dy * dy + dz * dz), (double)fabsf(dx), (double)fabsf(dy), (double)fabsf(dz));
}

void ge_facts_line(char *buf, int len, const gs_mesh *m)
{
    gs_facts f;
    float cm3;
    if (!m || !m->n || gm_facts(m, &f)) {
        snprintf(buf, (size_t)len, "No part to measure.");
        return;
    }
    cm3 = f.volume / 1000.f;
    snprintf(buf, (size_t)len, "%.*f cm3, %.1f g of PLA. %.1f x %.1f x %.1f mm, %d triangles.", cm3 >= 100.f ? 0 : 2,
             (double)cm3, (double)(cm3 * 1.24f), (double)f.sx, (double)f.sy, (double)f.sz, f.tris);
}

void ge_scene_free(ge_scene *s)
{
    if (!s) return;
    gs_mesh_free(&s->stock);
    gs_mesh_free(&s->added);
    s->has_added = s->on_added = 0;
}

ge_hist *ge_hist_new(void)
{
    ge_hist *h = (ge_hist *)calloc(1, sizeof *h);
    return h;
}

void ge_hist_free(ge_hist *h)
{
    int i;
    if (!h) return;
    for (i = 0; i < h->n; i++) ge_scene_free(&h->slot[i]);
    free(h);
}

int ge_hist_push_scene(ge_hist *h, const ge_scene *s)
{
    ge_scene *d;
    if (!h || !s) return -1;
    if (h->n == HIST) {
        ge_scene_free(&h->slot[0]);
        memmove(&h->slot[0], &h->slot[1], (size_t)(HIST - 1) * sizeof h->slot[0]);
        memset(&h->slot[HIST - 1], 0, sizeof h->slot[0]);
        h->n = HIST - 1;
    }
    d = &h->slot[h->n];
    memset(d, 0, sizeof *d);
    if (ge_copy(&d->stock, &s->stock) || (s->has_added && ge_copy(&d->added, &s->added))) {
        ge_scene_free(d);
        return -1;
    }
    d->has_added = s->has_added;
    d->on_added = s->on_added;
    h->n++;
    return 0;
}

int ge_hist_pop_scene(ge_hist *h, ge_scene *s)
{
    if (!h || !s || h->n < 1) return -1;
    h->n--;
    ge_scene_free(s);
    *s = h->slot[h->n];
    memset(&h->slot[h->n], 0, sizeof h->slot[h->n]);
    return 0;
}

int ge_hist_push(ge_hist *h, const gs_mesh *m)
{
    ge_scene t;
    if (!m) return -1;
    memset(&t, 0, sizeof t);
    t.stock = *m;
    return ge_hist_push_scene(h, &t);
}

int ge_hist_pop(ge_hist *h, gs_mesh *m)
{
    ge_scene t;
    if (!m) return -1;
    memset(&t, 0, sizeof t);
    if (ge_hist_pop_scene(h, &t)) return -1;
    gs_mesh_free(m);
    *m = t.stock;
    gs_mesh_free(&t.added);
    return 0;
}

int ge_hist_depth(const ge_hist *h)
{
    return h ? h->n : 0;
}

int ge_bool(ge_scene *s, int op, ge_hist *h)
{
    gs_mesh out;
    const gs_mesh *a, *b;
    int rc;
    if (!s || !s->has_added) return -1;
    a = &s->stock;
    b = &s->added;
    if (op == 1 && !s->on_added) {
        a = &s->added;
        b = &s->stock;
    }
    if (h && ge_hist_push_scene(h, s)) return -2;
    memset(&out, 0, sizeof out);
    rc = op == 0 ? gm_union(&out, a, b) : op == 1 ? gm_subtract(&out, a, b) : gm_intersect(&out, a, b);
    if (rc || out.n < 1) {
        gs_mesh_free(&out);
        if (h) {
            h->n--;
            ge_scene_free(&h->slot[h->n]);
        }
        return -2;
    }
    ge_scene_free(s);
    s->stock = out;
    return 0;
}

struct ge_steps {
    gs_parts *back[HIST], *ahead[HIST];
    int nb, na;
};

ge_steps *ge_steps_new(void) { return (ge_steps *)calloc(1, sizeof(ge_steps)); }

static void drop_ahead(ge_steps *s)
{
    while (s->na > 0) gs_parts_free(s->ahead[--s->na]);
}

void ge_steps_free(ge_steps *s)
{
    if (!s) return;
    drop_ahead(s);
    while (s->nb > 0) gs_parts_free(s->back[--s->nb]);
    free(s);
}

/* p onto a stack of HIST, the oldest falling off. */
static void stack(gs_parts **st, int *n, gs_parts *p)
{
    if (*n == HIST) {
        gs_parts_free(st[0]);
        memmove(&st[0], &st[1], (size_t)(HIST - 1) * sizeof st[0]);
        *n = HIST - 1;
    }
    st[(*n)++] = p;
}

int ge_steps_push(ge_steps *s, const gs_parts *now)
{
    gs_parts *c;
    if (!s || !now || !(c = gs_parts_copy(now))) return -1;
    drop_ahead(s);
    stack(s->back, &s->nb, c);
    return 0;
}

gs_parts *ge_steps_undo(ge_steps *s, gs_parts *now)
{
    if (!s || !now || s->nb < 1) return NULL;
    stack(s->ahead, &s->na, now);
    return s->back[--s->nb];
}

gs_parts *ge_steps_redo(ge_steps *s, gs_parts *now)
{
    if (!s || !now || s->na < 1) return NULL;
    stack(s->back, &s->nb, now);
    return s->ahead[--s->na];
}

gs_parts *ge_steps_forget(ge_steps *s, gs_parts *now)
{
    if (!s || !now || s->nb < 1) return NULL;
    gs_parts_free(now);
    return s->back[--s->nb];
}

int ge_steps_back(const ge_steps *s) { return s ? s->nb : 0; }
int ge_steps_ahead(const ge_steps *s) { return s ? s->na : 0; }

int ge_combine(gs_parts *ps, int op, char *line, int len)
{
    int t = gs_parts_picked(ps), k, colour, rc;
    char stock[GS_PARTS_NAME + 1], tool[GS_PARTS_NAME + 1], name[2 * GS_PARTS_NAME + 8];
    gs_mesh out;
    if (gs_parts_count(ps) < 2 || t < 0 || op < 0 || op > 2) {
        snprintf(line, (size_t)len, "Need two parts. The picked one is the tool.");
        return -1;
    }
    k = t > 0 ? t - 1 : 1;
    memset(&out, 0, sizeof out);
    rc = op == 0 ? gm_union(&out, gs_parts_mesh(ps, k), gs_parts_mesh(ps, t))
         : op == 1 ? gm_subtract(&out, gs_parts_mesh(ps, k), gs_parts_mesh(ps, t))
                   : gm_intersect(&out, gs_parts_mesh(ps, k), gs_parts_mesh(ps, t));
    if (rc || out.n < 1) {
        gs_mesh_free(&out);
        snprintf(line, (size_t)len, "Those two do not meet, so the solid was left as it was.");
        return -2;
    }
    snprintf(stock, sizeof stock, "%s", gs_parts_name(ps, k));
    snprintf(tool, sizeof tool, "%s", gs_parts_name(ps, t));
    colour = gs_parts_colour(ps, k);
    gs_parts_remove(ps, t > k ? t : k);
    gs_parts_remove(ps, t > k ? k : t);
    if (op == 2) snprintf(name, sizeof name, "%s and %s", stock, tool);
    else snprintf(name, sizeof name, "%s", stock);
    k = gs_parts_add(ps, &out, name);
    gs_mesh_free(&out);
    if (k < 0) {
        snprintf(line, (size_t)len, "Not enough memory for the result.");
        return -2;
    }
    gs_parts_set_colour(ps, k, colour);
    if (op == 1)
        snprintf(line, (size_t)len, "Cut %s out of %s. %d triangles.", tool, stock, gs_parts_mesh(ps, k)->n);
    else if (op == 0)
        snprintf(line, (size_t)len, "Joined into %s. %d triangles.", gs_parts_name(ps, k), gs_parts_mesh(ps, k)->n);
    else snprintf(line, (size_t)len, "Kept only the overlap, as %s. %d triangles.", gs_parts_name(ps, k), gs_parts_mesh(ps, k)->n);
    return 0;
}

int ge_open(gs_mesh *m, const char *path, char *err, int errlen)
{
    gs_mesh_free(m);
    if (gs_stl_load(m, path, err, errlen)) return -1;
    gs_mesh_repair(m);
    ge_drop(m);
    return 0;
}

int ge_start(gs_mesh *m, const char *path, char *err, int errlen)
{
    gs_mesh box, peg;
    int rc;
    gs_mesh_free(m);
    if (path && path[0]) return ge_open(m, path, err, errlen);
    memset(&box, 0, sizeof box);
    memset(&peg, 0, sizeof peg);
    rc = gm_cube(&box, 0, 0, 0, 20, 20, 20) || gm_cylinder(&peg, 10, 10, -1, 6, 22, 32) ||
         gm_subtract(m, &box, &peg);
    gs_mesh_free(&box);
    gs_mesh_free(&peg);
    if (rc) {
        snprintf(err, (size_t)errlen, "the solid did not build");
        return -1;
    }
    /* In the middle of the plate, and off it, so Drop is a visible move. */
    ge_centre(m);
    ge_move(m, 0.f, 0.f, 8.f);
    return 0;
}

int ge_extrude(const gs_mesh *part, int kind, int op, float w, float d, float h, gs_mesh *out, char *line, int len)
{
    static const char *const names[3] = {"block", "round", "slot"};
    gs_mesh tool, r;
    float pos[3], size[3], x = GS_BED * 0.5f, y = GS_BED * 0.5f;
    char foot[32];
    int rc;
    if (kind < GM_RECT || kind > GM_SLOT || op < GM_ON_BED || op > GM_FROM_TOP || !out) {
        snprintf(line, (size_t)len, "There is no such extrude.");
        return -1;
    }
    if (!(w >= 1.f && w <= 200.f) || (kind != GM_ROUND && !(d >= 1.f && d <= 200.f))) {
        snprintf(line, (size_t)len, "The footprint has to be 1 to 200 mm each way.");
        return -1;
    }
    if (!(h >= 0.4f && h <= 120.f)) {
        snprintf(line, (size_t)len, "The height has to be 0.4 to 120 mm.");
        return -1;
    }
    if (op != GM_ON_BED) { /* about the middle of the part, as the bench */
        if (!part || !part->t || !part->n) {
            snprintf(line, (size_t)len, "There is no part to stand it on or cut.");
            return -1;
        }
        ge_where(part, pos, size);
        x = pos[0];
        y = pos[1];
    }
    memset(&tool, 0, sizeof tool);
    memset(&r, 0, sizeof r);
    if (gm_extrude(&tool, part, kind, op, x, y, w, d, h)) {
        gs_mesh_free(&tool);
        snprintf(line, (size_t)len, "The %s did not build.", names[kind]);
        return -1;
    }
    if (op == GM_ON_BED) r = tool;
    else {
        rc = op == GM_ON_TOP ? gm_union(&r, part, &tool) : gm_subtract(&r, part, &tool);
        gs_mesh_free(&tool);
        if (rc || !r.n) {
            gs_mesh_free(&r);
            snprintf(line, (size_t)len, op == GM_ON_TOP ? "The boss did not join." : "That cut takes the whole part.");
            return -1;
        }
    }
    gs_mesh_free(out);
    *out = r;
    if (kind == GM_ROUND) snprintf(foot, sizeof foot, "%.1f across", (double)w);
    else snprintf(foot, sizeof foot, "%.1f x %.1f", (double)w, (double)d);
    if (op == GM_ON_BED)
        snprintf(line, (size_t)len, "A %s %s, %.1f mm tall, is on the bed. Place it, then Join or Cut.", names[kind], foot,
                 (double)h);
    else if (op == GM_ON_TOP)
        snprintf(line, (size_t)len, "A %s %s stands %.1f mm on the part, joined.", names[kind], foot, (double)h);
    else if (part->max.z - part->min.z - 0.05f <= h)
        snprintf(line, (size_t)len, "A %s %s cut right through.", names[kind], foot);
    else snprintf(line, (size_t)len, "A %s %s cut %.1f mm down from the top.", names[kind], foot, (double)h);
    return 0;
}

static const char *const shape_names[GE_SHAPES] = {"Box", "Cylinder", "Sphere", "Cone", "Torus", "Wedge", "Slot"};

const char *ge_shape_name(int kind) { return kind >= 0 && kind < GE_SHAPES ? shape_names[kind] : NULL; }

int ge_shape(gs_mesh *m, int kind, const gs_mesh *stock)
{
    float x0 = stock->min.x, y0 = stock->min.y, z0 = stock->min.z, zt = stock->max.z;
    float cx = (stock->min.x + stock->max.x) * 0.5f, cy = (stock->min.y + stock->max.y) * 0.5f;
    float h = stock->max.z - stock->min.z + 4.f, r = (stock->max.x - stock->min.x) * 0.22f;
    if (!stock->n) x0 = y0 = z0 = cx = cy = 0.f, zt = 3.f, h = 24.f, r = 5.f;
    if (h < 8.f) h = 8.f;
    if (r < 3.f) r = 3.f;
    switch (kind) {
    case GE_BOX: /* beside the stock's corner, to join */
        return gm_cube(m, x0 + 12.f, y0 + 4.f, z0, 16.f, 16.f, 16.f);
    case GE_CYLINDER: /* down through the stock's middle, to cut a hole */
        return gm_cylinder(m, cx, cy, z0 - 2.f, r, h, 24);
    case GE_SPHERE:
        return gm_sphere(m, cx, cy, z0 + 8.f, 8.f, 24, 12);
    case GE_CONE:
        return gm_cone(m, cx, cy, z0, 8.f, 16.f, 24);
    case GE_TORUS:
        /* Half in the top face, inside its edges: a cut leaves a groove, a
         * join a ring (8 and 1.5 on the 20 mm start). */
        return gm_torus(m, cx, cy, zt, r * (8.f / 4.4f), r * (1.5f / 4.4f), 24, 12);
    case GE_WEDGE:
        return gm_wedge(m, cx - 10.f, cy - 8.f, z0, 20.f, 16.f, 12.f);
    case GE_SLOT: /* through the stock and inside its ends, to cut a slot */
        return gm_slot(m, cx - r * (7.f / 4.4f), cy - r, z0 - 2.f, r * (14.f / 4.4f), 2.f * r, h, 16);
    }
    return -1;
}

void ge_where(const gs_mesh *m, float pos[3], float size[3])
{
    int k;
    for (k = 0; k < 3; k++) pos[k] = size[k] = 0.f;
    if (!m || !m->t || !m->n) return;
    pos[0] = (m->min.x + m->max.x) * 0.5f;
    pos[1] = (m->min.y + m->max.y) * 0.5f;
    pos[2] = m->min.z;
    size[0] = m->max.x - m->min.x;
    size[1] = m->max.y - m->min.y;
    size[2] = m->max.z - m->min.z;
}

void ge_place(gs_mesh *m, float x, float y, float z)
{
    float pos[3], size[3];
    if (!m || !m->t || !m->n) return;
    ge_where(m, pos, size);
    ge_move(m, x - pos[0], y - pos[1], z - pos[2]);
}

int ge_fit(gs_mesh *m, int axis, float mm)
{
    float pos[3], size[3], s;
    if (!m || !m->t || !m->n || axis < 0 || axis > 2 || !(mm > 0.f)) return -1;
    ge_where(m, pos, size);
    if (size[axis] < 0.05f) return -1;
    s = mm / size[axis];
    ge_scale(m, s, s, s);
    ge_place(m, pos[0], pos[1], pos[2]);
    return 0;
}

int ge_hole_tool(gs_mesh *tool, int kind, const ge_holes *h, const gs_mesh *part, char *why, int whylen)
{
    float cx, cy, half, thick, z0, len, r = h->d * 0.5f;
    int n = h->n;
    why[0] = 0;
    if (!part || !part->t || !part->n) {
        snprintf(why, (size_t)whylen, "There is no part to drill.");
        return -1;
    }
    if (!(h->d >= 0.5f)) {
        snprintf(why, (size_t)whylen, "The drill has to be at least 0.5 mm.");
        return -1;
    }
    cx = (part->min.x + part->max.x) * 0.5f;
    cy = (part->min.y + part->max.y) * 0.5f;
    half = part->max.x - cx;
    if (part->max.y - cy > half) half = part->max.y - cy;
    thick = part->max.z - part->min.z;
    /* From 4 mm under the part to 4 over it, as the bench's drills. */
    z0 = part->min.z - 4.f;
    len = thick + 8.f;
    if (kind == GE_BOLT_CIRCLE) {
        float pr = h->pcd * 0.5f;
        if (n < 2) n = 2;
        if (n > 12) n = 12;
        if (!(h->pcd * sinf(3.14159265f / (float)n) >= h->d + 0.3f)) {
            snprintf(why, (size_t)whylen, "Those holes overlap. Use fewer, or a wider pitch circle.");
            return -1;
        }
        if (pr + r > half + 0.4f) {
            snprintf(why, (size_t)whylen, "A %.1f mm pitch circle is wider than the part.", (double)h->pcd);
            return -1;
        }
        return gm_bolt_circle(tool, cx, cy, z0, len, h->d, h->pcd, n) ? (snprintf(why, (size_t)whylen, "The holes did not build."), -1) : 0;
    }
    if (kind == GE_HOLE_ROW) {
        float span;
        if (n < 2) n = 2;
        if (n > 8) n = 8;
        if (!(h->pitch >= h->d + 0.3f)) {
            snprintf(why, (size_t)whylen, "Those holes overlap. Widen the pitch.");
            return -1;
        }
        span = (float)(n - 1) * h->pitch;
        if (cx - span * 0.5f - r < part->min.x - 0.2f || cx + span * 0.5f + r > part->max.x + 0.2f) {
            snprintf(why, (size_t)whylen, "That row is wider than the part.");
            return -1;
        }
        return gm_hole_row(tool, cx, cy, z0, len, h->d, h->pitch, n) ? (snprintf(why, (size_t)whylen, "The holes did not build."), -1) : 0;
    }
    if (kind == GE_COUNTERBORE) {
        if (!(h->cb >= h->d + 0.4f)) {
            snprintf(why, (size_t)whylen, "The counterbore has to be wider than the drill.");
            return -1;
        }
        if (!(h->deep >= 0.4f) || h->deep >= thick - 0.15f) {
            snprintf(why, (size_t)whylen, "That counterbore is as deep as the part, or too shallow.");
            return -1;
        }
        if (h->cb * 0.5f > half + 0.2f) {
            snprintf(why, (size_t)whylen, "That counterbore is wider than the part.");
            return -1;
        }
        /* The drill ends 0.4 over the top, the bore as deep again below it. */
        return gm_counterbore(tool, cx, cy, z0, thick + 4.4f, h->d, h->cb, h->deep + 0.4f)
                   ? (snprintf(why, (size_t)whylen, "The counterbore did not build."), -1)
                   : 0;
    }
    snprintf(why, (size_t)whylen, "No such hole tool.");
    return -1;
}

int ge_drill(gs_mesh *part, int kind, const ge_holes *h, char *why, int whylen)
{
    gs_mesh tool, out;
    int rc;
    memset(&tool, 0, sizeof tool);
    memset(&out, 0, sizeof out);
    if (ge_hole_tool(&tool, kind, h, part, why, whylen)) {
        gs_mesh_free(&tool);
        return -1;
    }
    rc = gm_subtract(&out, part, &tool);
    gs_mesh_free(&tool);
    if (rc || !out.n) {
        gs_mesh_free(&out);
        snprintf(why, (size_t)whylen, "The holes did not cut.");
        return -1;
    }
    gs_mesh_free(part);
    *part = out;
    return 0;
}

/* The face's plane: its unit normal and how far along it; 0, or -1 for a
 * face with no area. */
static int plane_of(const gs_tri *t, gs_v *n, float *w)
{
    float ux = t->b.x - t->a.x, uy = t->b.y - t->a.y, uz = t->b.z - t->a.z;
    float vx = t->c.x - t->a.x, vy = t->c.y - t->a.y, vz = t->c.z - t->a.z;
    float l;
    n->x = uy * vz - uz * vy;
    n->y = uz * vx - ux * vz;
    n->z = ux * vy - uy * vx;
    l = sqrtf(n->x * n->x + n->y * n->y + n->z * n->z);
    if (l < 1e-6f) return -1;
    n->x /= l, n->y /= l, n->z /= l;
    *w = n->x * t->a.x + n->y * t->a.y + n->z * t->a.z;
    return 0;
}

/* Marks the faces in face's plane, as the bench's regionVerts. How many. */
static int region(const gs_mesh *m, int face, gs_v *n0, unsigned char *on)
{
    float w0, w;
    int i, k = 0;
    if (plane_of(&m->t[face], n0, &w0)) return 0;
    for (i = 0; i < m->n; i++) {
        const gs_tri *t = &m->t[i];
        gs_v n;
        on[i] = 0;
        if (plane_of(t, &n, &w) || n.x * n0->x + n.y * n0->y + n.z * n0->z < 0.992f) continue;
        if (fabsf(n0->x * t->a.x + n0->y * t->a.y + n0->z * t->a.z - w0) > 0.4f ||
            fabsf(n0->x * t->b.x + n0->y * t->b.y + n0->z * t->b.z - w0) > 0.4f ||
            fabsf(n0->x * t->c.x + n0->y * t->c.y + n0->z * t->c.z - w0) > 0.4f)
            continue;
        on[i] = 1;
        k++;
    }
    return k;
}

/* A face's k-th point (0 a, 1 b, 2 c). */
static gs_v *corner(gs_tri *t, int k) { return k == 0 ? &t->a : k == 1 ? &t->b : &t->c; }

static int same_point(const gs_v *a, const gs_v *b)
{
    return fabsf(a->x - b->x) < 1e-3f && fabsf(a->y - b->y) < 1e-3f && fabsf(a->z - b->z) < 1e-3f;
}

int ge_pull(gs_mesh *m, int face, float mm, char *line, int len)
{
    unsigned char *on, *go;
    gs_v n0;
    int i, j, f, g, moved = 0;
    if (!m || !m->t || face < 0 || face >= m->n) {
        snprintf(line, (size_t)len, "Click a face, then pull it by a distance.");
        return -1;
    }
    if (!(fabsf(mm) >= 0.05f)) {
        snprintf(line, (size_t)len, "Pull needs a distance of at least 0.1 mm.");
        return -1;
    }
    on = (unsigned char *)calloc((size_t)m->n, 1);
    go = (unsigned char *)calloc((size_t)m->n * 3, 1);
    if (!on || !go || !region(m, face, &n0, on)) {
        free(on);
        free(go);
        snprintf(line, (size_t)len, "That face has nothing to pull.");
        return -1;
    }
    /* every point on the face's: the face's own, and the walls' at them */
    for (i = 0; i < m->n; i++)
        for (j = 0; j < 3; j++) {
            const gs_v *p = corner(&m->t[i], j);
            for (f = 0; f < m->n && !go[i * 3 + j]; f++)
                for (g = 0; on[f] && g < 3; g++)
                    if (same_point(p, corner(&m->t[f], g))) {
                        go[i * 3 + j] = 1;
                        break;
                    }
            /* the face's points, each once */
            if (on[i] && go[i * 3 + j]) {
                int first = 1;
                for (f = 0; f <= i && first; f++)
                    for (g = 0; on[f] && g < (f == i ? j : 3); g++)
                        if (same_point(p, corner(&m->t[f], g))) first = 0;
                moved += first;
            }
        }
    {
        /* The face's box: the other faces' points in it on the plane (where
         * the walls meet its edges between its corners) go with it; pushed
         * in, the points it passes stay on the new plane, so no wall folds
         * over it. */
        gs_v lo = n0, hi = n0;
        float w0 = n0.x * m->t[face].a.x + n0.y * m->t[face].a.y + n0.z * m->t[face].a.z;
        int any = 0;
        for (i = 0; i < m->n * 3; i++)
            if (on[i / 3]) {
                const gs_v *p = corner(&m->t[i / 3], i % 3);
                if (!any++) lo = hi = *p;
                lo.x = fminf(lo.x, p->x), lo.y = fminf(lo.y, p->y), lo.z = fminf(lo.z, p->z);
                hi.x = fmaxf(hi.x, p->x), hi.y = fmaxf(hi.y, p->y), hi.z = fmaxf(hi.z, p->z);
            }
        for (i = 0; i < m->n * 3; i++) {
            gs_v *p = corner(&m->t[i / 3], i % 3);
            float h = n0.x * p->x + n0.y * p->y + n0.z * p->z - w0, step;
            float fx = p->x - n0.x * h, fy = p->y - n0.y * h, fz = p->z - n0.z * h;
            if (go[i]) step = mm;
            else if (fx < lo.x - 1e-3f || fy < lo.y - 1e-3f || fz < lo.z - 1e-3f || fx > hi.x + 1e-3f ||
                     fy > hi.y + 1e-3f || fz > hi.z + 1e-3f)
                continue;
            else if (fabsf(h) < 1e-3f) step = mm;
            else if (mm < 0.f && h > mm && h < 0.f) step = mm - h;
            else continue;
            p->x += n0.x * step, p->y += n0.y * step, p->z += n0.z * step;
        }
    }
    free(on);
    free(go);
    bound(m);
    snprintf(line, (size_t)len, "Pulled %.1f mm. %d points moved on that face.", (double)mm, moved);
    return moved;
}

/* Where a to b crosses height cut (ha, hb theirs): the same point whichever
 * way round, so the faces either side of an edge split it alike. */
static gs_v cross_at(gs_v a, float ha, gs_v b, float hb, float cut)
{
    gs_v r;
    float t;
    if (a.x > b.x || (a.x == b.x && (a.y > b.y || (a.y == b.y && a.z > b.z)))) {
        gs_v v = a;
        float h = ha;
        a = b, b = v, ha = hb, hb = h;
    }
    t = (cut - ha) / (hb - ha);
    r.x = a.x + (b.x - a.x) * t, r.y = a.y + (b.y - a.y) * t, r.z = a.z + (b.z - a.z) * t;
    return r;
}

/* m's faces across the plane at height cut along n (w the face's) split
 * there, each into three, no edge split on one side only. 0, or -1. */
static int split_at(gs_mesh *m, const gs_v *n, float w, float cut)
{
    gs_tri *out = (gs_tri *)malloc(sizeof(gs_tri) * (size_t)m->n * 3);
    int i, k, o = 0;
    if (!out) return -1;
    for (i = 0; i < m->n; i++) {
        gs_v v[3], up[4], down[4];
        float h[3];
        int s[3], nu = 0, nd = 0, f;
        for (k = 0; k < 3; k++) {
            v[k] = *corner(&m->t[i], k);
            h[k] = n->x * v[k].x + n->y * v[k].y + n->z * v[k].z - w;
            s[k] = h[k] > cut + 1e-4f ? 1 : h[k] < cut - 1e-4f ? -1 : 0;
        }
        if ((s[0] >= 0 && s[1] >= 0 && s[2] >= 0) || (s[0] <= 0 && s[1] <= 0 && s[2] <= 0)) {
            out[o++] = m->t[i];
            continue;
        }
        for (k = 0; k < 3; k++) {
            int j = (k + 1) % 3;
            if (s[k] >= 0) up[nu++] = v[k];
            if (s[k] <= 0) down[nd++] = v[k];
            if (s[k] * s[j] < 0) up[nu++] = down[nd++] = cross_at(v[k], h[k], v[j], h[j], cut);
        }
        for (f = 1; f + 1 < nu; f++) out[o].a = up[0], out[o].b = up[f], out[o++].c = up[f + 1];
        for (f = 1; f + 1 < nd; f++) out[o].a = down[0], out[o].b = down[f], out[o++].c = down[f + 1];
    }
    free(m->t);
    m->t = out;
    m->n = o;
    m->cap = (int)(m->n > 0 ? m->n : 1);
    return 0;
}

int ge_chamfer(gs_mesh *m, int face, float mm, char *line, int len)
{
    unsigned char *on = NULL;
    gs_v *at = NULL, *wa = NULL, *wb = NULL, *go = NULL, n0, lo, hi;
    int *cnt = NULL, i, j, k, nr = 0, edges = 0;
    float d = fabsf(mm) > 0.f ? fabsf(mm) : 1.f, deep_lo = 0.f, deep_hi = 0.f, w0, least = -1.f;
    if (!m || !m->t || face < 0 || face >= m->n) {
        snprintf(line, (size_t)len, "Click a face, then chamfer it.");
        return -1;
    }
    d = d < 0.2f ? 0.2f : d > 20.f ? 20.f : d;
    on = (unsigned char *)calloc((size_t)m->n, 1);
    at = (gs_v *)malloc(sizeof(gs_v) * (size_t)m->n * 3);
    wa = (gs_v *)calloc((size_t)m->n * 3, sizeof(gs_v));
    wb = (gs_v *)calloc((size_t)m->n * 3, sizeof(gs_v));
    go = (gs_v *)calloc((size_t)m->n * 3, sizeof(gs_v));
    cnt = (int *)calloc((size_t)m->n * 3, sizeof(int));
    if (!on || !at || !wa || !wb || !go || !cnt) {
        snprintf(line, (size_t)len, "Not enough memory to chamfer.");
        goto fail;
    }
    if (!region(m, face, &n0, on)) {
        snprintf(line, (size_t)len, "That face has nothing to chamfer.");
        goto fail;
    }
    w0 = n0.x * m->t[face].a.x + n0.y * m->t[face].a.y + n0.z * m->t[face].a.z;
    lo = hi = m->t[face].a;
    for (i = 0; i < m->n * 3; i++) {
        const gs_v *p = corner(&m->t[i / 3], i % 3);
        float h = n0.x * p->x + n0.y * p->y + n0.z * p->z - w0;
        deep_lo = fminf(deep_lo, h), deep_hi = fmaxf(deep_hi, h);
        if (!on[i / 3]) continue;
        at[nr++] = *p; /* the face's points */
        lo.x = fminf(lo.x, p->x), lo.y = fminf(lo.y, p->y), lo.z = fminf(lo.z, p->z);
        hi.x = fmaxf(hi.x, p->x), hi.y = fmaxf(hi.y, p->y), hi.z = fmaxf(hi.z, p->z);
    }
    /* each face point's walls: their ways out, along the face, the two
     * most apart kept */
    for (i = 0; i < m->n; i++) {
        gs_v n;
        float w, a, l;
        if (on[i] || plane_of(&m->t[i], &n, &w)) continue;
        a = n.x * n0.x + n.y * n0.y + n.z * n0.z;
        n.x -= n0.x * a, n.y -= n0.y * a, n.z -= n0.z * a;
        l = sqrtf(n.x * n.x + n.y * n.y + n.z * n.z);
        if (l < 0.1f) continue; /* along the face: not a wall */
        n.x /= l, n.y /= l, n.z /= l;
        for (j = 0; j < 3; j++)
            for (k = 0; k < nr; k++) {
                float c;
                if (!same_point(corner(&m->t[i], j), &at[k])) continue;
                c = n.x * wa[k].x + n.y * wa[k].y + n.z * wa[k].z;
                if (!cnt[k]) wa[k] = n, cnt[k] = 1, edges++;
                else if (c > 0.999f) continue;
                else if (cnt[k] == 1 || c < wa[k].x * wb[k].x + wa[k].y * wb[k].y + wa[k].z * wb[k].z) wb[k] = n, cnt[k] = 2;
            }
    }
    if (!edges) {
        snprintf(line, (size_t)len, "That face has no edge to chamfer.");
        goto fail;
    }
    if (hi.x - lo.x > 1e-3f) least = hi.x - lo.x;
    if (hi.y - lo.y > 1e-3f && (least < 0.f || hi.y - lo.y < least)) least = hi.y - lo.y;
    if (hi.z - lo.z > 1e-3f && (least < 0.f || hi.z - lo.z < least)) least = hi.z - lo.z;
    if (d >= deep_hi - deep_lo || (least > 0.f && 2.f * d >= least)) {
        snprintf(line, (size_t)len, "A %.1f mm chamfer is too big for that face.", (double)d);
        goto fail;
    }
    /* each edge point in from its walls by d: from one wall, along it; from
     * two, d from each */
    for (k = 0; k < nr; k++) {
        float c = wa[k].x * wb[k].x + wa[k].y * wb[k].y + wa[k].z * wb[k].z;
        if (cnt[k] == 1 || (cnt[k] == 2 && c < -0.9f))
            go[k].x = -d * wa[k].x, go[k].y = -d * wa[k].y, go[k].z = -d * wa[k].z;
        else if (cnt[k] == 2) {
            float f = -d / (1.f + c);
            go[k].x = f * (wa[k].x + wb[k].x), go[k].y = f * (wa[k].y + wb[k].y), go[k].z = f * (wa[k].z + wb[k].z);
        }
    }
    /* the walls cut at d down, then the edge points moved in: the bevel */
    if (split_at(m, &n0, w0, -d)) {
        snprintf(line, (size_t)len, "Not enough memory to chamfer.");
        goto fail;
    }
    for (i = 0; i < m->n * 3; i++) {
        gs_v *p = corner(&m->t[i / 3], i % 3);
        for (k = 0; k < nr; k++)
            if (cnt[k] && same_point(p, &at[k])) {
                p->x += go[k].x, p->y += go[k].y, p->z += go[k].z;
                break;
            }
    }
    free(on), free(at), free(wa), free(wb), free(go), free(cnt);
    bound(m);
    snprintf(line, (size_t)len, "Chamfered that face's edges by %.1f mm.", (double)d);
    return 0;
fail:
    free(on), free(at), free(wa), free(wb), free(go), free(cnt);
    return -1;
}
