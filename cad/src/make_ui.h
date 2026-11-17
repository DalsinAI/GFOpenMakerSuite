/* GFOpenMake's window. GFOpenLayout lays it out and draws it; make.c
 * only answers the buttons and draws the view. It is here, not in make.c,
 * so cad/tests/test_make_ui.c can lay it out on the host. The markup is the
 * window itself: wrapped in a <window> element, the view got no height.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef GS_MAKE_UI_H
#define GS_MAKE_UI_H

#include <stdio.h>
#include <string.h>
#include "gs_parts.h"
#include "gs_view.h"
#include "omview_settings.h"

/* OpenLayout Clear (docs/HANDOVER.md): one light window, the tools in
 * grouped boxes down the left, a label beside its field, one status line
 * along the bottom, Save STL the accent primary button. The chosen tool
 * is :selected and lit in the accent too. Slice turns the part into a
 * toolpath, shown in the view, its layers up to the Layers slider's;
 * Save G-code writes it for the printer picked (core/ui's machine picker).
 * 640 by 480 inside, to fit a 640 by 512 PAL screen with the window's
 * borders: the name and version are in the title bar, not a heading row,
 * and the groups are a row shorter where they can be. The side is five
 * tabs, titles of five letters so the strip fits the side in Topaz 8:
 * Model (File, Tool, Place), Parts (Make: Add, Join, Cut, Intersect; and
 * the bench's parts list: the list, the picked part's name, its six
 * metals, Hide and Delete; Arrange: Copy, Line X and Y, Ring, Mirror,
 * Turn, Stack and Solo), Holes (the bench's bolt circle, hole row,
 * counterbore, one drill, a 90 degree countersink and a washer), Shape (the
 * bench's Extrude: a block, round or slot added on the bed, stood on the
 * picked part's top, or cut down from it; Face: the picked face pulled
 * by a distance, chamfered, or dragged; and Section: hide what is above
 * a height, or split the part there into two. Draw drags that footprint
 * on the bed) and Print (Layers, and Slice
 * with core/ui's print settings panel, omview_settings.h, put in
 * "settings" by make.c), as wide as that panel. Place's Size and At, a row
 * for each axis, are the
 * bench's Size and place: the picked part's box in mm, and its bottom middle. */
/* A key no field took. With Amiga held (command): Amiga-Z undoes,
 * Amiga-Shift-Z and Amiga-Y redo, as the Amiga's programs. Without: P
 * drags the picked face to pull it, Escape leaves that and Draw, as the bench's.
 * C hides what is above a height. [ and ] move that height. Anything
 * else is ignored. */
enum { GM_KEY_UNDO = 1, GM_KEY_REDO, GM_KEY_PULL, GM_KEY_LEAVE, GM_KEY_SECTION, GM_KEY_SECT_DOWN, GM_KEY_SECT_UP };
static int gm_command_key(unsigned code, int command, int shift)
{
    if (!command) {
        if (code == 'p' || code == 'P') return GM_KEY_PULL;
        if (code == 27) return GM_KEY_LEAVE;
        if (code == 'c' || code == 'C') return GM_KEY_SECTION;
        if (code == '[') return GM_KEY_SECT_DOWN;
        if (code == ']') return GM_KEY_SECT_UP;
        return 0;
    }
    if (code == 'y' || code == 'Y' || ((code == 'z' || code == 'Z') && shift)) return GM_KEY_REDO;
    return code == 'z' || code == 'Z' ? GM_KEY_UNDO : 0;
}

/* The side's tabs, in order. */
enum { GM_TAB_MODEL, GM_TAB_PARTS, GM_TAB_HOLES, GM_TAB_SHAPE, GM_TAB_PRINT };

/* In two, each under C99's 4095 a string; gm_markup() joins them. */
static const char gm_markup_a[] =
    "<row id=\"body\">"
    "<column id=\"side\">"
    "<tabs id=\"side-tabs\"><tab title=\"Model\">"
    "<fieldset><legend>File</legend>"
    "<row><button id=\"open\">Open</button><button id=\"save\" class=\"primary\">Save STL</button></row>"
    "<row><button id=\"undo\">Undo</button><button id=\"redo\">Redo</button></row>"
    "</fieldset>"
    "<fieldset><legend>Tool</legend>"
    "<row><button id=\"orbit\">Orbit</button><button id=\"move\">Move</button></row>"
    "<row><button id=\"rot\">Rotate</button><button id=\"scale\">Scale</button><button id=\"measure\">Measure</button></row>"
    "</fieldset>"
    "<fieldset><legend>Place</legend>"
    "<row><button id=\"drop\">Drop</button><button id=\"centre\">Centre</button><button id=\"lay\">Lay flat</button></row>"
    "<row><label></label><span class=\"head\">Size mm</span><span class=\"head\">At mm</span></row>"
    "<row><label for=\"sx\">X</label><spinner id=\"sx\" min=\"0.1\" max=\"999.9\" step=\"0.1\" value=\"0.0\"><spinner id=\"px\" min=\"-999.9\" max=\"999.9\" step=\"0.1\" value=\"0.0\"></row>"
    "<row><label for=\"sy\">Y</label><spinner id=\"sy\" min=\"0.1\" max=\"999.9\" step=\"0.1\" value=\"0.0\"><spinner id=\"py\" min=\"-999.9\" max=\"999.9\" step=\"0.1\" value=\"0.0\"></row>"
    "<row><label for=\"sz\">Z</label><spinner id=\"sz\" min=\"0.1\" max=\"999.9\" step=\"0.1\" value=\"0.0\"><spinner id=\"pz\" min=\"-999.9\" max=\"999.9\" step=\"0.1\" value=\"0.0\"></row>"
    "<row><label for=\"face\">Face</label><span id=\"face\">no face</span></row>"
    "</fieldset>"
    "</tab><tab title=\"Parts\">"
    "<fieldset><legend>Make</legend>"
    "<row><select id=\"shape\"></select><button id=\"add\">Add</button></row>"
    "<row><button id=\"join\">Join</button><button id=\"cut\">Cut</button><button id=\"meet\">Intersect</button></row>"
    "</fieldset>"
    "<fieldset><legend>List</legend>"
    "<list id=\"plist\"></list>"
    "<row><label for=\"pname\">Name</label><input id=\"pname\" maxlength=\"48\"></row>"
    "<row><button id=\"c0\" class=\"sw\" title=\"Brass\"></button><button id=\"c1\" class=\"sw\" title=\"Sand\"></button>"
    "<button id=\"c2\" class=\"sw\" title=\"Copper\"></button><button id=\"c3\" class=\"sw\" title=\"Steel\"></button>"
    "<button id=\"c4\" class=\"sw\" title=\"Teal\"></button><button id=\"c5\" class=\"sw\" title=\"Sage\"></button></row>"
    "<row><button id=\"pshow\">Hide</button><button id=\"pdel\">Delete</button></row>"
    "</fieldset>"
    "<fieldset><legend>Arrange</legend>"
    "<row><label for=\"acount\">Copies</label><spinner id=\"acount\" min=\"2\" max=\"12\" step=\"1\" value=\"3\">"
    "<label for=\"agap\">Gap</label><spinner id=\"agap\" min=\"0.0\" max=\"80.0\" step=\"0.1\" value=\"6.0\"></row>"
    "<row><button id=\"dup\" class=\"ar\" title=\"A copy beside the picked part\">Copy</button>"
    "<button id=\"linex\" class=\"ar\" title=\"Copies in all, along X, gap apart\">Line X</button>"
    "<button id=\"liney\" class=\"ar\" title=\"Copies in all, along Y, gap apart\">Line Y</button>"
    "<button id=\"ring\" class=\"ar\" title=\"Copies in all, round the bed's middle\">Ring</button></row>"
    "<row><button id=\"mirror\" class=\"ar\" title=\"Mirrored through X\">Mirror</button>"
    "<button id=\"turn\" class=\"ar\" title=\"Turned 90 degrees around Z\">Turn</button>"
    "<button id=\"stack\" class=\"ar\" title=\"On the part before it in the list\">Stack</button>"
    "<button id=\"solo\" class=\"ar\" title=\"Only this part shown, or every part again\">Solo</button></row>"
    "</fieldset>"
    ;
static const char gm_markup_b[] =
    "</tab><tab title=\"Holes\">"
    "<fieldset><legend>Drill</legend>"
    "<row><label for=\"hd\">Drill</label><spinner id=\"hd\" min=\"0.5\" max=\"40.0\" step=\"0.1\" value=\"3.0\"><span class=\"unit\">mm</span></row>"
    "<row><label for=\"hn\">Holes</label><spinner id=\"hn\" min=\"2\" max=\"12\" step=\"1\" value=\"4\"><span class=\"unit\"></span></row>"
    "</fieldset>"
    "<fieldset><legend>Pattern</legend>"
    "<row><label for=\"hpcd\">PCD</label><spinner id=\"hpcd\" min=\"1.0\" max=\"180.0\" step=\"0.1\" value=\"16.0\"><span class=\"unit\">mm</span></row>"
    "<row><label for=\"hpitch\">Pitch</label><spinner id=\"hpitch\" min=\"1.0\" max=\"80.0\" step=\"0.1\" value=\"8.0\"><span class=\"unit\">mm</span></row>"
    "<row><button id=\"bolt\">Bolt circle</button><button id=\"row\">Hole row</button></row>"
    "</fieldset>"
    "<fieldset><legend>Counterbore</legend>"
    "<row><label for=\"hcb\">Bore</label><spinner id=\"hcb\" min=\"1.0\" max=\"80.0\" step=\"0.1\" value=\"16.0\"><span class=\"unit\">mm</span></row>"
    "<row><label for=\"hdeep\">Deep</label><spinner id=\"hdeep\" min=\"0.4\" max=\"40.0\" step=\"0.1\" value=\"4.0\"><span class=\"unit\">mm</span></row>"
    "<row><button id=\"drill\">Drill through</button><button id=\"cbore\">Counterbore</button></row>"
    "<row><button id=\"csink\">Countersink</button><button id=\"washer\">32 mm washer</button></row>"
    "</fieldset>"
    "</tab><tab title=\"Shape\">"
    "<fieldset><legend>Sketch</legend>"
    "<row><label for=\"xkind\">Shape</label><select id=\"xkind\"></select></row>"
    "<row><label for=\"xw\">Width</label><spinner id=\"xw\" min=\"1.0\" max=\"200.0\" step=\"0.1\" value=\"20.0\"><span class=\"unit\">mm</span></row>"
    "<row><label for=\"xd\">Depth</label><spinner id=\"xd\" min=\"1.0\" max=\"200.0\" step=\"0.1\" value=\"10.0\"><span class=\"unit\">mm</span></row>"
    "<row><label for=\"xh\">Height</label><spinner id=\"xh\" min=\"0.4\" max=\"120.0\" step=\"0.1\" value=\"8.0\"><span class=\"unit\">mm</span></row>"
    "</fieldset>"
    "<fieldset><legend>Extrude</legend>"
    "<row><button id=\"xadd\">Add</button><button id=\"xon\">On top</button><button id=\"xcut\">Cut</button></row>"
    "<row><button id=\"xdraw\" title=\"Drag the footprint on the bed. Escape leaves\">Draw</button></row>"
    "</fieldset>"
    "<fieldset><legend>Face</legend>"
    "<row><label for=\"fmm\">By</label><spinner id=\"fmm\" min=\"-99.9\" max=\"99.9\" step=\"0.1\" value=\"5.0\"><span class=\"unit\">mm</span></row>"
    "<row><button id=\"pull\" title=\"The picked face along its normal\">Pull</button>"
    "<button id=\"chamfer\" title=\"The picked face's edges cut in\">Chamfer</button>"
    "<button id=\"fdrag\" title=\"Drag the picked face along its normal (P); Escape leaves\">Drag</button></row>"
    "<row><checkbox id=\"sect\" title=\"Hide what is above At (C). [ and ] move it\">Section</checkbox>"
    "<label for=\"sectz\">At</label><spinner id=\"sectz\" min=\"0.0\" max=\"220.0\" step=\"0.1\" value=\"10.0\"></row>"
    "<row><button id=\"split\" title=\"Cut the picked part into two at At\">Split</button></row>"
    "</fieldset>"
    "</tab><tab title=\"Print\">"
    "<fieldset><legend>Layers</legend>"
    "<row><checkbox id=\"show\">Show a layer</checkbox></row>"
    "<row><button id=\"prev\">Prev</button><slider id=\"layer\" min=\"1\" max=\"100\" value=\"1\"><button id=\"next\">Next</button></row>"
    "</fieldset>"
    "<fieldset><legend>Slice</legend>"
    "<row><label for=\"machine\">Printer</label><select id=\"machine\"></select></row>"
    "<div id=\"settings\"></div>"
    "<row><button id=\"slice\">Slice</button><button id=\"gcode\">Save G-code</button></row>"
    "</fieldset>"
    "</tab></tabs>"
    "</column>"
    "<div id=\"view\"></div>"
    "</row>"
    "<hr>"
    "<div id=\"status\">A 20 mm cube, with a hole.</div>";

static const char *gm_markup(void)
{
    static char m[sizeof gm_markup_a + sizeof gm_markup_b];
    if (!m[0]) {
        memcpy(m, gm_markup_a, sizeof gm_markup_a - 1);
        memcpy(m + sizeof gm_markup_a - 1, gm_markup_b, sizeof gm_markup_b);
    }
    return m;
}

/* The Parts tab from a parts list: a line for each part (its name, and
 * "(hidden)" when hidden), the picked one selected and its name in the
 * field, Hide or Show, its metal lit; Delete off for the last part. */
/* The view's colours: a run per shown part, its metal, in the order
 * gs_parts_scene puts their faces. How many, up to cap. */
static int gm_tints(gs_parts *ps, gs_tint *t, int cap)
{
    int i, n = 0;
    for (i = 0; ps && i < gs_parts_count(ps) && n < cap; i++)
        if (gs_parts_shown(ps, i) == 1) {
            t[n].n = gs_parts_mesh(ps, i)->n;
            t[n].rgb = gs_parts_rgb(gs_parts_colour(ps, i));
            n++;
        }
    return n;
}

static void gm_parts_list(olui *ui, const gs_parts *ps)
{
    olui_el *list = olui_find(ui, "plist"), *field = olui_find(ui, "pname"), *item = NULL;
    int n = gs_parts_count(ps), at = gs_parts_picked(ps), i;
    const char *name = at >= 0 ? gs_parts_name(ps, at) : "";
    olui_list_clear(ui, list);
    for (i = 0; i < n; i++) {
        char line[64], v[12];
        olui_el *el;
        snprintf(line, sizeof line, "%s%s", gs_parts_name(ps, i), gs_parts_shown(ps, i) ? "" : " (hidden)");
        snprintf(v, sizeof v, "%d", i);
        el = olui_list_add(ui, list, NULL, line, v);
        if (i == at) item = el;
    }
    if (item) olui_list_select(ui, list, item);
    if (field && strcmp(olui_get_text(field), name)) olui_set_text(ui, field, name);
    olui_set_text(ui, olui_find(ui, "pshow"), at >= 0 && !gs_parts_shown(ps, at) ? "Show" : "Hide");
    olui_set_disabled(ui, olui_find(ui, "pdel"), n < 2);
    for (i = 0; i < 6; i++) {
        char id[4] = {'c', (char)('0' + i), 0};
        olui_set_selected(ui, olui_find(ui, id), at >= 0 && gs_parts_colour(ps, at) == i);
    }
}

static const char gm_css[] =
    "window{background:#f9f9f9;color:#1b1b1b;font-size:12px;padding:8px;gap:6px}"
    "#body{flex:1;padding:0;gap:8px}"
    "#side{width:" OMV_SETTINGS_W_CSS ";gap:4px}"
    "#side-tabs{flex:1;gap:4px}"
    "#side-tabs fieldset{padding:0 2px 6px}"
    "fieldset{border:1px solid #d0d0d0;border-radius:8px;padding:0 8px 6px}"
    "fieldset row{padding:0;gap:6px}"
    "button{flex:1;border:1px solid #d0d0d0;border-radius:4px;color:#1b1b1b;"
    "background:linear-gradient(#fefefe,#fbfbfb)}"
    "button:active{background:#f0f0f0}"
    "button:focus{border-color:#0067c0}"
    "button:selected,button.primary{background:#0067c0;border-color:#0067c0;color:#ffffff}"
    "button.primary:active{background:#005ba1}"
    "label{width:40px;color:#616161}"
    "select{flex:1}"
    "#machine{width:150px}"
    "spinner{flex:1}"
    "span.unit{width:24px;color:#616161}"
    "span.head{flex:1;color:#616161}"
    "slider{width:96px}"
    "#view{flex:1;border:1px solid #d0d0d0;background-color:#14161c}"
    "#side-tabs tabbutton{flex:1;padding:3px 1px}"
    "#plist{height:98px;overflow:auto;border:1px solid #d0d0d0;background:#ffffff}"
    "#acount{width:52px}#agap{width:64px}button.ar{flex:1;padding:3px 2px}"
    "#pname{flex:1;border:1px solid #d0d0d0;padding:3px 4px;background:#ffffff}"
    "button.sw{flex:1;height:20px;padding:0;border-radius:4px}"
    "#c0{background:#c4a15a}#c1{background:#d7c4a2}#c2{background:#c47a5a}"
    "#c3{background:#8aa0a8}#c4{background:#3e8f96}#c5{background:#a3b18a}"
    "button.sw:selected{border:2px solid #0067c0}"
    "hr{background:#d0d0d0}";

#endif
