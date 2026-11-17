# Examples for the media volume

17 November 2026. Each Cradle instance gets a media volume of example
files: things to open, slice, burn and look at, not a library to scrape.
This is the first set, and it is in `examples/`. The big meshes are for opening and slicing. They are not parts to drill.

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

These were copied into `examples/cad/`, not downloaded again. `workshop/` is this suite's, under the MIT licence. `nasa/` is the NASA models already on the bench. `teapot/` is the bench's teapot.

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

`burn/tests/data/` is copied to `examples/laser/`. Those files are this suite's. They are enough to open Burn. They are not a real job.

## Fetched

Each row is one example, not a whole repository. The licence note is in that folder.

| Where | What | Licence |
| --- | --- | --- |
| `examples/print/3DBenchy.stl` | The single-part file from [CreativeTools/3DBenchy](https://github.com/CreativeTools/3DBenchy), 225,706 triangles, 11 MB. A slice test | The repository says it may be downloaded, made and shared. NTI, Daniel Norée and Paulo Kiefe put the model in the public domain in February 2025. The repository itself has no licence file |
| `examples/print/xyzCalibration_cube.amf` | The 20 mm cube from PrusaSlicer commit `45d0532`, which is where that folder still is. Current PrusaSlicer no longer has it. The file is a zip of an AMF | CC BY-SA 3.0, iDig3Dprinting. The note that came with it is `examples/print/cube-LICENCE.txt` |
| `examples/print/xyzCalibration_cube.stl` | The same cube, written here as an STL (136 triangles, 20 mm) so the editor and the slicer can open it. The AMF is the source | The same CC BY-SA 3.0 |
| `examples/print/Voron_Design_Cube_v8.STL` | The Voron test cube, from Voron-Extras `Test_Prints` | GPL-3.0. `examples/print/GPL-3.0.txt` is that repository's licence |
| `examples/print/Mars-2020-sample-tube.stl` | NASA's Mars 2020 sample tube, 2 MB | NASA 3D Resources: free and without copyright. Not the insignia |
| `examples/electronics/pic_programmer/` | KiCad's `pic_programmer` demo, the project files only | Shipped with KiCad. The program is GPL-3.0-or-later. `GPL-3.0.txt` is that licence. Kept as taken from the KiCad source on 17 November 2026 |
| `examples/electronics/uno-kicad/` | A recreation of an Uno, KiCad 8, with `gerbers.zip` | WTFPL. Not Arduino's files. `LICENSE.txt` is in the folder |
| `examples/cnc/3dtest.ngc` | LinuxCNC's `nc_files/3dtest.ngc`, Ray Henry, 10 February 2000 | LinuxCNC is GPL-2.0. The file itself has no licence line. `examples/cnc/GPL-2.0.txt` is that repository's COPYING |

The official Arduino Uno design-files zip was not fetched. The page that used to point at it was not checked in this pass, and a guessed address is worse than a missing file. The Uno recreation above is the board and the Gerbers until that zip is taken from the board's own documentation.

## Not in this list

Support material, a brim drawn as an example, and a toolpath laid back
on the solid are still not done. They do not need a sample until the
app can show them.

A second pass can add one CERN-OHL board, one CC0 Smithsonian object
whose record was read, and one real laser job (a panel, not a square).
Each one needs the same licence check as the rows above.
