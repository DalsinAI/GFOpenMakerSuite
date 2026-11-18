# ARexx in the GFOpenMake suite

Every windowed app in the suite hosts a public ARexx port, so a script or
another OpenMake program can drive it. The ports and the command parser
live in a shared library, `openmakerexx.library`, written from the public
Amiga ARexx and rexxsyslib documentation (the ARexx User's Reference and
the rexxsyslib autodocs). It is not written from any other application's
source. MIT.

This note is the command set and the shape of that library. The library,
the hooks in each app, and the example scripts come in the PRs after this
one.

## The hub

**GFOpenMakerWorkshop** (`workshop/`) is the hub. Its port,
`OPENMAKEWORKSHOP.1`, is the master port: it adds project items and starts
and controls the other apps. A script that is running a project talks to
Workshop; Workshop talks to GFOpenMake, OpenMakeSlicer and OpenMakeBurn
through the same library's client side.

Workshop hosts the shared commands below. Its own project commands are
Workshop's to list, in this file, when that app is in.

## Ports

Each app opens a public message port with a unique name. The first copy
takes `.1`; if that name is already out there, the next copy takes `.2`,
and so on.

| App | First port | Program |
| --- | --- | --- |
| GFOpenMakerWorkshop | `OPENMAKEWORKSHOP.1` | the Workshop window (the hub) |
| GFOpenMake | `GFOPENMAKE.1` | the solid editor |
| OpenMakeSlicer | `OPENMAKESLICER.1` | the slicer window |
| OpenMakeBurn | `OPENMAKEBURN.1` | the laser window |
| GFOpenCNC | `GFOPENCNC.1` | the mill window |

A script addresses one copy:

```
ADDRESS 'GFOPENMAKE.1'
'STATUS'
SAY RESULT
```

The command-line tools (`gfopenslicer`, `openmake-burn`) run and exit, so
they do not open a port. A script can still run them with
`ADDRESS COMMAND`. The windows are the ones a port talks to.

If `rexxsyslib.library` is not available, the app still opens and is used
from the window; it just has no port.

## Return codes and results

Return codes follow the usual ARexx convention:

| RC | Meaning |
| --- | --- |
| 0 | The command ran. |
| 5 | A warning: the command ran, but something was off. |
| 10 | An error: a bad argument, or the command could not be done. |
| 20 | A failure: the port or the program could not go on. |

A successful command that has something to say puts it in `RESULT` (the
ARexx special variable), as a string.

Commands that return several values also take:

- `VAR/K` — the same string is written to that ARexx variable.
- `STEM/K` — the values are written to that stem (`stem.0` is how many,
  then `stem.1`, `stem.2`, …, and named tails such as `stem.LAYERS` where
  the command lists them).

`VAR` and `STEM` are applied with rexxsyslib's `SetRexxVar` on the message
that carried the command, so they land in the program that sent it.

## How a command is written

Commands are case-insensitive. Arguments follow a ReadArgs-style template,
as dos.library documents it:

- `/A` — required
- `/K` — a keyword, `NAME value` or `NAME/value`
- `/S` — a switch, present or not
- `/N` — a decimal integer
- `/F` — the rest of the line
- `/M` — more than one string

A keyword that is a number in the template (`MM/K`, `X/K`) is still taken
as text by the library; the app parses it. The library itself does not use
floating point.

An unknown command, or a template that does not match, is RC 10, with why
in `RESULT`.

## Shared commands

Every windowed app hosts these, including Workshop.

### HELP

Template: `HELP COMMAND/K`

With no `COMMAND`, `RESULT` is the app's commands, one name per word. With
`COMMAND`, `RESULT` is that command's template and a short line of what it
does. An unknown name is RC 10.

### VERSION

Template: `VERSION`

`RESULT` is the program name and version, as the window's `$VER` string
(for example `GFOpenMake 0.1.0`).

### QUIT

Template: `QUIT FORCE/S`

Closes the window at once and ends the program. There is no "save first?"
requester. A document that has not been saved is discarded. With no
`FORCE`, a dirty document is refused (RC 10, `RESULT` saying so). `FORCE`
quits anyway.

### OPEN

Template: `OPEN FILE/A`

Opens that file as the app's document (an STL in GFOpenMake and the
slicer, a drawing or picture in OpenMakeBurn, a project in Workshop). RC
10 if it cannot be read, with why in `RESULT`.

### SAVE

Template: `SAVE FILE/K`

Writes the app's primary document. GFOpenMake writes an STL; the slicer
and OpenMakeBurn write G-code; Workshop writes the project. If `FILE` is
omitted, the last path is used; RC 10 if there is not one yet.

### ACTIVATE

Template: `ACTIVATE`

Brings the window to the front and activates it.

### HIDE

Template: `HIDE`

Sends the window to the back. It does not iconify; the apps have no
AppIcon yet.

### GET

Template: `GET KEY/K STEM/K VAR/K`

Reads a setting by its key. In GFOpenMake and the slicer the keys are the
print settings (`layer-height`, `line-width`, `walls`, `infill`,
`pattern`, `top-layers`, `bottom-layers`, `speed`, `travel-speed`,
`first-layer-speed`, `hotend`, `bed`, `filament`, `around`, `loops`,
`distance`, `retract`, `retract-speed`). In OpenMakeBurn they are the
job's keys (`power`, `speed`, `travel`, `passes`, `constant`, `s100`,
`mode`, `interval`, `cuts_last`, `start`, `end`, `invert`, `width`, `x`, `y`,
`rot`, `kerf`, `tabs`, `tabw`). `start` and `end` are
lines of your own, after the machine's start and before its end. Quote a
line that has a space. `invert` is 1 to burn the white of a picture, and
`width` is that picture's width in millimetres (0 uses the fill gap). The
window has no field for those four. `x`, `y` and `rot` are the place fields.
Workshop's keys are `name` and `notes`.

With no `KEY`, `STEM` receives every key: `stem.0` is how many, `stem.1`
the first key, `stem.1.VALUE` its value, and so on. With `KEY`, `RESULT`
(and `VAR`, if given) is that value. An unknown key is RC 10.

### SET

Template: `SET KEY/A VALUE/A`

Sets that key. The value is held to the setting's range, as the window
already does. An unknown key, or a value the setting will not take, is RC
10, with why in `RESULT`.

### STATUS

Template: `STATUS STEM/K VAR/K`

`RESULT` is a short line (the port name, the file open, whether it is
dirty). `STEM`, when given, also has:

- `stem.PORT` — the public port name
- `stem.FILE` — the file open, or empty
- `stem.DIRTY` — `1` or `0`
- and the app's own tails, listed with that app below

## GFOpenMake (port `GFOPENMAKE.1`)

The solid editor. `OPEN` and `SAVE` are STLs. `STATUS` also has
`stem.PARTS`, `stem.PICKED` (1-based, or 0), and `stem.FACE` (the picked
face, or -1).

### ADD

Template: `ADD SHAPE/A X/K Y/K Z/K W/K D/K H/K`

Adds a part. `SHAPE` is `box`, `cylinder`, `sphere`, `cone`, `torus`,
`wedge` or `slot` (as Parts' Make). With no place or size, the part is
placed and sized from the stock, as the window. `X`, `Y`, `Z` place it
(the middle of its box in X and Y, its bottom in Z); `W`, `D`, `H` size
its box. `RESULT` is the new part's 1-based index and its name.

### SHELF

Template: `SHELF NAME/K INDEX/N/K`

Puts one of the shelf examples (`examples/cad/`) on the bed, as the
window's Shelf. `NAME` is the example's name (`Angle bracket`, `Knob`,
and the rest). `INDEX` is 1-based in that same list. With neither, the
first example is used. `RESULT` says it is on the bed. RC 10 if there
is no such example, or the list is full.

### EXTRUDE

Template: `EXTRUDE KIND/A OP/K W/K D/K H/K`

The window's Extrude, from the width, depth and height. `KIND` is
`block`, `round` or `slot`. `OP` is `add` (a new part on the bed; the
default), `on` (joined to the top of the picked part) or `cut` (that
depth off the top). `W`, `D` and `H` are millimetres; the defaults are
20, 10 and 8, the same as the fields. A round uses `W` as the diameter.
`RESULT` says what was done. RC 10 if the size is out of range, there
is no part to join or cut, or the solid does not build.

### DROP / CENTRE / LAY

Templates: `DROP`, `CENTRE`, `LAY FACE/N/K`

`DROP` sits the picked part on the bed. `CENTRE` moves it to the middle
of the bed in X and Y, and leaves its height. `LAY` turns the part so
a face lies on the bed. `FACE` is that face's index; without it, the
picked face is used, or the largest face. `RESULT` says what happened.
A face that is already flat is not an undo step. RC 10 if there is no
part, or the face will not lie flat.

### RENAME / SHOW / COLOUR

Templates: `RENAME NAME/A`, `SHOW ON/N/K`, `COLOUR NAME/K INDEX/N/K`

`RENAME` is the name field. The same name again is not an undo step.
`SHOW` hides or shows the picked part. `ON` is `1` or `0`; without it,
the part toggles, as the window's Hide button. `COLOUR` is one of
`brass`, `sand`, `copper`, `steel`, `teal` or `sage`, or `INDEX` 1 to
6 in that order. Each is an undo step. RC 10 if there is no part, or
the name or metal is refused.

### LOOK / SHADE / SNAP

Templates: `LOOK NAME/A`, `SHADE NAME/A`, `SNAP ON/N/K`

`LOOK` is Top, Front, Side or Frame. `NAME` is `top`, `front`, `side`
or `frame`. `SHADE` is `metal`, `clay` or `wire`. `SNAP` is the Snap
button: without `ON` it toggles, and `ON` is `1` or `0` (any other number
is on). None of these changes the part, so none is an undo step. RC 10
if the name is not one of those, or `ON` is not a number.

### SECTION

Template: `SECTION ON/N/K Z/K`

Hides the picture above a height. It does not cut the part; Split does
that. With neither `ON` nor `Z`, it toggles, as the section key. `Z` is
the height in millimetres, held to 0 to 220 and to a tenth, and turns
the section on, as the height field. `ON` is `1` or `0`. RC 10 if `ON`
or `Z` is not a number.

### TOOL

Template: `TOOL NAME/A`

The tool buttons. `NAME` is `orbit`, `move`, `rot` (or `rotate`),
`scale`, `measure`, `pull` or `draw` (or `sketch`). `pull` needs a
face already picked, as the Drag button. None of these changes the
part, so none is an undo step. RC 10 if the name is not one of those,
or Pull has no face.

### SELECT

Template: `SELECT PART/N NAME/K`

Picks a part by 1-based index (`PART`) or by its name (`NAME`). RC 10 if
there is no such part.

### DELETE

Template: `DELETE`

Deletes the picked part. Refused (RC 10) if it is the last one, as the
window.

### UNDO / REDO

Templates: `UNDO` and `REDO`

One step back or forward, as the window. RC 10 if there is no step that
way.

### JOIN / CUT / INTERSECT

Templates: `JOIN`, `CUT`, `INTERSECT`

The picked part is the tool, the one beside it the stock, as the window.
`RESULT` says what was done. RC 10 with no second part, or when they do
not meet, with why in `RESULT`.

### MOVE

Template: `MOVE X/K Y/K Z/K`

Moves the picked part by that many millimetres (any of `X`, `Y`, `Z` may
be omitted, and then that axis is left).

### PLACE

Template: `PLACE X/K Y/K Z/K`

Puts the picked part so the middle of its box in X and Y, and its bottom
in Z, is at that point. At least one of `X`, `Y`, `Z` is required.

### SIZE

Template: `SIZE X/K Y/K Z/K AXIS/K MM/K`

Sizes the picked part's box. `X`, `Y`, `Z` are the box in millimetres on
those axes; or `AXIS` (`x`, `y` or `z`) and `MM` scale evenly to that
length, as the window's Size.

### HOLE

Template: `HOLE KIND/A D/K N/K PCD/K PITCH/K CB/K DEEP/K`

Cuts holes in the picked part, as the Holes tab. `KIND` is `circle` (a
bolt circle: `D`, `N`, `PCD`), `row` (`D`, `N`, `PITCH`), `counterbore`
(`D`, `CB`, `DEEP`), `drill` (one hole, `D` wide, through the middle) or
`countersink` (`D` the hole, `CB` the 90 degree mouth). `RESULT` says
what was done. RC 10 if the values are out of range or the cut cannot
be made, with why in `RESULT`.

### WASHER

Template: `WASHER`

Adds the bench's washer: 32 mm across, a 14 mm hole, 8 mm thick, on the
bed past the parts shown. `RESULT` says so. RC 10 if the list is full or
the washer does not build.

### COPY

Template: `COPY`

One copy of the picked part, 6 mm on in X, and that copy is picked. The
same as the window's Copy. `RESULT` names it. RC 10 if the list is full.

### LINE

Template: `LINE AXIS/K COUNT/N/K GAP/K`

Copies of the picked part in a line, as the window's Line. `AXIS` is
`x`, `y` or `z` (the default is `x`). `COUNT` is how many in all, held
to 2..8 (the default is 3). `GAP` is the space between them in
millimetres, held to 0..80 (the default is 6). The last copy is picked.
RC 10 if the list cannot hold them.

### RING

Template: `RING COUNT/N/K`

Copies of the picked part round the bed's middle, as the window's Ring.
`COUNT` is how many in all, held to 3..12 (the default is 3). A part on
the middle has its copies round it. RC 10 if the list cannot hold them.

### MIRROR

Template: `MIRROR`

Mirrors the picked part through X about its middle. `RESULT` says so.

### TURN

Template: `TURN AXIS/K DEG/K`

Turns the picked part about its middle. `AXIS` is `x`, `y` or `z` (the
default is `z`). `DEG` is degrees (the default is 90, the window's
button). `RESULT` says the turn.

### STACK

Template: `STACK`

Sits the picked part on the one before it in the list, over that part's
middle. The first part stacks on the second. RC 10 if it is the only
part.

### SOLO

Template: `SOLO`

Hides every part but the picked one. Sent again, shows them all. RC 10
if it is the only part.

### SPLIT

Template: `SPLIT Z/K`

Cuts the picked part on a horizontal plane into two closed halves, as
the window's Split. `Z` is the height in millimetres. Without it, the
plane is through the middle. The lower half keeps the part's place.
The upper half is added and picked. A part over 2,500 triangles is
refused. `RESULT` says the height. RC 10 if the plane misses the part
or the cut does not make two solids.

### PULL

Template: `PULL MM/A FACE/N/K`

Pulls a face of the picked part along its normal by `MM` millimetres
(less than 0: pushed in), as the window's Pull by a distance. `FACE`
defaults to the face picked in the view. RC 10 if there is no face, with
why in `RESULT`.

### CHAMFER

Template: `CHAMFER MM/A FACE/N/K`

Chamfers that face's edges by `MM` millimetres (0.2 to 20), as the
window's Chamfer. `FACE` defaults to the face picked in the view. RC 10 if
it cannot be done, with why in `RESULT`.

### EXPORT

Template: `EXPORT FILE/A`

Writes the parts as an STL to `FILE`, as the window's Save STL. The same
as `SAVE FILE`.

### PRINTER

Template: `PRINTER DRIVER/K PROFILE/K`

Picks the printer, as the window's machine list. `DRIVER` is a driver
file and `PROFILE` a machine profile. At least one is required. A laser
is refused. When that printer is in the window's list, the list shows
it and the settings are held to it. RC 10 if the file cannot be read,
or it is not a printer.

### GCODE

Template: `GCODE FILE/A`

Slices the shown parts with the current print settings and printer, and
writes G-code to `FILE`, as the window's Save G-code.

### MEASURE

Template: `MEASURE X1/K Y1/K Z1/K X2/K Y2/K Z2/K STEM/K VAR/K`

The distance between two points, and the distance along each axis. With
`X1`…`Z2`, those points (in millimetres). With none, the last two points
clicked in Measure; RC 10 if there are not two yet. `RESULT` is the same
line the window shows. `STEM` has `stem.DISTANCE`, `stem.X`, `stem.Y`,
`stem.Z`.

### FACTS

Template: `FACTS PART/N/K STEM/K VAR/K`

A part's volume, surface, size and triangles, as the window's Facts. The
picked part, or `PART` (1-based). `RESULT` is the same line the window
shows. `STEM` has `stem.VOLUME`, `stem.SURFACE`, `stem.X`, `stem.Y`,
`stem.Z`, `stem.TRIANGLES`.

## OpenMakeSlicer (port `OPENMAKESLICER.1`)

The slicer window. `OPEN` and `LOAD` both open an STL. `SAVE` writes
G-code. `STATUS` also has `stem.LOADED`, `stem.SLICED`, `stem.LAYERS`,
`stem.TIME` and `stem.FILAMENT`. `GET` and `SET` take the print settings,
and also `support`, `start` and `end` by name: the Settings side of the
window, the same as `SUPPORT=`, `START=` and `END=` on the command.
`support` is `1` or `0`. `start` and `end` are lines of your own, after
the machine's start and before its end. An empty line clears that one.
The list with no `KEY` stays the print settings.

### LOAD

Template: `LOAD FILE/A`

Opens an STL, as `OPEN`. The part is repaired and set on the bed, as the
window.

### PROFILE

Template: `PROFILE FILE/A`

Loads a print-settings profile (`settings/standard` and the rest). Keys
can still be changed with `SET` after it.

### PRINTER

Template: `PRINTER DRIVER/K PROFILE/K`

Picks the machine. `DRIVER` is a driver file (`machines/printer/marlin/driver.om`
and the rest); `PROFILE` is a machine profile. At least one is required.
RC 10 if the file cannot be read, or if it is not a printer. When that
printer is in the window's list, the list keeps it, and the settings
are held to it.

### SLICE

Template: `SLICE STEM/K VAR/K`

Slices the loaded part with the current settings and printer. `RESULT` is
a short line (layers, time, filament). `STEM` has `stem.LAYERS`,
`stem.TIME` (seconds) and `stem.FILAMENT` (millimetres). RC 10 if there is
no part, or if the printer cannot print it, with why in `RESULT`.

### STATS

Template: `STATS STEM/K VAR/K`

The last slice's figures, in the same form as `SLICE`. RC 10 if it has not
been sliced yet.

## OpenMakeBurn (port `OPENMAKEBURN.1`)

The laser window. `OPEN` reads an SVG, a DXF or a Netpbm picture. `SAVE`
writes Grbl laser G-code. `STATUS` also has `stem.KIND` (`SVG`, `DXF` or
`Picture`), `stem.LAYERS` and `stem.MODE`. `GET` and `SET` also take `invert`
(1 burns the white of a picture) and `width` (the picture's width in
millimetres; 0 uses the fill gap). The window has no field for them. The
port keeps them when the window rebuilds the job. `x`, `y` and `rot`
move the drawing, the same as the place fields, and the window shows
the values the port set. `SET` of `power`, `speed`, `passes`, `mode` or
`interval` is written onto every layer row, and the next rebuild reads
the first row back as the job. `SET kerf`, `SET tabs` and `SET tabw` do
the same for the holding tabs. `travel` and `s100` have no row; the port
keeps them across a rebuild. `order` is the layer list, 1-based and comma
separated, the same as `order=` on the command and the window's Up and Dn.
An empty `order` is the file order. Layers left out follow, in file order.
The port keeps the list when the window rebuilds the job. Opening a
drawing clears it. `cuts_last` still sorts the burn when it is on; turn
it off to burn in that list order. `MACHINE DRIVER PROFILE` picks the
laser, as the window's list. A printer is refused. When that laser is
in the list, the list keeps it.

### LAYER

Template: `LAYER INDEX/N/K COLOUR/K POWER/K SPEED/K PASSES/N/K ON/N/K MODE/K INTERVAL/K`

Sets one layer. Identify it with `INDEX` (1-based in the document's layer
list) or `COLOUR` (`#rrggbb`). The other keywords are that layer's
overrides, as the window and `layer=` on the command. `MODE` is `line`,
`fill` or `offset`. `ON` is `1` or `0`. RC 10 if there is no such layer,
or if a value is bad, with why in `RESULT`.

### MATERIAL

Template: `MATERIAL FILE/A`

Loads a starting-point materials profile (`materials/laser/card.om` and
the rest). Power, speed and the other keys after it can still be changed
with `LASER` or `SET`.

### LASER

Template: `LASER POWER/K SPEED/K TRAVEL/K PASSES/N/K CONSTANT/N/K S100/K`

The job's default power (0 to 100), speed and travel (mm/s), passes, and
whether the laser is constant (`CONSTANT` 1: M3, 0: M4). `S100` is S at
100% power. Any keyword may be omitted; those are left as they are.

### MODE

Template: `MODE NAME/A`

The job's default mode: `line`, `fill` or `offset`.

### PREVIEW

Template: `PREVIEW FILE/K`

Refreshes the window's preview of the job. With `FILE`, also writes a PPM
of that preview to the file.

### FRAME

Template: `FRAME FILE/A`

Writes G-code that travels around the job's box with the laser off, as
the window's Frame, to `FILE`. RC 10 if there is no job yet.

## GFOpenCNC (port `GFOPENCNC.1`)

The shared commands. `OPEN` loads an SVG or a DXF. `SAVE` writes mill
G-code for the mill picked, and is RC 10 when none is picked. `PREVIEW`
plans the cut the window shows. With `FILE` it also writes a PPM of that
picture. `GET` and `SET` use the cut's keys: `tool`, `depth`, `stepdown`,
`stepover`, `feed`, `plunge`, `safe`, `rpm`, `mode` (`profile`, `pocket`
or `drill`), `x`, `y`, `start`, `end`. A number is held to that field's range on the
window. `start` and `end` are lines of your own, after the machine's start
and before its end. The window has no field for them. `STATUS` adds `MODE`.
`MILL DRIVER PROFILE` picks the mill, as the window's list. A printer is
refused. When that mill is in the list, the list keeps it.

## GFOpenMakerWorkshop (port `OPENMAKEWORKSHOP.1`)

The project window. `OPEN` and `SAVE` are the project drawer (or its
`project.om`). `SAVE` writes that project; a different `FILE` is refused.
`GET` and `SET` are `name` and `notes`. `STATUS` also has `stem.ITEMS`.

`NEW PATH NAME` creates a drawer and opens it. `ADD`, `REMOVE` and
`RENAME` change the manifest; removing an item does not delete its file.
`ITEMS` lists them, or one `KIND`. `OPENITEM` opens that item in its app:
if the app's port is up, `OPEN FILE` is sent and `RESULT` is that port;
if it is not, the launch hook starts the program and `RESULT` is the
port stem, or the program name when the kind has no port. `SEND` passes
`CMD` to `APP` (`CAD`, `SLICER`, `BURN`, `CNC`, `KICAD`, or a port name)
and does not start the app. A port that is not up is RC 10. `RECENT` is
the recent project list.

## The library

`openmakerexx.library` (in `core/`) is generated from an `.sfd` like the
other core libraries: an append-only ABI, an FPU build and a soft build.
The soft build holds no FPU instructions; both builds are clean under
`nm -u` and hold no 68040-trapped instructions, checked with
`fpu-count.sh`. The library does not use floating point.

It has two sides.

### The port (what each app hosts)

The app opens a port at start, and closes it when it quits. The main loop
waits on the window and the port together, with Exec `Wait()` on both
signals. There is no busy waiting.

On a signal from the port, the app takes each pending message, looks the
verb up in its command table, parses the arguments against the template,
runs the command, and replies with an RC and a `RESULT` string.

If the library cannot open (no `rexxsyslib.library`, or no memory for the
port), the app goes on without a port.

### The client (what Workshop, and a test, uses)

The same library finds or starts another app's port, sends a command, and
waits for the reply, with a timeout.

- **Find.** Look up a public port by its base name (`GFOPENMAKE`) or a
  full name (`GFOPENMAKE.1`). The name that was found is returned.
- **Start.** If no port is there, run the program (a path under `PROGDIR:`
  or an explicit path) and wait until its port appears, or until the
  timeout.
- **Send.** Put a command on that port and wait for the reply, or until
  the timeout. The RC and the `RESULT` string are returned to the caller.
  A timeout is RC 20, with why in the caller's result buffer. The wait is
  a timer.device request plus `Wait()` on the reply and the timer, not a
  busy loop.

Workshop uses this to start GFOpenMake, the slicer and OpenMakeBurn, and
to drive them as project items are added and run. A host test of the
library uses the same calls against an in-process stub, so `make test`
(plain and under ASan/UBSan) covers the parser, the dispatch, and the
client's find / send / timeout path. The real port and the timer are
compiled only for Amiga.

The calls (structs start with their size; never reorder or take a call
out, add new ones at the end):

- `OMR_Open` / `OMR_Close` / `OMR_Mask` / `OMR_Name` / `OMR_Get` — the
  port. `OMR_Mask` is the Exec signal bit to OR into `Wait()`.
- `OMR_Text` / `OMR_Parse` / `OMR_Why` / `OMR_Arg` / `OMR_Switch` — the
  command line and its arguments.
- `OMR_SetResult` / `OMR_SetVar` / `OMR_SetStem` / `OMR_Reply` /
  `OMR_Dispatch` — run a command and reply. `OMR_DoLine` does that
  in-process for a single line (the host tests, and a useful shortcut).
- `OMR_Attach` — serve a client in this process from a command table,
  with no second thread.
- `OMR_Find` / `OMR_Connect` / `OMR_Disconnect` / `OMR_Send` — the
  client. `OMR_Connect` finds the port, or starts a program and waits
  until it appears or the timeout. `OMR_Send` waits for the reply the
  same way. A timeout is RC 20, with why in the caller's buffer.

The C names are the same in snake case (`omr_open`, `omr_send`).

## Example scripts

These live under `docs/rexx/`. They are ordinary ARexx: each one looks
for the app's public port, starts the program with AmigaDOS `Run` if it
is not there yet, and `ADDRESS`es it. Workshop's C path starts and talks
to the same ports through `OMR_Connect` / `OMR_Send`.

- `batch-slice.rexx` — ask the slicer to load each STL named on the
  command, slice it, and write G-code beside it.
- `make-and-slice.rexx` — ask GFOpenMake to add a box, export an STL, then
  ask the slicer to load it and slice it. Workshop is the usual way to
  run that as a project; this script talks to the two apps directly.
- `burn-card.rexx` — open a drawing in OpenMakeBurn, apply the card
  material, and write G-code.

The programs (`GFOpenMake`, `GFOpenSlicer`, `GFOpenBurn`) need to be on
the command path, or the scripts run from the suite drawer. Nothing here
has been run on an Amiga until a lab run says so.

## Building and tests

The parser, the dispatch table, `HELP`, and the client's find / send /
timeout path are built and tested on the host. The Amiga port, the
rexxsyslib messages, and timer.device are compiled only into the Amiga
library and the apps.

`make test` must cover them, plain and under ASan/UBSan. Nothing here has
been run on an Amiga until a lab run says so.
