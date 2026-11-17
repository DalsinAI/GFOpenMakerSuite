/* GFOpenMake's window. GFOpenLayout lays it out and draws it; make.c
 * only answers the buttons and draws the view. It is here, not in make.c,
 * so cad/tests/test_make_ui.c can lay it out on the host. The markup is the
 * window itself: wrapped in a <window> element, the view got no height.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef GS_MAKE_UI_H
#define GS_MAKE_UI_H

#include "omview_settings.h"

/* OpenLayout Clear (docs/HANDOVER.md): one light window, the tools in
 * grouped boxes down the left, a label beside its field, one status line
 * along the bottom, Save STL the accent primary button. The chosen tool
 * is :selected and lit in the accent too. Slice turns the part into a
 * toolpath, shown in the view, its layers up to the Layers slider's;
 * Save G-code writes it for the printer picked (core/ui's machine picker).
 * 640 by 480 inside, to fit a 640 by 512 PAL screen with the window's
 * borders: the name and version are in the title bar, not a heading row,
 * and the groups are a row shorter where they can be. The side is two
 * tabs, Model (File to Place) and Print (Layers, and Slice with core/ui's
 * print settings panel, omview_settings.h, put in "settings" by make.c),
 * as wide as that panel. Place's Size and At, a row for each axis, are the
 * bench's Size and place: the picked part's box in mm, and its bottom middle. */
static const char gm_markup[] =
    "<row id=\"body\">"
    "<column id=\"side\">"
    "<tabs id=\"side-tabs\"><tab title=\"Model\">"
    "<fieldset><legend>File</legend>"
    "<row><button id=\"open\">Open</button><button id=\"undo\">Undo</button>"
    "<button id=\"save\" class=\"primary\">Save STL</button></row>"
    "</fieldset>"
    "<fieldset><legend>Tool</legend>"
    "<row><button id=\"orbit\">Orbit</button><button id=\"move\">Move</button></row>"
    "<row><button id=\"rot\">Rotate</button><button id=\"scale\">Scale</button></row>"
    "</fieldset>"
    "<fieldset><legend>Parts</legend>"
    "<row><select id=\"shape\"></select><button id=\"add\">Add</button></row>"
    "<row><button id=\"join\">Join</button><button id=\"cut\">Cut</button><button id=\"meet\">Intersect</button></row>"
    "</fieldset>"
    "<fieldset><legend>Place</legend>"
    "<row><button id=\"drop\">Drop</button><button id=\"centre\">Centre</button><button id=\"lay\">Lay flat</button></row>"
    "<row><label></label><span class=\"head\">Size mm</span><span class=\"head\">At mm</span></row>"
    "<row><label for=\"sx\">X</label><spinner id=\"sx\" min=\"0.1\" max=\"999.9\" step=\"0.1\" value=\"0.0\"><spinner id=\"px\" min=\"-999.9\" max=\"999.9\" step=\"0.1\" value=\"0.0\"></row>"
    "<row><label for=\"sy\">Y</label><spinner id=\"sy\" min=\"0.1\" max=\"999.9\" step=\"0.1\" value=\"0.0\"><spinner id=\"py\" min=\"-999.9\" max=\"999.9\" step=\"0.1\" value=\"0.0\"></row>"
    "<row><label for=\"sz\">Z</label><spinner id=\"sz\" min=\"0.1\" max=\"999.9\" step=\"0.1\" value=\"0.0\"><spinner id=\"pz\" min=\"-999.9\" max=\"999.9\" step=\"0.1\" value=\"0.0\"></row>"
    "<row><label for=\"face\">Face</label><span id=\"face\">no face</span></row>"
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
    "spinner{flex:1}"
    "span.head{flex:1;color:#616161}"
    "slider{width:96px}"
    "#view{flex:1;border:1px solid #d0d0d0;background-color:#14161c}"
    "hr{background:#d0d0d0}";

#endif
