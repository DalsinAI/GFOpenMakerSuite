/* GFOpenMakerWorkshop's window (Clear look). The name and version are in
 * the title bar, not a row. New, Open and Save are at the top with the
 * product name and Recent; the left column is the item list, grouped by
 * kind (CAD, Print, Laser, CNC, Electronics, and any other kind the
 * project has), then the item name, Add, Remove, Rename and Launch,
 * and the notes; the right is the preview of the picked item; the status
 * line is shared (omview_ui.h). The markup is the window itself: wrapped
 * in a <window> element, the view got no height.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef WORKSHOP_MARKUP_H
#define WORKSHOP_MARKUP_H

static const char workshop_markup[] =
    "<row id=\"bar\">"
    "<button id=\"new\">New</button>"
    "<button id=\"open\">Open</button>"
    "<button id=\"save\">Save</button>"
    "<label for=\"product\">Product</label>"
    "<input id=\"product\" maxlength=\"63\">"
    "<select id=\"recent\"><option value=\"\">Recent</option></select>"
    "</row>"
    "<row id=\"body\">"
    "<column id=\"side\">"
    "<list id=\"items\"></list>"
    "<row><label for=\"itemname\">Name</label><input id=\"itemname\" maxlength=\"63\"></row>"
    "<row><button id=\"add\">Add</button>"
    "<select id=\"addkind\">"
    "<option value=\"cad\">CAD</option>"
    "<option value=\"print\">Print</option>"
    "<option value=\"laser\">Laser</option>"
    "<option value=\"cnc\">CNC</option>"
    "<option value=\"electronics\">Electronics</option>"
    "</select></row>"
    "<row><button id=\"remove\">Remove</button>"
    "<button id=\"rename\">Rename</button>"
    "<button id=\"launch\">Launch</button></row>"
    "<row><label for=\"notes\">Notes</label><input id=\"notes\" maxlength=\"255\"></row>"
    "</column>"
    "<div id=\"view\"></div>"
    "</row>"
    "<div id=\"status\"></div>";

static const char workshop_css[] =
    "window{background:#f9f9f9;color:#1b1b1b;font-size:12px;padding:8px;gap:6px}"
    "row{gap:6px;align-items:center}"
    "column{gap:6px}"
    "button{padding:4px 10px;background:#e8e8e8;border:1px solid #c0c0c0;border-radius:3px}"
    "input,select{background:#ffffff;border:1px solid #c0c0c0;padding:3px 6px}"
    "label{color:#333}"
    "#bar{gap:6px;align-items:center}"
    "#product{flex:1;min-width:80px}"
    "#recent{width:150px}"
    "#body{flex:1;padding:0;gap:8px;align-items:stretch}"
    "#side{width:236px;gap:6px;align-self:stretch}"
    "#items{flex:1;min-height:80px;overflow:auto;border:1px solid #d0d0d0;background:#ffffff}"
    "#itemname{flex:1;min-width:60px}"
    "#addkind{flex:1}"
    "#notes{flex:1;min-width:60px}"
    "#view{flex:1;min-width:200px;border:1px solid #d0d0d0;background-color:#14161c}"
    "#status{font-size:11px;color:#444}";

#endif
