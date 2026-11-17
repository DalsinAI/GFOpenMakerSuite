# OpenMakeSlicer

STL to G-code for 3D printers, in the GFOpenMake suite. The binary is still
GFOpenSlicer.

- `src/slice.c`: the slicer: loops, a wall, infill, the skirt and G-code.
- `src/job.c`, `include/gs_job.h`: a slice with its layers, for the window.
- `src/amiga.c`, `src/markup.h`: the Amiga window (GFOpenLayout).
- `src/cli.c`: the same from the shell.
- `src/openmake_slice.c`: `openmake-slice`, the new slice from the shell: OpenMakeCore's
  layers (brim, walls, skin, infill) to G-code for a machine from `../machines`,
  checked against the machine first. `TASKS=n` makes the layers on n tasks at once
  (one a core by default); the G-code is the same on any number.
- `tools/preview.c`, `tools/paint.py`: a host picture of the window.
- `tests/`: the host tests.

It builds on OpenMakeCore in `../core`.
