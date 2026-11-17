# Examples for the media volume

17 November 2026. Each Cradle instance gets a media volume of example
files: things to open, slice, burn and look at, not a library to scrape.
This is the first list. Nothing here has been downloaded yet.

The files follow Workshop's kinds (`docs/workshop-plan.md`). A folder
keeps the license text that came with the files. A file with no license
named on it does not go on the volume.

```
examples/cad/            STL the editor and the slicer can open
examples/print/          the same meshes, for a slice
examples/laser/          SVG, DXF, Netpbm
examples/electronics/    a KiCad project, and Gerbers beside it
examples/cnc/            a short program, for later
```

GFOpenMake opens STL. GFOpenSlicer opens STL and writes G-code.
GFOpenBurn opens SVG, DXF and Netpbm. Workshop sends a `.kicad_pro` to
gfamigakicad. Nothing in this suite reads a Gerber. The Gerbers are on
the volume so an electronics example is the files a board house gets,
not only the KiCad project.

## What may be copied

Public domain, CC0, CC BY, CC BY-SA, GPL, MIT, BSD, Apache, CERN-OHL or
WTFPL. The license file is copied with the model. CC BY and CC BY-SA
need the attribution kept. Share-alike stays share-alike.

Not copied, even when a site says the download is free:

- Thingiverse, Printables, MakerWorld, Cults, Thangs and the other
  catalogues. The model license and the site's terms are not the same
  thing, and a search result is not a license.
- A scan or a museum object until that object's own record says public
  domain or CC0. Smithsonian Open Access and NIH 3D are mixed.
- The Stanford bunny and the other Stanford scans.
- The NASA insignia folder.

## How big

The plate is 220 mm. Join, cut, intersect and the hole tools stop at
2,500 triangles. A result over 20,000 triangles is refused. A mesh past
that can still be opened, turned and sliced. It is not a part to drill.

Counts below were read from the files already in `cad/bench/library/`.

## Already in this tree

Copy these. Do not download them again.

| File | Triangles | What it is | On the volume |
| --- | --- | --- | --- |
| `hole-block.stl` | 336 | Made here | Editor |
| `spur-gear.stl` | 576 | Made here | Editor |
| `knob.stl` | 1,404 | Made here | Editor |
| `bracket.stl` | 1,491 | Made here | Editor |
| `hubble-body.stl` | 3,630 | NASA, public domain | Open and slice |
| `voyager-dish.stl` | 3,734 | NASA, public domain | Open and slice |
| `utah-teapot.stl` | 9,438 | Public domain | Open and slice |
| `nasa-wrench.stl` | 14,564 | NASA, public domain | Open and slice. 130 layers at 0.2 mm |
| `galileo.stl` | 17,473 | NASA, public domain | Open and slice |
| `curiosity-body.stl` | 18,338 | NASA, public domain | Open and slice |
| `jwst-body.stl` | 21,580 | NASA, public domain | Open. Too many triangles to cut |

The four small ones are this suite's, under the MIT licence at the top
of the tree. The NASA README says the models are free and without
copyright. Read the usage guidelines before adding more of them, and do
not take the insignia. The teapot is the one already on the bench.

`burn/tests/data/` is the first laser set: squares, a hole, two
rectangles, an open line, a small picture. Those files are this suite's.
They are enough to open Burn. They are not a real job.

## To fetch

Each row is one example, not a whole repository.

| Kind | Example | Licence | Take this | Leave this |
| --- | --- | --- | --- | --- |
| Print | 3DBenchy | Public domain (NTI, Daniel Norée and Paulo Kiefe, February 2025). The original CAD file is also given as CC0 | The STL from [3dbenchy.com](https://www.3dbenchy.com/download/) | A reupload. Count the triangles before it goes on a small volume. It is a slice test, not a part to drill |
| Print | 20 mm calibration cube | CC BY-SA 3.0, iDig3Dprinting | The cube in [PrusaSlicer's calibration resources](https://github.com/prusa3d/PrusaSlicer/tree/master/resources/calibration), with the attribution in that folder | The rest of PrusaSlicer |
| Print | Voron calibration cube | GPL-3.0, Voron Design | The one cube | The printer |
| CAD | One more NASA printable | NASA, free and without copyright | A single small model from [NASA-3D-Resources `3D Printing`](https://github.com/nasa/NASA-3D-Resources/tree/master/3D%20Printing), after a triangle count. The sample tube and the maintenance tool are the kind of thing. Terrain, nebulae and the insignia are not | The rest of the repository |
| Electronics | KiCad `pic_programmer` | Shipped with KiCad. The program is GPL-3.0-or-later. Read the `COPYING` beside the demo and keep it | [The demo](https://gitlab.com/kicad/code/kicad/-/tree/master/demos/pic_programmer) | The other demos, until someone has opened this one |
| Electronics | Uno in KiCad, with Gerbers | WTFPL, a recreation, not Arduino's files | [sabogalc/KiCad-Arduino-Boards](https://github.com/sabogalc/KiCad-Arduino-Boards), the Uno folder: the `.kicad_pro` and `gerbers.zip` | The other boards in that repository |
| Electronics | Arduino Uno design files | Whatever licence is printed in the zip. The older Uno files have been shared as CC BY-SA. Read the zip | The design-files archive linked from that board's own documentation | The Uno Q. Its published files are manufacturing files, not an editable design |
| CNC | One LinuxCNC sample | GPL-2.0, with LinuxCNC | A short program from the LinuxCNC samples | A machine configuration |

Gerbers from the Uno recreation sit next to that KiCad project. They are
the example of a board-house output. No app in this suite opens them.

## Not in this list

Support material, a brim drawn as an example, and a toolpath laid back
on the solid are still not done. They do not need a sample until the
app can show them.

A second pass can add one CERN-OHL board, one CC0 Smithsonian object
whose record was read, and one real laser job (a panel, not a square).
Each one needs the same licence check as the rows above.
