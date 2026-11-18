# Grbl (CNC)

A mill or a router running Grbl. The driver is `driver.om`. The generic
300 mm profile is `profiles/generic-300.om`. Written from Grbl's public
command documentation, not from firmware source.

M3 and S run the spindle. M5 stops it. G0 travels, G1 cuts. G21, G90 and
G94 are the start. Probing (G38) is not in a job yet.
