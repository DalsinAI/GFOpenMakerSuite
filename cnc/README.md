# OpenMakeCNC

The CNC app of the GFOpenMake suite: a profile, a pocket or a drill, from
an SVG or a DXF, to a mill's G-code.

    openmake-cnc in.svg out.gcode mode=profile tool=3 depth=2 stepdown=1

Profile follows the drawn edge. The tool stands outside it by its radius,
so the drawing is the finished size. Pocket clears the inside, one
step-over at a time, the tool kept in. Drill plunges at the centre of
each closed shape. Open paths are left out.

The machine is the generic 300 mm Grbl mill in `machines/cnc/grbl` unless
`DRIVER=` or `PROFILE=` name another. Lengths are millimetres. Feed and
plunge are millimetres a second, written as millimetres a minute.

The window is `src/amiga.c`, binary GFOpenCNC. Open an SVG or a DXF,
pick Profile, Pocket or Drill, Preview the cut, and Save G-code for the
mill picked. It fits 640 by 480 inside and is resizable. The view is the
toolpath, drawn with the pens. ARexx follows.
