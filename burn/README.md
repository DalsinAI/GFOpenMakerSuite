# OpenMakeBurn

The laser app of the GFOpenMake suite: vectors and pictures to a laser's
G-code. The plan is [docs/burn-plan.md](../docs/burn-plan.md).

The command, `openmake-burn`, reads an SVG or a DXF and writes Grbl laser G-code
in line mode, for the generic 400 mm diode unless DRIVER or PROFILE say
otherwise. `START=` and `END=` are lines of your own, after the machine's start and before its end. The port takes the same lines as `SET start` and `SET end`. `SET invert` burns the white of a picture, and `SET width` is that picture's width in millimetres. `SET x`, `SET y` and `SET rot` move the drawing, the same as the place fields. `STATUS` reads them. `SET power`, `SET speed`, `SET passes`, `SET mode` and `SET interval` are written onto every layer row, and so are `SET kerf`, `SET tabs` and `SET tabw`. `travel` and `s100` have no row; the port keeps them. `SET order` is the layer list, as `order=` on the command, and Up and Dn are that same list. `ORDER` moves one layer, as those buttons. `LAYER` with only the index reads that row, the same numbers as the list, and does not mark the job dirty. `KERF`, `TABS` and `TABW` set that one row, as its spinners. `MATERIAL` names that profile, and the list keeps it when it is one of the window's. The same power, speed, passes, mode and gap are written onto every layer row. `STATUS` names it. `MACHINE` picks the laser, as the window's list. `STATUS` names that laser. `LOOK` is top, front, side or frame. Frame here is the view at open, not the Frame button. The window has no button for it. `PREVIEW` with no file says the window's line. `SAVE` keeps the file as `RESULT`, and `STEM` has the moves and the seconds the window shows. The window has no field for the lines, invert or width. A `layer=` sets power, speed, passes, mode or on for one colour
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
