/* The whole window. GFOpenLayout lays this out; the program answers
 * Open, Slice, the layer slider and Save G-code, and draws the view. The
 * print settings are core/ui's panel (omview_settings.h), in the side
 * column beside the view, under the printer picked. 640 by 480
 * inside, to fit a 640 by 512 PAL screen with the window's borders: the
 * name and version are in the title bar. OpenLayout Clear, as GFOpenMake's
 * (cad/src/make_ui.h): one light window, grouped boxes, one status line
 * along the bottom, Slice the accent primary button. The markup is the
 * window itself: wrapped in a <window> element, the view got no height.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef GS_MARKUP_H
#define GS_MARKUP_H

#include "omview_settings.h"

static const char gs_markup[] =
    "<fieldset><legend>Part</legend>"
    "<row>"
    "<label for=\"path\">STL</label>"
    "<input id=\"path\">"
    "<button id=\"open\">Open</button>"
    "<button id=\"slice\" class=\"primary\">Slice</button>"
    "</row>"
    "</fieldset>"
    "<row id=\"body\">"
    "<column id=\"side\">"
    "<fieldset><legend>Settings</legend>"
    "<row><label for=\"machine\">Printer</label><select id=\"machine\"></select></row>"
    "<row><label for=\"support\">Support</label><checkbox id=\"support\">On</checkbox></row>"
    "<row><label for=\"start\">Start</label><input id=\"start\"></row>"
    "<row><label for=\"end\">End</label><input id=\"end\"></row>"
    "<div id=\"settings\"></div>"
    "</fieldset>"
    "</column>"
    "<div id=\"view\"></div>"
    "</row>"
    "<row id=\"layerbar\">"
    "<checkbox id=\"show\">Show layers to</checkbox>"
    "<button id=\"prev\">Prev</button>"
    "<slider id=\"layerno\" min=\"1\" max=\"100\" value=\"100\">"
    "<button id=\"next\">Next</button>"
    "<button id=\"gcode\">Save G-code</button>"
    "</row>"
    "<hr>"
    "<div id=\"status\">Open an STL.</div>";

static const char gs_css[] =
    "window{background:#f9f9f9;color:#1b1b1b;font-size:12px;padding:8px;gap:6px}"
    "fieldset{border:1px solid #d0d0d0;border-radius:8px;padding:0 8px 6px;gap:6px}"
    "fieldset row{padding:0;gap:6px}"
    "button{border:1px solid #d0d0d0;border-radius:4px;color:#1b1b1b;background:linear-gradient(#fefefe,#fbfbfb)}"
    "button:active{background:#f0f0f0}"
    "button:focus{border-color:#0067c0}"
    "button.primary{background:#0067c0;border-color:#0067c0;color:#ffffff}"
    "button.primary:active{background:#005ba1}"
    "label{color:#616161}"
    "#body{flex:1;padding:0;gap:8px}"
    "#side{width:" OMV_SETTINGS_W_CSS "}"
    "fieldset row label{width:48px}"
    "#path,#start,#end{flex:1}"
    "#layerbar{padding:0;gap:6px}"
    "slider{flex:1}"
    "select{flex:1}"
    "#view{flex:1;border:1px solid #d0d0d0;background-color:#14161c}"
    "hr{background:#d0d0d0}";

#endif
