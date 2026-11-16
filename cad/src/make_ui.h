/* GFOpenMake's window. GFOpenLayout lays it out and draws it; make.c
 * only answers the buttons and draws the view. It is here, not in make.c,
 * so cad/tests/test_make_ui.c can lay it out on the host. The markup is the
 * window itself: wrapped in a <window> element, the view got no height.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef GS_MAKE_UI_H
#define GS_MAKE_UI_H

/* OpenLayout Clear (docs/HANDOVER.md): one light window, the tools in
 * grouped boxes down the left, a label beside its field, one status line
 * along the bottom, Save STL the accent primary button. The chosen tool
 * is :selected and lit in the accent too. */
static const char gm_markup[] =
    "<row><h2>GFOpenMake</h2><spacer></spacer><span id=\"ver\">0.1.0</span></row>"
    "<row id=\"body\">"
    "<column id=\"side\">"
    "<fieldset><legend>File</legend>"
    "<row><button id=\"open\">Open</button><button id=\"undo\">Undo</button></row>"
    "<button id=\"save\" class=\"primary\">Save STL</button>"
    "</fieldset>"
    "<fieldset><legend>Tool</legend>"
    "<row><button id=\"orbit\">Orbit</button><button id=\"move\">Move</button></row>"
    "<row><button id=\"rot\">Rotate</button><button id=\"scale\">Scale</button></row>"
    "</fieldset>"
    "<fieldset><legend>Parts</legend>"
    "<row><button id=\"addcube\">Cube</button><button id=\"addcyl\">Cylinder</button></row>"
    "<row><button id=\"join\">Join</button><button id=\"cut\">Cut</button><button id=\"meet\">Intersect</button></row>"
    "</fieldset>"
    "<fieldset><legend>Place</legend>"
    "<row><button id=\"drop\">Drop</button><button id=\"centre\">Centre</button><button id=\"lay\">Lay flat</button></row>"
    "<row><label for=\"face\">Face</label><span id=\"face\">no face</span></row>"
    "</fieldset>"
    "</column>"
    "<div id=\"view\"></div>"
    "</row>"
    "<hr>"
    "<div id=\"status\">A 20 mm cube, with a hole.</div>";

static const char gm_css[] =
    "window{background:#f9f9f9;color:#1b1b1b;font-size:12px;padding:8px;gap:8px}"
    "#body{flex:1;padding:0;gap:8px}"
    "#side{width:224px;gap:8px}"
    "fieldset{border:1px solid #d0d0d0;border-radius:8px;padding:0 8px 8px}"
    "fieldset row{padding:0;gap:6px}"
    "button{flex:1;border:1px solid #d0d0d0;border-radius:4px;color:#1b1b1b;"
    "background:linear-gradient(#fefefe,#fbfbfb)}"
    "button:active{background:#f0f0f0}"
    "button:focus{border-color:#0067c0}"
    "button:selected,button.primary{background:#0067c0;border-color:#0067c0;color:#ffffff}"
    "button.primary:active{background:#005ba1}"
    "label{width:40px;color:#616161}"
    "#view{flex:1;border:1px solid #d0d0d0;background-color:#14161c}"
    "hr{background:#d0d0d0}";

#endif
