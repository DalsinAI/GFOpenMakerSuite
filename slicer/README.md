# OpenMakeSlicer

STL to G-code for 3D printers, in the GFOpenMake suite. The binary is still
GFOpenSlicer.

- `src/slice.c`: the slicer: loops, a wall, infill, the skirt and G-code.
- `src/job.c`, `include/gs_job.h`: a slice with its layers, for the window.
- `src/amiga.c`, `src/markup.h`: the Amiga window (GFOpenLayout).
- `src/cli.c`: the same from the shell.
- `tools/preview.c`, `tools/paint.py`: a host picture of the window.
- `tests/`: the host tests.

It builds on OpenMakeCore in `../core`.
