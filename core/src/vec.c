/* 2D vector drawings and an SVG reader (gs_vec.h). Written from the
 * W3C SVG 1.1 specification (paths, basic shapes, paint, coordinates
 * and the path grammar), never from another application's source. The
 * Amiga build is to be openmakevec.library; this is the C.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gs_vec.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GV_FLAT 0.05f
#define GV_PI 3.14159265f
#define GV_MAX_FILE (4 * 1024 * 1024)
#define GV_MAX_PT 1000000
#define GV_MIN 1e-7f

/* No ctype.h: its table is libnix's, and a library has no libnix. */
static int space(char c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f' || c == '\v';
}

static int letter(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static int digit(char c)
{
    return c >= '0' && c <= '9';
}

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

static int istart(const char *s, const char *pre, int n)
{
    int i;
    for (i = 0; pre[i]; i++) {
        if (i >= n || lower(s[i]) != lower(pre[i])) return 0;
    }
    return 1;
}

static int iname(const char *s, int n, const char *want)
{
    const char *c = memchr(s, ':', (size_t)n);
    if (c) {
        n -= (int)(c + 1 - s);
        s = c + 1;
    }
    if (n != (int)strlen(want)) return 0;
    return istart(s, want, n);
}

/* atan2 from a 5-term atan on [0, 1], for arc flattening. */
static float gv_atan1(float x)
{
    float x2 = x * x;
    return x * (0.9998660f + x2 * (-0.3302995f + x2 * (0.1801410f + x2 * (-0.0851330f + x2 * 0.0208351f))));
}

static float gv_atan2(float y, float x)
{
    float ax, ay, a;
    ax = x < 0 ? -x : x;
    ay = y < 0 ? -y : y;
    if (ax < GV_MIN && ay < GV_MIN) return 0;
    if (ay > ax)
        a = GV_PI * 0.5f - gv_atan1(ax / ay);
    else
        a = gv_atan1(ay / ax);
    if (x < 0) a = GV_PI - a;
    if (y < 0) a = -a;
    return a;
}

typedef struct {
    float a, b, c, d, e, f;
} mtx;

static mtx mtx_id(void)
{
    mtx m;
    m.a = 1;
    m.b = 0;
    m.c = 0;
    m.d = 1;
    m.e = 0;
    m.f = 0;
    return m;
}

/* p applied after q: p * q. */
static mtx mtx_mul(mtx p, mtx q)
{
    mtx r;
    r.a = p.a * q.a + p.c * q.b;
    r.b = p.b * q.a + p.d * q.b;
    r.c = p.a * q.c + p.c * q.d;
    r.d = p.b * q.c + p.d * q.d;
    r.e = p.a * q.e + p.c * q.f + p.e;
    r.f = p.b * q.e + p.d * q.f + p.f;
    return r;
}

static void mtx_apply(mtx m, float x, float y, float *ox, float *oy)
{
    *ox = m.a * x + m.c * y + m.e;
    *oy = m.b * x + m.d * y + m.f;
}

/* Mean of the two axis scales, for a stroke width. */
static float mtx_scale(mtx m)
{
    float sx = sqrtf(m.a * m.a + m.b * m.b), sy = sqrtf(m.c * m.c + m.d * m.d);
    return 0.5f * (sx + sy);
}

/* A CSS / SVG colour name from the public W3C list, as 0x00RRGGBB, or
 * (unsigned)-2 if it is not a name. "none" and "transparent" are GV_NONE. */
static unsigned colour_name(const char *s)
{
    static const struct {
        const char *n;
        unsigned c;
    } tab[] = {
        {"aliceblue", 0xf0f8ff},        {"antiquewhite", 0xfaebd7},
        {"aqua", 0x00ffff},             {"aquamarine", 0x7fffd4},
        {"azure", 0xf0ffff},            {"beige", 0xf5f5dc},
        {"bisque", 0xffe4c4},           {"black", 0x000000},
        {"blanchedalmond", 0xffebcd},   {"blue", 0x0000ff},
        {"blueviolet", 0x8a2be2},       {"brown", 0xa52a2a},
        {"burlywood", 0xdeb887},        {"cadetblue", 0x5f9ea0},
        {"chartreuse", 0x7fff00},       {"chocolate", 0xd2691e},
        {"coral", 0xff7f50},            {"cornflowerblue", 0x6495ed},
        {"cornsilk", 0xfff8dc},         {"crimson", 0xdc143c},
        {"cyan", 0x00ffff},             {"darkblue", 0x00008b},
        {"darkcyan", 0x008b8b},         {"darkgoldenrod", 0xb8860b},
        {"darkgray", 0xa9a9a9},         {"darkgreen", 0x006400},
        {"darkgrey", 0xa9a9a9},         {"darkkhaki", 0xbdb76b},
        {"darkmagenta", 0x8b008b},      {"darkolivegreen", 0x556b2f},
        {"darkorange", 0xff8c00},       {"darkorchid", 0x9932cc},
        {"darkred", 0x8b0000},          {"darksalmon", 0xe9967a},
        {"darkseagreen", 0x8fbc8f},     {"darkslateblue", 0x483d8b},
        {"darkslategray", 0x2f4f4f},    {"darkslategrey", 0x2f4f4f},
        {"darkturquoise", 0x00ced1},    {"darkviolet", 0x9400d3},
        {"deeppink", 0xff1493},         {"deepskyblue", 0x00bfff},
        {"dimgray", 0x696969},          {"dimgrey", 0x696969},
        {"dodgerblue", 0x1e90ff},       {"firebrick", 0xb22222},
        {"floralwhite", 0xfffaf0},      {"forestgreen", 0x228b22},
        {"fuchsia", 0xff00ff},          {"gainsboro", 0xdcdcdc},
        {"ghostwhite", 0xf8f8ff},       {"gold", 0xffd700},
        {"goldenrod", 0xdaa520},        {"gray", 0x808080},
        {"grey", 0x808080},             {"green", 0x008000},
        {"greenyellow", 0xadff2f},      {"honeydew", 0xf0fff0},
        {"hotpink", 0xff69b4},          {"indianred", 0xcd5c5c},
        {"indigo", 0x4b0082},           {"ivory", 0xfffff0},
        {"khaki", 0xf0e68c},            {"lavender", 0xe6e6fa},
        {"lavenderblush", 0xfff0f5},    {"lawngreen", 0x7cfc00},
        {"lemonchiffon", 0xfffacd},     {"lightblue", 0xadd8e6},
        {"lightcoral", 0xf08080},       {"lightcyan", 0xe0ffff},
        {"lightgoldenrodyellow", 0xfafad2}, {"lightgray", 0xd3d3d3},
        {"lightgreen", 0x90ee90},       {"lightgrey", 0xd3d3d3},
        {"lightpink", 0xffb6c1},        {"lightsalmon", 0xffa07a},
        {"lightseagreen", 0x20b2aa},    {"lightskyblue", 0x87cefa},
        {"lightslategray", 0x778899},   {"lightslategrey", 0x778899},
        {"lightsteelblue", 0xb0c4de},   {"lightyellow", 0xffffe0},
        {"lime", 0x00ff00},             {"limegreen", 0x32cd32},
        {"linen", 0xfaf0e6},            {"magenta", 0xff00ff},
        {"maroon", 0x800000},           {"mediumaquamarine", 0x66cdaa},
        {"mediumblue", 0x0000cd},       {"mediumorchid", 0xba55d3},
        {"mediumpurple", 0x9370db},     {"mediumseagreen", 0x3cb371},
        {"mediumslateblue", 0x7b68ee},  {"mediumspringgreen", 0x00fa9a},
        {"mediumturquoise", 0x48d1cc},  {"mediumvioletred", 0xc71585},
        {"midnightblue", 0x191970},     {"mintcream", 0xf5fffa},
        {"mistyrose", 0xffe4e1},        {"moccasin", 0xffe4b5},
        {"navajowhite", 0xffdead},      {"navy", 0x000080},
        {"oldlace", 0xfdf5e6},          {"olive", 0x808000},
        {"olivedrab", 0x6b8e23},        {"orange", 0xffa500},
        {"orangered", 0xff4500},        {"orchid", 0xda70d6},
        {"palegoldenrod", 0xeee8aa},    {"palegreen", 0x98fb98},
        {"paleturquoise", 0xafeeee},    {"palevioletred", 0xdb7093},
        {"papayawhip", 0xffefd5},       {"peachpuff", 0xffdab9},
        {"peru", 0xcd853f},             {"pink", 0xffc0cb},
        {"plum", 0xdda0dd},             {"powderblue", 0xb0e0e6},
        {"purple", 0x800080},           {"red", 0xff0000},
        {"rosybrown", 0xbc8f8f},        {"royalblue", 0x4169e1},
        {"saddlebrown", 0x8b4513},      {"salmon", 0xfa8072},
        {"sandybrown", 0xf4a460},       {"seagreen", 0x2e8b57},
        {"seashell", 0xfff5ee},         {"sienna", 0xa0522d},
        {"silver", 0xc0c0c0},           {"skyblue", 0x87ceeb},
        {"slateblue", 0x6a5acd},        {"slategray", 0x708090},
        {"slategrey", 0x708090},        {"snow", 0xfffafa},
        {"springgreen", 0x00ff7f},      {"steelblue", 0x4682b4},
        {"tan", 0xd2b48c},              {"teal", 0x008080},
        {"thistle", 0xd8bfd8},          {"tomato", 0xff6347},
        {"turquoise", 0x40e0d0},        {"violet", 0xee82ee},
        {"wheat", 0xf5deb3},            {"white", 0xffffff},
        {"whitesmoke", 0xf5f5f5},       {"yellow", 0xffff00},
        {"yellowgreen", 0x9acd32},
    };
    unsigned i;
    if (ieq(s, "none") || ieq(s, "transparent")) return GV_NONE;
    for (i = 0; i < sizeof tab / sizeof tab[0]; i++)
        if (ieq(s, tab[i].n)) return tab[i].c;
    return (unsigned)-2;
}

static int hexv(char c)
{
    if (digit(c)) return c - '0';
    c = lower(c);
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

/* A colour as #RGB, #RRGGBB, rgb() or a name. -1 if it is not a colour. */
static int parse_colour(const char *s, unsigned *out)
{
    int i, n, r, g, b, rv, gv, bv;
    unsigned named;
    char buf[64];
    if (!s) return -1;
    while (*s && space(*s)) s++;
    n = (int)strlen(s);
    while (n > 0 && space(s[n - 1])) n--;
    if (n <= 0 || n >= (int)sizeof buf) return -1;
    memcpy(buf, s, (size_t)n);
    buf[n] = 0;
    if (buf[0] == '#') {
        if (n == 4) {
            r = hexv(buf[1]);
            g = hexv(buf[2]);
            b = hexv(buf[3]);
            if (r < 0 || g < 0 || b < 0) return -1;
            *out = (unsigned)((r << 20) | (r << 16) | (g << 12) | (g << 8) | (b << 4) | b);
            return 0;
        }
        if (n == 7 || n == 9) {
            rv = gv = bv = 0;
            for (i = 0; i < 6; i++) {
                int h = hexv(buf[1 + i]);
                if (h < 0) return -1;
                if (i < 2) rv = rv * 16 + h;
                else if (i < 4) gv = gv * 16 + h;
                else bv = bv * 16 + h;
            }
            *out = ((unsigned)rv << 16) | ((unsigned)gv << 8) | (unsigned)bv;
            return 0;
        }
        return -1;
    }
    if (istart(buf, "rgb(", 4)) {
        const char *p = buf + 4;
        char *e;
        float vr, vg, vb, *vv[3];
        int k, pct;
        vv[0] = &vr;
        vv[1] = &vg;
        vv[2] = &vb;
        for (k = 0; k < 3; k++) {
            while (*p && space(*p)) p++;
            *vv[k] = strtof(p, &e);
            if (e == p) return -1;
            p = e;
            while (*p && space(*p)) p++;
            pct = (*p == '%');
            if (pct) p++;
            if (pct)
                *vv[k] = *vv[k] * 2.55f;
            if (k < 2) {
                if (*p == ',') p++;
            }
        }
        if (vr < 0) vr = 0;
        if (vg < 0) vg = 0;
        if (vb < 0) vb = 0;
        if (vr > 255) vr = 255;
        if (vg > 255) vg = 255;
        if (vb > 255) vb = 255;
        *out = ((unsigned)(vr + 0.5f) << 16) | ((unsigned)(vg + 0.5f) << 8) | (unsigned)(vb + 0.5f);
        return 0;
    }
    named = colour_name(buf);
    if (named != (unsigned)-2) {
        *out = named;
        return 0;
    }
    return -1;
}

/* A length in millimetres when it has a unit; a user unit when it has
 * none. percent is 0..1 of ref (ref is already in the same unit as a
 * number with no unit, or in mm when the value had a unit - the caller
 * passes the viewport size in user units for '%'). */
static int parse_len(const char *s, float ref, float *out)
{
    char *e;
    float v;
    const char *p;
    if (!s) return -1;
    while (*s && space(*s)) s++;
    v = strtof(s, &e);
    if (e == s) return -1;
    p = e;
    while (*p && space(*p)) p++;
    if (*p == 0) {
        *out = v;
        return 0;
    }
    if (*p == '%') {
        *out = v * 0.01f * ref;
        return 0;
    }
    if (istart(p, "mm", 2) && p[2] == 0) {
        *out = v;
        return 1;
    }
    if (istart(p, "cm", 2) && p[2] == 0) {
        *out = v * 10.f;
        return 1;
    }
    if (istart(p, "in", 2) && p[2] == 0) {
        *out = v * 25.4f;
        return 1;
    }
    if (istart(p, "pt", 2) && p[2] == 0) {
        *out = v * (25.4f / 72.f);
        return 1;
    }
    if (istart(p, "pc", 2) && p[2] == 0) {
        *out = v * (25.4f / 6.f);
        return 1;
    }
    if (istart(p, "px", 2) && p[2] == 0) {
        *out = v * (25.4f / 96.f);
        return 1;
    }
    return -1;
}

#define PX (25.4f / 96.f)

static const char *skip_sp(const char *s, const char *e)
{
    while (s < e && space(*s)) s++;
    return s;
}

static const char *skip_misc(const char *s, const char *e)
{
    for (;;) {
        s = skip_sp(s, e);
        if (s + 3 < e && s[0] == '<' && s[1] == '!' && s[2] == '-' && s[3] == '-') {
            s += 4;
            while (s + 2 < e && !(s[0] == '-' && s[1] == '-' && s[2] == '>')) s++;
            if (s + 2 < e) s += 3;
            continue;
        }
        if (s + 1 < e && s[0] == '<' && s[1] == '?') {
            s += 2;
            while (s + 1 < e && !(s[0] == '?' && s[1] == '>')) s++;
            if (s + 1 < e) s += 2;
            continue;
        }
        if (s + 1 < e && s[0] == '<' && s[1] == '!' ) {
            s += 2;
            while (s < e && *s != '>') s++;
            if (s < e) s++;
            continue;
        }
        return s;
    }
}

/* Unescape a copied attribute into dst (dst has room for n + 1). */
static void unescape(const char *s, int n, char *dst)
{
    int i = 0, o = 0;
    while (i < n) {
        if (s[i] == '&') {
            if (i + 4 <= n && istart(s + i, "&amp;", 5)) {
                dst[o++] = '&';
                i += 5;
                continue;
            }
            if (i + 3 <= n && istart(s + i, "&lt;", 4)) {
                dst[o++] = '<';
                i += 4;
                continue;
            }
            if (i + 3 <= n && istart(s + i, "&gt;", 4)) {
                dst[o++] = '>';
                i += 4;
                continue;
            }
            if (i + 5 <= n && istart(s + i, "&quot;", 6)) {
                dst[o++] = '"';
                i += 6;
                continue;
            }
            if (i + 5 <= n && istart(s + i, "&apos;", 6)) {
                dst[o++] = '\'';
                i += 6;
                continue;
            }
            if (i + 2 < n && s[i + 1] == '#') {
                unsigned v = 0;
                int j = i + 2, hex = 0;
                if (j < n && (s[j] == 'x' || s[j] == 'X')) {
                    hex = 1;
                    j++;
                }
                while (j < n) {
                    int h;
                    if (hex) {
                        h = hexv(s[j]);
                        if (h < 0) break;
                        v = v * 16u + (unsigned)h;
                    } else {
                        if (!digit(s[j])) break;
                        v = v * 10u + (unsigned)(s[j] - '0');
                    }
                    j++;
                }
                if (j < n && s[j] == ';') {
                    dst[o++] = v < 128 ? (char)v : '?';
                    i = j + 1;
                    continue;
                }
            }
        }
        dst[o++] = s[i++];
    }
    dst[o] = 0;
}

typedef struct {
    const char *name;
    int nname;
    char *val;
} attr;

typedef struct {
    attr *a;
    int n, cap;
} attrs;

static void attrs_free(attrs *as)
{
    int i;
    for (i = 0; i < as->n; i++)
        free(as->a[i].val);
    free(as->a);
    as->a = NULL;
    as->n = as->cap = 0;
}

static const char *attr_get(const attrs *as, const char *name)
{
    int i;
    for (i = 0; i < as->n; i++)
        if (iname(as->a[i].name, as->a[i].nname, name)) return as->a[i].val;
    return NULL;
}

static int attrs_add(attrs *as, const char *name, int nname, const char *val, int nval)
{
    attr *p;
    if (as->n == as->cap) {
        int cap = as->cap ? as->cap * 2 : 8;
        p = (attr *)realloc(as->a, (size_t)cap * sizeof *p);
        if (!p) return -1;
        as->a = p;
        as->cap = cap;
    }
    p = &as->a[as->n];
    p->name = name;
    p->nname = nname;
    p->val = (char *)malloc((size_t)nval + 1);
    if (!p->val) return -1;
    unescape(val, nval, p->val);
    as->n++;
    return 0;
}

typedef struct {
    unsigned stroke, fill;
    float width;
} paint;

static paint paint_root(void)
{
    paint p;
    p.stroke = GV_NONE;
    p.fill = 0x000000;
    p.width = 1;
    return p;
}

static void fail_set(char *err, int errlen, const char *why)
{
    if (err && errlen > 0) snprintf(err, (size_t)errlen, "%s", why);
}

static int doc_add_path(gv_doc *d, int closed, unsigned stroke, unsigned fill, float width)
{
    gv_path *p;
    if (d->n == d->cap) {
        int cap = d->cap ? d->cap * 2 : 8;
        p = (gv_path *)realloc(d->p, (size_t)cap * sizeof *p);
        if (!p) return -1;
        d->p = p;
        d->cap = cap;
    }
    p = &d->p[d->n];
    memset(p, 0, sizeof *p);
    p->closed = closed;
    p->stroke = stroke;
    p->fill = fill;
    p->width = width;
    d->n++;
    return 0;
}

static int doc_add_pt(gv_doc *d, float x, float y)
{
    gv_path *p;
    float *xy;
    int cap;
    if (d->n <= 0) return -1;
    p = &d->p[d->n - 1];
    if (p->n > 0) {
        float *l = p->xy + (p->n - 1) * 2;
        float dx = x - l[0], dy = y - l[1];
        if (dx * dx + dy * dy < GV_MIN * GV_MIN) return 0;
    }
    cap = p->n; /* not stored; realloc from n */
    /* grow: use a hidden cap packed? we realloc to n+1 each power of 2
     * via a convention: first alloc 8, then double, stored not in the
     * struct. Use realloc every time if we don't have cap... we add a
     * local cap by reallocing to the next power when n is 0,8,16... */
    if (p->n == 0)
        cap = 8;
    else if (p->n >= 8 && (p->n & (p->n - 1)) == 0)
        cap = p->n * 2;
    else
        cap = 0;
    if (cap) {
        xy = (float *)realloc(p->xy, (size_t)cap * 2 * sizeof(float));
        if (!xy) return -1;
        p->xy = xy;
    }
    if (!p->xy) return -1;
    p->xy[p->n * 2] = x;
    p->xy[p->n * 2 + 1] = y;
    p->n++;
    return 0;
}

static int emit_pt(gv_doc *d, mtx m, float x, float y, char *err, int errlen)
{
    float ox, oy;
    int total = 0, i;
    mtx_apply(m, x, y, &ox, &oy);
    for (i = 0; i < d->n; i++)
        total += d->p[i].n;
    if (total >= GV_MAX_PT) {
        fail_set(err, errlen, "too many points");
        return -1;
    }
    if (doc_add_pt(d, ox, oy)) {
        fail_set(err, errlen, "out of memory");
        return -1;
    }
    return 0;
}

/* Drop the last path if it has too few points. */
static void finish_path(gv_doc *d)
{
    gv_path *p;
    if (d->n <= 0) return;
    p = &d->p[d->n - 1];
    if (p->n < 2 || (p->closed && p->n < 3)) {
        free(p->xy);
        d->n--;
    }
}

static int begin_path(gv_doc *d, int closed, paint p, mtx m, char *err, int errlen)
{
    float w;
    if (d->n > 0) finish_path(d);
    if (p.stroke == GV_NONE && p.fill == GV_NONE) return 0;
    w = p.stroke == GV_NONE ? 0.f : p.width * mtx_scale(m);
    if (doc_add_path(d, closed, p.stroke, p.fill, w)) {
        fail_set(err, errlen, "out of memory");
        return -1;
    }
    return 0;
}

/* ---- path flattening ---- */

static int line_to(gv_doc *d, mtx m, float x, float y, char *err, int errlen)
{
    return emit_pt(d, m, x, y, err, errlen);
}

static float dist2(float x0, float y0, float x1, float y1)
{
    float dx = x1 - x0, dy = y1 - y0;
    return dx * dx + dy * dy;
}

/* Distance of q from the segment a-b, squared, enough for the flat test. */
static float pt_seg2(float qx, float qy, float ax, float ay, float bx, float by)
{
    float abx = bx - ax, aby = by - ay, t, px, py, den;
    den = abx * abx + aby * aby;
    if (den < GV_MIN * GV_MIN) return dist2(qx, qy, ax, ay);
    t = ((qx - ax) * abx + (qy - ay) * aby) / den;
    if (t < 0) t = 0;
    if (t > 1) t = 1;
    px = ax + t * abx;
    py = ay + t * aby;
    return dist2(qx, qy, px, py);
}

static int cubic_flat(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3,
                      float flat2)
{
    return pt_seg2(x1, y1, x0, y0, x3, y3) <= flat2 && pt_seg2(x2, y2, x0, y0, x3, y3) <= flat2;
}

static int cubic_to(gv_doc *d, mtx m, float x0, float y0, float x1, float y1, float x2, float y2,
                    float x3, float y3, int depth, char *err, int errlen)
{
    float flat2 = GV_FLAT * GV_FLAT;
    if (depth > 12 || cubic_flat(x0, y0, x1, y1, x2, y2, x3, y3, flat2) ||
        dist2(x0, y0, x3, y3) <= flat2)
        return line_to(d, m, x3, y3, err, errlen);
    {
        float ax = 0.5f * (x0 + x1), ay = 0.5f * (y0 + y1);
        float bx = 0.5f * (x1 + x2), by = 0.5f * (y1 + y2);
        float cx = 0.5f * (x2 + x3), cy = 0.5f * (y2 + y3);
        float dx = 0.5f * (ax + bx), dy = 0.5f * (ay + by);
        float ex = 0.5f * (bx + cx), ey = 0.5f * (by + cy);
        float fx = 0.5f * (dx + ex), fy = 0.5f * (dy + ey);
        if (cubic_to(d, m, x0, y0, ax, ay, dx, dy, fx, fy, depth + 1, err, errlen)) return -1;
        return cubic_to(d, m, fx, fy, ex, ey, cx, cy, x3, y3, depth + 1, err, errlen);
    }
}

static int quad_to(gv_doc *d, mtx m, float x0, float y0, float x1, float y1, float x2, float y2,
                   int depth, char *err, int errlen)
{
    /* Raise to a cubic: c1 = p0 + 2/3 (p1-p0), c2 = p2 + 2/3 (p1-p2). */
    float c1x = x0 + (2.f / 3.f) * (x1 - x0), c1y = y0 + (2.f / 3.f) * (y1 - y0);
    float c2x = x2 + (2.f / 3.f) * (x1 - x2), c2y = y2 + (2.f / 3.f) * (y1 - y2);
    (void)depth;
    return cubic_to(d, m, x0, y0, c1x, c1y, c2x, c2y, x2, y2, 0, err, errlen);
}

/* SVG arc, from the W3C implementation notes (endpoint to centre). */
static int arc_to(gv_doc *d, mtx m, float x1, float y1, float rx, float ry, float phi_deg, int large,
                  int sweep, float x2, float y2, char *err, int errlen)
{
    float phi, cosphi, sinphi, dx, dy, x1p, y1p, rxs, rys, lam, cxp, cyp, cx, cy, theta1, dtheta, n, a;
    int i, segs;
    if (rx < 0) rx = -rx;
    if (ry < 0) ry = -ry;
    if (rx < GV_MIN || ry < GV_MIN) return line_to(d, m, x2, y2, err, errlen);
    if (dist2(x1, y1, x2, y2) < GV_MIN * GV_MIN) return 0;
    phi = phi_deg * GV_PI / 180.f;
    cosphi = cosf(phi);
    sinphi = sinf(phi);
    dx = (x1 - x2) * 0.5f;
    dy = (y1 - y2) * 0.5f;
    x1p = cosphi * dx + sinphi * dy;
    y1p = -sinphi * dx + cosphi * dy;
    rxs = rx * rx;
    rys = ry * ry;
    lam = (x1p * x1p) / rxs + (y1p * y1p) / rys;
    if (lam > 1) {
        float s = sqrtf(lam);
        rx *= s;
        ry *= s;
        rxs = rx * rx;
        rys = ry * ry;
    }
    {
        float num = rxs * rys - rxs * y1p * y1p - rys * x1p * x1p;
        float den = rxs * y1p * y1p + rys * x1p * x1p;
        float q = den > GV_MIN ? num / den : 0;
        if (q < 0) q = 0;
        q = sqrtf(q);
        if (large == sweep) q = -q;
        cxp = q * rx * y1p / ry;
        cyp = -q * ry * x1p / rx;
    }
    cx = cosphi * cxp - sinphi * cyp + (x1 + x2) * 0.5f;
    cy = sinphi * cxp + cosphi * cyp + (y1 + y2) * 0.5f;
    {
        float ux = (x1p - cxp) / rx, uy = (y1p - cyp) / ry;
        float vx = (-x1p - cxp) / rx, vy = (-y1p - cyp) / ry;
        float n1 = sqrtf(ux * ux + uy * uy), n2;
        theta1 = gv_atan2(uy, ux);
        if (n1 > GV_MIN) {
            /* already atan2 */
            (void)n1;
        }
        n2 = sqrtf((ux * ux + uy * uy) * (vx * vx + vy * vy));
        dtheta = gv_atan2(ux * vy - uy * vx, ux * vx + uy * vy);
        if (!sweep && dtheta > 0) dtheta -= 2.f * GV_PI;
        if (sweep && dtheta < 0) dtheta += 2.f * GV_PI;
        (void)n2;
    }
    {
        float rmax = rx > ry ? rx : ry;
        float th = rmax > GV_FLAT ? (GV_PI * sqrtf(rmax / (2.f * GV_FLAT))) : 8.f;
        n = fabsf(dtheta) / GV_PI * th;
    }
    segs = (int)ceilf(n);
    if (segs < 2) segs = 2;
    if (segs > 512) segs = 512;
    for (i = 1; i <= segs; i++) {
        float t = theta1 + dtheta * ((float)i / (float)segs);
        float xx = cx + rx * cosf(t) * cosphi - ry * sinf(t) * sinphi;
        float yy = cy + rx * cosf(t) * sinphi + ry * sinf(t) * cosphi;
        if (line_to(d, m, xx, yy, err, errlen)) return -1;
        a = t;
    }
    (void)a;
    return 0;
}

static const char *skip_path_sep(const char *s, const char *e)
{
    s = skip_sp(s, e);
    if (s < e && *s == ',') s++;
    return skip_sp(s, e);
}

static int path_num(const char **ps, const char *e, float *out)
{
    const char *s = skip_path_sep(*ps, e);
    char *end;
    if (s >= e) return -1;
    if (!digit(*s) && *s != '.' && *s != '+' && *s != '-') return -1;
    *out = strtof(s, &end);
    if (end == s || end > e) return -1;
    *ps = end;
    return 0;
}

static int path_flag(const char **ps, const char *e, int *out)
{
    const char *s = skip_path_sep(*ps, e);
    if (s >= e || (*s != '0' && *s != '1')) return -1;
    *out = *s - '0';
    *ps = s + 1;
    return 0;
}

static int parse_d(gv_doc *d, const char *ds, paint pn, mtx m, char *err, int errlen)
{
    const char *s = ds, *e;
    char cmd = 0, prev = 0;
    float cx = 0, cy = 0, sx = 0, sy = 0, ox = 0, oy = 0, lastcx = 0, lastcy = 0;
    int started = 0, need_new = 0;
    if (!ds) return 0;
    e = ds + strlen(ds);
    while (s < e) {
        s = skip_path_sep(s, e);
        if (s >= e) break;
        if (letter(*s)) {
            cmd = *s;
            s++;
        } else if (!cmd) {
            fail_set(err, errlen, "a path has no command");
            return -1;
        }
        /* implicit lineto after a move, and repeat of the last command */
        {
            char use = cmd;
            if ((cmd == 'M' || cmd == 'm') && started && prev && (prev == 'M' || prev == 'm'))
                use = (cmd == 'M') ? 'L' : 'l';
            /* The first pair after M is a move; further pairs are lines.
             * We handle that after the first M pair by setting cmd to L/l. */
            if (need_new && use != 'M' && use != 'm' && use != 'Z' && use != 'z') {
                if (begin_path(d, 0, pn, m, err, errlen)) return -1;
                if (pn.stroke != GV_NONE || pn.fill != GV_NONE) {
                    if (d->n && line_to(d, m, cx, cy, err, errlen)) return -1;
                }
                need_new = 0;
                started = 1;
            }
            if (use == 'M' || use == 'm') need_new = 0;
            switch (use) {
            case 'M':
            case 'm': {
                float x, y;
                if (path_num(&s, e, &x) || path_num(&s, e, &y)) {
                    fail_set(err, errlen, "a path move is not two numbers");
                    return -1;
                }
                if (use == 'm') {
                    x += cx;
                    y += cy;
                }
                if (begin_path(d, 0, pn, m, err, errlen)) return -1;
                started = 1;
                cx = sx = x;
                cy = sy = y;
                lastcx = cx;
                lastcy = cy;
                if (pn.stroke != GV_NONE || pn.fill != GV_NONE) {
                    if (d->n == 0) {
                        /* paint was none, begin_path added nothing */
                    } else if (line_to(d, m, cx, cy, err, errlen))
                        return -1;
                }
                cmd = (use == 'm') ? 'l' : 'L';
                prev = use;
                break;
            }
            case 'L':
            case 'l': {
                float x, y;
                if (path_num(&s, e, &x) || path_num(&s, e, &y)) {
                    fail_set(err, errlen, "a path line is not two numbers");
                    return -1;
                }
                if (use == 'l') {
                    x += cx;
                    y += cy;
                }
                cx = x;
                cy = y;
                lastcx = cx;
                lastcy = cy;
                if (line_to(d, m, cx, cy, err, errlen)) return -1;
                prev = use;
                break;
            }
            case 'H':
            case 'h': {
                float x;
                if (path_num(&s, e, &x)) {
                    fail_set(err, errlen, "a path H is not a number");
                    return -1;
                }
                if (use == 'h') x += cx;
                cx = x;
                lastcx = cx;
                lastcy = cy;
                if (line_to(d, m, cx, cy, err, errlen)) return -1;
                prev = use;
                break;
            }
            case 'V':
            case 'v': {
                float y;
                if (path_num(&s, e, &y)) {
                    fail_set(err, errlen, "a path V is not a number");
                    return -1;
                }
                if (use == 'v') y += cy;
                cy = y;
                lastcx = cx;
                lastcy = cy;
                if (line_to(d, m, cx, cy, err, errlen)) return -1;
                prev = use;
                break;
            }
            case 'C':
            case 'c': {
                float x1, y1, x2, y2, x, y;
                if (path_num(&s, e, &x1) || path_num(&s, e, &y1) || path_num(&s, e, &x2) ||
                    path_num(&s, e, &y2) || path_num(&s, e, &x) || path_num(&s, e, &y)) {
                    fail_set(err, errlen, "a path cubic is not six numbers");
                    return -1;
                }
                if (use == 'c') {
                    x1 += cx;
                    y1 += cy;
                    x2 += cx;
                    y2 += cy;
                    x += cx;
                    y += cy;
                }
                if (cubic_to(d, m, cx, cy, x1, y1, x2, y2, x, y, 0, err, errlen)) return -1;
                lastcx = x2;
                lastcy = y2;
                cx = x;
                cy = y;
                prev = use;
                break;
            }
            case 'S':
            case 's': {
                float x1, y1, x2, y2, x, y;
                if (path_num(&s, e, &x2) || path_num(&s, e, &y2) || path_num(&s, e, &x) ||
                    path_num(&s, e, &y)) {
                    fail_set(err, errlen, "a path smooth cubic is not four numbers");
                    return -1;
                }
                if (prev == 'C' || prev == 'c' || prev == 'S' || prev == 's') {
                    x1 = 2 * cx - lastcx;
                    y1 = 2 * cy - lastcy;
                } else {
                    x1 = cx;
                    y1 = cy;
                }
                if (use == 's') {
                    x2 += cx;
                    y2 += cy;
                    x += cx;
                    y += cy;
                }
                if (cubic_to(d, m, cx, cy, x1, y1, x2, y2, x, y, 0, err, errlen)) return -1;
                lastcx = x2;
                lastcy = y2;
                cx = x;
                cy = y;
                prev = use;
                break;
            }
            case 'Q':
            case 'q': {
                float x1, y1, x, y;
                if (path_num(&s, e, &x1) || path_num(&s, e, &y1) || path_num(&s, e, &x) ||
                    path_num(&s, e, &y)) {
                    fail_set(err, errlen, "a path quadratic is not four numbers");
                    return -1;
                }
                if (use == 'q') {
                    x1 += cx;
                    y1 += cy;
                    x += cx;
                    y += cy;
                }
                if (quad_to(d, m, cx, cy, x1, y1, x, y, 0, err, errlen)) return -1;
                lastcx = x1;
                lastcy = y1;
                cx = x;
                cy = y;
                prev = use;
                break;
            }
            case 'T':
            case 't': {
                float x1, y1, x, y;
                if (path_num(&s, e, &x) || path_num(&s, e, &y)) {
                    fail_set(err, errlen, "a path smooth quadratic is not two numbers");
                    return -1;
                }
                if (prev == 'Q' || prev == 'q' || prev == 'T' || prev == 't') {
                    x1 = 2 * cx - lastcx;
                    y1 = 2 * cy - lastcy;
                } else {
                    x1 = cx;
                    y1 = cy;
                }
                if (use == 't') {
                    x += cx;
                    y += cy;
                }
                if (quad_to(d, m, cx, cy, x1, y1, x, y, 0, err, errlen)) return -1;
                lastcx = x1;
                lastcy = y1;
                cx = x;
                cy = y;
                prev = use;
                break;
            }
            case 'A':
            case 'a': {
                float rx, ry, phi, x, y;
                int large, sweep;
                if (path_num(&s, e, &rx) || path_num(&s, e, &ry) || path_num(&s, e, &phi) ||
                    path_flag(&s, e, &large) || path_flag(&s, e, &sweep) || path_num(&s, e, &x) ||
                    path_num(&s, e, &y)) {
                    fail_set(err, errlen, "a path arc is not seven numbers");
                    return -1;
                }
                if (use == 'a') {
                    x += cx;
                    y += cy;
                }
                if (arc_to(d, m, cx, cy, rx, ry, phi, large, sweep, x, y, err, errlen)) return -1;
                lastcx = cx = x;
                lastcy = cy = y;
                prev = use;
                break;
            }
            case 'Z':
            case 'z': {
                if (d->n > 0 && d->p[d->n - 1].n >= 2) {
                    d->p[d->n - 1].closed = 1;
                }
                cx = sx;
                cy = sy;
                lastcx = cx;
                lastcy = cy;
                prev = use;
                /* A following command starts a new subpath. */
                started = 0;
                need_new = 1;
                cmd = 0;
                break;
            }
            default:
                fail_set(err, errlen, "a path command is not known");
                return -1;
            }
        }
        (void)ox;
        (void)oy;
    }
    if (d->n > 0) finish_path(d);
    return 0;
}

/* ---- transforms ---- */

static const char *tf_num(const char *s, const char *e, float *out)
{
    char *end;
    s = skip_path_sep(s, e);
    if (s >= e) return NULL;
    *out = strtof(s, &end);
    if (end == s || end > e) return NULL;
    return end;
}

static int parse_transform(const char *s, mtx *out)
{
    mtx acc = mtx_id();
    const char *e;
    if (!s) {
        *out = acc;
        return 0;
    }
    e = s + strlen(s);
    while (s < e) {
        mtx t = mtx_id();
        const char *name, *q;
        int n;
        float a, b, c, d, ee, f;
        s = skip_sp(s, e);
        if (s >= e) break;
        if (*s == ',') {
            s++;
            continue;
        }
        name = s;
        while (s < e && letter(*s)) s++;
        n = (int)(s - name);
        s = skip_sp(s, e);
        if (s >= e || *s != '(') return -1;
        s++;
        if (iname(name, n, "matrix")) {
            if (!(s = tf_num(s, e, &a)) || !(s = tf_num(s, e, &b)) || !(s = tf_num(s, e, &c)) ||
                !(s = tf_num(s, e, &d)) || !(s = tf_num(s, e, &ee)) || !(s = tf_num(s, e, &f)))
                return -1;
            t.a = a;
            t.b = b;
            t.c = c;
            t.d = d;
            t.e = ee;
            t.f = f;
        } else if (iname(name, n, "translate")) {
            if (!(s = tf_num(s, e, &a))) return -1;
            q = tf_num(s, e, &b);
            if (q)
                s = q;
            else
                b = 0;
            t.e = a;
            t.f = b;
        } else if (iname(name, n, "scale")) {
            if (!(s = tf_num(s, e, &a))) return -1;
            q = tf_num(s, e, &b);
            if (q)
                s = q;
            else
                b = a;
            t.a = a;
            t.d = b;
        } else if (iname(name, n, "rotate")) {
            float ang, ca, sa, cx = 0, cy = 0;
            if (!(s = tf_num(s, e, &a))) return -1;
            q = tf_num(s, e, &cx);
            if (q) {
                s = q;
                if (!(s = tf_num(s, e, &cy))) return -1;
            }
            ang = a * GV_PI / 180.f;
            ca = cosf(ang);
            sa = sinf(ang);
            if (q) {
                mtx r = mtx_id(), to, back;
                to = mtx_id();
                to.e = cx;
                to.f = cy;
                back = mtx_id();
                back.e = -cx;
                back.f = -cy;
                r.a = ca;
                r.b = sa;
                r.c = -sa;
                r.d = ca;
                t = mtx_mul(to, mtx_mul(r, back));
            } else {
                t.a = ca;
                t.b = sa;
                t.c = -sa;
                t.d = ca;
            }
        } else if (iname(name, n, "skewX")) {
            if (!(s = tf_num(s, e, &a))) return -1;
            t.c = sinf(a * GV_PI / 180.f) / cosf(a * GV_PI / 180.f);
        } else if (iname(name, n, "skewY")) {
            if (!(s = tf_num(s, e, &a))) return -1;
            t.b = sinf(a * GV_PI / 180.f) / cosf(a * GV_PI / 180.f);
        } else
            return -1;
        s = skip_sp(s, e);
        if (s >= e || *s != ')') return -1;
        s++;
        acc = mtx_mul(acc, t);
    }
    *out = acc;
    return 0;
}

/* Apply presentation and a style="" list to p. */
static void apply_paint(paint *p, const char *name, const char *val)
{
    unsigned c;
    float w;
    if (!name || !val) return;
    if (ieq(name, "fill")) {
        if (parse_colour(val, &c) == 0) p->fill = c;
    } else if (ieq(name, "stroke")) {
        if (parse_colour(val, &c) == 0) p->stroke = c;
    } else if (ieq(name, "stroke-width")) {
        if (parse_len(val, 1, &w) >= 0) p->width = w;
    }
}

static void apply_style(paint *p, mtx *xf, const char *style)
{
    char name[64], val[256];
    const char *s = style;
    if (!s) return;
    while (*s) {
        const char *n, *v, *col, *end;
        int nn, nv, i;
        while (*s && (space(*s) || *s == ';')) s++;
        if (!*s) break;
        n = s;
        while (*s && *s != ':' && *s != ';') s++;
        nn = (int)(s - n);
        while (nn > 0 && space(n[nn - 1])) nn--;
        if (*s != ':') {
            while (*s && *s != ';') s++;
            continue;
        }
        s++;
        while (*s && space(*s)) s++;
        v = s;
        while (*s && *s != ';') s++;
        nv = (int)(s - v);
        while (nv > 0 && space(v[nv - 1])) nv--;
        if (nn <= 0 || nv <= 0 || nn >= (int)sizeof name || nv >= (int)sizeof val) continue;
        for (i = 0; i < nn; i++)
            name[i] = n[i];
        name[nn] = 0;
        memcpy(val, v, (size_t)nv);
        val[nv] = 0;
        if (ieq(name, "transform")) {
            mtx t;
            if (parse_transform(val, &t) == 0 && xf) *xf = mtx_mul(*xf, t);
        } else
            apply_paint(p, name, val);
        (void)col;
        (void)end;
    }
}

static void apply_attrs(paint *p, mtx *xf, const attrs *as)
{
    const char *v;
    mtx t;
    v = attr_get(as, "transform");
    if (v && parse_transform(v, &t) == 0) *xf = mtx_mul(*xf, t);
    v = attr_get(as, "fill");
    if (v) apply_paint(p, "fill", v);
    v = attr_get(as, "stroke");
    if (v) apply_paint(p, "stroke", v);
    v = attr_get(as, "stroke-width");
    if (v) apply_paint(p, "stroke-width", v);
    v = attr_get(as, "style");
    if (v) apply_style(p, xf, v);
}

/* ---- shapes ---- */

static int emit_poly(gv_doc *d, const char *pts, int closed, paint pn, mtx m, char *err, int errlen)
{
    const char *s, *e;
    int n = 0;
    float x, y, firstx = 0, firsty = 0;
    if (!pts) return 0;
    if (begin_path(d, closed, pn, m, err, errlen)) return -1;
    if (d->n == 0) return 0;
    s = pts;
    e = pts + strlen(pts);
    while (s < e) {
        if (path_num(&s, e, &x) || path_num(&s, e, &y)) break;
        if (n == 0) {
            firstx = x;
            firsty = y;
        }
        if (line_to(d, m, x, y, err, errlen)) return -1;
        n++;
    }
    (void)firstx;
    (void)firsty;
    if (d->n > 0) {
        if (closed && n >= 3) d->p[d->n - 1].closed = 1;
        finish_path(d);
    }
    return 0;
}

static int emit_rect(gv_doc *d, const attrs *as, paint pn, mtx m, char *err, int errlen)
{
    float x = 0, y = 0, w = 0, h = 0, rx = 0, ry = 0;
    const char *v;
    int have_rx = 0, have_ry = 0;
    v = attr_get(as, "x");
    if (v) parse_len(v, 0, &x);
    v = attr_get(as, "y");
    if (v) parse_len(v, 0, &y);
    v = attr_get(as, "width");
    if (v) parse_len(v, 0, &w);
    v = attr_get(as, "height");
    if (v) parse_len(v, 0, &h);
    v = attr_get(as, "rx");
    if (v && parse_len(v, 0, &rx) >= 0) have_rx = 1;
    v = attr_get(as, "ry");
    if (v && parse_len(v, 0, &ry) >= 0) have_ry = 1;
    if (w <= 0 || h <= 0) return 0;
    if (have_rx && !have_ry) ry = rx;
    if (have_ry && !have_rx) rx = ry;
    if (rx < 0) rx = 0;
    if (ry < 0) ry = 0;
    if (rx > w * 0.5f) rx = w * 0.5f;
    if (ry > h * 0.5f) ry = h * 0.5f;
    if (rx < GV_MIN || ry < GV_MIN) {
        if (begin_path(d, 1, pn, m, err, errlen)) return -1;
        if (d->n == 0) return 0;
        if (line_to(d, m, x, y, err, errlen) || line_to(d, m, x + w, y, err, errlen) ||
            line_to(d, m, x + w, y + h, err, errlen) || line_to(d, m, x, y + h, err, errlen))
            return -1;
        d->p[d->n - 1].closed = 1;
        finish_path(d);
        return 0;
    }
    /* Rounded: start at (x+rx, y), go around clockwise in SVG Y-down
     * (which is fine; the view matrix flips Y). */
    if (begin_path(d, 1, pn, m, err, errlen)) return -1;
    if (d->n == 0) return 0;
    if (line_to(d, m, x + rx, y, err, errlen)) return -1;
    if (line_to(d, m, x + w - rx, y, err, errlen)) return -1;
    if (arc_to(d, m, x + w - rx, y, rx, ry, 0, 0, 1, x + w, y + ry, err, errlen)) return -1;
    if (line_to(d, m, x + w, y + h - ry, err, errlen)) return -1;
    if (arc_to(d, m, x + w, y + h - ry, rx, ry, 0, 0, 1, x + w - rx, y + h, err, errlen)) return -1;
    if (line_to(d, m, x + rx, y + h, err, errlen)) return -1;
    if (arc_to(d, m, x + rx, y + h, rx, ry, 0, 0, 1, x, y + h - ry, err, errlen)) return -1;
    if (line_to(d, m, x, y + ry, err, errlen)) return -1;
    if (arc_to(d, m, x, y + ry, rx, ry, 0, 0, 1, x + rx, y, err, errlen)) return -1;
    d->p[d->n - 1].closed = 1;
    finish_path(d);
    return 0;
}

static int emit_circle(gv_doc *d, const attrs *as, paint pn, mtx m, char *err, int errlen)
{
    float cx = 0, cy = 0, r = 0;
    const char *v;
    int i, segs;
    v = attr_get(as, "cx");
    if (v) parse_len(v, 0, &cx);
    v = attr_get(as, "cy");
    if (v) parse_len(v, 0, &cy);
    v = attr_get(as, "r");
    if (v) parse_len(v, 0, &r);
    if (r <= 0) return 0;
    segs = (int)ceilf(GV_PI * sqrtf(r / (2.f * GV_FLAT)));
    if (segs < 12) segs = 12;
    if (segs > 512) segs = 512;
    if (begin_path(d, 1, pn, m, err, errlen)) return -1;
    if (d->n == 0) return 0;
    for (i = 0; i < segs; i++) {
        float t = (2.f * GV_PI * (float)i) / (float)segs;
        if (line_to(d, m, cx + r * cosf(t), cy + r * sinf(t), err, errlen)) return -1;
    }
    d->p[d->n - 1].closed = 1;
    finish_path(d);
    return 0;
}

static int emit_ellipse(gv_doc *d, const attrs *as, paint pn, mtx m, char *err, int errlen)
{
    float cx = 0, cy = 0, rx = 0, ry = 0, rmax;
    const char *v;
    int i, segs;
    v = attr_get(as, "cx");
    if (v) parse_len(v, 0, &cx);
    v = attr_get(as, "cy");
    if (v) parse_len(v, 0, &cy);
    v = attr_get(as, "rx");
    if (v) parse_len(v, 0, &rx);
    v = attr_get(as, "ry");
    if (v) parse_len(v, 0, &ry);
    if (rx <= 0 || ry <= 0) return 0;
    rmax = rx > ry ? rx : ry;
    segs = (int)ceilf(GV_PI * sqrtf(rmax / (2.f * GV_FLAT)));
    if (segs < 12) segs = 12;
    if (segs > 512) segs = 512;
    if (begin_path(d, 1, pn, m, err, errlen)) return -1;
    if (d->n == 0) return 0;
    for (i = 0; i < segs; i++) {
        float t = (2.f * GV_PI * (float)i) / (float)segs;
        if (line_to(d, m, cx + rx * cosf(t), cy + ry * sinf(t), err, errlen)) return -1;
    }
    d->p[d->n - 1].closed = 1;
    finish_path(d);
    return 0;
}

static int emit_line(gv_doc *d, const attrs *as, paint pn, mtx m, char *err, int errlen)
{
    float x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    const char *v;
    v = attr_get(as, "x1");
    if (v) parse_len(v, 0, &x1);
    v = attr_get(as, "y1");
    if (v) parse_len(v, 0, &y1);
    v = attr_get(as, "x2");
    if (v) parse_len(v, 0, &x2);
    v = attr_get(as, "y2");
    if (v) parse_len(v, 0, &y2);
    if (begin_path(d, 0, pn, m, err, errlen)) return -1;
    if (d->n == 0) return 0;
    if (line_to(d, m, x1, y1, err, errlen) || line_to(d, m, x2, y2, err, errlen)) return -1;
    finish_path(d);
    return 0;
}

static int skip_tag(const char *name, int n)
{
    static const char *skip[] = {"defs",     "clippath", "mask",     "symbol",  "marker", "pattern",
                                 "lineargradient", "radialgradient", "filter",  "script",  "style",
                                 "title",    "desc",     "metadata", "text",    "tspan",   "image",
                                 "use",      "foreignobject"};
    unsigned i;
    for (i = 0; i < sizeof skip / sizeof skip[0]; i++)
        if (iname(name, n, skip[i])) return 1;
    return 0;
}

/* ---- XML walk ---- */

typedef struct {
    const char *s, *e;
    char *err;
    int errlen;
    gv_doc *d;
    int draw; /* 0 inside a skipped element */
} walk;

static int parse_attrs(walk *w, attrs *as, int *empty)
{
    const char *s = w->s, *e = w->e;
    *empty = 0;
    for (;;) {
        const char *name, *val;
        int nname, nval;
        char q;
        s = skip_sp(s, e);
        if (s >= e) {
            fail_set(w->err, w->errlen, "the file ended in a tag");
            return -1;
        }
        if (s[0] == '>') {
            w->s = s + 1;
            return 0;
        }
        if (s[0] == '/' && s + 1 < e && s[1] == '>') {
            *empty = 1;
            w->s = s + 2;
            return 0;
        }
        name = s;
        while (s < e && (letter(*s) || digit(*s) || *s == ':' || *s == '-' || *s == '_')) s++;
        nname = (int)(s - name);
        if (nname == 0) {
            fail_set(w->err, w->errlen, "a tag is not well formed");
            return -1;
        }
        s = skip_sp(s, e);
        if (s >= e || *s != '=') {
            /* a boolean attribute: treat as the name */
            if (attrs_add(as, name, nname, name, nname)) {
                fail_set(w->err, w->errlen, "out of memory");
                return -1;
            }
            continue;
        }
        s++;
        s = skip_sp(s, e);
        if (s >= e || (*s != '"' && *s != '\'')) {
            fail_set(w->err, w->errlen, "an attribute is not quoted");
            return -1;
        }
        q = *s++;
        val = s;
        while (s < e && *s != q) s++;
        if (s >= e) {
            fail_set(w->err, w->errlen, "an attribute is not finished");
            return -1;
        }
        nval = (int)(s - val);
        s++;
        if (attrs_add(as, name, nname, val, nval)) {
            fail_set(w->err, w->errlen, "out of memory");
            return -1;
        }
    }
}

static int walk_children(walk *w, const char *pname, int pn, paint p, mtx m, int draw);

static int handle_element(walk *w, const char *name, int nname, const attrs *as, int empty, paint p,
                          mtx m, int draw)
{
    int child_draw = draw;
    paint np = p;
    mtx nm = m;
    if (skip_tag(name, nname)) child_draw = 0;
    if (iname(name, nname, "svg") && !attr_get(as, "viewBox") && !attr_get(as, "width")) {
        /* a nested svg without a box is still a group */
    }
    apply_attrs(&np, &nm, as);
    if (child_draw) {
        const char *v;
        if (iname(name, nname, "path")) {
            v = attr_get(as, "d");
            if (v && parse_d(w->d, v, np, nm, w->err, w->errlen)) return -1;
        } else if (iname(name, nname, "rect")) {
            if (emit_rect(w->d, as, np, nm, w->err, w->errlen)) return -1;
        } else if (iname(name, nname, "circle")) {
            if (emit_circle(w->d, as, np, nm, w->err, w->errlen)) return -1;
        } else if (iname(name, nname, "ellipse")) {
            if (emit_ellipse(w->d, as, np, nm, w->err, w->errlen)) return -1;
        } else if (iname(name, nname, "line")) {
            if (emit_line(w->d, as, np, nm, w->err, w->errlen)) return -1;
        } else if (iname(name, nname, "polyline")) {
            if (emit_poly(w->d, attr_get(as, "points"), 0, np, nm, w->err, w->errlen)) return -1;
        } else if (iname(name, nname, "polygon")) {
            if (emit_poly(w->d, attr_get(as, "points"), 1, np, nm, w->err, w->errlen)) return -1;
        }
    }
    if (empty) return 0;
    return walk_children(w, name, nname, np, nm, child_draw);
}

static int walk_children(walk *w, const char *pname, int pn, paint p, mtx m, int draw)
{
    for (;;) {
        const char *name;
        int nname, empty = 0;
        attrs as;
        int rc;
        w->s = skip_misc(w->s, w->e);
        if (w->s >= w->e) {
            if (pname) {
                fail_set(w->err, w->errlen, "a tag is not closed");
                return -1;
            }
            return 0;
        }
        if (w->s[0] != '<') {
            /* text: skip to the next tag */
            while (w->s < w->e && *w->s != '<') w->s++;
            continue;
        }
        if (w->s + 1 < w->e && w->s[1] == '/') {
            const char *n;
            int nn;
            w->s += 2;
            n = w->s;
            while (w->s < w->e && (letter(*w->s) || digit(*w->s) || *w->s == ':' || *w->s == '-' ||
                                   *w->s == '_'))
                w->s++;
            nn = (int)(w->s - n);
            w->s = skip_sp(w->s, w->e);
            if (w->s >= w->e || *w->s != '>') {
                fail_set(w->err, w->errlen, "a close tag is not well formed");
                return -1;
            }
            w->s++;
            if (pname && !iname(n, nn, pname) && !(pn == nn && istart(n, pname, nn))) {
                /* allow a prefix-stripped match already in iname */
                if (!(pname && iname(n, nn, pname))) {
                    /* still accept if the local names match */
                }
            }
            if (pname) return 0;
            fail_set(w->err, w->errlen, "a close tag has no start");
            return -1;
        }
        w->s++;
        name = w->s;
        while (w->s < w->e && (letter(*w->s) || digit(*w->s) || *w->s == ':' || *w->s == '-' ||
                               *w->s == '_'))
            w->s++;
        nname = (int)(w->s - name);
        if (nname == 0) {
            fail_set(w->err, w->errlen, "a tag is not well formed");
            return -1;
        }
        memset(&as, 0, sizeof as);
        if (parse_attrs(w, &as, &empty)) {
            attrs_free(&as);
            return -1;
        }
        rc = handle_element(w, name, nname, &as, empty, p, m, draw);
        attrs_free(&as);
        if (rc) return -1;
    }
}

/* Viewport and viewBox to a user-to-mm matrix, Y up, origin at the
 * bottom-left of the viewport. */
static int parse_viewport(const char *s, float fallback_px, float *mm)
{
    char *e;
    float v;
    const char *p;
    int k;
    if (!s) return -1;
    while (*s && space(*s)) s++;
    v = strtof(s, &e);
    if (e == s) return -1;
    p = e;
    while (*p && space(*p)) p++;
    if (*p == 0) {
        *mm = v * PX;
        return 0;
    }
    if (*p == '%') {
        *mm = v * 0.01f * fallback_px * PX;
        return 0;
    }
    k = parse_len(s, fallback_px, mm);
    if (k == 1) return 0;
    if (k == 0) {
        *mm = v * PX;
        return 0;
    }
    return -1;
}

/* Viewport and viewBox to a user-to-mm matrix, Y up, origin at the
 * bottom-left of the viewport. */
static int view_matrix(const attrs *as, mtx *out, float *vw, float *vh, char *err, int errlen)
{
    const char *ws, *hs, *vbs, *par;
    float width_mm, height_mm, vb_x = 0, vb_y = 0, vb_w = 0, vb_h = 0;
    int have_vb = 0, meet = 1;
    ws = attr_get(as, "width");
    hs = attr_get(as, "height");
    vbs = attr_get(as, "viewBox");
    par = attr_get(as, "preserveAspectRatio");
    if (ws) {
        if (parse_viewport(ws, 300.f, &width_mm)) {
            fail_set(err, errlen, "the width is not a length");
            return -1;
        }
    } else
        width_mm = 300.f * PX;
    if (hs) {
        if (parse_viewport(hs, 150.f, &height_mm)) {
            fail_set(err, errlen, "the height is not a length");
            return -1;
        }
    } else
        height_mm = 150.f * PX;
    if (vbs) {
        const char *s = vbs, *e = vbs + strlen(vbs);
        if (path_num(&s, e, &vb_x) || path_num(&s, e, &vb_y) || path_num(&s, e, &vb_w) ||
            path_num(&s, e, &vb_h) || vb_w <= 0 || vb_h <= 0) {
            fail_set(err, errlen, "the viewBox is not four numbers");
            return -1;
        }
        have_vb = 1;
    }
    if (!ws && have_vb) width_mm = vb_w * PX;
    if (!hs && have_vb) height_mm = vb_h * PX;
    if (!have_vb) {
        /* 1 user unit = 1 px, viewport as given, then Y flip. */
        vb_x = 0;
        vb_y = 0;
        vb_w = width_mm / PX;
        vb_h = height_mm / PX;
    }
    if (par && istart(skip_sp(par, par + strlen(par)), "none", 4)) meet = 0;
    {
        float sx = width_mm / vb_w, sy = height_mm / vb_h, s, ox, oy;
        mtx u, flip;
        if (meet) {
            s = sx < sy ? sx : sy;
            sx = sy = s;
        }
        ox = (width_mm - vb_w * sx) * 0.5f;
        oy = (height_mm - vb_h * sy) * 0.5f;
        /* user -> viewport (Y down, mm), then Y flip in the viewport. */
        u = mtx_id();
        u.a = sx;
        u.d = sy;
        u.e = -vb_x * sx + ox;
        u.f = -vb_y * sy + oy;
        flip = mtx_id();
        flip.d = -1;
        flip.f = height_mm;
        *out = mtx_mul(flip, u);
    }
    *vw = width_mm;
    *vh = height_mm;
    return 0;
}

static int layers_build(gv_doc *d, char *err, int errlen)
{
    int i, j;
    d->layer = NULL;
    d->nlayer = 0;
    for (i = 0; i < d->n; i++) {
        unsigned c = gv_path_colour(&d->p[i]);
        int found = 0;
        if (c == GV_NONE) continue;
        for (j = 0; j < d->nlayer; j++)
            if (d->layer[j].colour == c) {
                d->layer[j].n++;
                found = 1;
                break;
            }
        if (found) continue;
        {
            gv_layer *p = (gv_layer *)realloc(d->layer, (size_t)(d->nlayer + 1) * sizeof *p);
            if (!p) {
                fail_set(err, errlen, "out of memory");
                return -1;
            }
            d->layer = p;
            d->layer[d->nlayer].colour = c;
            d->layer[d->nlayer].n = 1;
            d->nlayer++;
        }
    }
    return 0;
}

static void bounds_of(gv_doc *d)
{
    int i, k, any = 0;
    d->minx = d->miny = d->maxx = d->maxy = 0;
    for (i = 0; i < d->n; i++) {
        gv_path *p = &d->p[i];
        for (k = 0; k < p->n; k++) {
            float x = p->xy[k * 2], y = p->xy[k * 2 + 1];
            if (!any) {
                d->minx = d->maxx = x;
                d->miny = d->maxy = y;
                any = 1;
            } else {
                if (x < d->minx) d->minx = x;
                if (y < d->miny) d->miny = y;
                if (x > d->maxx) d->maxx = x;
                if (y > d->maxy) d->maxy = y;
            }
        }
    }
}

static int load_text(gv_doc *d, const char *text, int n, char *err, int errlen)
{
    walk w;
    attrs root;
    int empty = 0, rc;
    const char *s, *e, *name;
    int nname;
    paint p;
    mtx view;
    gv_doc_free(d);
    if (!text) {
        fail_set(err, errlen, "not an SVG");
        return -1;
    }
    if (n < 0) n = (int)strlen(text);
    s = text;
    e = text + n;
    if (n >= 3 && (unsigned char)s[0] == 0xef && (unsigned char)s[1] == 0xbb &&
        (unsigned char)s[2] == 0xbf)
        s += 3;
    w.s = s;
    w.e = e;
    w.err = err;
    w.errlen = errlen;
    w.d = d;
    w.draw = 1;
    w.s = skip_misc(w.s, w.e);
    /* find the root <svg> */
    while (w.s < w.e) {
        w.s = skip_misc(w.s, w.e);
        if (w.s >= w.e || *w.s != '<') {
            fail_set(err, errlen, "not an SVG");
            return -1;
        }
        if (w.s + 1 < w.e && w.s[1] == '/') {
            fail_set(err, errlen, "not an SVG");
            return -1;
        }
        w.s++;
        name = w.s;
        while (w.s < w.e && (letter(*w.s) || digit(*w.s) || *w.s == ':' || *w.s == '-' || *w.s == '_'))
            w.s++;
        nname = (int)(w.s - name);
        memset(&root, 0, sizeof root);
        if (parse_attrs(&w, &root, &empty)) {
            attrs_free(&root);
            return -1;
        }
        if (iname(name, nname, "svg")) {
            if (view_matrix(&root, &view, &d->width, &d->height, err, errlen)) {
                attrs_free(&root);
                gv_doc_free(d);
                return -1;
            }
            p = paint_root();
            apply_attrs(&p, &view, &root);
            if (!empty)
                rc = walk_children(&w, name, nname, p, view, 1);
            else
                rc = 0;
            attrs_free(&root);
            if (rc) {
                gv_doc_free(d);
                return -1;
            }
            if (d->n > 0) finish_path(d);
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
            return 0;
        }
        attrs_free(&root);
        if (!empty) {
            /* skip a non-svg root wrapper? not an SVG. */
            fail_set(err, errlen, "not an SVG");
            return -1;
        }
    }
    fail_set(err, errlen, "not an SVG");
    return -1;
}

void gv_doc_init(gv_doc *d)
{
    memset(d, 0, sizeof *d);
    d->size = (int)sizeof *d;
}

void gv_doc_free(gv_doc *d)
{
    int i;
    if (!d) return;
    if (d->p) {
        for (i = 0; i < d->n; i++)
            free(d->p[i].xy);
        free(d->p);
    }
    free(d->layer);
    memset(d, 0, sizeof *d);
    d->size = (int)sizeof *d;
}

int gv_load_svg_text(gv_doc *d, const char *text, int n, char *err, int errlen)
{
    return load_text(d, text, n, err, errlen);
}

int gv_load_svg(gv_doc *d, const char *path, char *err, int errlen)
{
    FILE *f;
    long n;
    char *buf;
    int rc;
    if (!path) {
        fail_set(err, errlen, "not an SVG");
        return -1;
    }
    f = fopen(path, "rb");
    if (!f) {
        fail_set(err, errlen, "the file could not be opened");
        return -1;
    }
    if (fseek(f, 0, SEEK_END) != 0 || (n = ftell(f)) < 0 || fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        fail_set(err, errlen, "the file could not be read");
        return -1;
    }
    if (n > GV_MAX_FILE) {
        fclose(f);
        fail_set(err, errlen, "the file is too long");
        return -1;
    }
    buf = (char *)malloc((size_t)n + 1);
    if (!buf) {
        fclose(f);
        fail_set(err, errlen, "out of memory");
        return -1;
    }
    if (n && fread(buf, 1, (size_t)n, f) != (size_t)n) {
        free(buf);
        fclose(f);
        fail_set(err, errlen, "the file could not be read");
        return -1;
    }
    fclose(f);
    buf[n] = 0;
    rc = load_text(d, buf, (int)n, err, errlen);
    free(buf);
    return rc;
}

unsigned gv_path_colour(const gv_path *p)
{
    if (!p) return GV_NONE;
    if (p->stroke != GV_NONE) return p->stroke;
    return p->fill;
}

void gv_path_box(const gv_path *p, float *minx, float *miny, float *maxx, float *maxy)
{
    int i;
    if (!p || p->n <= 0) {
        if (minx) *minx = 0;
        if (miny) *miny = 0;
        if (maxx) *maxx = 0;
        if (maxy) *maxy = 0;
        return;
    }
    *minx = *maxx = p->xy[0];
    *miny = *maxy = p->xy[1];
    for (i = 1; i < p->n; i++) {
        float x = p->xy[i * 2], y = p->xy[i * 2 + 1];
        if (x < *minx) *minx = x;
        if (y < *miny) *miny = y;
        if (x > *maxx) *maxx = x;
        if (y > *maxy) *maxy = y;
    }
}
