# OpenMakeWorkshop plan

OpenMakeWorkshop is the project app of the GFOpenMake suite. Its window
title is GFOpenMakerWorkshop. A project is one product: the CAD, print,
laser, CNC and electronics work for that product, housed together in one
Amiga drawer, and shown as a single product view.

It is written from this suite's own file conventions (the same plain-text
`key = value` files as `machines/` and `settings/`) and from the public
Amiga ARexx documentation, together with `docs/arexx.md` when that plan
is in. It is not written from any other application's source, assets,
names or branding. MIT.

## The first cut

A project drawer with a `project.om` manifest, a shared library that
opens and edits it, a command that can create a project and add items,
and a window: OpenLayout in the Clear look, the preview drawn with
OpenGPU and a pen fallback. The window fits 640×512 PAL at its smallest,
is resizable, and opens filling the screen. It shows one product, with
items grouped by kind, and opens an item in its app.

## A project is a drawer

A project is an Amiga drawer. `project.om` sits at its root. The first
cut also uses a subdrawer for each kind, so the files themselves live
with the product:

    MyWidget/
        project.om
        cad/            STL, and later the CAD's own files
        print/          slicer jobs and printer G-code
        laser/          SVG, DXF, pictures, and laser G-code
        cnc/            SVG and DXF for the mill
        electronics/    KiCad work that has been placed here

Paths in the manifest are relative to the project drawer when the file
is inside it, and an Amiga absolute path otherwise.

**Adding a suite file** (an STL, a drawing, a picture, or G-code): the
file is copied into the matching subdrawer unless it already lives
inside the project drawer. The manifest then stores the relative path.

**Adding a KiCad project:** the existing `.kicad_pro` (and its sibling
`.kicad_pcb` / `.kicad_sch`, found by name in the same drawer, not by
reading the files) is referenced in place. Workshop does not copy,
rewrite or parse a KiCad project. The user may already have placed that
folder under `electronics/`; then the path is relative, as for any other
file.

This copy-or-reference rule is the default for the first cut; it is
called out under Decisions if it should be the other way.

## The manifest (`project.om`)

The same style as `machines/` and `settings/`: one `key = value` a line,
`#` a comment, blank lines allowed. Units, where a later key needs them,
are millimetres.

    # GFOpenMakerWorkshop project
    format = 1
    name = Widget
    notes = A small desktop widget

    item.1.kind = cad
    item.1.name = Body
    item.1.file = cad/body.stl

    item.2.kind = print
    item.2.name = Body for the printer
    item.2.file = print/body.gcode
    item.2.from = 1

    item.3.kind = laser
    item.3.name = Front panel
    item.3.file = laser/panel.svg

    item.4.kind = electronics
    item.4.name = Controller
    item.4.file = electronics/controller/controller.kicad_pro

`format = 1` is this contract:

- Readers ignore a key they do not know, and write it back unchanged, so
  a later app can add keys without breaking an older Workshop.
- `kind` is an open token, not a closed list in the file. An unknown
  kind is valid: it is kept, shown in its own group, and written back.
  A later kind can therefore be added without a format bump and without
  breaking old projects.
- Item numbers are 1-based and need not be compact (a gap after a
  removal is allowed). On save they are rewritten 1..N in list order.

Known kinds in the first cut:

| kind | Group | Files | App |
| --- | --- | --- | --- |
| `cad` | CAD | `.stl`, and later the CAD's own files | GFOpenMake |
| `print` | Print | printer G-code, and a slicer job's other files | GFOpenSlicer |
| `laser` | Laser | `.svg`, `.dxf`, Netpbm pictures, laser G-code | GFOpenBurn |
| `cnc` | CNC | `.svg`, `.dxf` | GFOpenCNC |
| `electronics` | Electronics | `.kicad_pro`, `.kicad_pcb`, `.kicad_sch` | gfamigakicad |

A later **Render** kind (a Blender-style 3D rendering app, planned, not
now) is why kinds stay open. Nothing for Render is built in this work:
no group specially named, no app mapping, no file types reserved beyond
the rule that an unknown kind is kept.

Optional item keys in format 1, all ignored if missing:

- `from` — the 1-based number of another item this one was made from
  (a slice from a CAD part, laser G-code from a drawing).
- `settings` — a settings or material file used to make this item.

CAD today opens and saves STL (and can write G-code). A native CAD file,
when there is one, is still `kind = cad`, chosen by its extension. This
plan does not invent that format.

`.gcode` is used by both print and laser, so `kind` is required when an
item is added; it is not inferred from the suffix alone. Other suffixes
may still suggest a default kind in the window's Add requester
(`.stl` → cad, `.svg`/`.dxf`/Netpbm → laser, `.kicad_pro` → electronics).

## KiCad

gfamigakicad is trusted with KiCad files. Workshop lists a `.kicad_pro`,
`.kicad_pcb` or `.kicad_sch` by name and hands that path to gfamigakicad.
It does not read, draw, edit or write those files, and it does not touch
the gfamigakicad repository. A sibling board or schematic is found by
its file name only, so an item can point at the project. Workshop does
not create a KiCad project.

## Shared library: `openmakeproject.library`

New work goes into OpenMakeCore's shared Amiga libraries. The project
file, the item list, the kind registry and the recent-project list live
in `openmakeproject.library` (`core/`, header `gs_project.h`). It has an
FPU build and a soft build, `nm -u` clean, no FPU instructions in the
soft build, and no 68040-trapped instructions. Calls are append-only in
its `.sfd`. A settings-style struct starts with its own size, so a
shorter one from an older program is read as the defaults.

The first cut's calls (names to be settled with the generator when the
library is written):

- create, open and save a project (the drawer and `project.om`)
- get and set the product's name and notes
- add, remove and rename items; list them, optionally by kind
- resolve an item's file to a full path
- kind helpers: the known kinds, the group title, whether a kind is
  known, and which app opens it
- find the app for an item: the program name, and its ARexx port stem
  (for example `GFOpenMake` and `GFOPENMAKE`)
- a recent-project list (a small plain-text file of paths, most recent
  first; the first cut writes `PROGDIR:workshop-recent.om`)

Unknown kinds and unknown item keys survive a load/save round trip. The
kind-to-app map can grow later (Render, CNC) without changing old
projects.

The existing library generator, `build-libraries.sh` and `build-amiga.sh`
are used as they are. `gfopenlayout.ref` stays pinned.

## The command: `openmake-workshop`

    openmake-workshop new <drawer> [name=]
    openmake-workshop list <drawer>
    openmake-workshop add <drawer> kind= file= [name=] [from=]

`new` makes the drawer, the kind subdrawers and a `project.om`. `list`
prints the name, the notes and the items. `add` copies or references the
file as above and writes the manifest. Tests cover a new project, a
round trip of the manifest (including an unknown kind and an unknown
key), and add/remove/rename.

## The window

The app lives in `workshop/`. The program name is OpenMakeWorkshop; the
window title is GFOpenMakerWorkshop; the binary is GFOpenWorkshop (the
same short `GFOpen…` shape as GFOpenMake, GFOpenSlicer and GFOpenBurn).

OpenLayout only, in the Clear look, like the other apps. The preview is
drawn with OpenGPU, with a pen fallback. At its smallest the window fits
640×512 PAL. It is resizable, and it opens filling the screen. As in the
other apps, that is 640 by 480 inside, on a 640 by 512 PAL screen with
its borders.

### Sketch, 640×480 inside

    +------------------------------------------------------------------+
    | GFOpenMakerWorkshop                                    [_][.][X] |
    +------------------------------------------------------------------+
    | [New] [Open] [Save]     Widget                        Recent [v] |
    +-------------------------------------+----------------------------+
    | CAD                                 |                            |
    |   Body                 body.stl     |                            |
    | Print                               |         preview            |
    |   Body for printer     body.gcode   |     (the picked item)      |
    | Laser                               |                            |
    |   Front panel          panel.svg    |                            |
    | CNC                                 |                            |
    |   (none yet)                        |                            |
    | Electronics                         |                            |
    |   Controller           controller…  |                            |
    +-------------------------------------+----------------------------+
    | Notes: A small desktop widget                                    |
    +------------------------------------------------------------------+
    | [Add] [Remove] [Rename] [Open in app]                     Ready  |
    +------------------------------------------------------------------+

The left side is a list grouped by kind. The known groups are always
shown, even when empty (CNC in the first cut). An unknown kind gets its
own group under that kind's token, so a later Render item appears
without a layout change.

The right side is a preview of the picked item, where a core library can
already draw it:

- CAD / STL — `openmakeview` (the solid on the plate)
- Laser SVG or DXF — `openmakevec` paths
- G-code — `openmakeview`'s G-code lines, when the file is plain text
  G-code
- Netpbm — the laser's reader. Black is ink, white is paper, fitted in
  the view
- KiCad — not drawn. The status says gfamigakicad opens it.
- CNC — the file is drawn the same way (a solid, strokes, a toolpath
  or a picture). There is no separate mill picture

Buttons: New, Open and Save for the project; Add, Remove, Rename, and
Open in app for an item; a Recent picker of recently opened projects.
The notes field is the product's notes, saved with the manifest.

## Handing a file to an app

Open in app, and the ARexx `OPENITEM` command, do the same thing:

1. If `openmakerexx.library` is open, use its client: find the app's
   port (`GFOPENMAKE.1`, `OPENMAKESLICER.1`, `OPENMAKEBURN.1`, …); if
   the port is not up, start the program from `PROGDIR:` (or an explicit
   path) and wait until the port appears, or until the client's timeout;
   then send `OPEN FILE` with the item's resolved path, and return that
   reply.
2. If that library is not in yet, a small launch hook in Workshop starts
   the program with the file on its command line. GFOpenMake, GFOpenSlicer,
   GFOpenBurn and GFOpenCNC open a file named at launch, the same way their
   Open button does. The hook is documented and is switched to the client
   in (1) once the ARexx library is merged. Workshop does not duplicate
   `openmakerexx.library`.

| Kind | Binary | Port stem |
| --- | --- | --- |
| cad | GFOpenMake | `GFOPENMAKE` |
| print | GFOpenSlicer | `OPENMAKESLICER` |
| laser | GFOpenBurn | `OPENMAKEBURN` |
| cnc | GFOpenCNC | `GFOPENCNC`. Workshop still starts it with the file. The port has the shared commands and `PREVIEW` |
| electronics | gfamigakicad | none. The program is started with the file. The files are not read here |

gfamigakicad is launched only. Its repository is not touched, and no port
name is assumed for it.

## ARexx: `OPENMAKEWORKSHOP.1` is the master port

A script that is running a project talks to Workshop. Workshop talks to
the other apps through `openmakerexx.library`'s client. The first copy
of the window takes `OPENMAKEWORKSHOP.1`; a second copy would take
`.2`, as the other apps do.

Workshop hosts the shared commands in `docs/arexx.md` (`HELP`,
`VERSION`, `QUIT`, `OPEN`, `SAVE`, `ACTIVATE`, `HIDE`, `GET`, `SET`,
`STATUS`). For Workshop, `OPEN` and `SAVE` are the project drawer (or
the `project.om` path). `GET` / `SET` keys in the first cut are `name`
and `notes`. `STATUS` has the shared tails (`PORT`, `FILE`, `DIRTY`) and
`stem.ITEMS` (how many items are in the project).

These are Workshop's own commands, to be added to `docs/arexx.md` when
both plans are in. Arguments follow the same ReadArgs-style templates.
Return codes are the suite's (0, 5, 10, 20).

### NEW

Template: `NEW PATH/A NAME/K`

Creates a project drawer at `PATH` and opens it. `NAME` becomes the
product's name; otherwise the drawer's name is used.

### ADD

Template: `ADD KIND/A FILE/A NAME/K FROM/N/K`

Adds an item, copying or referencing `FILE` as above. `RESULT` is the
new item's 1-based number and its name.

### REMOVE

Template: `REMOVE ITEM/A`

Drops that item from the manifest. `ITEM` is a 1-based number or the
item's name. The file on disk is not deleted. RC 10 if there is no such
item.

### RENAME

Template: `RENAME ITEM/A NAME/A`

Renames the item in the manifest, not the file.

### ITEMS

Template: `ITEMS KIND/K STEM/K VAR/K`

Lists the items (or one kind). `RESULT` is a short line (how many).
`STEM`, when given, has `stem.0` as the count, then `stem.1` the first
item's name, `stem.1.KIND`, `stem.1.FILE`, `stem.1.FROM` (empty if
none), and so on.

### OPENITEM

Template: `OPENITEM ITEM/A`

Opens that item in its app, starting the app if its port is not up, as
"Handing a file to an app" above. `RESULT` is the port that was used.
RC 10 if the item or the app cannot be opened, with why in `RESULT`.

### SEND

Template: `SEND APP/A CMD/F/A`

Passes a command through to another app's port and returns that app's
`RC` and `RESULT` unchanged when the command was delivered. For example:

    ADDRESS 'OPENMAKEWORKSHOP.1'
    'SEND APP BURN CMD SAVE FILE RAM:panel.gcode'

`APP` is `CAD`, `SLICER`, `BURN`, `CNC` or `KICAD`, or a port name
(`OPENMAKEBURN.1`, `BURN`, `OPENMAKEBURN`). `CMD` is the rest of the
line, sent as-is.

`SEND` does not start the app. If the port is not up, the command fails
with RC 10 and why in `RESULT`. A script that needs the app running
calls `OPENITEM` first (or the client's start, once a script has it).
A timeout from the client is RC 20, as in `docs/arexx.md`.

### RECENT

Template: `RECENT STEM/K VAR/K`

`RESULT` is the most recent project path, or empty. `STEM`, when given,
is the recent list (`stem.0` how many, `stem.1` the first path).

`OPENMAKEWORKSHOP.1` is open on `openmakerexx.library`. Open in app, and
`OPENITEM` when the other app's port is not up, still use the launch
hook. `OPENITEM`, and the window's Launch button, send `OPEN FILE` when that port is already up. `SEND`
does not start the other app.

## Order of work

1. This plan.
2. `openmakeproject.library`, with tests (create / open / save, items,
   kinds including an unknown kind round-tripped, recent projects, and
   the app for an item).
3. The command: `openmake-workshop new|list|add`, with tests.
4. The window, as above, opening items through the launch hook.
5. The master port on `openmakerexx.library`, once that library is in,
   and the Workshop commands listed in `docs/arexx.md`.

`make test` covers each of these, plain and under ASan/UBSan. Nothing
here has been run on an Amiga until a lab run says so.

## Not in the first cut

- A Render kind, or any work for a rendering app.
- Creating a KiCad project here, or reading one. gfamigakicad owns those files.
- Inventing a native CAD document format.
- PNG or JPEG previews.
- Deleting an item's file when the item is removed.
- An `OPENMAKE:` assign, or a fixed default drawer for new projects
  (New asks for a drawer).
- Iconify / AppIcon (as the other apps).

## Decisions

These are the defaults this plan takes, and will go with unless they
should be otherwise:

- Suite files are copied into the project drawer; KiCad projects are
  referenced in place. gfamigakicad is trusted with those files.
- The binary is GFOpenWorkshop; the window title is GFOpenMakerWorkshop.
- Recent projects are stored in `PROGDIR:workshop-recent.om`.
- `from=` and `settings=` are stored. The item line shows `from N` when
  it is set. The window does not edit it, and it does not show `settings`.
- `SEND` does not start the app; `OPENITEM` does.
- Workshop adds existing KiCad projects only; it does not create them.
- New asks for a drawer; there is no default projects folder yet.
- gfamigakicad is looked for as `PROGDIR:gfamigakicad`, then by name on
  the path. A later note can pin a better place.
