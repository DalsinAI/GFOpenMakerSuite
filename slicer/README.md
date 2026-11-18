# OpenMakeSlicer

STL to G-code for 3D printers, in the GFOpenMake suite. The binary is still
GFOpenSlicer.

- `src/cli.c`: `gfopenslicer`, the slice from the shell: the window's job (`sl_job`) for the generic Marlin printer, or `DRIVER=`/`PROFILE=`, checked against it first; the same G-code as `openmake-slice`.
- `include/sl_args.h`, `src/sl_args.c`: both commands' print settings: `SETTINGS=profile` (`../settings/standard`, the defaults), the older `LAYER=` ... `SKIRT=` names over it, and any profile key as `key=value` over those.
- `include/sl_job.h`, `src/sl_job.c`: the slicer's job on OpenMakeCore's toolpath engine: a part loaded onto the bed, sliced on several tasks (`gp_part_par`), estimated, checked against the printer and written. `START=` and `END=` are the user's own lines, after the machine's start and before its end. `SUPPORT=1` is a wall and a sparse fill under a steep overhang. The window and `openmake-slice` both run it.
- `src/amiga.c`, `src/markup.h`, `src/slicer_ui.c`: the Amiga window (GFOpenLayout, Clear): Open, Slice, the settings, the printer picker, the layer slider and Save G-code, resizable from 640 by 480 inside, on core/ui's helpers. Support, Start and End are on the Settings side: the same as `SUPPORT=1`, `START=` and `END=` on the shell, and as `SET support`, `SET start` and `SET end` on the port. `STATUS` names those three. `SET tasks` is `TASKS=`, held from 1 to 64. The window has no field for it, and Slice uses that count. `LAYERS` is the layer bar: layers 1 to an index. `STEP` is Prev and Next. A slice leaves the bar on the top layer. `LOOK` is top, front, side or frame. The window has no button for it.
- `src/openmake_slice.c`: `openmake-slice`, the new slice from the shell: OpenMakeCore's
  layers (brim, walls, skin, infill) to G-code for a machine from `../machines`,
  checked against the machine first. `TASKS=n` makes the layers on n tasks at once
  (one a core by default); the G-code is the same on any number.
- `tools/preview.c`, `tools/paint.py`: a host picture of the window.
- `tests/`: the host tests.

It builds on OpenMakeCore in `../core`.
