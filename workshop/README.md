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

The window follows.
