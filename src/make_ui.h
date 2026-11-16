/* GFOpenMake's window. GFOpenLayout lays it out and draws it; make.c
 * only answers the buttons and draws the view. It is here, not in make.c,
 * so tests/test_make_ui.c can lay it out on the host. The markup is the
 * window itself: wrapped in a <window> element, the view got no height.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef GS_MAKE_UI_H
#define GS_MAKE_UI_H

static const char gm_markup[] =
    "<row><h2>GFOpenMake</h2><spacer></spacer><span id=\"ver\">0.1.0</span></row>"
    "<row>"
    "<button id=\"open\">Open</button>"
    "<button id=\"save\">Save</button>"
    "<button id=\"undo\">Undo</button>"
    "<spacer></spacer>"
    "<button id=\"orbit\">Orbit</button>"
    "<button id=\"move\">Move</button>"
    "<button id=\"rot\">Rotate</button>"
    "<button id=\"scale\">Scale</button>"
    "</row>"
    "<row>"
    "<button id=\"addcube\">Cube</button>"
    "<button id=\"addcyl\">Cylinder</button>"
    "<button id=\"join\">Join</button>"
    "<button id=\"cut\">Cut</button>"
    "<button id=\"meet\">Intersect</button>"
    "</row>"
    "<row>"
    "<button id=\"drop\">Drop</button>"
    "<button id=\"centre\">Centre</button>"
    "<button id=\"lay\">Lay flat</button>"
    "<spacer></spacer>"
    "<span id=\"face\">no face</span>"
    "</row>"
    "<div id=\"view\" style=\"flex:1\"></div>"
    "<span id=\"status\">A 20 mm cube, with a hole.</span>";

static const char gm_css[] =
    "#view{flex:1;background-color:#14161c}";

#endif
