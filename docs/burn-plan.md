# OpenMakeBurn plan

OpenMakeBurn is the laser app of the GFOpenMake suite. It takes drawings to
a laser as G-code. This note is the plan for it.

It is written from public feature descriptions of laser CAM applications of
this kind, and from Grbl's published laser-mode documentation. It is not
written from any other application's source, assets, names or branding.
MIT.

## The first cut

A command that reads an SVG, applies layer settings, and writes Grbl laser
G-code, with tests. Then a window: OpenLayout in the Clear look, the picture
drawn with OpenGPU and a pen fallback. The window fits 640×512 PAL at its
smallest, is resizable, and opens filling the screen. It has a layer list
with power and speed, a preview, and Save G-code.

A Grbl laser driver and a generic diode-laser profile live in
`machines/laser`.

## Features

### Drawings in

- **SVG first.** Paths, groups, and the usual shapes (line, rectangle,
  circle, ellipse, polyline, polygon). A distinct stroke or fill colour
  becomes a layer; the stroke colour wins when both are set. Text is not
  read unless it has already been turned into paths. Pictures inside an
  SVG are left for later.
- **DXF later.** The same layers from colours, with lines and arcs. Not in
  the first cut.

### Layers

Each layer has:

- **Power**, as a percentage of the machine's most.
- **Speed**, in mm/s, held to what the machine can do.
- **Passes**, how many times the layer is run. A later cut may also step Z
  between passes.
- **Mode:**
  - **Line:** follow the paths (a cut or a score, according to power and
    speed).
  - **Fill:** scan back and forth inside a closed shape, with a set gap
    between the lines (the interval).
  - **Offset fill:** follow the shape inwards in concentric loops, with
    the same interval. Better for a hollow shape that would otherwise
    spend a long time travelling; not the first choice for a complex one.
- **On or off:** a layer can be left out of the preview and the job.
- **Interval**, for fill and offset fill, in mm.

### Order

The job is run in the order of the layer list, which the user can change.
Engraving and scoring should usually run before a cut that goes through, so
the piece does not move first. A later "cuts last" sort can do that on its
own: fills first, then lines, with the heavier cuts (more power, less speed,
more passes) last among the lines.

### Preview

The window shows the job before it is written: lines as strokes, fills as
the scan (or a solid when the interval is fine), and travels in a second
colour that can be hidden. A time estimate uses the machine's speed,
acceleration and jerk, as the printer's G-code writer already does. A later
cut can play the moves in order.

### Framing

Framing runs the head around the job (the bounding box first; the outline
later) with the laser off, or at a very low power, so the work can be lined
up on the bed. The first cut writes that path as G-code. Sending it to the
machine comes with send-over-serial, which is later.

### Grbl laser G-code

Written from Grbl's public laser-mode documentation only, never from
another application's output.

- The machine is expected to have laser mode on (`$32=1`), so a change of
  power does not stop the move.
- **M4** is the default: power is scaled with the actual speed (dynamic
  power), and is off when the machine is still. **M3** holds power constant,
  including when it slows for a corner.
- **S** is the power, scaled to the machine's S at 100% (Grbl's `$30`,
  often 1000, so 50% is S500). The profile says that number.
- Travels are G0 and leave the laser off. Cutting and engraving moves are
  G1 (and later G2/G3 where a path is an arc).
- **M5** turns the laser off at the end. The driver's start and end G-code
  come from the profile, as they do for printers.
- The writer lives in `openmakegcode.library`, as new calls at the end of
  its list (append-only). A program sets the size of any new options
  struct, so a shorter one from an older program is read as the defaults.

### Machines

- `machines/laser/grbl`: the driver, from Grbl's public documentation only.
  Family `laser`, dialect `grbl`, features `laser-power` and `homing`, sent
  as a file (serial later).
- A generic diode-laser profile: a common bed size and speeds, from
  published diode-laser figures, not any maker's name. A number a sheet does
  not give is 0 (not checked), and the file says so.
- `machines/laser/ruida-later` stays later: Ruida controllers take their
  own binary job format, not G-code.

## Shared code

New work goes into OpenMakeCore's shared Amiga libraries wherever it can:

- A 2D vector library (paths from SVG, later DXF; offset and fill of those
  paths). New, with an FPU build and a soft build, `nm -u` clean, no FPU
  instructions in the soft build, and no 68040-trapped instructions.
- Laser G-code in `openmakegcode.library`, new calls at the end.
- Reuse `openmakepath` where a path or an inset is the same work,
  `openmakeview` for the preview, `openmaketask` if a long fill can be split
  over cores, and `core/ui` for the settings panel and the machine picker.

The existing library generator, `build-libraries.sh` and `build-amiga.sh`
are used as they are. `gfopenlayout.ref` stays pinned.

## The window

OpenLayout only, in the Clear look, like the other apps. The picture is
drawn with OpenGPU, with a pen fallback. At its smallest the window fits
640×512 PAL. It is resizable, and it opens filling the screen.

The first window has:

- the preview of the bed and the job
- the layer list, with power, speed, mode, passes and on/off
- the machine picker
- Open (SVG), Preview, Frame, and Save G-code

## Order of work

1. This plan.
2. The 2D vector and SVG reader, with tests.
3. The Grbl laser driver and the generic diode-laser profile, with tests.
4. The laser G-code writer in `openmakegcode`, with tests (M3/M4, S
   scaling, and travels with the laser off).
5. The command: SVG in, laser G-code out, with tests.
6. The window, as above.

## Not in the first cut

Pictures (raster engraving), text that is still a font, DXF, Ruida, sending
over serial or the network, a rotary axis, air assist as a live control,
galvo marking heads, and the user's own start and end G-code in the window
(the profile already has start and end). A materials library comes later.

A laser job must never leave the beam on during a travel or at the end of
the file. Tests hold that.
