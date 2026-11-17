# Grbl (laser)

Written from Grbl's public laser-mode documentation only, no firmware
source. Laser mode ($32=1) is set in the firmware, not in the job. M3
holds power constant; M4 scales it with speed. S is the power, scaled
to $30 (often 1000). For OpenMakeBurn.

A generic 400 mm diode-laser profile is in `profiles/`. Change its
numbers to the machine's own.
