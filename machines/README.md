# Machines

The machine drivers the GFOpenMake suite shares: the slicer, the laser app
and the CNC app read the same files.

    machines/<family>/<driver>/driver.om          the dialect and its defaults
    machines/<family>/<driver>/profiles/*.om      one machine each

Families are `printer`, `laser` and `cnc`. A driver says the machine's
dialect, how a job is sent (file, serial, network), its start and end
G-code, and what every machine with that firmware has. A profile is read
over its driver and gives one machine's numbers.

Every machine declares:

- **Limits:** the work area; the most speed, acceleration and jerk on each axis
  (X, Y, Z, E); the hotend and bed temperatures; laser power; spindle RPM.
- **Features:** heated bed, extruder count, bed levelling, rotary axis, air
  assist, tool changer, spindle control, laser power, homing and probing.

An app reads these to grey out what the machine lacks, to clamp what it
asks for, and to warn before it writes a job that won't fit
(`om_check`). The C API is `include/om_machine.h`; on the Amiga it is to
be `openmakemachine.library`, loaded once for every app.

## The file

    # a comment
    key = value
    key += value        a line more of start or end; a word more of features or send

| Key | Value |
| --- | --- |
| `family` | `printer`, `laser` or `cnc` |
| `dialect` | `marlin`, `klipper`, `reprap`, `grbl`, `linuxcnc` |
| `name` | The machine's name |
| `send` | Any of `file`, `serial`, `network` |
| `features` | Any of `heated-bed`, `bed-levelling`, `rotary`, `air-assist`, `tool-changer`, `spindle`, `laser-power`, `homing`, `probing` |
| `area` | X Y Z in mm |
| `speed` | X Y Z E in mm/s |
| `accel` | X Y Z E in mm/s² |
| `jerk` | X Y Z E in mm/s: the speed change made at once, at a corner (0 or none: it stops there); with `accel`, for the time estimate |
| `hotend`, `bed` | The most °C |
| `laser` | The most power, % |
| `rpm` | The most spindle speed |
| `extruders` | How many |
| `start`, `end` | G-code; `{hotend}` and `{bed}` are filled in |

## Drivers

| Driver | State |
| --- | --- |
| `printer/marlin` | Written: a generic 220 mm profile, the Creality Ender-3 V2 and the Original Prusa i3 MK3S+ |
| `printer/klipper` | Written: the Voron 2.4 (300 mm) |
| `printer/reprap` | Written (RepRapFirmware, Duet): a generic 300 mm profile |
| `laser/grbl` | Written: a generic 400 mm diode-laser profile |
| `laser/ruida-later` | Later: a binary job, not G-code |
| `cnc/grbl`, `cnc/linuxcnc` | Not yet |

Clean room: every driver is written from the firmware's public G-code
documentation, never from its source, and every maker's profile from its
published specifications only; a number the sheet does not give is 0 (not
checked) and the file says so. `machines/tests/test_profiles.c` loads each
profile, and `make test` slices a part for each. MIT.
