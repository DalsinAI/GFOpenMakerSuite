/* A GFOpenMakerWorkshop project: a drawer and its project.om manifest.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gs_project.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#if defined(__amigaos__) || defined(AMIGA) || defined(__AMIGA__)
#include <dos/dos.h>
#include <proto/dos.h>
#define GPR_AMIGA 1
#else
#include <sys/stat.h>
#include <errno.h>
#endif

static const char *const kind_sub[] = {"cad", "print", "laser", "cnc", "electronics", NULL};

static int lower_ch(int c)
{
    return c >= 'A' && c <= 'Z' ? c + 32 : c;
}

static int lower_eq(const char *a, const char *b)
{
    if (!a || !b) return a == b;
    for (; *a || *b; a++, b++) {
        if (lower_ch((unsigned char)*a) != lower_ch((unsigned char)*b)) return 0;
    }
    return 1;
}

static int ends_ci(const char *s, const char *suf)
{
    size_t n, m;
    if (!s || !suf) return 0;
    n = strlen(s);
    m = strlen(suf);
    return n >= m && lower_eq(s + n - m, suf);
}

static void lower_copy(char *out, int n, const char *s)
{
    int i = 0;
    if (n <= 0) return;
    if (!s) s = "";
    while (s[i] && i < n - 1) {
        out[i] = (char)lower_ch((unsigned char)s[i]);
        i++;
    }
    out[i] = 0;
}

static int kind_ok(const char *k)
{
    const char *s;
    if (!k || !*k) return 0;
    for (s = k; *s; s++) {
        int c = lower_ch((unsigned char)*s);
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-')) return 0;
    }
    return 1;
}

static int copy_str(char *out, int n, const char *s)
{
    size_t len;
    if (!out || n <= 0) return -1;
    if (!s) s = "";
    len = strlen(s);
    if ((int)len >= n) return -1;
    memcpy(out, s, len + 1);
    return 0;
}

static char *trim(char *s)
{
    char *e;
    while (*s == ' ' || *s == '\t' || *s == '\r') s++;
    e = s + strlen(s);
    while (e > s && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\r')) *--e = 0;
    return s;
}

static int parse_int(const char *s, int *out)
{
    const char *t = s;
    int v = 0, any = 0;
    if (!s) return -1;
    if (*t == '+') t++;
    while (*t >= '0' && *t <= '9') {
        if (v > 999999999 / 10) return -1;
        v = v * 10 + (*t++ - '0');
        any = 1;
    }
    if (!any || *t) return -1;
    *out = v;
    return 0;
}

static const char *file_part(const char *path)
{
    const char *a, *b;
    if (!path || !*path) return "";
    a = strrchr(path, '/');
    b = strrchr(path, ':');
    if (b && (!a || b > a)) a = b;
    return a ? a + 1 : path;
}

static void stem_copy(char *out, int n, const char *path)
{
    const char *base = file_part(path);
    const char *suf[] = {".kicad_pro", ".kicad_pcb", ".kicad_sch", NULL};
    size_t len = strlen(base);
    int i;
    const char *dot;
    for (i = 0; suf[i]; i++) {
        size_t sl = strlen(suf[i]);
        if (len > sl && lower_eq(base + len - sl, suf[i])) {
            if ((int)(len - sl) >= n) len = (size_t)(n - 1) + sl;
            memcpy(out, base, len - sl);
            out[len - sl] = 0;
            return;
        }
    }
    dot = strrchr(base, '.');
    if (dot && dot != base) len = (size_t)(dot - base);
    if ((int)len >= n) len = (size_t)(n - 1);
    memcpy(out, base, len);
    out[len] = 0;
}

static int path_join(char *out, int n, const char *drawer, const char *file)
{
    size_t len = drawer ? strlen(drawer) : 0;
    int slash = len && (drawer[len - 1] == '/' || drawer[len - 1] == ':');
    int wr;
    if (!out || n <= 0) return -1;
    wr = snprintf(out, (size_t)n, slash ? "%s%s" : "%s/%s", drawer ? drawer : "", file ? file : "");
    return wr < 0 || wr >= n ? -1 : 0;
}

/* Drop a trailing slash, but keep a device name's colon (RAM:). */
static int strip_slash(char *path)
{
    size_t len;
    if (!path) return -1;
    len = strlen(path);
    while (len > 1 && path[len - 1] == '/') {
        if (len >= 2 && path[len - 2] == ':') break;
        path[--len] = 0;
    }
    return 0;
}

static int path_is_abs(const char *path)
{
    const char *c;
    if (!path || !*path) return 0;
    if (path[0] == '/') return 1;
    c = strchr(path, ':');
    return c && (strchr(path, '/') == NULL || c < strchr(path, '/'));
}

#ifdef GPR_AMIGA
static int drawer_exists(const char *path)
{
    BPTR l = Lock((STRPTR)path, ACCESS_READ);
    if (!l) return 0;
    UnLock(l);
    return 1;
}

static int make_drawer(const char *path)
{
    BPTR l;
    if (drawer_exists(path)) return 0;
    l = CreateDir((STRPTR)path);
    if (!l) return -1;
    UnLock(l);
    return 0;
}
#else
static int make_drawer(const char *path)
{
    struct stat st;
    if (mkdir(path, 0777) == 0) return 0;
    if (errno == EEXIST && stat(path, &st) == 0 && S_ISDIR(st.st_mode)) return 0;
    return -1;
}
#endif

static int file_exists(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    fclose(f);
    return 1;
}

static int copy_file(const char *from, const char *to, char *err, int errlen)
{
    FILE *in, *out;
    char buf[4096];
    size_t n;
    int ok = 1;
    in = fopen(from, "rb");
    if (!in) {
        if (err && errlen > 0) snprintf(err, (size_t)errlen, "%s: cannot open", from);
        return -1;
    }
    out = fopen(to, "wb");
    if (!out) {
        fclose(in);
        if (err && errlen > 0) snprintf(err, (size_t)errlen, "%s: cannot write", to);
        return -1;
    }
    while ((n = fread(buf, 1, sizeof buf, in)) > 0) {
        if (fwrite(buf, 1, n, out) != n) {
            ok = 0;
            break;
        }
    }
    if (ferror(in)) ok = 0;
    if (fclose(out)) ok = 0;
    fclose(in);
    if (!ok && err && errlen > 0) snprintf(err, (size_t)errlen, "%s: cannot write", to);
    return ok ? 0 : -1;
}

static int fail(char *err, int errlen, int line, const char *why)
{
    if (err && errlen > 0) {
        if (line > 0) snprintf(err, (size_t)errlen, "line %d: %s", line, why);
        else snprintf(err, (size_t)errlen, "%s", why);
    }
    return -1;
}

static int extra_add(gpr_extra **list, int *n, const char *key, const char *value)
{
    gpr_extra *grow;
    if (!key || !*key) return -1;
    if ((int)strlen(key) > GPR_KEY_MAX || (int)strlen(value ? value : "") > GPR_PATH_MAX) return -1;
    grow = (gpr_extra *)realloc(*list, (size_t)(*n + 1) * sizeof **list);
    if (!grow) return -1;
    *list = grow;
    memset(&grow[*n], 0, sizeof grow[*n]);
    memcpy(grow[*n].key, key, strlen(key) + 1);
    memcpy(grow[*n].value, value ? value : "", strlen(value ? value : "") + 1);
    (*n)++;
    return 0;
}

static void item_free(gpr_item *it)
{
    free(it->extra);
    memset(it, 0, sizeof *it);
}

void gpr_init(gpr_project *p)
{
    memset(p, 0, sizeof *p);
    p->size = (int)sizeof *p;
}

void gpr_free(gpr_project *p)
{
    int i;
    if (!p) return;
    for (i = 0; i < p->n; i++) item_free(&p->items[i]);
    free(p->items);
    free(p->extra);
    gpr_init(p);
}

static int items_grow(gpr_project *p, int need)
{
    int cap;
    gpr_item *grow;
    if (need <= p->cap) return 0;
    if (need > GPR_ITEMS_MAX) return -1;
    cap = p->cap ? p->cap * 2 : 8;
    while (cap < need) cap *= 2;
    if (cap > GPR_ITEMS_MAX) cap = GPR_ITEMS_MAX;
    grow = (gpr_item *)realloc(p->items, (size_t)cap * sizeof *grow);
    if (!grow) return -1;
    if (cap > p->cap) memset(grow + p->cap, 0, (size_t)(cap - p->cap) * sizeof *grow);
    p->items = grow;
    p->cap = cap;
    return 0;
}

/* During parse, the original item number is kept in a parallel array. */
typedef struct {
    int num;
    gpr_item it;
} gpr_loaded;

static int loaded_find(gpr_loaded *a, int n, int num)
{
    int i;
    for (i = 0; i < n; i++)
        if (a[i].num == num) return i;
    return -1;
}

int gpr_kind_known(const char *kind)
{
    char k[GPR_KIND_MAX + 1];
    lower_copy(k, (int)sizeof k, kind);
    return !strcmp(k, "cad") || !strcmp(k, "print") || !strcmp(k, "laser") || !strcmp(k, "cnc") ||
           !strcmp(k, "electronics");
}

const char *gpr_kind_group(const char *kind)
{
    char k[GPR_KIND_MAX + 1];
    lower_copy(k, (int)sizeof k, kind);
    if (!strcmp(k, "cad")) return "CAD";
    if (!strcmp(k, "print")) return "Print";
    if (!strcmp(k, "laser")) return "Laser";
    if (!strcmp(k, "cnc")) return "CNC";
    if (!strcmp(k, "electronics")) return "Electronics";
    return kind && *kind ? kind : "";
}

const char *gpr_kind_app(const char *kind)
{
    char k[GPR_KIND_MAX + 1];
    lower_copy(k, (int)sizeof k, kind);
    if (!strcmp(k, "cad")) return "GFOpenMake";
    if (!strcmp(k, "print")) return "GFOpenSlicer";
    if (!strcmp(k, "laser")) return "GFOpenBurn";
    if (!strcmp(k, "electronics")) return "gfamigakicad";
    return NULL;
}

const char *gpr_kind_port(const char *kind)
{
    char k[GPR_KIND_MAX + 1];
    lower_copy(k, (int)sizeof k, kind);
    if (!strcmp(k, "cad")) return "GFOPENMAKE";
    if (!strcmp(k, "print")) return "OPENMAKESLICER";
    if (!strcmp(k, "laser")) return "OPENMAKEBURN";
    return NULL;
}

const char *gpr_kind_suggest(const char *path)
{
    if (!path) return NULL;
    if (ends_ci(path, ".stl")) return "cad";
    if (ends_ci(path, ".svg") || ends_ci(path, ".dxf") || ends_ci(path, ".pbm") || ends_ci(path, ".pgm") ||
        ends_ci(path, ".ppm"))
        return "laser";
    if (ends_ci(path, ".kicad_pro") || ends_ci(path, ".kicad_pcb") || ends_ci(path, ".kicad_sch"))
        return "electronics";
    return NULL;
}

static int set_item_key(gpr_item *it, const char *key, const char *val, char *err, int errlen, int line)
{
    if (!strcmp(key, "kind")) {
        char k[GPR_KIND_MAX + 1];
        if (!kind_ok(val)) return fail(err, errlen, line, "kind is not a token");
        lower_copy(k, (int)sizeof k, val);
        return copy_str(it->kind, (int)sizeof it->kind, k);
    }
    if (!strcmp(key, "name")) return copy_str(it->name, (int)sizeof it->name, val) ? fail(err, errlen, line, "name is too long") : 0;
    if (!strcmp(key, "file")) return copy_str(it->file, (int)sizeof it->file, val) ? fail(err, errlen, line, "file is too long") : 0;
    if (!strcmp(key, "settings"))
        return copy_str(it->settings, (int)sizeof it->settings, val) ? fail(err, errlen, line, "settings is too long") : 0;
    if (!strcmp(key, "from")) {
        int v;
        if (parse_int(val, &v) || v < 0) return fail(err, errlen, line, "from is not a number");
        it->from = v;
        return 0;
    }
    if (extra_add(&it->extra, &it->nextra, key, val)) return fail(err, errlen, line, "cannot keep an extra key");
    return 0;
}

int gpr_parse(gpr_project *p, const char *text, char *err, int errlen)
{
    gpr_loaded *loaded = NULL;
    int nloaded = 0, line = 0, format = 1, i, k;
    char path_keep[GPR_PATH_MAX + 1];
    if (err && errlen > 0) err[0] = 0;
    if (!p || !text) return fail(err, errlen, 0, "no project");
    copy_str(path_keep, (int)sizeof path_keep, p->path);
    gpr_free(p);
    copy_str(p->path, (int)sizeof p->path, path_keep);

    while (*text) {
        char buf[512], *key, *val, *eq, *hash, *dot, *dot2;
        size_t n = strcspn(text, "\n");
        int num;
        line++;
        if (n >= sizeof buf) {
            free(loaded);
            return fail(err, errlen, line, "too long");
        }
        memcpy(buf, text, n);
        buf[n] = 0;
        text += n + (text[n] == '\n');
        if (n > 0 && buf[n - 1] == '\r') buf[n - 1] = 0;
        hash = strchr(buf, '#');
        if (hash) *hash = 0;
        key = trim(buf);
        if (!*key) continue;
        eq = strchr(key, '=');
        if (!eq) {
            free(loaded);
            return fail(err, errlen, line, "wants key = value");
        }
        *eq = 0;
        val = trim(eq + 1);
        key = trim(key);
        if (!strncmp(key, "item.", 5)) {
            gpr_loaded *grow;
            int at;
            dot = strchr(key + 5, '.');
            if (!dot || dot == key + 5 || !dot[1]) {
                free(loaded);
                return fail(err, errlen, line, "item key is not item.N.field");
            }
            *dot = 0;
            if (parse_int(key + 5, &num) || num < 1) {
                free(loaded);
                return fail(err, errlen, line, "item number is not 1 or more");
            }
            *dot = '.';
            dot2 = dot + 1;
            at = loaded_find(loaded, nloaded, num);
            if (at < 0) {
                if (nloaded >= GPR_ITEMS_MAX) {
                    free(loaded);
                    return fail(err, errlen, line, "too many items");
                }
                grow = (gpr_loaded *)realloc(loaded, (size_t)(nloaded + 1) * sizeof *grow);
                if (!grow) {
                    free(loaded);
                    return fail(err, errlen, line, "not enough memory");
                }
                loaded = grow;
                memset(&loaded[nloaded], 0, sizeof loaded[nloaded]);
                loaded[nloaded].num = num;
                at = nloaded++;
            }
            if (set_item_key(&loaded[at].it, dot2, val, err, errlen, line)) {
                for (i = 0; i < nloaded; i++) item_free(&loaded[i].it);
                free(loaded);
                return -1;
            }
        } else if (!strcmp(key, "format")) {
            if (parse_int(val, &format) || format < 1) {
                free(loaded);
                return fail(err, errlen, line, "format is not 1 or more");
            }
        } else if (!strcmp(key, "name")) {
            if (copy_str(p->name, (int)sizeof p->name, val)) {
                free(loaded);
                return fail(err, errlen, line, "name is too long");
            }
        } else if (!strcmp(key, "notes")) {
            if (copy_str(p->notes, (int)sizeof p->notes, val)) {
                free(loaded);
                return fail(err, errlen, line, "notes is too long");
            }
        } else {
            if (extra_add(&p->extra, &p->nextra, key, val)) {
                free(loaded);
                return fail(err, errlen, line, "cannot keep an extra key");
            }
        }
    }

    /* Item order is the item number. Compact gaps and remap from=. */
    for (i = 0; i < nloaded - 1; i++) {
        int best = i;
        for (k = i + 1; k < nloaded; k++)
            if (loaded[k].num < loaded[best].num) best = k;
        if (best != i) {
            gpr_loaded tmp = loaded[i];
            loaded[i] = loaded[best];
            loaded[best] = tmp;
        }
    }
    if (items_grow(p, nloaded)) {
        for (i = 0; i < nloaded; i++) item_free(&loaded[i].it);
        free(loaded);
        return fail(err, errlen, 0, "not enough memory");
    }
    for (i = 0; i < nloaded; i++) {
        p->items[i] = loaded[i].it;
        memset(&loaded[i].it, 0, sizeof loaded[i].it);
    }
    p->n = nloaded;
    for (i = 0; i < p->n; i++) {
        int old = p->items[i].from, neu = 0;
        if (old <= 0) continue;
        for (k = 0; k < nloaded; k++)
            if (loaded[k].num == old) {
                neu = k + 1;
                break;
            }
        p->items[i].from = neu;
    }
    free(loaded);
    for (i = 0; i < p->n; i++) {
        if (!p->items[i].kind[0]) return fail(err, errlen, 0, "an item has no kind");
        if (!p->items[i].file[0]) return fail(err, errlen, 0, "an item has no file");
        if (!p->items[i].name[0]) stem_copy(p->items[i].name, (int)sizeof p->items[i].name, p->items[i].file);
    }
    p->dirty = 0;
    (void)format;
    return 0;
}

static int append(char *buf, int len, int *at, const char *fmt, const char *a, const char *b, int n)
{
    int wr;
    if (*at < 0 || *at >= len) return -1;
    if (b && n == 0)
        wr = snprintf(buf + *at, (size_t)(len - *at), fmt, a, b);
    else if (b)
        wr = snprintf(buf + *at, (size_t)(len - *at), fmt, a, n, b);
    else if (n)
        wr = snprintf(buf + *at, (size_t)(len - *at), fmt, a, n);
    else
        wr = snprintf(buf + *at, (size_t)(len - *at), fmt, a);
    if (wr < 0 || wr >= len - *at) return -1;
    *at += wr;
    return 0;
}

int gpr_format(const gpr_project *p, char *buf, int len)
{
    int at = 0, i, e, wr;
    if (!p || !buf || len <= 0) return -1;
    wr = snprintf(buf, (size_t)len,
                  "# GFOpenMakerWorkshop project\n"
                  "format = 1\n"
                  "name = %s\n",
                  p->name);
    if (wr < 0 || wr >= len) return -1;
    at = wr;
    if (p->notes[0]) {
        if (append(buf, len, &at, "notes = %s\n", p->notes, NULL, 0)) return -1;
    }
    for (e = 0; e < p->nextra; e++) {
        if (append(buf, len, &at, "%s = %s\n", p->extra[e].key, p->extra[e].value, 0)) return -1;
    }
    for (i = 0; i < p->n; i++) {
        const gpr_item *it = &p->items[i];
        char pref[32];
        snprintf(pref, sizeof pref, "item.%d", i + 1);
        if (append(buf, len, &at, "\n%s.kind = %s\n", pref, it->kind, 0)) return -1;
        if (append(buf, len, &at, "%s.name = %s\n", pref, it->name, 0)) return -1;
        if (append(buf, len, &at, "%s.file = %s\n", pref, it->file, 0)) return -1;
        if (it->from > 0) {
            wr = snprintf(buf + at, (size_t)(len - at), "%s.from = %d\n", pref, it->from);
            if (wr < 0 || wr >= len - at) return -1;
            at += wr;
        }
        if (it->settings[0]) {
            if (append(buf, len, &at, "%s.settings = %s\n", pref, it->settings, 0)) return -1;
        }
        for (e = 0; e < it->nextra; e++) {
            wr = snprintf(buf + at, (size_t)(len - at), "%s.%s = %s\n", pref, it->extra[e].key, it->extra[e].value);
            if (wr < 0 || wr >= len - at) return -1;
            at += wr;
        }
    }
    return at;
}

static int read_all(const char *path, char **text, char *err, int errlen)
{
    FILE *f = fopen(path, "rb");
    long len;
    char *buf;
    if (!f) {
        if (err && errlen > 0) snprintf(err, (size_t)errlen, "%s: cannot open", path);
        return -1;
    }
    if (fseek(f, 0, SEEK_END) || (len = ftell(f)) < 0 || fseek(f, 0, SEEK_SET)) {
        fclose(f);
        if (err && errlen > 0) snprintf(err, (size_t)errlen, "%s: cannot read", path);
        return -1;
    }
    buf = (char *)malloc((size_t)len + 1);
    if (!buf || (len > 0 && fread(buf, 1, (size_t)len, f) != (size_t)len)) {
        fclose(f);
        free(buf);
        if (err && errlen > 0) snprintf(err, (size_t)errlen, "%s: cannot read", path);
        return -1;
    }
    fclose(f);
    buf[len] = 0;
    *text = buf;
    return 0;
}

static int write_all(const char *path, const char *text, int n, char *err, int errlen)
{
    FILE *f = fopen(path, "wb");
    int ok = f && fwrite(text, 1, (size_t)n, f) == (size_t)n;
    if (f && fclose(f)) ok = 0;
    if (!ok && err && errlen > 0) snprintf(err, (size_t)errlen, "%s: cannot write", path);
    return ok ? 0 : -1;
}

static int make_kind_drawers(const char *drawer, char *err, int errlen)
{
    char sub[GPR_PATH_MAX + 1];
    int i;
    if (make_drawer(drawer)) return fail(err, errlen, 0, "cannot make the project drawer");
    for (i = 0; kind_sub[i]; i++) {
        if (path_join(sub, (int)sizeof sub, drawer, kind_sub[i])) return fail(err, errlen, 0, "path is too long");
        if (make_drawer(sub)) return fail(err, errlen, 0, "cannot make a kind drawer");
    }
    return 0;
}

static int om_path(char *out, int n, const char *drawer)
{
    return path_join(out, n, drawer, "project.om");
}

/* path is a drawer, or a project.om file; the drawer is written to out. */
static int drawer_of(const char *path, char *out, int n)
{
    const char *base;
    size_t len;
    if (!path || !*path) return -1;
    if (copy_str(out, n, path)) return -1;
    strip_slash(out);
    base = file_part(out);
    if (lower_eq(base, "project.om")) {
        if (base != out) {
            char *cut = (char *)base;
            if (cut > out && (cut[-1] == '/' || cut[-1] == ':')) {
                if (cut[-1] == '/') cut[-1] = 0;
                else *cut = 0;
            }
        } else {
            return copy_str(out, n, ".");
        }
        strip_slash(out);
    }
    len = strlen(out);
    return len == 0 ? -1 : 0;
}

int gpr_create(gpr_project *p, const char *drawer, const char *name, char *err, int errlen)
{
    char dir[GPR_PATH_MAX + 1], om[GPR_PATH_MAX + 1], buf[1024];
    int n;
    if (err && errlen > 0) err[0] = 0;
    if (!p || !drawer || !*drawer) return fail(err, errlen, 0, "no drawer");
    if (copy_str(dir, (int)sizeof dir, drawer)) return fail(err, errlen, 0, "path is too long");
    strip_slash(dir);
    if (om_path(om, (int)sizeof om, dir)) return fail(err, errlen, 0, "path is too long");
    if (file_exists(om)) return fail(err, errlen, 0, "already a project");
    if (make_kind_drawers(dir, err, errlen)) return -1;
    gpr_free(p);
    copy_str(p->path, (int)sizeof p->path, dir);
    if (name && *name) {
        if (copy_str(p->name, (int)sizeof p->name, name)) return fail(err, errlen, 0, "name is too long");
    } else {
        const char *base = file_part(dir);
        if (!*base) base = dir;
        if (copy_str(p->name, (int)sizeof p->name, base)) return fail(err, errlen, 0, "name is too long");
    }
    n = gpr_format(p, buf, (int)sizeof buf);
    if (n < 0) return fail(err, errlen, 0, "cannot write the manifest");
    if (write_all(om, buf, n, err, errlen)) return -1;
    p->dirty = 0;
    return 0;
}

int gpr_open(gpr_project *p, const char *path, char *err, int errlen)
{
    char dir[GPR_PATH_MAX + 1], om[GPR_PATH_MAX + 1], *text = NULL;
    int rc;
    if (err && errlen > 0) err[0] = 0;
    if (!p || !path || !*path) return fail(err, errlen, 0, "no path");
    if (drawer_of(path, dir, (int)sizeof dir)) return fail(err, errlen, 0, "path is too long");
    if (om_path(om, (int)sizeof om, dir)) return fail(err, errlen, 0, "path is too long");
    if (read_all(om, &text, err, errlen)) return -1;
    rc = gpr_parse(p, text, err, errlen);
    free(text);
    if (rc) return rc;
    copy_str(p->path, (int)sizeof p->path, dir);
    p->dirty = 0;
    return 0;
}

int gpr_save(gpr_project *p, char *err, int errlen)
{
    char om[GPR_PATH_MAX + 1], *buf;
    int n, rc, cap = 4096;
    if (err && errlen > 0) err[0] = 0;
    if (!p || !p->path[0]) return fail(err, errlen, 0, "no project drawer");
    if (om_path(om, (int)sizeof om, p->path)) return fail(err, errlen, 0, "path is too long");
    for (;;) {
        buf = (char *)malloc((size_t)cap);
        if (!buf) return fail(err, errlen, 0, "not enough memory");
        n = gpr_format(p, buf, cap);
        if (n >= 0) break;
        free(buf);
        cap *= 2;
        if (cap > 1 << 20) return fail(err, errlen, 0, "the manifest is too long");
    }
    rc = write_all(om, buf, n, err, errlen);
    free(buf);
    if (rc) return rc;
    p->dirty = 0;
    return 0;
}

int gpr_set_name(gpr_project *p, const char *name)
{
    if (!p || !name) return -1;
    if (copy_str(p->name, (int)sizeof p->name, name)) return -1;
    p->dirty = 1;
    return 0;
}

int gpr_set_notes(gpr_project *p, const char *notes)
{
    if (!p) return -1;
    if (copy_str(p->notes, (int)sizeof p->notes, notes ? notes : "")) return -1;
    p->dirty = 1;
    return 0;
}

void gpr_add_defaults(gpr_add_opts *o)
{
    memset(o, 0, sizeof *o);
    o->size = (int)sizeof *o;
    o->copy = -1;
}

static void add_of(const gpr_add_opts *o, gpr_add_opts *to)
{
    gpr_add_defaults(to);
    if (!o) return;
    if (o->size >= (int)(offsetof(gpr_add_opts, kind) + sizeof o->kind)) to->kind = o->kind;
    if (o->size >= (int)(offsetof(gpr_add_opts, file) + sizeof o->file)) to->file = o->file;
    if (o->size >= (int)(offsetof(gpr_add_opts, name) + sizeof o->name)) to->name = o->name;
    if (o->size >= (int)(offsetof(gpr_add_opts, from) + sizeof o->from)) to->from = o->from;
    if (o->size >= (int)(offsetof(gpr_add_opts, copy) + sizeof o->copy)) to->copy = o->copy;
    if (o->size >= (int)(offsetof(gpr_add_opts, settings) + sizeof o->settings)) to->settings = o->settings;
}

static int default_copy(const char *kind)
{
    char k[GPR_KIND_MAX + 1];
    lower_copy(k, (int)sizeof k, kind);
    if (!strcmp(k, "electronics")) return 0;
    if (gpr_kind_known(k)) return 1;
    return 0;
}

static int is_inside(const char *drawer, const char *full)
{
    size_t n;
    if (!drawer || !full) return 0;
    n = strlen(drawer);
    if (n == 0) return 0;
    if (strncmp(full, drawer, n) != 0) return 0;
    if (full[n] == 0) return 1;
    if (drawer[n - 1] == ':') return 1;
    return full[n] == '/';
}

static int rel_inside(const char *drawer, const char *full, char *out, int n)
{
    size_t d = strlen(drawer);
    const char *rest;
    if (!is_inside(drawer, full)) return -1;
    rest = full + d;
    if (*rest == '/') rest++;
    if (drawer[d ? d - 1 : 0] == ':' && *rest == '/') rest++;
    if (!*rest) return copy_str(out, n, file_part(full));
    return copy_str(out, n, rest);
}

int gpr_add(gpr_project *p, const gpr_add_opts *opts, char *err, int errlen)
{
    gpr_add_opts o;
    char kind[GPR_KIND_MAX + 1], src[GPR_PATH_MAX + 1], dest[GPR_PATH_MAX + 1], stored[GPR_PATH_MAX + 1];
    char name[GPR_NAME_MAX + 1];
    const char *base;
    int copy, i;
    gpr_item *it;
    if (err && errlen > 0) err[0] = 0;
    if (!p || !p->path[0]) return fail(err, errlen, 0, "no project drawer");
    add_of(opts, &o);
    if (!o.kind || !kind_ok(o.kind)) return fail(err, errlen, 0, "kind is not a token");
    if (!o.file || !*o.file) return fail(err, errlen, 0, "no file");
    lower_copy(kind, (int)sizeof kind, o.kind);
    if (o.from < 0 || (o.from > 0 && o.from > p->n)) return fail(err, errlen, 0, "from is not an item");
    if (p->n >= GPR_ITEMS_MAX) return fail(err, errlen, 0, "too many items");

    if (path_is_abs(o.file)) {
        if (copy_str(src, (int)sizeof src, o.file)) return fail(err, errlen, 0, "path is too long");
    } else if (path_join(src, (int)sizeof src, p->path, o.file)) {
        return fail(err, errlen, 0, "path is too long");
    }
    if (!file_exists(src)) return fail(err, errlen, 0, "cannot open the file");

    if (o.name && *o.name) {
        if (copy_str(name, (int)sizeof name, o.name)) return fail(err, errlen, 0, "name is too long");
    } else {
        stem_copy(name, (int)sizeof name, o.file);
        if (!name[0]) return fail(err, errlen, 0, "no name");
    }

    copy = o.copy;
    if (copy < 0) copy = default_copy(kind);
    base = file_part(src);

    if (copy) {
        char sub[GPR_PATH_MAX + 1];
        if (path_join(sub, (int)sizeof sub, p->path, kind)) return fail(err, errlen, 0, "path is too long");
        if (make_drawer(sub)) return fail(err, errlen, 0, "cannot make a kind drawer");
        if (path_join(dest, (int)sizeof dest, sub, base)) return fail(err, errlen, 0, "path is too long");
        if (is_inside(p->path, src) && !strcmp(src, dest)) {
            /* Already in place. */
        } else if (file_exists(dest) && !is_inside(p->path, src)) {
            return fail(err, errlen, 0, "that file is already in the project");
        } else if (strcmp(src, dest) != 0) {
            if (copy_file(src, dest, err, errlen)) return -1;
        }
        if (rel_inside(p->path, dest, stored, (int)sizeof stored))
            return fail(err, errlen, 0, "path is too long");
    } else if (is_inside(p->path, src)) {
        if (rel_inside(p->path, src, stored, (int)sizeof stored)) return fail(err, errlen, 0, "path is too long");
    } else {
        if (copy_str(stored, (int)sizeof stored, src)) return fail(err, errlen, 0, "path is too long");
    }

    if (items_grow(p, p->n + 1)) return fail(err, errlen, 0, "not enough memory");
    it = &p->items[p->n];
    memset(it, 0, sizeof *it);
    memcpy(it->kind, kind, strlen(kind) + 1);
    memcpy(it->name, name, strlen(name) + 1);
    memcpy(it->file, stored, strlen(stored) + 1);
    it->from = o.from;
    if (o.settings && *o.settings && copy_str(it->settings, (int)sizeof it->settings, o.settings)) {
        return fail(err, errlen, 0, "settings is too long");
    }
    i = p->n++;
    p->dirty = 1;
    return i;
}

int gpr_remove(gpr_project *p, int index)
{
    int i;
    if (!p || index < 0 || index >= p->n) return -1;
    item_free(&p->items[index]);
    for (i = index; i < p->n - 1; i++) p->items[i] = p->items[i + 1];
    memset(&p->items[p->n - 1], 0, sizeof p->items[0]);
    p->n--;
    /* from= that pointed at the removed item is cleared; later numbers move down. */
    for (i = 0; i < p->n; i++) {
        if (p->items[i].from == index + 1) p->items[i].from = 0;
        else if (p->items[i].from > index + 1) p->items[i].from--;
    }
    p->dirty = 1;
    return 0;
}

int gpr_rename(gpr_project *p, int index, const char *name)
{
    if (!p || index < 0 || index >= p->n || !name || !*name) return -1;
    if (copy_str(p->items[index].name, (int)sizeof p->items[index].name, name)) return -1;
    p->dirty = 1;
    return 0;
}

int gpr_find(const gpr_project *p, const char *item)
{
    int n, i;
    if (!p || !item || !*item) return -1;
    if (parse_int(item, &n) == 0) {
        if (n >= 1 && n <= p->n) return n - 1;
        return -1;
    }
    for (i = 0; i < p->n; i++)
        if (lower_eq(p->items[i].name, item)) return i;
    return -1;
}

int gpr_resolve(const gpr_project *p, int index, char *buf, int buflen)
{
    const char *file;
    if (!p || index < 0 || index >= p->n || !buf || buflen <= 0) return -1;
    file = p->items[index].file;
    if (path_is_abs(file)) return copy_str(buf, buflen, file);
    return path_join(buf, buflen, p->path, file);
}

int gpr_kicad_files(const char *pro, char *pcb, char *sch, int buflen)
{
    char stem[GPR_PATH_MAX + 1], dir[GPR_PATH_MAX + 1], want[GPR_PATH_MAX + 1];
    const char *base;
    char *cut;
    int n = 0;
    if (pcb && buflen > 0) pcb[0] = 0;
    if (sch && buflen > 0) sch[0] = 0;
    if (!pro || !ends_ci(pro, ".kicad_pro") || buflen <= 0) return -1;
    if (copy_str(dir, (int)sizeof dir, pro)) return -1;
    base = file_part(dir);
    stem_copy(stem, (int)sizeof stem, base);
    if (base != dir) {
        cut = (char *)base;
        if (cut > dir && (cut[-1] == '/' || cut[-1] == ':')) {
            if (cut[-1] == '/') cut[-1] = 0;
            else *cut = 0;
        }
    } else {
        copy_str(dir, (int)sizeof dir, ".");
    }
    if (path_join(want, (int)sizeof want, dir, stem)) return -1;
    {
        char path[GPR_PATH_MAX + 1];
        int wr = snprintf(path, sizeof path, "%s.kicad_pcb", want);
        if (wr < 0 || wr >= (int)sizeof path) return -1;
        if (file_exists(path) && pcb) {
            if (copy_str(pcb, buflen, path)) return -1;
            n++;
        }
        wr = snprintf(path, sizeof path, "%s.kicad_sch", want);
        if (wr < 0 || wr >= (int)sizeof path) return -1;
        if (file_exists(path) && sch) {
            if (copy_str(sch, buflen, path)) return -1;
            n++;
        }
    }
    return n;
}

void gpr_recent_init(gpr_recent *r)
{
    memset(r, 0, sizeof *r);
    r->size = (int)sizeof *r;
}

int gpr_recent_load(gpr_recent *r, const char *path, char *err, int errlen)
{
    char *text = NULL, *s;
    int line = 0;
    if (err && errlen > 0) err[0] = 0;
    if (!r || !path) return fail(err, errlen, 0, "no path");
    gpr_recent_init(r);
    if (read_all(path, &text, err, errlen)) {
        /* A missing recent list is empty, not an error. */
        if (err && errlen > 0 && strstr(err, "cannot open")) {
            err[0] = 0;
            return 0;
        }
        return -1;
    }
    s = text;
    while (*s) {
        char buf[GPR_PATH_MAX + 64], *key, *val, *eq, *hash;
        size_t n = strcspn(s, "\n");
        line++;
        if (n >= sizeof buf) {
            free(text);
            return fail(err, errlen, line, "too long");
        }
        memcpy(buf, s, n);
        buf[n] = 0;
        s += n + (s[n] == '\n');
        hash = strchr(buf, '#');
        if (hash) *hash = 0;
        key = trim(buf);
        if (!*key) continue;
        eq = strchr(key, '=');
        if (!eq) {
            /* A bare path is also taken, so an older one-path-a-line file works. */
            if (r->n < GPR_RECENT_MAX) {
                if (copy_str(r->path[r->n], (int)sizeof r->path[0], key)) {
                    free(text);
                    return fail(err, errlen, line, "path is too long");
                }
                r->n++;
            }
            continue;
        }
        *eq = 0;
        val = trim(eq + 1);
        key = trim(key);
        if (strcmp(key, "path") != 0) continue;
        if (r->n < GPR_RECENT_MAX) {
            if (copy_str(r->path[r->n], (int)sizeof r->path[0], val)) {
                free(text);
                return fail(err, errlen, line, "path is too long");
            }
            r->n++;
        }
    }
    free(text);
    return 0;
}

int gpr_recent_save(const gpr_recent *r, const char *path, char *err, int errlen)
{
    char buf[GPR_RECENT_MAX * (GPR_PATH_MAX + 16) + 80];
    int at = 0, i, wr;
    if (err && errlen > 0) err[0] = 0;
    if (!r || !path) return fail(err, errlen, 0, "no path");
    wr = snprintf(buf, sizeof buf, "# GFOpenMakerWorkshop recent projects\n");
    if (wr < 0) return -1;
    at = wr;
    for (i = 0; i < r->n; i++) {
        wr = snprintf(buf + at, sizeof buf - (size_t)at, "path = %s\n", r->path[i]);
        if (wr < 0 || at + wr >= (int)sizeof buf) return fail(err, errlen, 0, "too long");
        at += wr;
    }
    return write_all(path, buf, at, err, errlen);
}

int gpr_recent_touch(gpr_recent *r, const char *project)
{
    char keep[GPR_PATH_MAX + 1];
    int i, at = -1;
    if (!r || !project || !*project) return -1;
    if (copy_str(keep, (int)sizeof keep, project)) return -1;
    for (i = 0; i < r->n; i++)
        if (!strcmp(r->path[i], keep)) {
            at = i;
            break;
        }
    if (at >= 0) {
        for (i = at; i > 0; i--) memcpy(r->path[i], r->path[i - 1], sizeof r->path[0]);
    } else {
        if (r->n < GPR_RECENT_MAX) r->n++;
        for (i = r->n - 1; i > 0; i--) memcpy(r->path[i], r->path[i - 1], sizeof r->path[0]);
    }
    memcpy(r->path[0], keep, sizeof r->path[0]);
    return 0;
}
