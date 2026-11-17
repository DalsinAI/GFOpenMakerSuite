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
