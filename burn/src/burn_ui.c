/* OpenMakeBurn's layer list in the window (burn_ui.h).
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "burn_ui.h"

#include "omview_ui.h"

#include <stdio.h>
#include <string.h>

static const char layers_css[] =
    "#layers .swatch{width:12px;min-height:12px;border:1px solid #d0d0d0}"
    "#layers .shift{width:28px;min-width:28px;padding:0 2px}";

static void id_of(char *buf, int n, int i, const char *what)
{
    if (what && what[0])
        snprintf(buf, (size_t)n, "layer-%d-%s", i, what);
    else
        snprintf(buf, (size_t)n, "layer-%d", i);
}


static int layer_id(const char *id)
{
    int n = 0;
    if (!id || strncmp(id, "layer-", 6) != 0) return -1;
    id += 6;
    if (*id < '0' || *id > '9') return -1;
    while (*id >= '0' && *id <= '9') n = n * 10 + (*id++ - '0');
    if (*id) return -1;
    return n;
}

static void list_order(const burn_opts *o, int *ord, int n)
{
    int i, k = 0, used[256];
    for (i = 0; i < n; i++) used[i] = 0;
    if (o && o->seq && o->nseq > 0) {
        for (i = 0; i < o->nseq && k < n; i++) {
            int L = o->seq[i];
            if (L < 0 || L >= n || used[L]) continue;
            ord[k++] = L;
            used[L] = 1;
        }
    }
    for (i = 0; i < n; i++)
        if (!used[i]) ord[k++] = i;
}

int burn_ui_count(olui *ui)
{
    char id[32];
    int n = 0;
    for (;;) {
        id_of(id, (int)sizeof id, n, NULL);
        if (!olui_find(ui, id)) return n;
        n++;
        if (n > 10000) return n;
    }
}

int burn_ui_layers(olui *ui, const gv_doc *d, const burn_opts *o, char *err, int errlen)
{
    olui_el *parent = olui_find(ui, "layers");
    char m[2560], css[96], id[32];
    int i;
    if (!parent) {
        snprintf(err, (size_t)errlen, "the layer list is missing");
        return -1;
    }
    while (olui_first_child(parent)) olui_remove(ui, olui_first_child(parent));
    if (!olui_add_css(ui, layers_css)) {
        snprintf(err, (size_t)errlen, "the layer list's style did not load");
        return -1;
    }
    if (o) {
        olui_el *cl = olui_find(ui, "cuts_last");
        if (cl) olui_set_checked(ui, cl, o->cuts_last);
    }
    if (!d) return 0;
    {
        int ord[256];
        if (d->nlayer > 256) {
            snprintf(err, (size_t)errlen, "too many layers");
            return -1;
        }
        list_order(o, ord, d->nlayer);
        for (i = 0; i < d->nlayer; i++) {
        int L = ord[i];
        burn_layer ly;
        burn_layer_of(o, d, L, &ly);
        if (ly.colour == GV_NONE) continue;
        snprintf(m, sizeof m,
                 "<column id=\"layer-%d\">"
                 "<row>"
                 "<checkbox id=\"layer-%d-on\"></checkbox>"
                 "<div id=\"layer-%d-swatch\" class=\"swatch\"></div>"
                 "<label id=\"layer-%d-name\" class=\"name\">#%06x</label>"
                 "<button id=\"layer-%d-up\" class=\"shift\">Up</button>"
                 "<button id=\"layer-%d-down\" class=\"shift\">Dn</button>"
                 "</row>"
                 "<row><label for=\"layer-%d-power\">Power</label>"
                 "<spinner id=\"layer-%d-power\" min=\"0\" max=\"100\" step=\"1\" value=\"%.0f\">"
                 "<span>%%</span></row>"
                 "<row><label for=\"layer-%d-speed\">mm/s</label>"
                 "<spinner id=\"layer-%d-speed\" min=\"0.1\" max=\"500\" step=\"0.1\" value=\"%.1f\"></row>"
                 "<row><label for=\"layer-%d-passes\">Pass</label>"
                 "<spinner id=\"layer-%d-passes\" min=\"1\" max=\"100\" step=\"1\" value=\"%d\"></row>"
                 "<row><label for=\"layer-%d-mode\">Mode</label>"
                 "<select id=\"layer-%d-mode\"></select></row>"
                 "<row><label for=\"layer-%d-interval\">Gap</label>"
                 "<spinner id=\"layer-%d-interval\" min=\"0.01\" max=\"20\" step=\"0.01\" value=\"%.2f\">"
                 "<span>mm</span></row>"
                 "<row><label for=\"layer-%d-kerf\">Kerf</label>"
                 "<spinner id=\"layer-%d-kerf\" min=\"0\" max=\"20\" step=\"0.01\" value=\"%.2f\">"
                 "<span>mm</span></row>"
                 "<row><label class=\"pair\" for=\"layer-%d-tabs\">Tabs</label>"
                 "<spinner id=\"layer-%d-tabs\" min=\"0\" max=\"32\" step=\"1\" value=\"%d\">"
                 "<label class=\"pair\" for=\"layer-%d-tabw\">Width</label>"
                 "<spinner id=\"layer-%d-tabw\" min=\"0\" max=\"20\" step=\"0.01\" value=\"%.2f\"></row>"
                 "</column>",
                 L, L, L, L, (unsigned)ly.colour, L, L, L, L, (double)ly.power, L, L, (double)ly.speed, L, L, ly.passes, L, L,
                 L, L, (double)ly.interval, L, L, (double)ly.kerf, L, L, ly.tabs, L, L, (double)ly.tabw);
        snprintf(css, sizeof css, "#layer-%d-swatch{background-color:#%06x}", L, (unsigned)ly.colour);
        if (!olui_add_css(ui, css) || !olui_add(ui, parent, m, err, (size_t)errlen)) return -1;
        id_of(id, (int)sizeof id, L, "on");
        olui_set_checked(ui, olui_find(ui, id), ly.on);
        id_of(id, (int)sizeof id, L, "mode");
        {
            olui_el *sel = olui_find(ui, id);
            if (sel) {
                olui_select_add(ui, sel, "Line", "line");
                olui_select_add(ui, sel, "Fill", "fill");
                olui_select_add(ui, sel, "Offset", "offset");
                olui_select_set(ui, sel, ly.mode == BURN_OFFSET ? 2 : ly.mode == BURN_FILL ? 1 : 0);
            }
        }
        }
    }
    return 0;
}

void burn_ui_read(olui *ui, burn_opts *o, burn_layer *store, int cap)
{
    int i, n = burn_ui_count(ui);
    olui_el *con;
    if (n > cap) n = cap;
    if (o) {
        con = olui_find(ui, "constant");
        o->constant = con && olui_get_checked(con);
        con = olui_find(ui, "cuts_last");
        o->cuts_last = con && olui_get_checked(con);
        if (olui_find(ui, "place-x")) o->x = omv_number(ui, "place-x");
        if (olui_find(ui, "place-y")) o->y = omv_number(ui, "place-y");
        if (olui_find(ui, "place-rot")) o->rot = omv_number(ui, "place-rot");
        o->layer = (n && store) ? store : NULL;
        o->nlayer = (n && store) ? n : 0;
    }
    if (!store) return;
    for (i = 0; i < n; i++) {
        char id[32];
        const char *name;
        olui_el *el;
        memset(&store[i], 0, sizeof store[i]);
        store[i].size = (int)sizeof store[i];
        store[i].index = i + 1;
        store[i].colour = GV_NONE;
        id_of(id, (int)sizeof id, i, "name");
        el = olui_find(ui, id);
        name = el ? olui_get_text(el) : NULL;
        if (name && name[0] == '#') {
            unsigned c = 0;
            int k;
            for (k = 1; name[k]; k++) {
                char ch = name[k];
                int h = (ch >= '0' && ch <= '9') ? ch - '0' : (ch >= 'a' && ch <= 'f') ? ch - 'a' + 10
                        : (ch >= 'A' && ch <= 'F')                           ? ch - 'A' + 10
                                                                             : -1;
                if (h < 0) break;
                c = (c << 4) | (unsigned)h;
            }
            if (k == 7) store[i].colour = c;
        }
        id_of(id, (int)sizeof id, i, "power");
        store[i].power = omv_number(ui, id);
        store[i].set |= BURN_SET_POWER;
        id_of(id, (int)sizeof id, i, "speed");
        store[i].speed = omv_number(ui, id);
        store[i].set |= BURN_SET_SPEED;
        id_of(id, (int)sizeof id, i, "passes");
        store[i].passes = (int)(omv_number(ui, id) + 0.5f);
        store[i].set |= BURN_SET_PASSES;
        id_of(id, (int)sizeof id, i, "on");
        el = olui_find(ui, id);
        store[i].on = !el || olui_get_checked(el);
        store[i].set |= BURN_SET_ON;
        id_of(id, (int)sizeof id, i, "mode");
        el = olui_find(ui, id);
        {
            int idx = el ? olui_select_index(el) : 0;
            store[i].mode = idx == 2 ? BURN_OFFSET : idx == 1 ? BURN_FILL : BURN_LINE;
        }
        store[i].set |= BURN_SET_MODE;
        id_of(id, (int)sizeof id, i, "interval");
        store[i].interval = omv_number(ui, id);
        store[i].set |= BURN_SET_INTERVAL;
        id_of(id, (int)sizeof id, i, "kerf");
        store[i].kerf = omv_number(ui, id);
        store[i].set |= BURN_SET_KERF;
        id_of(id, (int)sizeof id, i, "tabs");
        store[i].tabs = (int)(omv_number(ui, id) + 0.5f);
        store[i].set |= BURN_SET_TABS;
        id_of(id, (int)sizeof id, i, "tabw");
        store[i].tabw = omv_number(ui, id);
        store[i].set |= BURN_SET_TABW;
    }
    burn_job_from_layers(o);
}


int burn_ui_order(olui *ui, int *seq, int cap)
{
    olui_el *parent, *ch;
    int n = 0;
    if (!ui || !seq || cap <= 0) return 0;
    parent = olui_find(ui, "layers");
    if (!parent) return 0;
    for (ch = olui_first_child(parent); ch && n < cap; ch = olui_next(ch)) {
        int L = layer_id(olui_id(ch));
        if (L < 0) continue;
        seq[n++] = L;
    }
    return n;
}

int burn_ui_move(olui *ui, const gv_doc *d, burn_opts *o, burn_layer *store, int cap, int *seq, int seqcap, int L,
                 int dir, char *err, int errlen)
{
    int n, i, at = -1, tmp;
    if (!ui || !d || !o || !seq) {
        if (err && errlen > 0) snprintf(err, (size_t)errlen, "the layer list is missing");
        return -1;
    }
    burn_ui_read(ui, o, store, cap);
    n = burn_ui_order(ui, seq, seqcap);
    for (i = 0; i < n; i++)
        if (seq[i] == L) at = i;
    if (at >= 0 && dir < 0 && at > 0) {
        tmp = seq[at];
        seq[at] = seq[at - 1];
        seq[at - 1] = tmp;
    } else if (at >= 0 && dir > 0 && at + 1 < n) {
        tmp = seq[at];
        seq[at] = seq[at + 1];
        seq[at + 1] = tmp;
    }
    o->seq = seq;
    o->nseq = n;
    return burn_ui_layers(ui, d, o, err, errlen);
}


static float scale_dec(int dec)
{
    float d = 1.f;
    int k;
    for (k = 0; k < dec; k++) d *= 10.f;
    return d;
}

static void set_place(olui *ui, const char *id, float v)
{
    olui_el *el = olui_find(ui, id);
    int n;
    float d;
    if (!el) return;
    d = scale_dec(olui_number_decimals(el));
    n = (int)(v * d + (v < 0.f ? -0.5f : 0.5f));
    olui_set_number(ui, el, n);
}

void burn_ui_place(olui *ui, const burn_opts *o)
{
    if (!ui || !o) return;
    set_place(ui, "place-x", o->x);
    set_place(ui, "place-y", o->y);
    set_place(ui, "place-rot", o->rot);
}

int burn_ui_materials_fill(olui *ui, const char *id, const burn_materials *l)
{
    olui_el *sel = olui_find(ui, id);
    int i;
    if (!sel) return 0;
    olui_select_clear(ui, sel);
    olui_select_add(ui, sel, "None", "none");
    if (l) {
        for (i = 0; i < l->n; i++) olui_select_add(ui, sel, l->at[i].name, NULL);
    }
    olui_select_set(ui, sel, 0);
    return l ? l->n : 0;
}

int burn_ui_materials_chosen(olui *ui, const char *id, const burn_materials *l)
{
    olui_el *sel = olui_find(ui, id);
    int i = sel ? olui_select_index(sel) : 0;
    if (!l || i <= 0 || i > l->n) return -1;
    return i - 1;
}

void burn_ui_material(olui *ui, burn_opts *o, const burn_material *m)
{
    int i, n;
    olui_el *el;
    if (!m) return;
    if (o) burn_material_apply(m, o);
    if (ui && (m->set & BURN_SET_CONSTANT)) {
        el = olui_find(ui, "constant");
        if (el) olui_set_checked(ui, el, m->constant);
    }
    if (!ui) return;
    n = burn_ui_count(ui);
    for (i = 0; i < n; i++) {
        char id[32];
        int dec;
        if (m->set & BURN_SET_POWER) {
            id_of(id, (int)sizeof id, i, "power");
            el = olui_find(ui, id);
            if (el) {
                dec = olui_number_decimals(el);
                olui_set_number(ui, el, (int)(m->power * scale_dec(dec) + 0.5f));
            }
        }
        if (m->set & BURN_SET_SPEED) {
            id_of(id, (int)sizeof id, i, "speed");
            el = olui_find(ui, id);
            if (el) {
                dec = olui_number_decimals(el);
                olui_set_number(ui, el, (int)(m->speed * scale_dec(dec) + 0.5f));
            }
        }
        if (m->set & BURN_SET_PASSES) {
            id_of(id, (int)sizeof id, i, "passes");
            el = olui_find(ui, id);
            if (el) olui_set_number(ui, el, m->passes);
        }
        if (m->set & BURN_SET_MODE) {
            id_of(id, (int)sizeof id, i, "mode");
            el = olui_find(ui, id);
            if (el) olui_select_set(ui, el, m->mode == BURN_OFFSET ? 2 : m->mode == BURN_FILL ? 1 : 0);
        }
        if (m->set & BURN_SET_INTERVAL) {
            id_of(id, (int)sizeof id, i, "interval");
            el = olui_find(ui, id);
            if (el) {
                dec = olui_number_decimals(el);
                olui_set_number(ui, el, (int)(m->interval * scale_dec(dec) + 0.5f));
            }
        }
    }
}

void burn_ui_kind(olui *ui, const char *path)
{
    olui_el *el = olui_find(ui, "kind");
    if (el) olui_set_text(ui, el, burn_drawing_kind(path));
}

void burn_ui_status(char *buf, int len, const char *name, int moves, float seconds)
{
    char time[40];
    if (seconds > 0.f && seconds < 60.f)
        snprintf(time, sizeof time, ", about %.1f s", (double)seconds);
    else if (seconds >= 60.f) {
        int s = (int)(seconds + 0.5f);
        snprintf(time, sizeof time, ", about %d min %d s", s / 60, s % 60);
    } else
        time[0] = 0;
    snprintf(buf, (size_t)len, "%d moves for %s%s", moves, (name && name[0]) ? name : "the laser", time);
}

