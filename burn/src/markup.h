/* The whole window. GFOpenLayout lays this out; the program answers Open,
 * Preview, Frame and Save G-code, and draws the view. The layer list and
 * the laser picker sit in the side column beside the view. 640 by 480
 * inside, to fit a 640 by 512 PAL screen with the window's borders: the
 * name and version are in the title bar. OpenLayout Clear, as the slicer's
 * (slicer/src/markup.h): one light window, grouped boxes, one status line
 * along the bottom, Preview the accent primary button. The markup is the
 * window itself: wrapped in a <window> element, the view got no height.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef BURN_MARKUP_H
#define BURN_MARKUP_H

#include "omview_settings.h"

static const char burn_markup[] =
    "<fieldset><legend>Drawing</legend>"
    "<row>"
    "<label for=\"path\">SVG</label>"
    "<input id=\"path\">"
    "<button id=\"open\">Open</button>"
    "<button id=\"preview\" class=\"primary\">Preview</button>"
    "</row>"
    "</fieldset>"
    "<row id=\"body\">"
    "<column id=\"side\">"
    "<fieldset id=\"settingsbox\"><legend>Settings</legend>"
    "<row><label for=\"machine\">Laser</label><select id=\"machine\"></select></row>"
    "<column id=\"layers\"></column>"
    "<row><checkbox id=\"cuts_last\" checked>Cuts last</checkbox></row>""<row><checkbox id=\"constant\">M3 (constant)</checkbox></row>"
    "</fieldset>"
    "</column>"
    "<div id=\"view\"></div>"
    "</row>"
    "<row id=\"jobbar\">"
    "<button id=\"frame\">Frame</button>"
    "<button id=\"gcode\">Save G-code</button>"
    "</row>"
    "<hr>"
    "<div id=\"status\">Open a drawing.</div>";

static const char burn_css[] =
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
    "#path{flex:1}"
    "#jobbar{padding:0;gap:6px}"
    "select{flex:1}"
    "#view{flex:1;border:1px solid #d0d0d0;background-color:#14161c}"
    "hr{background:#d0d0d0}"
    "#layers{gap:6px;width:100%;min-width:0}"
    "#layers column{width:100%;min-width:0;gap:2px}"
    "#layers row{padding:0;gap:4px;width:100%;min-width:0}"
    "#layers .swatch{width:12px;border:1px solid #d0d0d0}"
    "#layers label{width:48px;color:#616161}"
    "#layers label.name{width:auto;flex:1}"
    "#layers spinner{flex:1;min-width:0}"
    "#layers span{width:16px;color:#616161}"
    "#layers checkbox{width:16px}""#layers .shift{width:28px;min-width:28px;padding:0 2px}";

#endif
