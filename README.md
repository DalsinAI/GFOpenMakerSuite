# GFOpenMakerSuite

This repository, [DalsinAI/GFOpenMakerSuite](https://github.com/DalsinAI/GFOpenMakerSuite)
(it was `gfopenslicer`; the old address forwards here), holds the **GFOpenMake suite** for GFOS on AmigaOS 3.2:
one shared core and the apps on top of it. Every app's window is
GFOpenLayout (Clear look) and draws through OpenGPU, with the pen drawer
as the fallback.

| Folder | What | Name |
| --- | --- | --- |
| `core/` | Meshes and booleans, STL, the layer cut, the plate, the view and the OpenGPU batch | OpenMakeCore |
| `cad/` | The solid editor, and its browser bench in `cad/bench/` | OpenMakeCad |
| `slicer/` | STL to G-code for printers | OpenMakeSlicer |
| `burn/` | A laser app: `openmake-burn` and the OpenMakeBurn window (SVG, DXF or a Netpbm picture to Grbl G-code); the plan is [docs/burn-plan.md](docs/burn-plan.md) | OpenMakeBurn |
| `cnc/` | A mill: `openmake-cnc` and the GFOpenCNC window write a profile, a pocket or a drill from an SVG or a DXF, for Grbl or a generic LinuxCNC machine | OpenMakeCNC |
| `machines/` | Machine drivers and profiles every app reads: limits, features, start and end code | OpenMakeMachine |
| `workshop/` | A project drawer for one product: CAD, print, laser and CNC, plus a KiCad project listed and opened in gfamigakicad; the plan is [docs/workshop-plan.md](docs/workshop-plan.md) | OpenMakeWorkshop |

The binaries are GFOpenSlicer, GFOpenMake, GFOpenBurn, GFOpenCNC and GFOpenWorkshop. Each reports 0.1.0. `make test` builds and runs every host test.

Tags `v0.1.0-rc1` and `v0.2.0-rc1` are the slicer, then the solid editor on OpenGPU and the browser bench. No tag has been cut since. The same engine runs from the shell, and the host tests run it with no Amiga.

![The window, before a file is sliced](docs/window-empty.png)

![A 10 mm cube, three faces](docs/window-sliced.png)

Open asks for a file. Slice reads the fields and writes G-code next to the
STL, with `.gcode` in place of `.stl`. The list is one row a layer. The pane
is the solid on the plate. Once sliced, the toolpath is laid on that solid,
in the plate's frame. Drag turns it. Shift-drag pans. `gs_cam_zoom` zooms.

The slicer submits that picture to `gfopengpu.library` as
`OGPU_OP_TRIANGLES`, with a Z32 depth, on a retained surface, when the
library is open. The editor submits the same kind of batch. The pen drawer
is only the fallback. We already have OpenGPU. This tree does not grow a
second GPU.

From the shell:

```
gfopenslicer in.stl out.gcode SETTINGS=settings/standard pattern=grid around=brim loops=3
```

`SETTINGS=` names a print settings profile (`settings/standard` is the
built-in defaults, written out to copy); any of its keys can be given as
`key=value` after it. The older `LAYER=`, `INFILL=`, `SKIRT=` ... still work.
`START=` and `END=` are lines of your own, after the machine's start and
before its end. The heat, the home and M84 stay. `SUPPORT=1` adds one wall,
and a line every four widths, under an overhang steeper than 45 degrees,
from the bed up to it. The slicer window has the same three on the Settings side.

## Where the view comes from

Cura, PrusaSlicer, OrcaSlicer and OctoPrint are AGPL-3.0. Their viewports
are the picture this is aiming at, and none of that code is in here.

The camera we did take is [orbit-camera](https://github.com/mikolalysenko/orbit-camera)
(Mikola Lysenko, MIT, 2013): a point, a turn and a distance. A drag turns.
Shift-drag moves the point in the plane of the screen, and the move is
larger when the camera is further away. `gs_cam_zoom` changes the distance.
Ours keeps Z up, a turntable, which is what a bed wants. A free tumble would
roll the printer.

Also MIT, and not copied: [meshview](https://github.com/fogleman/meshview) is
that camera as an arcball on a GPU, in Go.
[gcode-viewer](https://github.com/aligator/gcode-viewer) draws a toolpath as
a mesh. That is the layer lines, next, not this release.

## Libraries

Use a library we already have. Write a new one only when we cannot take one
under MIT, BSD, Apache or BSL.

`gfopenlayout.library` is the window. `gfopengpu.library` is the chip and
its CPU back end: one batch, one retained surface. ASL is the file
requester. Cura, PrusaSlicer and OrcaSlicer are the picture to aim at, and
their code stays out. The booleans are ours, not a translation of JSCAD.

## ARexx

Every windowed app hosts a public ARexx port. GFOpenMakerWorkshop is the
hub (`OPENMAKEWORKSHOP.1`): it starts and controls the others
(`GFOPENMAKE.1`, `OPENMAKESLICER.1`, `OPENMAKEBURN.1`, `GFOPENCNC.1`). The ports, the
command parser, and a client that finds or starts an app and sends it a
command live in `openmakerexx.library`. The command set, the return codes
and the library's shape are in [docs/arexx.md](docs/arexx.md). Example scripts are in [docs/rexx/](docs/rexx/).

## What a slice does

It reads a binary STL or an ASCII one. A binary file is little-endian, which
is how a desktop saves it, so the same file loads on the 68000. Vertices that
land on each other are welded, empty triangles are dropped, and the outside
is turned out.

Each layer is the triangles cut by a plane. The pieces are joined into loops.
The outline is one wall. Infill is even-odd lines at 45 degrees. The first
layer can have a rectangular skirt. Extrusion comes from the layer height,
the line width and the filament diameter. A hotend or a bed of 0 leaves that
temperature command out.

The solid editor is `cad/src/make.c`. It starts from a 20 mm cube with a
hole, on a 220 mm plate. The window has the bench's move, turn, scale,
place, extrude, pull, chamfer, arrange, one layer of walls, and the hole
tools, including one drill, a 90 degree countersink, a washer and a
split. Top, front and side turn the plate. Frame fits the picked part.
Metal, clay and wire are the picture. Snap holds a move to 1 mm, a turn to 15 degrees and a scale to steps of 0.1. The Face row is the part's PLA, and it says when the part hangs off the plate. Section hides the picture above a height, and the layer walls
follow that cut. Draw drags a footprint on the bed, and the solid is
shown while the pointer moves: a block or a slot is the box between the
two points, a round is centred on the first. Shelf, on Model, puts one
of the examples in `examples/cad/` on the bed.

## Preview bench

`cad/bench/` is the browser copy of the same workshop, not a list of tools the window lacks. The window has taken the bench's tools; [docs/HANDOVER.md](docs/HANDOVER.md) is that record, and `docs/bench.md` is the 9 November note. To run the bench: `cd cad/bench && npm install && npm run dev`, then open http://127.0.0.1:5199. `npm run check` type-checks it and `npm run build` writes `cad/bench/dist/`; CI does both.

Add drops a block or a round on a 220 mm bed. On stands that shape on the selected part and joins it. Cut takes the height off the top. Place uses the W, D and H fields. A round uses W as the diameter. Keyway places a slot. Dragging still draws the footprint. Pull and Chamfer move the face you clicked. A washer, a slot, a line and a ring are there. Drill bores one hole. Bolt circle bores several of that diameter on a pitch circle, and Row bores them in a line. Counterbore adds a wider shallow step to the drill. Countersink is a 90° cone on that drill. Split cuts the selected part on the section, or at mid-height. Join, cut, intersect and drill stop at 2,500 triangles.

On the bench, layers draw the walls of the selected part at one height. Those segments are not joined, and that view has no infill and no G-code. The slicer joins walls. The Amiga window is OpenLayout Clear, laid out like the OpenQL launcher: groups, a label beside each field, a status line, and Save STL as the primary button.

The meshes in `cad/bench/library/` are the same shelf. The NASA ones are public domain. The Utah teapot is CC0. The wrench and the spacecraft are too heavy to cut. They can still be opened.

## Examples

Each instance's media volume gets a small set of free examples: STL, SVG,
DXF, a KiCad project and Gerbers. The files are in [examples/](examples/). The list, the licences, and what is
not copied, are in [docs/media-examples.md](docs/media-examples.md).

## Not in this release

The laser window has no field for your own start and end lines, or for `invert` and `width`. The port takes them as `SET start`, `SET end`, `SET invert` and `SET width`. The mill window has no start or end field either. `openmake-cnc` takes `START=` and `END=`, and `GFOPENCNC.1` takes `SET start` and `SET end`.

## Building

The host check:

```
make test
```

The Amiga programs are GFOpenMake (`cad/src/make.c`), GFOpenSlicer (`slicer/src/amiga.c`), GFOpenBurn (`burn/src/amiga.c`), GFOpenCNC (`cnc/src/amiga.c`) and GFOpenWorkshop (`workshop/src/amiga.c`), plus the core tests. `build-amiga.sh` builds
them twice, into `build-amiga/fpu` (68040/68060 or a 68881/68882, `-mhard-float`)
and `build-amiga/soft` (no FPU: `-msoft-float` with the SDK's
`m68k-soft-m68040.specs`). It fails if the soft build holds any FPU instruction
(libnix's start-up, which sets the FPCR only when AttnFlags shows an FPU, aside):

```
CC=.../m68k-amigaos-gcc SDK=.../gfopensdk LAYOUT=.../gfopenlayout \
OGT_ROOT=.../amicradle-guest/libraries/opengadtools ./build-amiga.sh
```

`LAYOUT` must be a GFOpenLayout checkout at the commit in `gfopenlayout.ref`,
the one CI tests with (`LAYOUT_ANY=1` to take another). The script builds
GFOpenLayout's 68040 UI link library, `libolui.a`, from it with its
`build-ui-amiga.sh` (that needs `OGT_ROOT`), or takes a ready one as
`OLUI=.../libolui.a`; either way it fails, naming them, if the library lacks
any `olui_` call the apps make. It has no floating point, so the one library
serves both builds.

The pictures above are the real GFOpenLayout draw list, painted on the host.
`make preview` rebuilds that tool when GFOpenLayout is checked out beside
this tree (`LAYOUT=` if it is not). `make test` needs it too, with
`make build/libopenlayout.a` run there: `test_make_ui` lays GFOpenMake's
window out with it.
