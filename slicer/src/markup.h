/* The whole window. GFOpenLayout lays this out; the program answers
 * Open, Slice, the layer slider and Save G-code, and draws the view. 640 by 480
 * inside, to fit a 640 by 512 PAL screen with the window's borders: the
 * name and version are in the title bar. OpenLayout Clear, as GFOpenMake's
 * (cad/src/make_ui.h): one light window, grouped boxes, one status line
 * along the bottom, Slice the accent primary button. The markup is the
 * window itself: wrapped in a <window> element, the view got no height.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef GS_MARKUP_H
#define GS_MARKUP_H

static const char gs_markup[] =
    "<fieldset><legend>Part</legend>"
    "<row>"
    "<label for=\"path\">STL</label>"
    "<input id=\"path\">"
    "<button id=\"open\">Open</button>"
    "<button id=\"slice\" class=\"primary\">Slice</button>"
    "</row>"
    "</fieldset>"
    "<fieldset><legend>Settings</legend>"
    "<row>"
    "<label for=\"layer\">Layer mm</label><spinner id=\"layer\" min=\"0.08\" max=\"0.80\" step=\"0.01\" value=\"0.20\">"
    "<label for=\"infill\">Infill %</label><spinner id=\"infill\" min=\"0\" max=\"100\" step=\"5\" value=\"20\">"
    "<label for=\"line\">Line mm</label><spinner id=\"line\" min=\"0.20\" max=\"1.20\" step=\"0.05\" value=\"0.40\">"
    "<label for=\"walls\">Walls</label><spinner id=\"walls\" min=\"1\" max=\"8\" step=\"1\" value=\"2\">"
    "</row>"
    "<row>"
    "<label for=\"hotend\">Hotend</label><spinner id=\"hotend\" min=\"0\" max=\"300\" step=\"5\" value=\"200\">"
    "<label for=\"bed\">Bed</label><spinner id=\"bed\" min=\"0\" max=\"120\" step=\"5\" value=\"60\">"
    "<label for=\"brim\">Brim</label><spinner id=\"brim\" min=\"0\" max=\"10\" step=\"1\" value=\"3\">"
    "<label for=\"machine\">Printer</label><select id=\"machine\"></select>"
    "</row>"
    "</fieldset>"
    "<div id=\"view\"></div>"
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
    "spinner{width:72px}"
    "#path{flex:1}"
    "#layerbar{padding:0;gap:6px}"
    "slider{flex:1}"
    "select{flex:1}"
    "#view{flex:1;border:1px solid #d0d0d0;background-color:#14161c}"
    "hr{background:#d0d0d0}";

#endif
