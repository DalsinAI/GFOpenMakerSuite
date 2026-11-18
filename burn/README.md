# OpenMakeBurn

The laser app of the GFOpenMake suite: vectors and pictures to a laser's
G-code. The plan is [docs/burn-plan.md](../docs/burn-plan.md).

The command, `openmake-burn`, reads an SVG or a DXF and writes Grbl laser G-code
in line mode, for the generic 400 mm diode unless DRIVER or PROFILE say
otherwise. `START=` and `END=` are lines of your own, after the machine's start and before its end. The port takes the same lines as `SET start` and `SET end`. `SET invert` burns the white of a picture, and `SET width` is that picture's width in millimetres. The window has no field for them. A `layer=` sets power, speed, passes, mode or on for one colour
(or a 1-based index). `mode=fill` scans back and forth inside closed shapes, `mode=offset`
follows them inwards, `interval` the gap in mm (0.2 by default).

The Amiga window (`src/amiga.c`, binary GFOpenBurn) is GFOpenLayout in
the Clear look: Open an SVG or a DXF, a layer list with power, speed, passes, mode and gap,
the laser picker, Preview, Frame (a travel around the job, laser off),
and Save G-code. The view is OpenGPU with a pen fallback. It fits 640 by
480 inside (a 640 by 512 PAL screen with its borders), is resizable, and
opens filling the screen. `tests/test_burn_ui.c` and `tools/preview.c`
run that layout on the host.

It will share OpenMakeCore's toolpaths, preview and G-code writer with the
slicer and the CNC app, and the machine drivers in `../machines/laser`.
