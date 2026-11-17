# OpenMakeBurn

The laser app of the GFOpenMake suite: vectors and pictures to a laser's
G-code. The plan is [docs/burn-plan.md](../docs/burn-plan.md).

The command, `openmake-burn`, reads an SVG and writes Grbl laser G-code
in line mode, for the generic 400 mm diode unless DRIVER or PROFILE say
otherwise. A `layer=` sets power, speed, passes or on for one colour
(or a 1-based index). The window is not started.

It will share OpenMakeCore's toolpaths, preview and G-code writer with the
slicer and the CNC app, and the machine drivers in `../machines/laser`.
