/* OpenMakeCNC's window. GFOpenLayout lays this out; the program answers
 * Open, Preview and Save G-code, and draws the view. The mill and the
 * cut sit in the side column. 640 by 480 inside, to fit a 640 by 512 PAL
 * screen with the window's borders. OpenLayout Clear: one light window,
 * grouped boxes, one status line along the bottom, Preview the accent
 * primary button.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef CNC_MARKUP_H
#define CNC_MARKUP_H

#include "omview_settings.h"

static const char cnc_markup[] =
    "<fieldset><legend>Drawing</legend>"
    "<row>"
    "<label id=\"kind\" for=\"path\">File</label>"
    "<input id=\"path\">"
    "<button id=\"open\">Open</button>"
    "<button id=\"preview\" class=\"primary\">Preview</button>"
    "</row>"
    "</fieldset>"
    "<row id=\"body\">"
    "<column id=\"side\">"
    "<fieldset><legend>Cut</legend>"
    "<row><label for=\"machine\">Mill</label><select id=\"machine\"></select></row>"
    "<row><button id=\"profile\">Profile</button><button id=\"pocket\">Pocket</button><button id=\"drill\">Drill</button></row>"
    "<row><label for=\"tool\">Tool</label><spinner id=\"tool\" min=\"0\" max=\"40\" step=\"0.1\" value=\"3.0\">"
    "<label for=\"depth\">Depth</label><spinner id=\"depth\" min=\"0.1\" max=\"80\" step=\"0.1\" value=\"1.0\"></row>"
    "<row><label for=\"stepdown\">Down</label><spinner id=\"stepdown\" min=\"0.1\" max=\"40\" step=\"0.1\" value=\"1.0\">"
    "<label for=\"stepover\">Over</label><spinner id=\"stepover\" min=\"0.1\" max=\"40\" step=\"0.1\" value=\"1.2\"></row>"
    "<row><label for=\"feed\">Feed</label><spinner id=\"feed\" min=\"0.1\" max=\"200\" step=\"0.1\" value=\"8.0\">"
    "<label for=\"plunge\">Plunge</label><spinner id=\"plunge\" min=\"0.1\" max=\"80\" step=\"0.1\" value=\"3.0\"></row>"
    "<row><label for=\"safe\">Safe</label><spinner id=\"safe\" min=\"0.1\" max=\"80\" step=\"0.1\" value=\"5.0\">"
    "<label for=\"rpm\">Rpm</label><spinner id=\"rpm\" min=\"0\" max=\"60000\" step=\"100\" value=\"10000\"></row>"
    "</fieldset>"
    "</column>"
    "<div id=\"view\"></div>"
    "</row>"
    "<row id=\"jobbar\">"
    "<button id=\"gcode\">Save G-code</button>"
    "<label for=\"x\">X</label><spinner id=\"x\" min=\"-10000\" max=\"10000\" step=\"0.1\" value=\"0.0\">"
    "<label for=\"y\">Y</label><spinner id=\"y\" min=\"-10000\" max=\"10000\" step=\"0.1\" value=\"0.0\">"
    "<span>mm</span>"
    "</row>"
    "<hr>"
    "<div id=\"status\">Open a drawing.</div>";

static const char cnc_css[] =
    "window{background:#f9f9f9;color:#1b1b1b;font-size:12px;padding:8px;gap:6px}"
    "fieldset{border:1px solid #d0d0d0;border-radius:8px;padding:0 8px 6px;gap:6px}"
    "fieldset row{padding:0;gap:6px}"
    "button{border:1px solid #d0d0d0;border-radius:4px;color:#1b1b1b;background:linear-gradient(#fefefe,#fbfbfb)}"
    "button:active{background:#f0f0f0}"
    "button:focus{border-color:#0067c0}"
    "button.primary{background:#0067c0;border-color:#0067c0;color:#ffffff}"
    "button.primary:active{background:#005ba1}"
    "button:selected{background:#0067c0;border-color:#0067c0;color:#ffffff}"
    "label{color:#616161}"
    "#body{flex:1;padding:0;gap:8px}"
    "#side{width:" OMV_SETTINGS_W_CSS "}"
    "#side label{width:44px}"
    "#side spinner{width:64px;min-width:0;flex:1}"
    "#kind{width:auto;min-width:36px}"
    "#path{flex:1}"
    "#jobbar{padding:0;gap:6px}"
    "#jobbar label{width:auto;padding-left:4px}"
    "#jobbar spinner{width:72px;min-width:0;flex:0}"
    "#jobbar span{width:auto;color:#616161}"
    "select{flex:1}"
    "#view{flex:1;border:1px solid #d0d0d0;background-color:#14161c}"
    "hr{background:#d0d0d0}";

#endif
