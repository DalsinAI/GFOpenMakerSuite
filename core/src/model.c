/* A box, a cylinder, and the three boolean joins: union, cut, overlap.
 * Each boolean is a BSP: a polygon is split by a plane, the pieces that
 * fall on the wrong side are dropped, and the two surfaces are joined.
 * That is the public method for mesh booleans. No other project's source
 * is in here.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gs_model.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define VMAX 16
#define EPS 1e-5f
#define COPLANAR 0.99f /* normals this close are one plane */

typedef struct {
    float x, y, z;
} mv;

/* A polygon keeps the plane it was made on. Pieces of a split take
 * their parent's plane rather than working one out from their own
 * corners, which for a long sliver is far off. */
typedef struct {
    mv v[VMAX];
    int n;
    mv nrm;
    float w;
} poly;

typedef struct {
    poly *p;
    int n, cap;
} bag;

typedef struct node {
    mv nrm;
    float w;
    int has;
    bag co;
    struct node *front, *back;
} node;

static mv cross(mv a, mv b)
{
    mv r;
    r.x = a.y * b.z - a.z * b.y;
    r.y = a.z * b.x - a.x * b.z;
    r.z = a.x * b.y - a.y * b.x;
    return r;
}

static float dot(mv a, mv b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static int unit(mv *n)
{
    float L = sqrtf(dot(*n, *n));
    if (L < 1e-12f) return 0;
    n->x /= L;
    n->y /= L;
    n->z /= L;
    return 1;
}

static void bag_free(bag *b)
{
    free(b->p);
    b->p = NULL;
    b->n = b->cap = 0;
}

static int bag_add(bag *b, const poly *p)
{
    if (p->n < 3 || p->n > VMAX) return 0;
    if (b->n >= b->cap) {
        int c = b->cap ? b->cap * 2 : 32;
        poly *np = (poly *)realloc(b->p, (size_t)c * sizeof *np);
        if (!np) return -1;
        b->p = np;
        b->cap = c;
    }
    b->p[b->n++] = *p;
    return 0;
}

/* A split can leave a piece with more corners than a poly holds; it is
 * convex, so it is cut in two along a diagonal from its first corner.
 * Both halves keep that diagonal's exact points, so no crack opens. */
static int bag_add_long(bag *b, const mv *v, int n, const poly *parent)
{
    poly p;
    int i;
    if (n < 3) return 0;
    p.nrm = parent->nrm;
    p.w = parent->w;
    if (n <= VMAX) {
        p.n = n;
        for (i = 0; i < n; i++) p.v[i] = v[i];
        return bag_add(b, &p);
    }
    p.n = VMAX;
    for (i = 0; i < VMAX; i++) p.v[i] = v[i];
    if (bag_add(b, &p)) return -1;
    {
        mv rest[VMAX + 2];
        int k = 0;
        rest[k++] = v[0];
        for (i = VMAX - 1; i < n; i++) rest[k++] = v[i];
        return bag_add_long(b, rest, k, parent);
    }
}

/* Newell's normal: every edge counts, so a sliver whose first three points
 * are nearly in a line still gets the plane of all its points. */
static int plane_of(const poly *p, mv *nrm, float *w)
{
    mv n = {0.f, 0.f, 0.f}, c = {0.f, 0.f, 0.f};
    int i;
    if (p->n < 3) return 0;
    for (i = 0; i < p->n; i++) {
        mv a = p->v[i], b = p->v[(i + 1) % p->n];
        n.x += (a.y - b.y) * (a.z + b.z);
        n.y += (a.z - b.z) * (a.x + b.x);
        n.z += (a.x - b.x) * (a.y + b.y);
        c.x += a.x;
        c.y += a.y;
        c.z += a.z;
    }
    if (!unit(&n)) return 0;
    *nrm = n;
    *w = (n.x * c.x + n.y * c.y + n.z * c.z) / (float)p->n;
    return 1;
}

static float side(const node *n, mv v)
{
    return n->nrm.x * v.x + n->nrm.y * v.y + n->nrm.z * v.z - n->w;
}

static int less(mv a, mv b)
{
    if (a.x != b.x) return a.x < b.x;
    if (a.y != b.y) return a.y < b.y;
    return a.z < b.z;
}

static void flip_poly(poly *p)
{
    int i, h = p->n / 2;
    p->nrm.x = -p->nrm.x;
    p->nrm.y = -p->nrm.y;
    p->nrm.z = -p->nrm.z;
    p->w = -p->w;
    for (i = 0; i < h; i++) {
        mv t = p->v[i];
        p->v[i] = p->v[p->n - 1 - i];
        p->v[p->n - 1 - i] = t;
    }
}

/* Front keeps what is outside the plane, back what is inside. */
static int split_poly(const node *nd, const poly *in, bag *front, bag *back)
{
    int i, nf = 0, nb = 0;
    float d[VMAX];
    int c[VMAX];
    mv f[VMAX + 2], b[VMAX + 2];
    for (i = 0; i < in->n; i++) {
        d[i] = side(nd, in->v[i]);
        if (d[i] > EPS) {
            c[i] = 1;
            nf++;
        } else if (d[i] < -EPS) {
            c[i] = -1;
            nb++;
        } else {
            c[i] = 0;
        }
    }
    if (!nf && !nb) {
        float k = dot(in->nrm, nd->nrm);
        /* A sliver can lie within EPS of a plane it crosses at an angle:
         * it is not on that plane, so its middle says which side it is. */
        if (k < COPLANAR && k > -COPLANAR) {
            float sum = 0.f;
            for (i = 0; i < in->n; i++) sum += d[i];
            return bag_add(sum > 0.f ? front : back, in);
        }
        if (k > 0.f) return bag_add(front, in);
        return bag_add(back, in);
    }
    if (!nb) return bag_add(front, in);
    if (!nf) return bag_add(back, in);
    nf = nb = 0;
    for (i = 0; i < in->n; i++) {
        int j = (i + 1) % in->n;
        if (c[i] != -1) f[nf++] = in->v[i];
        if (c[i] != 1) b[nb++] = in->v[i];
        if ((c[i] == 1 && c[j] == -1) || (c[i] == -1 && c[j] == 1)) {
            /* From the lower corner to the higher, whichever way this
             * polygon goes round: the neighbour across the edge goes the
             * other way and must land on the same bits. When the plane
             * runs nearly along the edge, t is touchy and any other
             * order would open a crack. */
            int lo = less(in->v[i], in->v[j]) ? i : j, hi = lo == i ? j : i;
            float t = d[lo] / (d[lo] - d[hi]);
            mv p;
            p.x = in->v[lo].x + (in->v[hi].x - in->v[lo].x) * t;
            p.y = in->v[lo].y + (in->v[hi].y - in->v[lo].y) * t;
            p.z = in->v[lo].z + (in->v[hi].z - in->v[lo].z) * t;
            f[nf++] = p;
            b[nb++] = p;
        }
    }
    if (bag_add_long(front, f, nf, in) || bag_add_long(back, b, nb, in)) return -1;
    return 0;
}

static void node_free(node *n)
{
    if (!n) return;
    node_free(n->front);
    node_free(n->back);
    bag_free(&n->co);
    free(n);
}

/* The plane to split on: of a few polygons spread through the bag, the
 * one whose plane crosses the fewest others. Taking the first, as before,
 * let a holed part's hole walls cut its big faces into thousands of
 * slivers, and the cut after it then left cracks. */
#define PICKS 12
static int pick_plane(const bag *src)
{
    int c, best = 0, least = -1, step = src->n / PICKS;
    if (step < 1) step = 1;
    for (c = 0; c < src->n && c / step < PICKS; c += step) {
        node t;
        int i, vi, cross = 0;
        t.nrm = src->p[c].nrm;
        t.w = src->p[c].w;
        for (i = 0; i < src->n && (least < 0 || cross < least); i++) {
            int out = 0, inn = 0;
            for (vi = 0; vi < src->p[i].n; vi++) {
                float d = side(&t, src->p[i].v[vi]);
                if (d > EPS) out = 1;
                else if (d < -EPS) inn = 1;
            }
            cross += out && inn;
        }
        if (least < 0 || cross < least) least = cross, best = c;
        if (!least) break;
    }
    return best;
}

static int node_build(node *n, const bag *src)
{
    bag front, back;
    int i, made = 0, pick = 0;
    memset(&front, 0, sizeof front);
    memset(&back, 0, sizeof back);
    if (!src->n) return 0;
    if (!n->has) {
        pick = pick_plane(src);
        n->nrm = src->p[pick].nrm;
        n->w = src->p[pick].w;
        n->has = 1;
        made = 1;
    }
    for (i = 0; i < src->n; i++) {
        const poly *p = &src->p[i];
        int vi, on = 1, out = 0, inn = 0;
        /* The polygon that gave this plane lies on it. Splitting it again
         * would hand the same polygon down for ever. */
        if (made && i == pick) {
            if (bag_add(&n->co, p)) goto fail;
            continue;
        }
        float d;
        for (vi = 0; vi < p->n; vi++) {
            d = side(n, p->v[vi]);
            if (d > EPS) {
                out = 1;
                on = 0;
            } else if (d < -EPS) {
                inn = 1;
                on = 0;
            }
        }
        if (on) {
            float k = dot(p->nrm, n->nrm), sum = 0.f;
            if (k >= COPLANAR || k <= -COPLANAR) {
                if (bag_add(&n->co, p)) goto fail;
                continue;
            }
            for (vi = 0; vi < p->n; vi++) sum += side(n, p->v[vi]);
            if (bag_add(sum > 0.f ? &front : &back, p)) goto fail;
            continue;
        }
        if (out && !inn) {
            if (bag_add(&front, p)) goto fail;
            continue;
        }
        if (inn && !out) {
            if (bag_add(&back, p)) goto fail;
            continue;
        }
        if (split_poly(n, p, &front, &back)) goto fail;
    }
    if (front.n) {
        if (!n->front) {
            n->front = (node *)calloc(1, sizeof *n->front);
            if (!n->front) goto fail;
        }
        if (node_build(n->front, &front)) goto fail;
    }
    if (back.n) {
        if (!n->back) {
            n->back = (node *)calloc(1, sizeof *n->back);
            if (!n->back) goto fail;
        }
        if (node_build(n->back, &back)) goto fail;
    }
    bag_free(&front);
    bag_free(&back);
    return 0;
fail:
    bag_free(&front);
    bag_free(&back);
    return -1;
}

static int node_clip(const node *n, const bag *src, bag *out)
{
    bag front, back;
    int i;
    memset(&front, 0, sizeof front);
    memset(&back, 0, sizeof back);
    if (!n || !n->has) {
        for (i = 0; i < src->n; i++)
            if (bag_add(out, &src->p[i])) return -1;
        return 0;
    }
    for (i = 0; i < src->n; i++) {
        if (split_poly(n, &src->p[i], &front, &back)) {
            bag_free(&front);
            bag_free(&back);
            return -1;
        }
    }
    if (n->front) {
        if (node_clip(n->front, &front, out)) {
            bag_free(&front);
            bag_free(&back);
            return -1;
        }
    } else {
        for (i = 0; i < front.n; i++)
            if (bag_add(out, &front.p[i])) {
                bag_free(&front);
                bag_free(&back);
                return -1;
            }
    }
    if (n->back) {
        if (node_clip(n->back, &back, out)) {
            bag_free(&front);
            bag_free(&back);
            return -1;
        }
    }
    bag_free(&front);
    bag_free(&back);
    return 0;
}

static int node_clip_to(node *n, const node *bsp)
{
    bag next;
    int i;
    if (!n) return 0;
    memset(&next, 0, sizeof next);
    if (node_clip(bsp, &n->co, &next)) return -1;
    bag_free(&n->co);
    n->co = next;
    if (node_clip_to(n->front, bsp)) return -1;
    if (node_clip_to(n->back, bsp)) return -1;
    (void)i;
    return 0;
}

static void node_invert(node *n)
{
    node *t;
    int i;
    if (!n) return;
    for (i = 0; i < n->co.n; i++) flip_poly(&n->co.p[i]);
    if (n->has) {
        n->nrm.x = -n->nrm.x;
        n->nrm.y = -n->nrm.y;
        n->nrm.z = -n->nrm.z;
        n->w = -n->w;
    }
    t = n->front;
    n->front = n->back;
    n->back = t;
    node_invert(n->front);
    node_invert(n->back);
}

static int node_all(const node *n, bag *out)
{
    int i;
    if (!n) return 0;
    for (i = 0; i < n->co.n; i++)
        if (bag_add(out, &n->co.p[i])) return -1;
    if (node_all(n->front, out)) return -1;
    if (node_all(n->back, out)) return -1;
    return 0;
}

static int mesh_from(gs_mesh *m, const bag *b)
{
    int i, k, count = 0;
    for (i = 0; i < b->n; i++)
        if (b->p[i].n >= 3) count += b->p[i].n - 2;
    m->t = count ? (gs_tri *)malloc((size_t)count * sizeof *m->t) : NULL;
    if (count && !m->t) return -1;
    m->n = 0;
    m->cap = count;
    m->min.x = m->min.y = m->min.z = 1e30f;
    m->max.x = m->max.y = m->max.z = -1e30f;
    for (i = 0; i < b->n; i++) {
        const poly *p = &b->p[i];
        for (k = 1; k + 1 < p->n; k++) {
            gs_tri t;
            int e;
            mv vs[3];
            t.a.x = p->v[0].x;
            t.a.y = p->v[0].y;
            t.a.z = p->v[0].z;
            t.b.x = p->v[k].x;
            t.b.y = p->v[k].y;
            t.b.z = p->v[k].z;
            t.c.x = p->v[k + 1].x;
            t.c.y = p->v[k + 1].y;
            t.c.z = p->v[k + 1].z;
            m->t[m->n++] = t;
            vs[0] = p->v[0];
            vs[1] = p->v[k];
            vs[2] = p->v[k + 1];
            for (e = 0; e < 3; e++) {
                if (vs[e].x < m->min.x) m->min.x = vs[e].x;
                if (vs[e].y < m->min.y) m->min.y = vs[e].y;
                if (vs[e].z < m->min.z) m->min.z = vs[e].z;
                if (vs[e].x > m->max.x) m->max.x = vs[e].x;
                if (vs[e].y > m->max.y) m->max.y = vs[e].y;
                if (vs[e].z > m->max.z) m->max.z = vs[e].z;
            }
        }
    }
    return 0;
}

static int tri(gs_mesh *m, float ax, float ay, float az, float bx, float by, float bz, float cx, float cy, float cz)
{
    gs_tri t;
    gs_v vs[3];
    int i;
    if (m->n >= m->cap) {
        int c = m->cap ? m->cap * 2 : 16;
        gs_tri *nt = (gs_tri *)realloc(m->t, (size_t)c * sizeof *nt);
        if (!nt) return -1;
        m->t = nt;
        m->cap = c;
    }
    t.a.x = ax;
    t.a.y = ay;
    t.a.z = az;
    t.b.x = bx;
    t.b.y = by;
    t.b.z = bz;
    t.c.x = cx;
    t.c.y = cy;
    t.c.z = cz;
    m->t[m->n++] = t;
    vs[0] = t.a;
    vs[1] = t.b;
    vs[2] = t.c;
    for (i = 0; i < 3; i++) {
        if (vs[i].x < m->min.x) m->min.x = vs[i].x;
        if (vs[i].y < m->min.y) m->min.y = vs[i].y;
        if (vs[i].z < m->min.z) m->min.z = vs[i].z;
        if (vs[i].x > m->max.x) m->max.x = vs[i].x;
        if (vs[i].y > m->max.y) m->max.y = vs[i].y;
        if (vs[i].z > m->max.z) m->max.z = vs[i].z;
    }
    return 0;
}

static void mesh_clear(gs_mesh *m)
{
    free(m->t);
    memset(m, 0, sizeof *m);
    m->min.x = m->min.y = m->min.z = 1e30f;
    m->max.x = m->max.y = m->max.z = -1e30f;
}

int gm_cube(gs_mesh *m, float x, float y, float z, float sx, float sy, float sz)
{
    float x1 = x + sx, y1 = y + sy, z1 = z + sz;
    mesh_clear(m);
    /* Outward faces, counter-clockwise from the outside. */
    if (tri(m, x, y, z1, x1, y, z1, x1, y1, z1) || tri(m, x, y, z1, x1, y1, z1, x, y1, z1)) return -1;
    if (tri(m, x, y, z, x, y1, z, x1, y1, z) || tri(m, x, y, z, x1, y1, z, x1, y, z)) return -1;
    if (tri(m, x, y, z, x, y, z1, x, y1, z1) || tri(m, x, y, z, x, y1, z1, x, y1, z)) return -1;
    if (tri(m, x1, y, z, x1, y1, z, x1, y1, z1) || tri(m, x1, y, z, x1, y1, z1, x1, y, z1)) return -1;
    if (tri(m, x, y, z, x1, y, z, x1, y, z1) || tri(m, x, y, z, x1, y, z1, x, y, z1)) return -1;
    if (tri(m, x, y1, z, x, y1, z1, x1, y1, z1) || tri(m, x, y1, z, x1, y1, z1, x1, y1, z)) return -1;
    return 0;
}

/* An upright round added to m: the cylinder, and each hole tool's drill. */
static int rod(gs_mesh *m, float x, float y, float z, float r, float h, int seg)
{
    int i;
    float z1 = z + h;
    for (i = 0; i < seg; i++) {
        float a0 = (float)i / (float)seg * 6.2831853f;
        float a1 = (float)(i + 1) / (float)seg * 6.2831853f;
        float c0 = cosf(a0), s0 = sinf(a0), c1 = cosf(a1), s1 = sinf(a1);
        float x0 = x + r * c0, y0 = y + r * s0, x2 = x + r * c1, y2 = y + r * s1;
        if (tri(m, x0, y0, z, x2, y2, z, x2, y2, z1) || tri(m, x0, y0, z, x2, y2, z1, x0, y0, z1)) return -1;
        if (tri(m, x, y, z1, x0, y0, z1, x2, y2, z1)) return -1;
        if (tri(m, x, y, z, x2, y2, z, x0, y0, z)) return -1;
    }
    return 0;
}

int gm_cylinder(gs_mesh *m, float x, float y, float z, float r, float h, int seg)
{
    if (seg < 3) seg = 3;
    mesh_clear(m);
    return rod(m, x, y, z, r, h, seg);
}

int gm_facts(const gs_mesh *m, gs_facts *f)
{
    double v = 0.0, a = 0.0;
    int i;
    if (!m || !f) return -1;
    memset(f, 0, sizeof *f);
    if (!m->t || m->n <= 0) return 0;
    /* About the box's corner, so a part far out on the plate keeps its
     * digits; double sums, the same with or without an FPU. */
    for (i = 0; i < m->n; i++) {
        double ax = m->t[i].a.x - m->min.x, ay = m->t[i].a.y - m->min.y, az = m->t[i].a.z - m->min.z;
        double bx = m->t[i].b.x - m->min.x, by = m->t[i].b.y - m->min.y, bz = m->t[i].b.z - m->min.z;
        double cx = m->t[i].c.x - m->min.x, cy = m->t[i].c.y - m->min.y, cz = m->t[i].c.z - m->min.z;
        double ux = bx - ax, uy = by - ay, uz = bz - az, wx = cx - ax, wy = cy - ay, wz = cz - az;
        double nx = uy * wz - uz * wy, ny = uz * wx - ux * wz, nz = ux * wy - uy * wx;
        v += ax * (by * cz - bz * cy) - ay * (bx * cz - bz * cx) + az * (bx * cy - by * cx);
        a += sqrtf((float)(nx * nx + ny * ny + nz * nz));
    }
    f->volume = (float)(v / 6.0);
    f->area = (float)(a / 2.0);
    f->sx = m->max.x - m->min.x;
    f->sy = m->max.y - m->min.y;
    f->sz = m->max.z - m->min.z;
    f->tris = m->n;
    return 0;
}

/* The hole tools' drills have the bench's 28 sides, a counterbore 32. */
#define DRILL_SIDES 28
#define BORE_SIDES 32

int gm_bolt_circle(gs_mesh *m, float x, float y, float z, float h, float d, float pcd, int n)
{
    int i;
    if (!(d > 0.f) || !(h > 0.f) || !(pcd > 0.f) || n < 1) return -1;
    /* Neighbours a chord apart, so they must not meet. */
    if (n > 1 && !(pcd * sinf(3.14159265f / (float)n) > d)) return -1;
    mesh_clear(m);
    for (i = 0; i < n; i++) {
        float a = (float)i / (float)n * 6.2831853f;
        if (rod(m, x + pcd * 0.5f * cosf(a), y + pcd * 0.5f * sinf(a), z, d * 0.5f, h, DRILL_SIDES)) return -1;
    }
    return 0;
}

int gm_hole_row(gs_mesh *m, float x, float y, float z, float h, float d, float pitch, int n)
{
    int i;
    if (!(d > 0.f) || !(h > 0.f) || n < 1 || (n > 1 && !(pitch > d))) return -1;
    mesh_clear(m);
    for (i = 0; i < n; i++)
        if (rod(m, x + ((float)i - (float)(n - 1) * 0.5f) * pitch, y, z, d * 0.5f, h, DRILL_SIDES)) return -1;
    return 0;
}

/* One closed solid, turned: the drill r up to the step, the step facing
 * down, the bore R to the top. */
int gm_counterbore(gs_mesh *m, float x, float y, float z, float h, float d, float cb, float deep)
{
    float r = d * 0.5f, R = cb * 0.5f, zs = z + h - deep, z1 = z + h;
    int i;
    if (!(d > 0.f) || !(h > 0.f) || !(cb > d) || !(deep > 0.f) || !(deep < h)) return -1;
    mesh_clear(m);
    for (i = 0; i < BORE_SIDES; i++) {
        float a0 = (float)i / (float)BORE_SIDES * 6.2831853f;
        float a1 = (float)(i + 1) / (float)BORE_SIDES * 6.2831853f;
        float c0 = cosf(a0), s0 = sinf(a0), c1 = cosf(a1), s1 = sinf(a1);
        float ix0 = x + r * c0, iy0 = y + r * s0, ix1 = x + r * c1, iy1 = y + r * s1;
        float ox0 = x + R * c0, oy0 = y + R * s0, ox1 = x + R * c1, oy1 = y + R * s1;
        if (tri(m, x, y, z, ix1, iy1, z, ix0, iy0, z)) return -1;
        if (tri(m, ix0, iy0, z, ix1, iy1, z, ix1, iy1, zs) || tri(m, ix0, iy0, z, ix1, iy1, zs, ix0, iy0, zs)) return -1;
        if (tri(m, ix0, iy0, zs, ix1, iy1, zs, ox1, oy1, zs) || tri(m, ix0, iy0, zs, ox1, oy1, zs, ox0, oy0, zs))
            return -1;
        if (tri(m, ox0, oy0, zs, ox1, oy1, zs, ox1, oy1, z1) || tri(m, ox0, oy0, zs, ox1, oy1, z1, ox0, oy0, z1))
            return -1;
        if (tri(m, x, y, z1, ox0, oy0, z1, ox1, oy1, z1)) return -1;
    }
    return 0;
}

/* The bench's other solids (cad/bench/modeller/solids.ts), written again
 * here: outward faces, counter-clockwise from the outside, and each
 * corner worked out the same way every time it is used, so neighbouring
 * triangles share it exactly and the solid is closed. */

static float turn(int i, int n) { return (float)(i % n) / (float)n * 6.2831853f; }

int gm_sphere(gs_mesh *m, float x, float y, float z, float r, int seg, int rings)
{
    int i, j;
    if (!(r > 0.f)) return -1;
    if (seg < 3) seg = 3;
    if (rings < 2) rings = 2;
    mesh_clear(m);
    for (i = 0; i < rings; i++) {
        /* From the top pole down: the poles exact, so the fans close. */
        float v0 = (float)i / (float)rings * 3.14159265f, v1 = (float)(i + 1) / (float)rings * 3.14159265f;
        float s0 = i == 0 ? 0.f : sinf(v0), c0 = i == 0 ? 1.f : cosf(v0);
        float s1 = i + 1 == rings ? 0.f : sinf(v1), c1 = i + 1 == rings ? -1.f : cosf(v1);
        for (j = 0; j < seg; j++) {
            float cu0 = cosf(turn(j, seg)), su0 = sinf(turn(j, seg));
            float cu1 = cosf(turn(j + 1, seg)), su1 = sinf(turn(j + 1, seg));
            float ax = x + r * s0 * cu0, ay = y + r * s0 * su0, az = z + r * c0;
            float bx = x + r * s0 * cu1, by = y + r * s0 * su1;
            float cx = x + r * s1 * cu0, cy = y + r * s1 * su0, cz = z + r * c1;
            float dx = x + r * s1 * cu1, dy = y + r * s1 * su1;
            if (i + 1 < rings && tri(m, ax, ay, az, cx, cy, cz, dx, dy, cz)) return -1;
            if (i > 0 && tri(m, ax, ay, az, dx, dy, cz, bx, by, az)) return -1;
        }
    }
    return 0;
}

int gm_cone(gs_mesh *m, float x, float y, float z, float r, float h, int seg)
{
    int i;
    if (!(r > 0.f) || !(h > 0.f)) return -1;
    if (seg < 3) seg = 3;
    mesh_clear(m);
    for (i = 0; i < seg; i++) {
        float x0 = x + r * cosf(turn(i, seg)), y0 = y + r * sinf(turn(i, seg));
        float x1 = x + r * cosf(turn(i + 1, seg)), y1 = y + r * sinf(turn(i + 1, seg));
        if (tri(m, x0, y0, z, x1, y1, z, x, y, z + h) || tri(m, x, y, z, x1, y1, z, x0, y0, z)) return -1;
    }
    return 0;
}

int gm_torus(gs_mesh *m, float x, float y, float z, float ring, float tube, int seg, int tseg)
{
    int i, j;
    if (!(tube > 0.f) || !(ring > tube)) return -1;
    if (seg < 3) seg = 3;
    if (tseg < 3) tseg = 3;
    mesh_clear(m);
    for (i = 0; i < seg; i++)
        for (j = 0; j < tseg; j++) {
            float p[4][3];
            int k;
            for (k = 0; k < 4; k++) {
                float u = turn(i + (k & 1), seg), v = turn(j + (k >> 1), tseg);
                float rr = ring + tube * cosf(v);
                p[k][0] = x + rr * cosf(u);
                p[k][1] = y + rr * sinf(u);
                p[k][2] = z + tube * sinf(v);
            }
            /* a = p[0], b = p[1] (round the ring), c = p[2] (round the tube), d = p[3] */
            if (tri(m, p[0][0], p[0][1], p[0][2], p[1][0], p[1][1], p[1][2], p[3][0], p[3][1], p[3][2]) ||
                tri(m, p[0][0], p[0][1], p[0][2], p[3][0], p[3][1], p[3][2], p[2][0], p[2][1], p[2][2]))
                return -1;
        }
    return 0;
}

int gm_wedge(gs_mesh *m, float x, float y, float z, float sx, float sy, float sz)
{
    float x1 = x + sx, y1 = y + sy, z1 = z + sz;
    if (!(sx > 0.f) || !(sy > 0.f) || !(sz > 0.f)) return -1;
    mesh_clear(m);
    /* The base, the upright end at x, the two sides, and the slope from
     * the top of that end down to the base at x1. */
    if (tri(m, x, y, z, x, y1, z, x1, y1, z) || tri(m, x, y, z, x1, y1, z, x1, y, z)) return -1;
    if (tri(m, x, y, z, x, y, z1, x, y1, z1) || tri(m, x, y, z, x, y1, z1, x, y1, z)) return -1;
    if (tri(m, x, y, z, x1, y, z, x, y, z1) || tri(m, x, y1, z, x, y1, z1, x1, y1, z)) return -1;
    if (tri(m, x, y, z1, x1, y, z, x1, y1, z) || tri(m, x, y, z1, x1, y1, z, x, y1, z1)) return -1;
    return 0;
}

int gm_slot(gs_mesh *m, float x, float y, float z, float sx, float sy, float sz, int seg)
{
    float px[130], py[130], cx = x + sx * 0.5f, cy = y + sy * 0.5f, r, straight, a0;
    float e1x, e1y, e2x, e2y;
    int along_x = sx >= sy, half, n = 0, k, end, i;
    if (!(sx > 0.f) || !(sy > 0.f) || !(sz > 0.f)) return -1;
    if (seg < 4) seg = 4;
    if (seg > 128) seg = 128;
    half = seg / 2;
    r = (along_x ? sy : sx) * 0.5f;
    straight = (along_x ? sx : sy) - 2.f * r;
    /* The two round ends' centres; the outline counter-clockwise from the
     * far end's right-hand side. */
    if (along_x) {
        e1x = x + sx - r, e1y = cy, e2x = x + r, e2y = cy, a0 = -1.5707963f;
    } else {
        e1x = cx, e1y = y + sy - r, e2x = cx, e2y = y + r, a0 = 0.f;
    }
    end = straight > 1e-3f ? half : half - 1; /* a round: the ends meet */
    for (k = 0; k <= end; k++) {
        float a = a0 + 3.14159265f * (float)k / (float)half;
        px[n] = e1x + r * cosf(a), py[n] = e1y + r * sinf(a), n++;
    }
    for (k = 0; k <= end; k++) {
        float a = a0 + 3.14159265f + 3.14159265f * (float)k / (float)half;
        px[n] = e2x + r * cosf(a), py[n] = e2y + r * sinf(a), n++;
    }
    mesh_clear(m);
    for (i = 0; i < n; i++) {
        int j = (i + 1) % n;
        if (tri(m, px[i], py[i], z, px[j], py[j], z, px[j], py[j], z + sz) ||
            tri(m, px[i], py[i], z, px[j], py[j], z + sz, px[i], py[i], z + sz))
            return -1;
        if (tri(m, cx, cy, z + sz, px[i], py[i], z + sz, px[j], py[j], z + sz) ||
            tri(m, cx, cy, z, px[j], py[j], z, px[i], py[i], z))
            return -1;
    }
    return 0;
}

/* Welding: corners closer than WELD on every axis are one vertex. A cut
 * reaches the same point from two polygons by different sums, so the two
 * copies can differ in the last bits. A vertex counts as on an edge
 * within ONEDGE, ten times tighter, so two welded-apart neighbours can't
 * each lie on the other's edges and split each other for ever. */
#define WELD 1e-3f
#define ONEDGE 1e-4f

typedef struct {
    float x, y, z;
    int i;
} wv;

static int by_x(const void *p, const void *q)
{
    float a = ((const wv *)p)->x, b = ((const wv *)q)->x;
    return a < b ? -1 : a > b ? 1 : 0;
}

static int root(int *up, int i)
{
    while (up[i] != i) {
        up[i] = up[up[i]];
        i = up[i];
    }
    return i;
}

/* idx[3n]: each corner's vertex. *pts: the vertices in x order (free it).
 * The number of vertices, or -1. */
static int weld(const gs_mesh *m, int *idx, wv **pts)
{
    int n = m->n * 3, i, j, nv = 0;
    size_t sz = (size_t)(n ? n : 1);
    wv *v = (wv *)malloc(sz * sizeof *v), *p = (wv *)malloc(sz * sizeof *p);
    int *up = (int *)malloc(sz * sizeof *up), *num = (int *)malloc(sz * sizeof *num);
    if (!v || !p || !up || !num) {
        free(v);
        free(p);
        free(up);
        free(num);
        return -1;
    }
    for (i = 0; i < m->n; i++) {
        const gs_v *c[3];
        c[0] = &m->t[i].a;
        c[1] = &m->t[i].b;
        c[2] = &m->t[i].c;
        for (j = 0; j < 3; j++) {
            v[i * 3 + j].x = c[j]->x;
            v[i * 3 + j].y = c[j]->y;
            v[i * 3 + j].z = c[j]->z;
            v[i * 3 + j].i = i * 3 + j;
        }
    }
    qsort(v, (size_t)n, sizeof *v, by_x);
    for (i = 0; i < n; i++) up[i] = i;
    for (i = 0; i < n; i++)
        for (j = i + 1; j < n && v[j].x - v[i].x <= WELD; j++)
            if (fabsf(v[j].y - v[i].y) <= WELD && fabsf(v[j].z - v[i].z) <= WELD) {
                int a = root(up, i), b = root(up, j);
                if (a < b) up[b] = a;
                else if (b < a) up[a] = b;
            }
    for (i = 0; i < n; i++)
        if (root(up, i) == i) {
            num[i] = nv;
            p[nv++] = v[i];
        }
    for (i = 0; i < n; i++) idx[v[i].i] = num[root(up, i)];
    free(v);
    free(up);
    free(num);
    *pts = p;
    return nv;
}

/* A sliver: its height is under SLIVER of its longest edge. Its own
 * corners give a poor plane, so it takes a neighbour's when that plane
 * holds all three of its corners. */
#define SLIVER 0.05f

static float shape(const poly *p)
{
    mv u, v, c;
    float l0, l1, l2, L;
    u.x = p->v[1].x - p->v[0].x;
    u.y = p->v[1].y - p->v[0].y;
    u.z = p->v[1].z - p->v[0].z;
    v.x = p->v[2].x - p->v[0].x;
    v.y = p->v[2].y - p->v[0].y;
    v.z = p->v[2].z - p->v[0].z;
    c = cross(u, v);
    l0 = dot(u, u);
    l1 = dot(v, v);
    u.x -= v.x;
    u.y -= v.y;
    u.z -= v.z;
    l2 = dot(u, u);
    L = l0 > l1 ? l0 : l1;
    if (l2 > L) L = l2;
    return L > 0.f ? sqrtf(dot(c, c)) / L : 0.f;
}

static int fits(const poly *s, const poly *g)
{
    int k;
    if (dot(s->nrm, g->nrm) < 0.99f) return 0;
    for (k = 0; k < 3; k++)
        if (fabsf(dot(g->nrm, s->v[k]) - g->w) > ONEDGE) return 0;
    return 1;
}

typedef struct {
    int lo, hi, t;
} side_of;

static int by_side(const void *p, const void *q)
{
    const side_of *a = (const side_of *)p, *b = (const side_of *)q;
    if (a->lo != b->lo) return a->lo < b->lo ? -1 : 1;
    return a->hi < b->hi ? -1 : a->hi > b->hi ? 1 : 0;
}

static int from_mesh(bag *b, const gs_mesh *m, const mv *off)
{
    int i, k, nv, round, ns = 0, changed = 1;
    int *idx = NULL;
    char *good = NULL;
    side_of *sd = NULL;
    wv *pts = NULL;
    memset(b, 0, sizeof *b);
    for (i = 0; i < m->n; i++) {
        poly p;
        p.n = 3;
        p.v[0].x = m->t[i].a.x - off->x;
        p.v[0].y = m->t[i].a.y - off->y;
        p.v[0].z = m->t[i].a.z - off->z;
        p.v[1].x = m->t[i].b.x - off->x;
        p.v[1].y = m->t[i].b.y - off->y;
        p.v[1].z = m->t[i].b.z - off->z;
        p.v[2].x = m->t[i].c.x - off->x;
        p.v[2].y = m->t[i].c.y - off->y;
        p.v[2].z = m->t[i].c.z - off->z;
        if (!plane_of(&p, &p.nrm, &p.w)) {
            /* No area, so no plane: kept on z so the bag lines up with the mesh. */
            p.nrm.x = p.nrm.y = 0.f;
            p.nrm.z = 1.f;
            p.w = p.v[0].z;
            p.n = 0;
        }
        if (b->n >= b->cap) {
            int c = b->cap ? b->cap * 2 : 32;
            poly *np = (poly *)realloc(b->p, (size_t)c * sizeof *np);
            if (!np) return -1;
            b->p = np;
            b->cap = c;
        }
        b->p[b->n++] = p;
    }
    if (!m->n) return 0;
    idx = (int *)malloc((size_t)m->n * 3 * sizeof *idx);
    good = (char *)malloc((size_t)m->n);
    sd = (side_of *)malloc((size_t)m->n * 3 * sizeof *sd);
    if (!idx || !good || !sd || (nv = weld(m, idx, &pts)) < 0) goto done;
    for (i = 0; i < m->n; i++) {
        good[i] = b->p[i].n == 3 && shape(&b->p[i]) >= SLIVER;
        for (k = 0; k < 3; k++) {
            int a = idx[i * 3 + k], c = idx[i * 3 + (k + 1) % 3];
            if (a == c) continue;
            sd[ns].lo = a < c ? a : c;
            sd[ns].hi = a < c ? c : a;
            sd[ns].t = i;
            ns++;
        }
    }
    qsort(sd, (size_t)ns, sizeof *sd, by_side);
    /* A run of slivers takes the plane in from the good triangle at its end. */
    for (round = 0; round < 32 && changed; round++) {
        changed = 0;
        for (i = 0; i < ns; i = k) {
            int j, g = -1;
            for (k = i; k < ns && sd[k].lo == sd[i].lo && sd[k].hi == sd[i].hi; k++)
                if (good[sd[k].t]) g = sd[k].t;
            if (g < 0) continue;
            for (j = i; j < k; j++) {
                int t = sd[j].t;
                if (good[t] || b->p[t].n != 3 || !fits(&b->p[t], &b->p[g])) continue;
                b->p[t].nrm = b->p[g].nrm;
                b->p[t].w = b->p[g].w;
                good[t] = 1;
                changed = 1;
            }
        }
    }
done:
    free(idx);
    free(good);
    free(sd);
    free(pts);
    /* Drop the empty ones now that the numbering is done with. */
    for (i = k = 0; i < b->n; i++)
        if (b->p[i].n == 3) b->p[k++] = b->p[i];
    b->n = k;
    (void)nv;
    return 0;
}

typedef struct {
    int lo, hi, way;
} edge;

static int by_ends(const void *p, const void *q)
{
    const edge *a = (const edge *)p, *b = (const edge *)q;
    if (a->lo != b->lo) return a->lo < b->lo ? -1 : 1;
    return a->hi < b->hi ? -1 : a->hi > b->hi ? 1 : 0;
}

int gm_open_edges(const gs_mesh *m)
{
    int *idx, nv, i, k, ne = 0, open = 0;
    wv *pts = NULL;
    edge *e;
    if (!m->n) return 0;
    idx = (int *)malloc((size_t)m->n * 3 * sizeof *idx);
    e = (edge *)malloc((size_t)m->n * 3 * sizeof *e);
    if (!idx || !e || (nv = weld(m, idx, &pts)) < 0) {
        free(idx);
        free(e);
        return -1;
    }
    free(pts);
    for (i = 0; i < m->n; i++)
        for (k = 0; k < 3; k++) {
            int a = idx[i * 3 + k], b = idx[i * 3 + (k + 1) % 3];
            if (a == b) continue;
            e[ne].lo = a < b ? a : b;
            e[ne].hi = a < b ? b : a;
            e[ne].way = a < b ? 1 : -1;
            ne++;
        }
    qsort(e, (size_t)ne, sizeof *e, by_ends);
    for (i = 0; i < ne; i = k) {
        int sum = 0;
        for (k = i; k < ne && e[k].lo == e[i].lo && e[k].hi == e[i].hi; k++) sum += e[k].way;
        open += sum < 0 ? -sum : sum;
    }
    free(idx);
    free(e);
    return open;
}

/* The first vertex strictly inside edge a-b, nearest a, or -1. pts is in
 * x order, so only the run between the ends' x is looked at. */
static int on_edge(const wv *pts, int nv, int a, int b)
{
    const wv *A = &pts[a], *B = &pts[b];
    float x0 = (A->x < B->x ? A->x : B->x) - WELD, x1 = (A->x < B->x ? B->x : A->x) + WELD;
    float vx = B->x - A->x, vy = B->y - A->y, vz = B->z - A->z;
    float L2 = vx * vx + vy * vy + vz * vz, best = 2.f;
    int lo = 0, hi = nv, k, got = -1;
    if (L2 <= WELD * WELD) return -1;
    while (lo < hi) {
        int mid = (lo + hi) / 2;
        if (pts[mid].x < x0) lo = mid + 1;
        else hi = mid;
    }
    for (k = lo; k < nv && pts[k].x <= x1; k++) {
        float px, py, pz, t, dx, dy, dz;
        if (k == a || k == b) continue;
        px = pts[k].x - A->x;
        py = pts[k].y - A->y;
        pz = pts[k].z - A->z;
        t = (px * vx + py * vy + pz * vz) / L2;
        if (t <= 0.f || t >= 1.f || t >= best) continue;
        dx = px - t * vx;
        dy = py - t * vy;
        dz = pz - t * vz;
        if (dx * dx + dy * dy + dz * dz <= ONEDGE * ONEDGE) {
            best = t;
            got = k;
        }
    }
    return got;
}

typedef struct {
    int v[3];
    int d; /* splits since the BSP's triangle */
} itri;

#define DEEPEST 64

/* A crack: a loop of open edges round a gap thinner than CRACK on
 * average. The BSP leaves one where a plane cuts a long sliver almost
 * along its length, and both sides of the cut disagree in the last bits
 * by more than a weld. The loop is filled with a fan, turned to face the
 * way the missing surface faced. A wider loop is a real hole and is left. */
#define CRACK 0.01f
#define CRACK_MAX 64

typedef struct {
    int a, b, way;
} dedge;

static int by_dedge(const void *p, const void *q)
{
    const dedge *x = (const dedge *)p, *y = (const dedge *)q;
    int xl = x->a < x->b ? x->a : x->b, yl = y->a < y->b ? y->a : y->b;
    int xh = x->a < x->b ? x->b : x->a, yh = y->a < y->b ? y->b : y->a;
    if (xl != yl) return xl < yl ? -1 : 1;
    return xh < yh ? -1 : xh > yh ? 1 : 0;
}

/* Adds the filling to *out (n of cap). 0, or -1 out of memory. */
static int fill_cracks(const wv *pts, int nv, itri **out, int *n, int *cap)
{
    int i, k, ne = 0, no = 0, rc = -1;
    dedge *e = (dedge *)malloc((size_t)(*n ? *n : 1) * 3 * sizeof *e);
    int *next = (int *)malloc((size_t)(nv ? nv : 1) * sizeof *next);
    dedge *open = NULL;
    if (!e || !next) goto done;
    for (i = 0; i < nv; i++) next[i] = -1;
    for (i = 0; i < *n; i++)
        for (k = 0; k < 3; k++) {
            e[ne].a = (*out)[i].v[k];
            e[ne].b = (*out)[i].v[(k + 1) % 3];
            e[ne].way = 1;
            ne++;
        }
    qsort(e, (size_t)ne, sizeof *e, by_dedge);
    open = (dedge *)malloc((size_t)(ne ? ne : 1) * sizeof *open);
    if (!open) goto done;
    for (i = 0; i < ne; i = k) {
        int sum = 0, lo = e[i].a < e[i].b ? e[i].a : e[i].b, hi = e[i].a < e[i].b ? e[i].b : e[i].a;
        for (k = i; k < ne && by_dedge(&e[k], &e[i]) == 0; k++) sum += e[k].a == lo ? 1 : -1;
        if (sum == 1 || sum == -1) {
            open[no].a = sum == 1 ? lo : hi;
            open[no].b = sum == 1 ? hi : lo;
            no++;
        } else if (sum) {
            no = -1; /* more than one sheet on an edge: not a crack this can mend */
            break;
        }
    }
    rc = 0;
    if (no <= 0) goto done;
    for (i = 0; i < no; i++) {
        if (next[open[i].a] >= 0) goto done; /* two ways on: leave it */
        next[open[i].a] = open[i].b;
    }
    for (i = 0; i < no; i++) {
        int loop[CRACK_MAX], len = 0, v = open[i].a, j;
        float ax = 0.f, ay = 0.f, az = 0.f, per = 0.f, area;
        if (next[v] < 0) continue;
        while (len < CRACK_MAX && next[v] >= 0) {
            loop[len++] = v;
            j = next[v];
            next[v] = -1;
            v = j;
            if (v == loop[0]) break;
        }
        if (v != loop[0] || len < 3) continue;
        for (j = 0; j < len; j++) {
            const wv *p = &pts[loop[j]], *q = &pts[loop[(j + 1) % len]];
            float dx = q->x - p->x, dy = q->y - p->y, dz = q->z - p->z;
            ax += p->y * q->z - p->z * q->y;
            ay += p->z * q->x - p->x * q->z;
            az += p->x * q->y - p->y * q->x;
            per += sqrtf(dx * dx + dy * dy + dz * dz);
        }
        area = 0.5f * sqrtf(ax * ax + ay * ay + az * az);
        if (per <= 0.f || area > CRACK * per) continue;
        for (j = 1; j + 1 < len; j++) {
            if (*n >= *cap) {
                itri *g = (itri *)realloc(*out, (size_t)*cap * 2 * sizeof *g);
                if (!g) {
                    rc = -1;
                    goto done;
                }
                *out = g;
                *cap *= 2;
            }
            /* The open edges run a to b on the surface there; the missing
             * piece runs them b to a. */
            (*out)[*n].v[0] = loop[0];
            (*out)[*n].v[1] = loop[j + 1];
            (*out)[*n].v[2] = loop[j];
            (*out)[*n].d = 0;
            (*n)++;
        }
    }
done:
    free(e);
    free(next);
    free(open);
    return rc;
}

/* Closes the cracks a BSP leaves: corners are welded, and a triangle
 * whose edge passes through another vertex is split there (a T-junction),
 * so every edge is met by one going the other way. 0, or -1. */
static int seal(gs_mesh *m)
{
    int *idx, nv, i, n = 0, cap, scap, top = 0;
    long splits = 0, most;
    wv *pts = NULL;
    itri *stack = NULL, *out = NULL;
    if (!m->n) return 0;
    idx = (int *)malloc((size_t)m->n * 3 * sizeof *idx);
    if (!idx || (nv = weld(m, idx, &pts)) < 0) {
        free(idx);
        return -1;
    }
    cap = scap = m->n * 2 + 16;
    most = (long)m->n * 16 + 1024; /* a guard: past it, the mesh is left as the BSP made it */
    stack = (itri *)malloc((size_t)scap * sizeof *stack);
    out = (itri *)malloc((size_t)cap * sizeof *out);
    if (!stack || !out) goto fail;
    for (i = 0; i < m->n; i++) {
        itri t;
        t.v[0] = idx[i * 3];
        t.v[1] = idx[i * 3 + 1];
        t.v[2] = idx[i * 3 + 2];
        t.d = 0;
        if (t.v[0] == t.v[1] || t.v[1] == t.v[2] || t.v[2] == t.v[0]) continue;
        stack[top++] = t;
    }
    while (top) {
        itri t = stack[--top];
        int e, k = -1;
        if (t.v[0] == t.v[1] || t.v[1] == t.v[2] || t.v[2] == t.v[0]) continue;
        /* A corner near an edge's end can be within ONEDGE of two edges
         * at once, so two slivers split each other back and forth for
         * ever. A triangle split DEEPEST times over is kept as it is. */
        if (t.d < DEEPEST)
            for (e = 0; e < 3 && k < 0; e++) k = on_edge(pts, nv, t.v[e], t.v[(e + 1) % 3]);
        if (k < 0) {
            if (n >= cap) {
                itri *g = (itri *)realloc(out, (size_t)cap * 2 * sizeof *g);
                if (!g) goto fail;
                out = g;
                cap *= 2;
            }
            out[n++] = t;
            continue;
        }
        e--;
        if (++splits > most) {
            /* Give up and keep the BSP's mesh: open edges, but a solid. */
            free(idx);
            free(pts);
            free(stack);
            free(out);
            return 1;
        }
        if (top + 2 > scap) {
            itri *g = (itri *)realloc(stack, (size_t)scap * 2 * sizeof *g);
            if (!g) goto fail;
            stack = g;
            scap *= 2;
        }
        {
            int a = t.v[e], b = t.v[(e + 1) % 3], c = t.v[(e + 2) % 3];
            stack[top].v[0] = a;
            stack[top].v[1] = k;
            stack[top].v[2] = c;
            stack[top].d = t.d + 1;
            top++;
            stack[top].v[0] = k;
            stack[top].v[1] = b;
            stack[top].v[2] = c;
            stack[top].d = t.d + 1;
            top++;
        }
    }
    if (fill_cracks(pts, nv, &out, &n, &cap)) goto fail;
    {
        gs_mesh fresh;
        memset(&fresh, 0, sizeof fresh);
        fresh.min.x = fresh.min.y = fresh.min.z = 1e30f;
        fresh.max.x = fresh.max.y = fresh.max.z = -1e30f;
        for (i = 0; i < n; i++) {
            const wv *a = &pts[out[i].v[0]], *b = &pts[out[i].v[1]], *c = &pts[out[i].v[2]];
            if (tri(&fresh, a->x, a->y, a->z, b->x, b->y, b->z, c->x, c->y, c->z)) {
                free(fresh.t);
                goto fail;
            }
        }
        free(m->t);
        *m = fresh;
    }
    free(idx);
    free(pts);
    free(stack);
    free(out);
    return 0;
fail:
    free(idx);
    free(pts);
    free(stack);
    free(out);
    return -1;
}

/* Join 0, cut 1, overlap 2. The same clips; only the turns of the
 * surface change, which is what picks inside from outside. */
/* Both solids are moved so their joint box sits on the origin while the
 * planes are cut: a float has most of its digits left there, so a round
 * part far out on the plate splits as cleanly as one at the origin. */
static void csg_centre(mv *off, const gs_mesh *a, const gs_mesh *b)
{
    float lx = a->min.x < b->min.x ? a->min.x : b->min.x, hx = a->max.x > b->max.x ? a->max.x : b->max.x;
    float ly = a->min.y < b->min.y ? a->min.y : b->min.y, hy = a->max.y > b->max.y ? a->max.y : b->max.y;
    float lz = a->min.z < b->min.z ? a->min.z : b->min.z, hz = a->max.z > b->max.z ? a->max.z : b->max.z;
    off->x = off->y = off->z = 0.f;
    if (!a->n || !b->n) return;
    off->x = (lx + hx) * 0.5f;
    off->y = (ly + hy) * 0.5f;
    off->z = (lz + hz) * 0.5f;
}

static void csg_back(gs_mesh *m, const mv *off)
{
    int i;
    for (i = 0; i < m->n; i++) {
        m->t[i].a.x += off->x, m->t[i].a.y += off->y, m->t[i].a.z += off->z;
        m->t[i].b.x += off->x, m->t[i].b.y += off->y, m->t[i].b.z += off->z;
        m->t[i].c.x += off->x, m->t[i].c.y += off->y, m->t[i].c.z += off->z;
    }
    if (!m->n) return;
    m->min.x += off->x, m->min.y += off->y, m->min.z += off->z;
    m->max.x += off->x, m->max.y += off->y, m->max.z += off->z;
}

static int gm_csg(gs_mesh *dst, const gs_mesh *a, const gs_mesh *b, int op)
{
    bag pa, pb, all;
    node *A, *B;
    mv off;
    int rc;
    csg_centre(&off, a, b);
    memset(&pa, 0, sizeof pa);
    memset(&pb, 0, sizeof pb);
    memset(&all, 0, sizeof all);
    A = (node *)calloc(1, sizeof *A);
    B = (node *)calloc(1, sizeof *B);
    if (!A || !B || from_mesh(&pa, a, &off) || from_mesh(&pb, b, &off)) {
        bag_free(&pa);
        bag_free(&pb);
        node_free(A);
        node_free(B);
        return -1;
    }
    rc = node_build(A, &pa) || node_build(B, &pb);
    bag_free(&pa);
    bag_free(&pb);
    if (rc) {
        node_free(A);
        node_free(B);
        return -1;
    }
    if (op != 0) node_invert(A);
    if (op == 2) node_invert(B);
    rc = node_clip_to(A, B) || node_clip_to(B, A);
    if (!rc) {
        node_invert(B);
        rc = node_clip_to(B, A);
    }
    if (!rc) {
        bag got;
        /* Turn the second solid back so its faces meet the first, then
         * build them in. A cut and an overlap turn A once more, which
         * puts the outside back out. */
        node_invert(B);
        memset(&got, 0, sizeof got);
        rc = node_all(B, &got) || node_build(A, &got);
        bag_free(&got);
    }
    if (!rc && op != 0) node_invert(A);
    if (!rc) rc = node_all(A, &all);
    node_free(A);
    node_free(B);
    if (rc) {
        bag_free(&all);
        return -1;
    }
    mesh_clear(dst);
    rc = mesh_from(dst, &all);
    bag_free(&all);
    if (!rc && seal(dst) < 0) rc = -1;
    if (!rc) csg_back(dst, &off);
    return rc;
}

int gm_union(gs_mesh *dst, const gs_mesh *a, const gs_mesh *b)
{
    return gm_csg(dst, a, b, 0);
}

int gm_subtract(gs_mesh *dst, const gs_mesh *a, const gs_mesh *b)
{
    return gm_csg(dst, a, b, 1);
}

int gm_intersect(gs_mesh *dst, const gs_mesh *a, const gs_mesh *b)
{
    return gm_csg(dst, a, b, 2);
}

/* Little-endian on every machine, so a 68k file loads on a desktop. */
static void put_le32(unsigned char *p, unsigned long u)
{
    p[0] = (unsigned char)(u & 0xffu);
    p[1] = (unsigned char)((u >> 8) & 0xffu);
    p[2] = (unsigned char)((u >> 16) & 0xffu);
    p[3] = (unsigned char)((u >> 24) & 0xffu);
}

static void put_lef32(unsigned char *p, float x)
{
    uint32_t u;
    memcpy(&u, &x, 4);
    put_le32(p, (unsigned long)u);
}

/* "GFOpenMake " and the file's name without its drawer or extension, in
 * at most 79 bytes, so the header never starts with "solid". */
static void stl_header(unsigned char *hdr, const char *path)
{
    const char *base = path, *p;
    size_t n;
    for (p = path; *p; p++)
        if (*p == '/' || *p == ':' || *p == '\\') base = p + 1;
    p = strrchr(base, '.');
    n = p ? (size_t)(p - base) : strlen(base);
    if (n > 68) n = 68;
    memcpy(hdr, "GFOpenMake ", 11);
    memcpy(hdr + 11, base, n);
}

int gm_stl_write(const gs_mesh *m, const char *path)
{
    FILE *f;
    unsigned char hdr[84], rec[50];
    int i, k;
    if (!m || !path) return -1;
    f = fopen(path, "wb");
    if (!f) return -1;
    memset(hdr, 0, sizeof hdr);
    stl_header(hdr, path);
    put_le32(hdr + 80, (unsigned long)m->n);
    if (fwrite(hdr, 1, 84, f) != 84) {
        fclose(f);
        return -1;
    }
    for (i = 0; i < m->n; i++) {
        float v[12];
        mv e1, e2, nn;
        e1.x = m->t[i].b.x - m->t[i].a.x;
        e1.y = m->t[i].b.y - m->t[i].a.y;
        e1.z = m->t[i].b.z - m->t[i].a.z;
        e2.x = m->t[i].c.x - m->t[i].a.x;
        e2.y = m->t[i].c.y - m->t[i].a.y;
        e2.z = m->t[i].c.z - m->t[i].a.z;
        nn = cross(e1, e2);
        if (!unit(&nn)) nn.x = nn.y = nn.z = 0.f; /* no direction to give */
        v[0] = nn.x;
        v[1] = nn.y;
        v[2] = nn.z;
        v[3] = m->t[i].a.x;
        v[4] = m->t[i].a.y;
        v[5] = m->t[i].a.z;
        v[6] = m->t[i].b.x;
        v[7] = m->t[i].b.y;
        v[8] = m->t[i].b.z;
        v[9] = m->t[i].c.x;
        v[10] = m->t[i].c.y;
        v[11] = m->t[i].c.z;
        for (k = 0; k < 12; k++) put_lef32(rec + 4 * k, v[k]);
        rec[48] = rec[49] = 0;
        if (fwrite(rec, 1, 50, f) != 50) {
            fclose(f);
            return -1;
        }
    }
    if (fclose(f)) return -1;
    return 0;
}
