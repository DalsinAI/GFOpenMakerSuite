/* OpenMakeBurn's layer list in the window (burn_ui.h).
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "burn_ui.h"

#include "omview_ui.h"

#include <stdio.h>
#include <string.h>

static const char layers_css[] =
    "#layers .swatch{width:12px;min-height:12px;border:1px solid #d0d0d0}";

static void id_of(char *buf, int n, int i, const char *what)
{
    if (what && what[0])
        snprintf(buf, (size_t)n, "layer-%d-%s", i, what);
    else
        snprintf(buf, (size_t)n, "layer-%d", i);
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
    char m[2048], css[96], id[32];
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
    if (!d) return 0;
    for (i = 0; i < d->nlayer; i++) {
        burn_layer ly;
        burn_layer_of(o, d, i, &ly);
        if (ly.colour == GV_NONE) continue;
        snprintf(m, sizeof m,
                 "<column id=\"layer-%d\">"
                 "<row>"
                 "<checkbox id=\"layer-%d-on\"></checkbox>"
                 "<div id=\"layer-%d-swatch\" class=\"swatch\"></div>"
                 "<label id=\"layer-%d-name\" class=\"name\">#%06x</label>"
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
                 "</column>",
                 i, i, i, i, (unsigned)ly.colour, i, i, (double)ly.power, i, i, (double)ly.speed, i, i, ly.passes, i, i,
                 i, i, (double)ly.interval);
        snprintf(css, sizeof css, "#layer-%d-swatch{background-color:#%06x}", i, (unsigned)ly.colour);
        if (!olui_add_css(ui, css) || !olui_add(ui, parent, m, err, (size_t)errlen)) return -1;
        id_of(id, (int)sizeof id, i, "on");
        olui_set_checked(ui, olui_find(ui, id), ly.on);
        id_of(id, (int)sizeof id, i, "mode");
        {
            olui_el *sel = olui_find(ui, id);
            if (sel) {
                olui_select_add(ui, sel, "Line", "line");
                olui_select_add(ui, sel, "Fill", "fill");
                olui_select_set(ui, sel, ly.mode == BURN_FILL ? 1 : 0);
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
        store[i].mode = (el && olui_select_index(el) == 1) ? BURN_FILL : BURN_LINE;
        store[i].set |= BURN_SET_MODE;
        id_of(id, (int)sizeof id, i, "interval");
        store[i].interval = omv_number(ui, id);
        store[i].set |= BURN_SET_INTERVAL;
    }
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

