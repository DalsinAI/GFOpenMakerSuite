# Handover

16 November 2026. The browser bench in `cad/bench/` was ahead of `cad/src/make.c`. This is the state of that bench. The sources in the same commit are the bench, so this note is not only in a chat. `docs/bench.md` is the 9 November record. It still holds.

17 November 2026. The window has since taken the bench's extrude, pull, chamfer, arrange, layer walls, bolt circle, hole row, counterbore, one drill, a countersink, the washer, split, a dragged sketch, and the shelf. Top, front, side and frame are on the window. Metal, clay and wire are the picture. Snap is 1 mm, 15 degrees, and steps of 0.1 scale. The Face row shows the picked part's PLA, and marks it when it hangs off the plate. The bench's keys are too: G move, R turn, S scale, F frame, M measure, I solo, Delete. Section hides the picture above a height, and the layer walls follow that cut. It does not cut the part until Split. Slice lays the toolpath back on the solid, in the plate's frame. The example files are in `examples/`, listed in `docs/media-examples.md`. Shelf puts one of `examples/cad/` on the bed.

18 November 2026. `gfopenslicer` and `openmake-slice` take `START=` and `END=`: lines of your own after the machine's start and before its end. The heat, the home and M84 stay. The slicer window has the same two lines, and Support, on the Settings side. Support on is one wall, and a line every four widths, under an overhang steeper than 45 degrees, from the bed up to it. The mill picker also has a generic LinuxCNC machine. Its program ends with M2. A CNC item in Workshop launches GFOpenCNC. The mill window's port is GFOPENCNC.1. The slicer's own view lays the toolpath on the solid, in the plate's frame, the same picture as the editor. GFOpenSlicer, GFOpenBurn and GFOpenCNC open the file named at launch, the way GFOpenMake does. `PREVIEW` on `GFOPENCNC.1` plans the cut, and writes a PPM when `FILE` is given. `openmake-burn` takes `START=` and `END=`: lines of your own after the machine's start and before its end. The laser window has no field for them yet. `OPENMAKEBURN.1` takes the same lines as `SET start` and `SET end`. `OPENMAKEWORKSHOP.1` is the project port: the shared commands, and `NEW`, `ADD`, `REMOVE`, `RENAME`, `ITEMS`, `OPENITEM`, `SEND` and `RECENT`. `OPENITEM` sends `OPEN FILE` when the other app's port is already up, and otherwise starts that program with the file. The window's Launch button does the same. A KiCad file is gfamigakicad's: Workshop lists it and does not draw or read it. A Netpbm picture is drawn, black as ink, from the same reader the laser uses. `openmake-cnc` takes `START=` and `END=`: lines of your own after the machine's start and before its end. The mill window has no field for them. `GFOPENCNC.1` takes the same lines as `SET start` and `SET end`. `OPENMAKEBURN.1` also takes `SET invert` and `SET width` for a picture. The laser window has no field for them. `SET x`, `SET y` and `SET rot` move the drawing, and the place fields show them. `SET power`, `SET speed`, `SET passes`, `SET mode` and `SET interval` are written onto every layer row, and so are `SET kerf`, `SET tabs` and `SET tabw`. `travel` and `s100` have no row; the port keeps them. An item made from another shows `from N` on its line. `GFOPENMAKE.1` takes `COPY`, `LINE`, `RING`, `MIRROR`, `TURN`, `STACK` and `SOLO`, the window's Arrange, and `SHELF` puts an example from `examples/cad/` on the bed. `EXTRUDE` adds, joins or cuts a block, a round or a slot. `DROP`, `CENTRE` and `LAY` put the picked part on the bed. `RENAME`, `SHOW` and `COLOUR` are the name, the Hide button and the metals. `OPENMAKEBURN.1` takes `SET order`, the layer list, as `order=` on the command. Up and Dn are that same list, and the port keeps it. `OPENMAKESLICER.1` takes `SET support`, `SET start` and `SET end`, the Settings side, as `SUPPORT=`, `START=` and `END=` on the command. `GFOPENMAKE.1` takes `LOOK`, `SHADE` and `SNAP`: Top, Front, Side and Frame, metal, clay and wire, and the Snap button. None of those changes the part. `SECTION` hides the picture above a height, as the section checkbox, and does not cut the part. `PRINTER` picks the machine, as the window's list. A laser is refused. The slicer's `PRINTER` stays selected in the list too, when that printer is one of the window's. `GFOPENCNC.1` takes `MILL`, the mill list. A printer is refused, and the list keeps the mill when it is one of the window's. `OPENMAKEBURN.1` takes `MACHINE`, the laser list. A printer is refused, and the list keeps that laser when it is one of the window's. `GFOPENMAKE.1` takes `TOOL`: orbit, move, rot, scale, measure, pull and draw. Pull needs a face. None of them changes the part. `OPENMAKESLICER.1` takes `SET tasks`, how many tasks the next slice runs on, the same as `TASKS=` on the command. A whole number is held from 1 to 64. The window has no field for it. `GFOPENMAKE.1` takes `LAYERS`: show one layer of the picture, and which one, as the Layers checkbox and slider. Prev and Next are `LAYERS STEP`. It does not cut the part. `OPENMAKESLICER.1` takes `LAYERS`: show layers 1 to an index, as the layer bar. Prev and Next are `LAYERS STEP`. A slice leaves the bar on the top layer. The window's Slice button uses `SET tasks`. `GFOPENMAKE.1` takes `SLICE`, the Slice button: the shown parts as a toolpath in the view. It does not write a file. The layers are made on one task a core, held from 1 to 64, as the Slice button. `GFOPENMAKE.1` takes `DRAW`: two points on the bed, the same solid as a dragged footprint. `GFOPENMAKE.1` takes `MASS`: the Face row's PLA, and a mark when the part hangs off the plate. `GFOPENMAKE.1` takes `WHERE`: the Place row, the size and where the part sits. `STATUS` also says whether the picked part is shown, its metal and its name. `OPENMAKESLICER.1` takes `LOOK`: top, front, side or frame. The window has no button for it. It does not change the part. `GFOPENCNC.1` takes `LOOK`: top, front, side or frame. Frame is the view at open. The window has no button for it. It does not change the cut. `PREVIEW` with no file says that window's line: the mill, the mode and the moves. `STATUS` adds `KIND`, the drawing label: SVG, DXF or File. `SAVE` keeps the file as `RESULT`, and reports the moves and the seconds the window shows. `OPENMAKEBURN.1` takes `LOOK`: top, front, side or frame. Frame is the view at open, not the Frame button. The window has no button for it. It does not change the job. `PREVIEW` with no file says that window's line: the moves, the laser and the time. `SAVE` keeps the file as `RESULT`, and reports the moves and the seconds the window shows.

## What it is

A mesh workshop on a 220 mm plate, Z up. It is not a slicer and not a parametric kernel. The slicer is OpenMakeCore's toolpath engine (`core/src/plan.c` and the rest, run by `slicer/src/sl_job.c`): walls, skins, infill and a brim. The first slicer, `slicer/src/slice.c`, is gone. The Amiga window is `cad/src/make.c`, on the same 220 mm plate. It has the bench's tools. Section on the window hides the picture, and the layer walls stop there too. Split is the cut. Draw drags the footprint, and the solid is shown while the pointer moves. Shelf puts an example from `examples/cad/` on the bed. Top, front and side turn the plate, and Frame fits the picked part. Metal, clay and wire change the picture, not the part. Snap holds a move, a turn or a scale until the next step. The Face row is the part's PLA at 1.24 g/cm³, and it says when the part hangs off the plate.

The window is OpenLayout Clear, laid out like the OpenQL launcher. One light window. A group is a box with a legend. A label sits to the left of its field. The status is one line along the bottom. Save STL is the blue primary button, in the place OpenQL puts Start.

Colours, from the Clear theme: window `#f9f9f9`, text `#1b1b1b`, accent `#0067c0`, buttons `#fefefe` to `#fbfbfb`, border `#d0d0d0`, radius 4 px, inputs `#ffffff` with border `#d6d6d6`, groups radius 8, font Segoe UI at 12 px. A selected tool, a selected part, and Save STL use the accent. Delete stays red.

## The window

Left: Tools, Picture, Extrude, Make, Library.

Middle: the plate. Left drag orbits. Right drag pans. The wheel zooms. Drop an STL on the plate to open it.

Right, once a part is selected: name and colour, size and place, arrange, layers, holes and joins.

Bottom: the status, then Open STL, Save part, Undo, Redo, and Save STL.

Keys, when the cursor is not in a field: G move, R turn, S scale, F frame, M measure, C section, `[` and `]` move the section, P pull, I solo. Delete removes the part. Escape leaves measure, sketch and pull. Ctrl-Z undoes. Ctrl-Shift-Z and Ctrl-Y redo. Ctrl-D copies.

## What was already there

The 9 November list in `docs/bench.md` still holds, and those checks were not redone for this note.

Add sits on the bed. On joins a boss to the selected part. Cut takes that depth off the top. Place uses width, depth and height. A round uses width as the diameter. Keyway is a slot. Pull and Chamfer move the face you clicked. Drill, bolt circle, row, counterbore and a 90° countersink. Split keeps both halves. Washer, line, ring, mirror, copy, stack, solo, section, measure. Drop, Centre, Lay flat. Metal, clay and wire. Top, front, side, frame.

The part under the selection is the stock. Join, Cut and Intersect combine the two. Drill bores the selection on its own.

## Layers

New. Show layer draws the triangle edges that cross one horizontal plane through the selected part. The height is 0.08 mm to 0.8 mm. The default is 0.2 mm. Previous, Next, and the slider on the picture step it. The line is blue, and it sits 0.05 mm above the plane so it reads on the surface.

Checked on the NASA wrench, which lands about 26 mm tall: 131 layers. Layer 1 is at 0.1 mm and was 443 mm of wall. The status says so, and it says there is no infill.

`contoursAt` in `cad/bench/modeller/layers.ts` is the whole of it. Each triangle that the plane cuts contributes one segment. The segments are not joined into loops. There is no infill and no G-code. Do not call this a slice. The slicer, when someone wires this up, still has to weld the loops the way the core's layer cut (`core/src/layer.c`) does.

Turning a part off hides the lines. Moving the part with the gizmo refreshes them when the drag ends, not on every move.

## Limits

- 24 parts. Undo keeps 24 steps.
- Join, cut, intersect, drill, bolt circle, row, counterbore, countersink and split stop at 2,500 triangles (`BOOLEAN_LIMIT` in `cad/bench/modeller/library.ts`). A result over 20,000 triangles is refused and the part is left as it was.
- The NASA wrench and the spacecraft can be opened, turned, measured and saved. They cannot be drilled or joined. The Utah teapot is CC0. The workshop parts are made here.
- A part that hangs off the 220 mm plate is marked. The mass line assumes solid PLA at 1.24 g/cm³. That is a guess for the solid, not a sliced estimate.
- Section is a clipping plane. The picture and the layer walls stop at it. It does not cut triangles until Split.

## Files

| Path | What it is |
| --- | --- |
| `cad/bench/modeller/studio.tsx` | The window |
| `cad/bench/modeller/bench.ts` | The plate, the tools, the snapshot |
| `cad/bench/modeller/layers.ts` | One plane of wall segments |
| `cad/bench/styles.css` | The Clear window |
| `cad/bench/modeller/gs.js` | The BSP booleans |
| `cad/bench/modeller/solids.ts` | Box, cylinder, sphere, cone, torus, wedge, slot |
| `cad/bench/library/` | The shelf. NASA models are public domain |

`library.ts`, `solids.ts`, `stl.ts` and `gs.js` were not changed for this note.

## Not done

- Draw drags a footprint on the bed, and the solid is shown while the pointer moves (blue to add, green on top, red to cut). It is not joined or cut until the button comes up. A block or a slot is the box between the two points. A round is centred on the first point, and its width is the diameter. Add, On top and Cut, while Draw is on, choose what the drag does. With Draw off they still place from the width, depth and height fields.
- The window's layer lines are core's cut (`core/src/layer.c`). The bench's segments are not joined. A slice welds the loops. Slice in the editor and in the slicer's own view lays that toolpath on the solid, in the plate's frame.
- The Amiga window was not rebuilt for this note.

## What was not run

The Amiga window was not rebuilt. `make test` was not run for this note. A host picture of the bench is not an Amiga run.
