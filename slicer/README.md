# OpenMakeSlicer

STL to G-code for 3D printers, in the GFOpenMake suite. The binary is still
GFOpenSlicer.

- `src/slice.c`, `src/cli.c`: the first slicer (loops, a wall, infill, a skirt) and its `gfopenslicer` command; the window no longer uses it.
- `include/sl_job.h`, `src/sl_job.c`: the slicer's job on OpenMakeCore's toolpath engine: a part loaded onto the bed, sliced on several tasks (`gp_part_par`), estimated, checked against the printer and written. The window and `openmake-slice` both run it.
- `src/amiga.c`, `src/markup.h`, `src/slicer_ui.c`: the Amiga window (GFOpenLayout, Clear): Open, Slice, the settings, the printer picker, the layer slider and Save G-code, resizable from 640 by 480 inside, on core/ui's helpers.
- `src/openmake_slice.c`: `openmake-slice`, the new slice from the shell: OpenMakeCore's
  layers (brim, walls, skin, infill) to G-code for a machine from `../machines`,
  checked against the machine first. `TASKS=n` makes the layers on n tasks at once
  (one a core by default); the G-code is the same on any number.
- `tools/preview.c`, `tools/paint.py`: a host picture of the window.
- `tests/`: the host tests.

It builds on OpenMakeCore in `../core`.
