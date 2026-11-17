# The OpenMake libraries

The suite's shared code is built as Amiga shared libraries, each loaded once
and shared by every app: `openmakemachine.library` first, then
`openmakepath`, `openmakemesh`, `openmakegcode`, `openmakeview`, `openmaketask`, `openmakevec`, `openmakeproject` and `openmakerexx`.

| file | what it is |
|---|---|
| `runtime.c`, `runtime.h` | The C runtime a library is built with: memory over `AllocVec`, strings, `qsort`, `snprintf`, `strtod` and the maths, on exec.library alone (a library has no start-up code). `test_runtime` holds it to the host's. |
| `omlib.h` | What a library and its callers share: `REG()`, and a float carried in a data register as its bits. |
| `libshell.c`, `libstart.c` | The library around the calls: romtag, init, open, close, expunge, the jump table (checked against the last bias when it builds). The FPU build's init fails on a machine without an FPU; the build without one opens the mathieee libraries, through which libnix's soft float works (and which use an FPU themselves when there is one). A library that needs another opens it in `LIB_OPEN_MORE()` and closes it in `LIB_CLOSE_MORE()`. |
| `runtime_dos.c` | `fopen` and the rest over dos.library, for the calls that read files. |
| `link_base.c` | A link library's base, opened before `main()`, and a second with `LINK_BASE2` for a program on two libraries; without one the program says so and stops. |
| `openmakepath_lib.*` | openmakepath.library: a part cut into layers, the walls welded into loops and inset (`core/src/layer.c`, `loops.c`, `inset.c`), the toolpath and its fill (`toolpath.c`, `fill.c`), and a printer's layers planned (`plan.c`); 1.3 adds the `_More` calls (`gp_more`: top and bottom apart, a grid, a skirt, a gap) and `GT_FillGrid`; 1.4 adds `GL_Tabs` (holding tabs on a closed loop). |
| `openmakemesh_lib.*` | openmakemesh.library: STL in and out, repair, and the solids and booleans (`core/src/model.c`, `parts.c`, `stl.c`); 1.1 adds the bench's other solids (`GM_Sphere`, `GM_Cone`, `GM_Torus`, `GM_Wedge`, `GM_Slot`); 1.2 keeps booleans clean on a round part far from the origin (no new calls); 1.3 adds the bench's hole tools (`GM_BoltCircle`, `GM_HoleRow`, `GM_Counterbore`); 1.4 cuts a holed part again without slivers or cracks (no new calls); 1.5 adds `GM_Facts` (a part's volume, surface, size and triangles, for Measure); 1.6 adds `GM_Extrude` (the bench's Extrude: a block, round or slot on the bed, on a part's top, or down from it); 1.7 adds the parts list (`GS_PartsNew` to `GS_PartsCopy`, `core/src/parts.c`, `gs_parts.h`: up to 24 named parts, each a colour, shown or hidden, one picked). |
| `openmakegcode_lib.*` | openmakegcode.library: a printer's G-code from a toolpath, its time and filament, and the job for om_check (`core/src/gcode.c`); 1.1 adds GC_EstimateHow, saying whether the time counted the acceleration; 1.2 the `_More` calls (`gc_more`: the first layer's speed); 1.3 Grbl laser G-code (`GC_LaserDefaults` to `GC_LaserWriteMore`: M4 dynamic power by default, M3 constant, S scaled to s100, travels with the laser off). It writes to the program's own `gc_put`, not a file, and opens openmakemachine.library at init for the start and end code: without that it does not open. |
| `openmakeview_lib.*` | openmakeview.library: the view every app shows: the turntable camera, the part and the plate framed, the CPU drawer's solid and plate pictures and their pens (`core/src/solid.c`, `view.c`), the walls and the toolpath drawn (`layer_paint.c`, `toolpath_paint.c`), the same as OpenGPU batches and streams (`gpu.c`), and (1.3) a point mapped to the screen, a face's colour, a G-code's lines into the program's room and its picture (`gs_view_map_at`, `gs_view_colour_of`, `gs_view_lines_into`, `gs_view_paint`), and (1.4) the faces in the parts' colours (`gs_view_bed_tint`, `gs_gpu_vertices_plate_tint`). |
| `openmaketask_lib.*` | openmaketask.library: work spread over the cores (`gs_par.h`, `core/src/par_exec.c`): n pieces of the program's own code, each run once on exec tasks, returning when all have. The pieces must not use DOS (tasks, not processes), and the program must not be built `-fbaserel`. |
| `openmakevec_lib.*` | openmakevec.library: 2D vector drawings and an SVG reader (`core/src/vec.c`, `gs_vec.h`), written from the W3C SVG 1.1 specification. Paths, groups and the usual shapes; coordinates in millimetres, origin at the bottom-left of the view box, Y up. A colour becomes a layer (the stroke, or the fill if there is no stroke). |
| `openmakeproject_lib.*` | openmakeproject.library: a GFOpenMakerWorkshop project (`core/src/project.c`, `gs_project.h`): a drawer and its `project.om` manifest (create, open, save, add, remove, rename, kinds, the app for an item, and recent projects). An unknown kind is kept, so a later kind can be added without breaking old projects. |
| `openmakerexx_lib.*` | openmakerexx.library 1.0: a public ARexx port and a client (`core/src/rexx.c`, `rexx_amiga.c`, `gs_rexx.h`), written from the public Amiga ARexx and rexxsyslib documentation. An app opens a port (`GFOPENMAKE.1` and so on); Workshop, the hub, finds or starts a port, sends a command, and waits for RC and RESULT, with a timeout (timer.device, not a busy loop). The parser and the host stub are tested on the host; the Amiga port is compiled only for Amiga. |
| `gen-library.py` | Writes a library's glue from its `.sfd`: the inline calls and proto header for programs, the library's side of each call and its jump table, and a link library with the C API through the library. `make test` checks the files are as it writes them. |

Each library's `.sfd` is the list of its calls, in order. Never reorder or
take one out; add new calls at the end. The C function is the call's name
in snake case (`OM_ClampSpeed` is `om_clamp_speed`). Run
`python3 core/library/gen-library.py <app>/library/<name>_lib.sfd` after
changing it.

Each library is built twice under one name, with the FPU and without it; the
installer picks one. The FPU build will not open on a machine without one.

`build-libraries.sh` at the top builds them, with each library's test linked
through it, and fails if a library needs a symbol from outside it or the
build without the FPU holds an FPU instruction:

```
CC=.../m68k-amigaos-gcc SDK=.../gfopensdk ./build-libraries.sh
```
