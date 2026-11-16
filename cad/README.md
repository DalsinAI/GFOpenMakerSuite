# OpenMakeCad

The solid editor of the GFOpenMake suite.

- `src/make.c`: the Amiga window (GFOpenLayout, Clear look; OpenGPU with
  the pen drawer as fallback). `src/make_ui.h` is its markup and CSS.
- `src/edit.c`, `include/gs_edit.h`: the editing that has no window in it
  (move, rotate, scale, drop, centre, lay flat, the scene and undo, open).
- `bench/`: the browser bench, ahead of the Amiga window.
- `tests/`: the host tests.

It builds on OpenMakeCore in `../core`.
