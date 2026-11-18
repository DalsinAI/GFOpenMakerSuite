# OpenMakeWorkshop

The project app of the GFOpenMake suite: one product's CAD, print, laser,
CNC and KiCad work in a single drawer, shown as one product view. The
window title is GFOpenMakerWorkshop. The plan is
[docs/workshop-plan.md](../docs/workshop-plan.md).

It builds on OpenMakeCore in `../core`. `openmakeproject.library` is the
project file. The command, `openmake-workshop`, creates a project, lists
it, and adds an item:

    openmake-workshop new MyWidget name=Widget
    openmake-workshop add MyWidget kind=cad file=body.stl name=Body
    openmake-workshop list MyWidget

The window is `workshop/src/amiga.c`, binary `GFOpenWorkshop`, title
GFOpenMakerWorkshop. New asks for a drawer, Open for a `project.om` (or
the drawer), Save writes it. The left list always shows the known kind
groups (CAD, Print, Laser, CNC, Electronics) and a group for any other
kind the project has. The view is the picked item: an STL as a solid,
an SVG or DXF as strokes, G-code as a toolpath; a KiCad file is listed
and opened in gfamigakicad, never drawn or parsed. Launch starts
the kind's program (`PROGDIR:` first, then the name) with the file on
its command line. GFOpenMake, GFOpenSlicer, GFOpenBurn and GFOpenCNC
open that file. That is a launch hook, used when the other app's port is
not up. The window's own port is `OPENMAKEWORKSHOP.1`: `NEW`, `ADD`,
`REMOVE`, `RENAME`, `ITEMS`, `OPENITEM`, `SEND` and `RECENT`, plus the
shared commands. `OPENITEM` sends `OPEN FILE` when the other port is
already up.

A host render of the window (not an Amiga run):

    make workshop_preview LAYOUT=... SDK=...
    build/workshop_preview 640 512 out.ops MyWidget
    python3 slicer/tools/paint.py out.ops out.png
    python3 workshop/tools/label_preview.py out.png labelled.png "what it shows"
