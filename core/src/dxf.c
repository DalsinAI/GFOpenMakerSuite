/* A DXF reader for 2D vector drawings (gs_vec.h). Written from Autodesk's
 * published DXF reference only (group codes, HEADER $INSUNITS, common
 * entity codes, LINE, CIRCLE, ARC, LWPOLYLINE, POLYLINE / VERTEX / SEQEND,
 * INSERT, the LAYER table, colour 62 / 420, vertex bulge, and the OCS
 * arbitrary-axis algorithm), never from another application's source.
 * ASCII DXF only; coordinates come out in millimetres, Y up. Binary DXF,
 * font text, hatches, splines, and ellipses are not read.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gs_vec.h"
#include "vec_int.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DXF_MAX_PAIR 500000
#define DXF_MAX_VERT 100000
#define DXF_MAX_LAYER 4096
#define DXF_MAX_BLOCK 4096
#define DXF_MAX_INSERT 16
#define DXF_NAME 256

typedef struct {
    int code;
    char *v;
} pair;

typedef struct {
    char name[DXF_NAME];
    unsigned colour;
    int off;
} dlayer;

typedef struct {
    char name[DXF_NAME];
    int from, to; /* pair indices of the block's entities (after BLOCK, before ENDBLK) */
} dblock;

typedef struct {
    pair *p;
    int n;
    dlayer *lay;
    int nlay;
    dblock *blk;
    int nblk;
    int units; /* $INSUNITS */
    float scale; /* drawing units to millimetres */
    gv_doc *d;
    char *err;
    int errlen;
} dxf;

static char lower(char c)
{
    return (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c;
}

static int ieq(const char *a, const char *b)
{
    if (!a || !b) return 0;
    while (*a && *b) {
        if (lower(*a) != lower(*b)) return 0;
        a++;
        b++;
    }
    return *a == 0 && *b == 0;
}

static int space(char c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f' || c == '\v';
}

static void skip_bom(const char **ps, const char *e)
{
    const char *s = *ps;
    if (e - s >= 3 && (unsigned char)s[0] == 0xef && (unsigned char)s[1] == 0xbb && (unsigned char)s[2] == 0xbf)
        s += 3;
    *ps = s;
}

static void name_copy(char *dst, const char *s, int n)
{
    int i, o = 0;
    if (n < 0) n = s ? (int)strlen(s) : 0;
    while (n > 0 && space(s[0])) {
        s++;
        n--;
    }
    while (n > 0 && space(s[n - 1])) n--;
    for (i = 0; i < n && o < DXF_NAME - 1; i++) dst[o++] = s[i];
    dst[o] = 0;
}

static int parse_int(const char *s, int *out)
{
    long v = 0, sign = 1;
    if (!s) return -1;
    while (*s && space(*s)) s++;
    if (*s == '-') {
        sign = -1;
        s++;
    } else if (*s == '+')
        s++;
    if (*s < '0' || *s > '9') return -1;
    while (*s >= '0' && *s <= '9') {
        v = v * 10 + (*s - '0');
        s++;
    }
    *out = (int)(v * sign);
    return 0;
}

static int parse_float(const char *s, float *out)
{
    double v = 0, frac = 0, div = 1, sign = 1, expn = 0, esign = 1;
    int any = 0;
    if (!s) return -1;
    while (*s && space(*s)) s++;
    if (*s == '-') {
        sign = -1;
        s++;
    } else if (*s == '+')
        s++;
    while (*s >= '0' && *s <= '9') {
        v = v * 10 + (*s - '0');
        s++;
        any = 1;
    }
    if (*s == '.') {
        s++;
        while (*s >= '0' && *s <= '9') {
            div *= 10;
            frac = frac * 10 + (*s - '0');
            s++;
            any = 1;
        }
    }
    if (!any) return -1;
    if (*s == 'e' || *s == 'E') {
        s++;
        if (*s == '-') {
            esign = -1;
            s++;
        } else if (*s == '+')
            s++;
        if (*s < '0' || *s > '9') return -1;
        while (*s >= '0' && *s <= '9') {
            expn = expn * 10 + (*s - '0');
            s++;
        }
        {
            int k;
            double p = 1, b = 10;
            int e = (int)expn;
            for (k = 0; k < e; k++) p *= b;
            if (esign < 0) v = (v + frac / div) / p;
            else v = (v + frac / div) * p;
        }
    } else
        v = v + frac / div;
    *out = (float)(v * sign);
    return 0;
}

/* Autodesk HEADER $INSUNITS (group 70) to millimetres. Unitless is mm. */
static float units_to_mm(int u)
{
    switch (u) {
    case 0:
    case 4:
        return 1.f;
    case 1:
        return 25.4f;
    case 2:
        return 304.8f;
    case 3:
        return 1609344.f;
    case 5:
        return 10.f;
    case 6:
        return 1000.f;
    case 7:
        return 1000000.f;
    case 8:
        return 25.4e-6f;
    case 9:
        return 0.0254f;
    case 10:
        return 914.4f;
    case 11:
        return 1e-7f;
    case 12:
        return 1e-6f;
    case 13:
        return 0.001f;
    case 14:
        return 100.f;
    case 15:
        return 10000.f;
    case 16:
        return 100000.f;
    case 17:
        return 1e12f;
    case 21:
        return (1200.f / 3937.f) * 1000.f;
    case 22:
        return (1200.f / 3937.f) * 1000.f / 12.f;
    case 23:
        return (1200.f / 3937.f) * 1000.f * 3.f;
    case 24:
        return (1200.f / 3937.f) * 1000.f * 5280.f;
    default:
        return 1.f;
    }
}

/* ACI 1-7 as Autodesk names them (7 is black here, for a light page).
 * 8 and 9 are the two greys beside them. Other index colours still get a
 * distinct RGB so they become their own layer, not Autodesk's full palette. */
static unsigned aci_rgb(int n)
{
    static const unsigned std[10] = {0,        0xff0000, 0xffff00, 0x00ff00, 0x00ffff,
                                     0x0000ff, 0xff00ff, 0x000000, 0x808080, 0xc0c0c0};
    unsigned r, g, b;
    if (n < 0) n = -n;
    if (n <= 9) return std[n];
    if (n >= 250 && n <= 255) {
        unsigned t = (unsigned)(255 - (n - 250) * 42);
        if (n == 255) t = 255;
        return (t << 16) | (t << 8) | t;
    }
    if (n > 255) n = 255;
    r = (unsigned)((n * 37u) % 200u + 40u);
    g = (unsigned)((n * 73u) % 200u + 40u);
    b = (unsigned)((n * 19u) % 200u + 40u);
    return (r << 16) | (g << 8) | b;
}

static int add_xy(gv_doc *d, float x, float y, char *err, int errlen)
{
    int i, total = 0;
    for (i = 0; i < d->n; i++) total += d->p[i].n;
    if (total >= GV_MAX_PT) {
        fail_set(err, errlen, "too many points");
        return -1;
    }
    if (doc_add_pt(d, x, y)) {
        fail_set(err, errlen, "out of memory");
        return -1;
    }
    return 0;
}

/* Flatten a circular arc, angles in degrees, going the signed way (CCW if
 * dtheta > 0). The start is added when first is set. r is positive. */
static int add_arc(gv_doc *d, float cx, float cy, float r, float a0, float dtheta, int first, float sc, char *err,
                   int errlen)
{
    float th, n, t;
    int i, segs;
    if (r < 0) r = -r;
    if (r < GV_MIN) return 0;
    th = r > GV_FLAT ? (GV_PI * sqrtf(r / (2.f * GV_FLAT))) : 8.f;
    n = (dtheta < 0 ? -dtheta : dtheta) / 180.f * th;
    segs = (int)ceilf(n);
    if (segs < 2) segs = 2;
    if (segs > 512) segs = 512;
    for (i = first ? 0 : 1; i <= segs; i++) {
        t = (a0 + dtheta * ((float)i / (float)segs)) * GV_PI / 180.f;
        if (add_xy(d, (cx + r * cosf(t)) * sc, (cy + r * sinf(t)) * sc, err, errlen)) return -1;
    }
    return 0;
}

/* Vertex bulge (Autodesk VERTEX group 42): tan of one quarter of the
 * included angle, negative if the arc is clockwise. */
static int add_bulge(gv_doc *d, float x0, float y0, float x1, float y1, float bulge, float sc, char *err, int errlen)
{
    float dx, dy, c, theta, s2, r, mx, my, lx, ly, cm, cx, cy, a0;
    dx = x1 - x0;
    dy = y1 - y0;
    c = sqrtf(dx * dx + dy * dy);
    if (c < GV_MIN || (bulge < GV_MIN && bulge > -GV_MIN)) return add_xy(d, x1 * sc, y1 * sc, err, errlen);
    theta = 4.f * gv_atan2(bulge, 1.f);
    s2 = sinf(theta * 0.5f);
    if (s2 < GV_MIN && s2 > -GV_MIN) return add_xy(d, x1 * sc, y1 * sc, err, errlen);
    r = c / (2.f * s2);
    mx = (x0 + x1) * 0.5f;
    my = (y0 + y1) * 0.5f;
    lx = -dy / c;
    ly = dx / c;
    cm = r * cosf(theta * 0.5f);
    cx = mx + lx * cm;
    cy = my + ly * cm;
    a0 = gv_atan2(y0 - cy, x0 - cx) * 180.f / GV_PI;
    if (r < 0) r = -r;
    return add_arc(d, cx, cy, r, a0, theta * 180.f / GV_PI, 0, sc, err, errlen);
}

/* Autodesk OCS arbitrary-axis algorithm, then scale to millimetres. */
static void ocs_xy(float x, float y, float z, float wx, float wy, float wz, float *ox, float *oy)
{
    float ax, ayx, ayy, ayz, ay, az, lx, ly, len, wlen;
    (void)z;
    wlen = sqrtf(wx * wx + wy * wy + wz * wz);
    if (wlen < GV_MIN) {
        wx = 0;
        wy = 0;
        wz = 1;
        wlen = 1;
    }
    wx /= wlen;
    wy /= wlen;
    wz /= wlen;
    if ((wx < 0 ? -wx : wx) < (1.f / 64.f) && (wy < 0 ? -wy : wy) < (1.f / 64.f)) {
        /* Ax = World Y × W = (wz, 0, -wx) */
        ax = wz;
        ay = 0.f;
        az = -wx;
    } else {
        /* Ax = World Z × W = (-wy, wx, 0) */
        ax = -wy;
        ay = wx;
        az = 0.f;
    }
    len = sqrtf(ax * ax + ay * ay + az * az);
    if (len < GV_MIN) {
        *ox = x;
        *oy = y;
        return;
    }
    ax /= len;
    ay /= len;
    az /= len;
    /* Ay = W × Ax */
    ayx = wy * az - wz * ay;
    ayy = wz * ax - wx * az;
    ayz = wx * ay - wy * ax;
    lx = x * ax + y * ayx;
    ly = x * ay + y * ayy;
    (void)ayz;
    *ox = lx;
    *oy = ly;
}

static int looks_binary_dxf(const char *s, int n)
{
    const char *e = s + n, *w = "AutoCAD Binary DXF";
    int i;
    skip_bom(&s, e);
    for (i = 0; w[i]; i++)
        if (s + i >= e || s[i] != w[i]) return 0;
    return 1;
}

static int looks_dxf(const char *s, int n)
{
    const char *e = s + n;
    int lines = 0, code, ok = 0;
    skip_bom(&s, e);
    while (s < e && lines < 40) {
        const char *a, *b, *v;
        while (s < e && (*s == ' ' || *s == '\t')) s++;
        a = s;
        while (s < e && *s != '\n' && *s != '\r') s++;
        b = s;
        if (s < e && *s == '\r') s++;
        if (s < e && *s == '\n') s++;
        while (b > a && (b[-1] == ' ' || b[-1] == '\t')) b--;
        if (a == b && s >= e) break;
        {
            char tmp[32];
            int k, m = (int)(b - a);
            if (m >= (int)sizeof tmp) m = (int)sizeof tmp - 1;
            for (k = 0; k < m; k++) tmp[k] = a[k];
            tmp[m] = 0;
            if (parse_int(tmp, &code)) return 0;
        }
        v = s;
        while (s < e && *s != '\n' && *s != '\r') s++;
        if (s < e && *s == '\r') s++;
        if (s < e && *s == '\n') s++;
        lines++;
        if (code == 999) continue;
        if (code == 0) {
            char name[DXF_NAME];
            name_copy(name, v, (int)((s > v && (s[-1] == '\n' || s[-1] == '\r')) ? (s - v - (s[-1] == '\n' && s - 2 >= v && s[-2] == '\r' ? 2 : 1)) : (s - v)));
            /* trim already in name_copy; the line end is messy — use v until CR/LF */
            {
                const char *q = v;
                while (q < e && *q != '\n' && *q != '\r') q++;
                name_copy(name, v, (int)(q - v));
            }
            if (ieq(name, "SECTION") || ieq(name, "LINE") || ieq(name, "CIRCLE") || ieq(name, "ARC") ||
                ieq(name, "LWPOLYLINE") || ieq(name, "POLYLINE") || ieq(name, "EOF") || ieq(name, "ENTITY") ||
                ieq(name, "ENTITIES") || ieq(name, "HEADER") || ieq(name, "TABLE") || ieq(name, "BLOCK") ||
                ieq(name, "INSERT"))
                ok = 1;
            return ok;
        }
        return 0;
    }
    return 0;
}

static int pair_int(const pair *p, int *out)
{
    return parse_int(p->v, out);
}

static int pair_float(const pair *p, float *out)
{
    return parse_float(p->v, out);
}

static int scan_pairs(char *buf, int n, pair **out, int *nout, char *err, int errlen)
{
    char *s = buf, *e = buf + n;
    pair *p = NULL;
    int cap = 0, np = 0;
    skip_bom((const char **)&s, e);
    while (s < e) {
        char *a, *b, *v;
        int code;
        while (s < e && (*s == ' ' || *s == '\t')) s++;
        if (s >= e) break;
        a = s;
        while (s < e && *s != '\n' && *s != '\r') s++;
        b = s;
        if (s < e && *s == '\r') s++;
        if (s < e && *s == '\n') s++;
        while (b > a && (b[-1] == ' ' || b[-1] == '\t')) b--;
        if (a == b && s >= e) break;
        *b = 0;
        if (parse_int(a, &code)) {
            free(p);
            fail_set(err, errlen, "not a DXF");
            return -1;
        }
        v = s;
        while (s < e && *s != '\n' && *s != '\r') s++;
        if (s < e) {
            char *end = s;
            if (*s == '\r') s++;
            if (s < e && *s == '\n') s++;
            *end = 0;
        }
        if (np >= DXF_MAX_PAIR) {
            free(p);
            fail_set(err, errlen, "the file is too long");
            return -1;
        }
        if (np == cap) {
            int ncap = cap ? cap * 2 : 256;
            pair *npairs = (pair *)realloc(p, (size_t)ncap * sizeof *npairs);
            if (!npairs) {
                free(p);
                fail_set(err, errlen, "out of memory");
                return -1;
            }
            p = npairs;
            cap = ncap;
        }
        p[np].code = code;
        p[np].v = v;
        np++;
    }
    *out = p;
    *nout = np;
    return 0;
}

static int find_section(const dxf *x, const char *name, int *from, int *to)
{
    int i;
    for (i = 0; i + 1 < x->n; i++) {
        if (x->p[i].code == 0 && ieq(x->p[i].v, "SECTION") && x->p[i + 1].code == 2 && ieq(x->p[i + 1].v, name)) {
            int j;
            *from = i + 2;
            for (j = *from; j < x->n; j++)
                if (x->p[j].code == 0 && ieq(x->p[j].v, "ENDSEC")) {
                    *to = j;
                    return 0;
                }
            *to = x->n;
            return 0;
        }
    }
    return -1;
}

static const dlayer *layer_of(const dxf *x, const char *name)
{
    int i;
    for (i = 0; i < x->nlay; i++)
        if (ieq(x->lay[i].name, name)) return &x->lay[i];
    return NULL;
}

static const dblock *block_of(const dxf *x, const char *name)
{
    int i;
    for (i = 0; i < x->nblk; i++)
        if (ieq(x->blk[i].name, name)) return &x->blk[i];
    return NULL;
}

static unsigned colour_of(const dxf *x, const char *layer, int aci, int truec, unsigned inherit)
{
    const dlayer *L;
    if (truec >= 0) return (unsigned)truec & 0x00ffffffu;
    if (aci == 0) return inherit ? inherit : 0x000000u; /* BYBLOCK */
    if (aci > 0 && aci != 256) return aci_rgb(aci);
    L = layer_of(x, layer);
    if (L && !L->off) return L->colour;
    return inherit ? inherit : 0x000000u;
}

static int entity_span(const dxf *x, int i, int end)
{
    int j = i + 1;
    while (j < end && x->p[j].code != 0) j++;
    return j;
}

static int read_header(dxf *x)
{
    int from, to, i;
    x->units = 0;
    x->scale = 1.f;
    if (find_section(x, "HEADER", &from, &to)) return 0;
    for (i = from; i < to; i++) {
        if (x->p[i].code == 9 && ieq(x->p[i].v, "$INSUNITS")) {
            int u = 0;
            if (i + 1 < to && x->p[i + 1].code == 70 && !pair_int(&x->p[i + 1], &u)) x->units = u;
        }
    }
    x->scale = units_to_mm(x->units);
    return 0;
}

static int read_layers(dxf *x)
{
    int from, to, i, in_layer = 0;
    dlayer cur;
    x->lay = NULL;
    x->nlay = 0;
    if (find_section(x, "TABLES", &from, &to)) return 0;
    memset(&cur, 0, sizeof cur);
    cur.colour = 0x000000u;
    for (i = from; i < to; i++) {
        if (x->p[i].code == 0) {
            if (in_layer && cur.name[0]) {
                dlayer *p;
                if (x->nlay >= DXF_MAX_LAYER) {
                    fail_set(x->err, x->errlen, "too many layers");
                    return -1;
                }
                p = (dlayer *)realloc(x->lay, (size_t)(x->nlay + 1) * sizeof *p);
                if (!p) {
                    fail_set(x->err, x->errlen, "out of memory");
                    return -1;
                }
                x->lay = p;
                x->lay[x->nlay++] = cur;
            }
            in_layer = ieq(x->p[i].v, "LAYER");
            memset(&cur, 0, sizeof cur);
            cur.colour = 0x000000u;
            continue;
        }
        if (!in_layer) continue;
        if (x->p[i].code == 2) name_copy(cur.name, x->p[i].v, -1);
        if (x->p[i].code == 62) {
            int c = 7;
            if (!pair_int(&x->p[i], &c)) {
                if (c < 0) {
                    cur.off = 1;
                    c = -c;
                }
                if (c && c != 256) cur.colour = aci_rgb(c);
            }
        }
        if (x->p[i].code == 420) {
            int c = 0;
            if (!pair_int(&x->p[i], &c) && c >= 0) cur.colour = (unsigned)c & 0x00ffffffu;
        }
        if (x->p[i].code == 70) {
            int f = 0;
            if (!pair_int(&x->p[i], &f) && (f & 1)) cur.off = 1; /* frozen */
        }
    }
    if (in_layer && cur.name[0]) {
        dlayer *p = (dlayer *)realloc(x->lay, (size_t)(x->nlay + 1) * sizeof *p);
        if (!p) {
            fail_set(x->err, x->errlen, "out of memory");
            return -1;
        }
        x->lay = p;
        x->lay[x->nlay++] = cur;
    }
    return 0;
}

static int read_blocks(dxf *x)
{
    int from, to, i;
    x->blk = NULL;
    x->nblk = 0;
    if (find_section(x, "BLOCKS", &from, &to)) return 0;
    for (i = from; i < to; i++) {
        if (x->p[i].code == 0 && ieq(x->p[i].v, "BLOCK")) {
            dblock b;
            int j, k;
            memset(&b, 0, sizeof b);
            j = entity_span(x, i, to);
            for (k = i + 1; k < j; k++)
                if (x->p[k].code == 2) name_copy(b.name, x->p[k].v, -1);
            b.from = j;
            b.to = to;
            for (k = j; k < to; k++)
                if (x->p[k].code == 0 && ieq(x->p[k].v, "ENDBLK")) {
                    b.to = k;
                    break;
                }
            if (b.name[0] == '*' && (lower(b.name[1]) == 'm' || lower(b.name[1]) == 'p')) {
                /* *Model_Space / *Paper_Space: entities already live in ENTITIES. */
                i = (b.to < to) ? b.to : i;
                continue;
            }
            if (x->nblk >= DXF_MAX_BLOCK) {
                fail_set(x->err, x->errlen, "too many blocks");
                return -1;
            }
            {
                dblock *p = (dblock *)realloc(x->blk, (size_t)(x->nblk + 1) * sizeof *p);
                if (!p) {
                    fail_set(x->err, x->errlen, "out of memory");
                    return -1;
                }
                x->blk = p;
                x->blk[x->nblk++] = b;
            }
            i = (b.to < to) ? b.to : i;
        }
    }
    return 0;
}

typedef struct {
    float x, y, bulge;
} vert;

static int emit_ents(dxf *x, int from, int to, float ox, float oy, float sx, float sy, float rot, unsigned inherit,
                     int depth);

static int xf_pt(float x, float y, float wx, float wy, float wz, float ox, float oy, float sx, float sy, float rot,
                 float *px, float *py)
{
    float tx, ty, c, s, a;
    ocs_xy(x, y, 0.f, wx, wy, wz, &tx, &ty);
    tx *= sx;
    ty *= sy;
    if (rot != 0.f) {
        a = rot * GV_PI / 180.f;
        c = cosf(a);
        s = sinf(a);
        {
            float rx = tx * c - ty * s, ry = tx * s + ty * c;
            tx = rx;
            ty = ry;
        }
    }
    *px = tx + ox;
    *py = ty + oy;
    return 0;
}

static int begin_stroke(dxf *x, int closed, unsigned col)
{
    if (x->d->n > 0) finish_path(x->d);
    if (col == GV_NONE) return 0;
    if (doc_add_path(x->d, closed, col, GV_NONE, 0.f)) {
        fail_set(x->err, x->errlen, "out of memory");
        return -1;
    }
    return 0;
}

static int emit_line(dxf *x, int i, int j, float ox, float oy, float sx, float sy, float rot, unsigned inherit)
{
    float x0 = 0, y0 = 0, x1 = 0, y1 = 0, wx = 0, wy = 0, wz = 1, px, py;
    char layer[DXF_NAME];
    int k, aci = 256, truec = -1, vis = 0, papers = 0;
    unsigned col;
    layer[0] = 0;
    name_copy(layer, "0", -1);
    for (k = i + 1; k < j; k++) {
        if (x->p[k].code == 8) name_copy(layer, x->p[k].v, -1);
        if (x->p[k].code == 62) pair_int(&x->p[k], &aci);
        if (x->p[k].code == 420) pair_int(&x->p[k], &truec);
        if (x->p[k].code == 60) pair_int(&x->p[k], &vis);
        if (x->p[k].code == 67) pair_int(&x->p[k], &papers);
        if (x->p[k].code == 10) pair_float(&x->p[k], &x0);
        if (x->p[k].code == 20) pair_float(&x->p[k], &y0);
        if (x->p[k].code == 11) pair_float(&x->p[k], &x1);
        if (x->p[k].code == 21) pair_float(&x->p[k], &y1);
        if (x->p[k].code == 210) pair_float(&x->p[k], &wx);
        if (x->p[k].code == 220) pair_float(&x->p[k], &wy);
        if (x->p[k].code == 230) pair_float(&x->p[k], &wz);
    }
    if (vis == 1 || papers == 1) return 0;
    if (layer_of(x, layer) && layer_of(x, layer)->off) return 0;
    col = colour_of(x, layer, aci, truec, inherit);
    if (begin_stroke(x, 0, col)) return -1;
    xf_pt(x0, y0, wx, wy, wz, ox, oy, sx, sy, rot, &px, &py);
    if (add_xy(x->d, px * x->scale, py * x->scale, x->err, x->errlen)) return -1;
    xf_pt(x1, y1, wx, wy, wz, ox, oy, sx, sy, rot, &px, &py);
    if (add_xy(x->d, px * x->scale, py * x->scale, x->err, x->errlen)) return -1;
    finish_path(x->d);
    return 0;
}

static int emit_arcish(dxf *x, int i, int j, int circle, float ox, float oy, float sx, float sy, float rot,
                       unsigned inherit)
{
    float cx = 0, cy = 0, r = 0, a0 = 0, a1 = 360, wx = 0, wy = 0, wz = 1, px, py, qx, qy, dth;
    char layer[DXF_NAME];
    int k, aci = 256, truec = -1, vis = 0, papers = 0;
    unsigned col;
    layer[0] = 0;
    name_copy(layer, "0", -1);
    for (k = i + 1; k < j; k++) {
        if (x->p[k].code == 8) name_copy(layer, x->p[k].v, -1);
        if (x->p[k].code == 62) pair_int(&x->p[k], &aci);
        if (x->p[k].code == 420) pair_int(&x->p[k], &truec);
        if (x->p[k].code == 60) pair_int(&x->p[k], &vis);
        if (x->p[k].code == 67) pair_int(&x->p[k], &papers);
        if (x->p[k].code == 10) pair_float(&x->p[k], &cx);
        if (x->p[k].code == 20) pair_float(&x->p[k], &cy);
        if (x->p[k].code == 40) pair_float(&x->p[k], &r);
        if (x->p[k].code == 50) pair_float(&x->p[k], &a0);
        if (x->p[k].code == 51) pair_float(&x->p[k], &a1);
        if (x->p[k].code == 210) pair_float(&x->p[k], &wx);
        if (x->p[k].code == 220) pair_float(&x->p[k], &wy);
        if (x->p[k].code == 230) pair_float(&x->p[k], &wz);
    }
    if (vis == 1 || papers == 1 || r <= 0.f) return 0;
    if (layer_of(x, layer) && layer_of(x, layer)->off) return 0;
    col = colour_of(x, layer, aci, truec, inherit);
    if (circle) a0 = 0.f, a1 = 360.f;
    dth = a1 - a0;
    while (dth <= 0.f) dth += 360.f;
    if (circle && dth < 360.f - 0.01f) dth = 360.f;
    /* Transform the centre and a point at a0, then flatten in the draw plane
     * after scale (arcs with rotation/scale stay circular if sx==sy; if not,
     * still flatten in WCS from the untransformed arc and transform points). */
    if (begin_stroke(x, circle ? 1 : 0, col)) return -1;
    {
        int segs, n;
        float th, a, xx, yy;
        th = r * (sx > sy ? sx : sy);
        if (th < 0) th = -th;
        th = th > GV_FLAT ? (GV_PI * sqrtf((th * x->scale) / (2.f * GV_FLAT))) : 8.f;
        n = (int)ceilf((dth / 180.f) * th);
        segs = n < 2 ? 2 : (n > 512 ? 512 : n);
        for (k = 0; k <= segs; k++) {
            if (circle && k == segs) break;
            a = a0 + dth * ((float)k / (float)segs);
            xx = cx + r * cosf(a * GV_PI / 180.f);
            yy = cy + r * sinf(a * GV_PI / 180.f);
            xf_pt(xx, yy, wx, wy, wz, ox, oy, sx, sy, rot, &px, &py);
            if (add_xy(x->d, px * x->scale, py * x->scale, x->err, x->errlen)) return -1;
        }
        (void)qx;
        (void)qy;
    }
    finish_path(x->d);
    return 0;
}

static int emit_pline(dxf *x, vert *vs, int nv, int closed, unsigned col, float ox, float oy, float sx, float sy,
                      float rot, float wx, float wy, float wz)
{
    int i;
    float x0, y0, x1, y1, px, py;
    if (nv < 2) return 0;
    if (begin_stroke(x, closed, col)) return -1;
    xf_pt(vs[0].x, vs[0].y, wx, wy, wz, ox, oy, sx, sy, rot, &px, &py);
    if (add_xy(x->d, px * x->scale, py * x->scale, x->err, x->errlen)) return -1;
    for (i = 0; i < nv - 1 || (closed && i < nv); i++) {
        vert *a = &vs[i], *b = &vs[(i + 1) % nv];
        xf_pt(a->x, a->y, wx, wy, wz, ox, oy, sx, sy, rot, &x0, &y0);
        xf_pt(b->x, b->y, wx, wy, wz, ox, oy, sx, sy, rot, &x1, &y1);
        /* Bulge is in OCS; after a uniform scale it stays a circular arc in
         * the same turn, so apply bulge in already-transformed millimetres
         * only when sx==sy and rot is absorbed in the ends. The ends are
         * already transformed; re-flatten using those ends and the same bulge
         * (angles are similar under uniform scale + rotation + translation). */
        if (a->bulge < GV_MIN && a->bulge > -GV_MIN) {
            if (add_xy(x->d, x1 * x->scale, y1 * x->scale, x->err, x->errlen)) return -1;
        } else {
            if (add_bulge(x->d, x0, y0, x1, y1, a->bulge, x->scale, x->err, x->errlen)) return -1;
        }
    }
    if (closed && x->d->n > 0) {
        gv_path *p = &x->d->p[x->d->n - 1];
        if (p->n >= 2) {
            float *a = p->xy, *b = p->xy + (p->n - 1) * 2;
            float dx = a[0] - b[0], dy = a[1] - b[1];
            if (dx * dx + dy * dy < GV_MIN * GV_MIN) p->n--;
        }
    }
    finish_path(x->d);
    return 0;
}

static int read_common(const dxf *x, int i, int j, char *layer, int *aci, int *truec, int *vis, int *papers, float *wx,
                       float *wy, float *wz)
{
    int k;
    name_copy(layer, "0", -1);
    *aci = 256;
    *truec = -1;
    *vis = 0;
    *papers = 0;
    *wx = 0;
    *wy = 0;
    *wz = 1;
    for (k = i + 1; k < j; k++) {
        if (x->p[k].code == 8) name_copy(layer, x->p[k].v, -1);
        if (x->p[k].code == 62) pair_int(&x->p[k], aci);
        if (x->p[k].code == 420) pair_int(&x->p[k], truec);
        if (x->p[k].code == 60) pair_int(&x->p[k], vis);
        if (x->p[k].code == 67) pair_int(&x->p[k], papers);
        if (x->p[k].code == 210) pair_float(&x->p[k], wx);
        if (x->p[k].code == 220) pair_float(&x->p[k], wy);
        if (x->p[k].code == 230) pair_float(&x->p[k], wz);
    }
    return 0;
}

static int emit_lwpoly(dxf *x, int i, int j, float ox, float oy, float sx, float sy, float rot, unsigned inherit)
{
    char layer[DXF_NAME];
    int k, aci, truec, vis, papers, flags = 0, nv = 0, nvert = 0;
    float wx, wy, wz;
    unsigned col;
    vert *vs, *p;
    vert cur;
    read_common(x, i, j, layer, &aci, &truec, &vis, &papers, &wx, &wy, &wz);
    if (vis == 1 || papers == 1) return 0;
    if (layer_of(x, layer) && layer_of(x, layer)->off) return 0;
    for (k = i + 1; k < j; k++)
        if (x->p[k].code == 70) pair_int(&x->p[k], &flags);
    col = colour_of(x, layer, aci, truec, inherit);
    vs = (vert *)calloc((size_t)DXF_MAX_VERT, sizeof *vs);
    if (!vs) {
        fail_set(x->err, x->errlen, "out of memory");
        return -1;
    }
    memset(&cur, 0, sizeof cur);
    for (k = i + 1; k < j; k++) {
        if (x->p[k].code == 10) {
            if (nvert) {
                if (nv >= DXF_MAX_VERT) {
                    free(vs);
                    fail_set(x->err, x->errlen, "too many points");
                    return -1;
                }
                vs[nv++] = cur;
                memset(&cur, 0, sizeof cur);
            }
            pair_float(&x->p[k], &cur.x);
            nvert = 1;
        } else if (x->p[k].code == 20)
            pair_float(&x->p[k], &cur.y);
        else if (x->p[k].code == 42)
            pair_float(&x->p[k], &cur.bulge);
    }
    if (nvert) {
        if (nv >= DXF_MAX_VERT) {
            free(vs);
            fail_set(x->err, x->errlen, "too many points");
            return -1;
        }
        vs[nv++] = cur;
    }
    p = vs;
    if (emit_pline(x, p, nv, flags & 1, col, ox, oy, sx, sy, rot, wx, wy, wz)) {
        free(vs);
        return -1;
    }
    free(vs);
    return 0;
}

static int emit_poly(dxf *x, int i, int end, int *next, float ox, float oy, float sx, float sy, float rot,
                     unsigned inherit)
{
    char layer[DXF_NAME];
    int j, k, aci, truec, vis, papers, flags = 0, nv = 0, mesh;
    float wx, wy, wz;
    unsigned col;
    vert *vs;
    j = entity_span(x, i, end);
    read_common(x, i, j, layer, &aci, &truec, &vis, &papers, &wx, &wy, &wz);
    for (k = i + 1; k < j; k++)
        if (x->p[k].code == 70) pair_int(&x->p[k], &flags);
    mesh = flags & (8 | 16 | 64); /* 3D polyline, 3D mesh, polyface */
    *next = j;
    /* VERTEX entities follow until SEQEND. */
    {
        int t = j;
        while (t < end && !(x->p[t].code == 0 && ieq(x->p[t].v, "SEQEND"))) t++;
        if (t < end) *next = t + 1;
        else *next = end;
        if (vis == 1 || papers == 1 || mesh) return 0;
        if (layer_of(x, layer) && layer_of(x, layer)->off) return 0;
        col = colour_of(x, layer, aci, truec, inherit);
        vs = (vert *)calloc((size_t)DXF_MAX_VERT, sizeof *vs);
        if (!vs) {
            fail_set(x->err, x->errlen, "out of memory");
            return -1;
        }
        k = j;
        while (k < end && x->p[k].code == 0 && ieq(x->p[k].v, "VERTEX")) {
            int u = entity_span(x, k, end), vflags = 0, m;
            vert cur;
            memset(&cur, 0, sizeof cur);
            for (m = k + 1; m < u; m++) {
                if (x->p[m].code == 10) pair_float(&x->p[m], &cur.x);
                if (x->p[m].code == 20) pair_float(&x->p[m], &cur.y);
                if (x->p[m].code == 42) pair_float(&x->p[m], &cur.bulge);
                if (x->p[m].code == 70) pair_int(&x->p[m], &vflags);
            }
            if (!(vflags & (8 | 16 | 32 | 64 | 128))) {
                if (nv >= DXF_MAX_VERT) {
                    free(vs);
                    fail_set(x->err, x->errlen, "too many points");
                    return -1;
                }
                vs[nv++] = cur;
            }
            k = u;
        }
        if (emit_pline(x, vs, nv, flags & 1, col, ox, oy, sx, sy, rot, wx, wy, wz)) {
            free(vs);
            return -1;
        }
        free(vs);
    }
    return 0;
}

static int emit_insert(dxf *x, int i, int j, int end, int *next, float ox, float oy, float sx, float sy, float rot,
                       unsigned inherit, int depth)
{
    char layer[DXF_NAME], name[DXF_NAME];
    int k, aci, truec, vis, papers, cols = 1, rows = 1, follow = 0;
    float wx, wy, wz, ix = 0, iy = 0, isx = 1, isy = 1, ir = 0, csp = 0, rsp = 0;
    unsigned col;
    const dblock *b;
    *next = j;
    read_common(x, i, j, layer, &aci, &truec, &vis, &papers, &wx, &wy, &wz);
    name[0] = 0;
    for (k = i + 1; k < j; k++) {
        if (x->p[k].code == 2) name_copy(name, x->p[k].v, -1);
        if (x->p[k].code == 10) pair_float(&x->p[k], &ix);
        if (x->p[k].code == 20) pair_float(&x->p[k], &iy);
        if (x->p[k].code == 41) pair_float(&x->p[k], &isx);
        if (x->p[k].code == 42) pair_float(&x->p[k], &isy);
        if (x->p[k].code == 50) pair_float(&x->p[k], &ir);
        if (x->p[k].code == 66) pair_int(&x->p[k], &follow);
        if (x->p[k].code == 70) pair_int(&x->p[k], &cols);
        if (x->p[k].code == 71) pair_int(&x->p[k], &rows);
        if (x->p[k].code == 44) pair_float(&x->p[k], &csp);
        if (x->p[k].code == 45) pair_float(&x->p[k], &rsp);
    }
    if (follow) {
        int t = j;
        while (t < end && !(x->p[t].code == 0 && ieq(x->p[t].v, "SEQEND"))) t++;
        if (t < end) *next = t + 1;
    }
    if (vis == 1 || papers == 1) return 0;
    if (layer_of(x, layer) && layer_of(x, layer)->off) return 0;
    if (depth >= DXF_MAX_INSERT) return 0;
    b = block_of(x, name);
    if (!b) return 0;
    col = colour_of(x, layer, aci, truec, inherit);
    if (cols < 1) cols = 1;
    if (rows < 1) rows = 1;
    {
        int c, r;
        for (r = 0; r < rows; r++)
            for (c = 0; c < cols; c++) {
                float px, py, tox, toy, tsx, tsy, trot;
                /* Insertion point is in the parent OCS, then the current transform. */
                xf_pt(ix + (float)c * csp, iy + (float)r * rsp, wx, wy, wz, ox, oy, sx, sy, rot, &px, &py);
                tox = px;
                toy = py;
                tsx = sx * isx;
                tsy = sy * isy;
                trot = rot + ir;
                if (emit_ents(x, b->from, b->to, tox, toy, tsx, tsy, trot, col, depth + 1)) return -1;
            }
    }
    return 0;
}

static int emit_ents(dxf *x, int from, int to, float ox, float oy, float sx, float sy, float rot, unsigned inherit,
                     int depth)
{
    int i = from;
    while (i < to) {
        int j;
        if (x->p[i].code != 0) {
            i++;
            continue;
        }
        j = entity_span(x, i, to);
        if (ieq(x->p[i].v, "LINE")) {
            if (emit_line(x, i, j, ox, oy, sx, sy, rot, inherit)) return -1;
            i = j;
        } else if (ieq(x->p[i].v, "CIRCLE")) {
            if (emit_arcish(x, i, j, 1, ox, oy, sx, sy, rot, inherit)) return -1;
            i = j;
        } else if (ieq(x->p[i].v, "ARC")) {
            if (emit_arcish(x, i, j, 0, ox, oy, sx, sy, rot, inherit)) return -1;
            i = j;
        } else if (ieq(x->p[i].v, "LWPOLYLINE")) {
            if (emit_lwpoly(x, i, j, ox, oy, sx, sy, rot, inherit)) return -1;
            i = j;
        } else if (ieq(x->p[i].v, "POLYLINE")) {
            int n = j;
            if (emit_poly(x, i, to, &n, ox, oy, sx, sy, rot, inherit)) return -1;
            i = n;
        } else if (ieq(x->p[i].v, "INSERT")) {
            int n = j;
            if (emit_insert(x, i, j, to, &n, ox, oy, sx, sy, rot, inherit, depth)) return -1;
            i = n;
        } else if (ieq(x->p[i].v, "SEQEND") || ieq(x->p[i].v, "ENDBLK") || ieq(x->p[i].v, "ENDSEC") ||
                   ieq(x->p[i].v, "EOF")) {
            i = j;
        } else {
            /* TEXT, MTEXT, HATCH, SPLINE, ELLIPSE, DIMENSION, and the rest: skip. */
            i = j;
        }
    }
    return 0;
}

static int parse_dxf(gv_doc *d, char *buf, int n, char *err, int errlen)
{
    dxf x;
    int from, to, ents_from, ents_to;
    memset(&x, 0, sizeof x);
    x.d = d;
    x.err = err;
    x.errlen = errlen;
    gv_doc_free(d);
    if (scan_pairs(buf, n, &x.p, &x.n, err, errlen)) return -1;
    if (x.n < 2) {
        free(x.p);
        fail_set(err, errlen, "not a DXF");
        return -1;
    }
    if (read_header(&x) || read_layers(&x) || read_blocks(&x)) {
        free(x.p);
        free(x.lay);
        free(x.blk);
        gv_doc_free(d);
        return -1;
    }
    if (find_section(&x, "ENTITIES", &from, &to)) {
        /* A bare entity list (no SECTION) is accepted. */
        ents_from = 0;
        ents_to = x.n;
    } else {
        ents_from = from;
        ents_to = to;
    }
    if (emit_ents(&x, ents_from, ents_to, 0.f, 0.f, 1.f, 1.f, 0.f, 0, 0)) {
        free(x.p);
        free(x.lay);
        free(x.blk);
        gv_doc_free(d);
        return -1;
    }
    if (d->n > 0) finish_path(d);
    free(x.p);
    free(x.lay);
    free(x.blk);
    if (d->n == 0) {
        fail_set(err, errlen, "no paths");
        gv_doc_free(d);
        return -1;
    }
    if (layers_build(d, err, errlen)) {
        gv_doc_free(d);
        return -1;
    }
    bounds_of(d);
    d->width = d->maxx - d->minx;
    d->height = d->maxy - d->miny;
    if (d->width < 0) d->width = 0;
    if (d->height < 0) d->height = 0;
    return 0;
}

static char *read_file(const char *path, int *n, char *err, int errlen)
{
    FILE *f;
    long len;
    char *buf;
    if (!path) {
        fail_set(err, errlen, "not a DXF");
        return NULL;
    }
    f = fopen(path, "rb");
    if (!f) {
        fail_set(err, errlen, "the file could not be opened");
        return NULL;
    }
    if (fseek(f, 0, SEEK_END) != 0 || (len = ftell(f)) < 0 || fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        fail_set(err, errlen, "the file could not be read");
        return NULL;
    }
    if (len > GV_MAX_FILE) {
        fclose(f);
        fail_set(err, errlen, "the file is too long");
        return NULL;
    }
    buf = (char *)malloc((size_t)len + 1);
    if (!buf) {
        fclose(f);
        fail_set(err, errlen, "out of memory");
        return NULL;
    }
    if (len && fread(buf, 1, (size_t)len, f) != (size_t)len) {
        free(buf);
        fclose(f);
        fail_set(err, errlen, "the file could not be read");
        return NULL;
    }
    fclose(f);
    buf[len] = 0;
    *n = (int)len;
    return buf;
}

int gv_load_dxf_text(gv_doc *d, const char *text, int n, char *err, int errlen)
{
    char *buf;
    int rc;
    if (!text) {
        fail_set(err, errlen, "not a DXF");
        gv_doc_free(d);
        return -1;
    }
    if (n < 0) n = (int)strlen(text);
    if (looks_binary_dxf(text, n)) {
        fail_set(err, errlen, "binary DXF is not read");
        gv_doc_free(d);
        return -1;
    }
    buf = (char *)malloc((size_t)n + 1);
    if (!buf) {
        fail_set(err, errlen, "out of memory");
        gv_doc_free(d);
        return -1;
    }
    memcpy(buf, text, (size_t)n);
    buf[n] = 0;
    rc = parse_dxf(d, buf, n, err, errlen);
    free(buf);
    return rc;
}

int gv_load_dxf(gv_doc *d, const char *path, char *err, int errlen)
{
    char *buf;
    int n, rc;
    buf = read_file(path, &n, err, errlen);
    if (!buf) {
        gv_doc_free(d);
        return -1;
    }
    if (looks_binary_dxf(buf, n)) {
        free(buf);
        fail_set(err, errlen, "binary DXF is not read");
        gv_doc_free(d);
        return -1;
    }
    rc = parse_dxf(d, buf, n, err, errlen);
    free(buf);
    return rc;
}

int gv_load_text(gv_doc *d, const char *text, int n, char *err, int errlen)
{
    if (!text) {
        fail_set(err, errlen, "not a drawing");
        gv_doc_free(d);
        return -1;
    }
    if (n < 0) n = (int)strlen(text);
    if (looks_binary_dxf(text, n)) {
        fail_set(err, errlen, "binary DXF is not read");
        gv_doc_free(d);
        return -1;
    }
    if (looks_dxf(text, n)) return gv_load_dxf_text(d, text, n, err, errlen);
    return gv_load_svg_text(d, text, n, err, errlen);
}

int gv_load(gv_doc *d, const char *path, char *err, int errlen)
{
    char *buf;
    int n, rc, is_dxf = 0;
    const char *dot;
    FILE *f;
    long len;
    if (!path) {
        fail_set(err, errlen, "not a drawing");
        gv_doc_free(d);
        return -1;
    }
    dot = strrchr(path, '.');
    if (dot && ieq(dot, ".dxf")) is_dxf = 1;
    f = fopen(path, "rb");
    if (!f) {
        fail_set(err, errlen, "the file could not be opened");
        gv_doc_free(d);
        return -1;
    }
    if (fseek(f, 0, SEEK_END) != 0 || (len = ftell(f)) < 0 || fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        fail_set(err, errlen, "the file could not be read");
        gv_doc_free(d);
        return -1;
    }
    if (len > GV_MAX_FILE) {
        fclose(f);
        fail_set(err, errlen, "the file is too long");
        gv_doc_free(d);
        return -1;
    }
    buf = (char *)malloc((size_t)len + 1);
    if (!buf) {
        fclose(f);
        fail_set(err, errlen, "out of memory");
        gv_doc_free(d);
        return -1;
    }
    if (len && fread(buf, 1, (size_t)len, f) != (size_t)len) {
        free(buf);
        fclose(f);
        fail_set(err, errlen, "the file could not be read");
        gv_doc_free(d);
        return -1;
    }
    fclose(f);
    buf[len] = 0;
    n = (int)len;
    if (looks_binary_dxf(buf, n)) {
        free(buf);
        fail_set(err, errlen, "binary DXF is not read");
        gv_doc_free(d);
        return -1;
    }
    if (is_dxf || looks_dxf(buf, n))
        rc = parse_dxf(d, buf, n, err, errlen);
    else
        rc = gv_load_svg_text(d, buf, n, err, errlen);
    free(buf);
    return rc;
}
