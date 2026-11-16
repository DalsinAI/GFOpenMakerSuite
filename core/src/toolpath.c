/* The toolpath (gs_toolpath.h). MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gs_toolpath.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

void gt_init(gt_path *p)
{
    memset(p, 0, sizeof *p);
}

void gt_free(gt_path *p)
{
    free(p->m);
    free(p->layer);
    gt_init(p);
}

/* Room for want moves (or layers) in total. 0, or -1. */
static int room(void **v, int *cap, int want, size_t size)
{
    void *g;
    int c = *cap ? *cap : 64;
    if (want <= *cap) return 0;
    while (c < want) c *= 2;
    g = realloc(*v, (size_t)c * size);
    if (!g) return -1;
    *v = g;
    *cap = c;
    return 0;
}

int gt_layer(gt_path *p)
{
    if (room((void **)&p->layer, &p->lcap, p->nlayers + 1, sizeof *p->layer)) return -1;
    p->layer[p->nlayers++] = p->n;
    return 0;
}

int gt_add(gt_path *p, int kind, float x, float y, float z, float w)
{
    gt_move *m;
    if (room((void **)&p->m, &p->cap, p->n + 1, sizeof *p->m)) return -1;
    m = &p->m[p->n++];
    m->x = x;
    m->y = y;
    m->z = z;
    m->w = kind == GT_TRAVEL ? 0.f : w;
    m->kind = (unsigned char)kind;
    return 0;
}

int gt_loop(gt_path *p, int kind, const gl_loop *l, float z, float w)
{
    int i, last = l->closed ? l->n : l->n - 1;
    if (l->n < 1) return 0;
    if (room((void **)&p->m, &p->cap, p->n + last + 1, sizeof *p->m)) return -1;
    gt_add(p, GT_TRAVEL, l->xy[0], l->xy[1], z, 0.f);
    for (i = 1; i <= last; i++) {
        int k = i % l->n;
        gt_add(p, kind, l->xy[2 * k], l->xy[2 * k + 1], z, w);
    }
    return 0;
}

int gt_append(gt_path *dst, const gt_path *src)
{
    int i;
    if (room((void **)&dst->m, &dst->cap, dst->n + src->n, sizeof *dst->m) ||
        room((void **)&dst->layer, &dst->lcap, dst->nlayers + src->nlayers, sizeof *dst->layer))
        return -1;
    if (src->n) memcpy(dst->m + dst->n, src->m, (size_t)src->n * sizeof *src->m);
    for (i = 0; i < src->nlayers; i++) dst->layer[dst->nlayers + i] = src->layer[i] + dst->n;
    dst->n += src->n;
    dst->nlayers += src->nlayers;
    return 0;
}

float gt_length(const gt_path *p, int kind)
{
    float sum = 0.f;
    int i;
    for (i = 1; i < p->n; i++) {
        const gt_move *a = &p->m[i - 1], *b = &p->m[i];
        float dx = b->x - a->x, dy = b->y - a->y, dz = b->z - a->z;
        if (kind < 0 || b->kind == kind) sum += sqrtf(dx * dx + dy * dy + dz * dz);
    }
    return sum;
}

static void grow(const gt_move *m, float min[3], float max[3])
{
    const float v[3] = {m->x, m->y, m->z};
    int k;
    for (k = 0; k < 3; k++) {
        if (v[k] < min[k]) min[k] = v[k];
        if (v[k] > max[k]) max[k] = v[k];
    }
}

int gt_bounds(const gt_path *p, float *min, float *max)
{
    int i, any = 0;
    for (i = 0; i < p->n; i++) {
        if (p->m[i].kind == GT_TRAVEL) continue;
        if (!any) {
            min[0] = max[0] = p->m[i].x;
            min[1] = max[1] = p->m[i].y;
            min[2] = max[2] = p->m[i].z;
            any = 1;
        }
        grow(&p->m[i], min, max);
        if (i) grow(&p->m[i - 1], min, max); /* where it starts from */
    }
    return any ? 0 : -1;
}
