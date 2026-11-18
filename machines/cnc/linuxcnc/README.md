# LinuxCNC

A mill running LinuxCNC. The driver is `driver.om`. The generic 300 mm
profile is `profiles/generic-300.om`. Written from LinuxCNC's public
G-code reference, not from the interpreter source.

G21, G90 and G94 are the start. M3 and S run the spindle. M5 stops it.
G0 travels, G1 cuts. M2 is the last line: LinuxCNC reads nothing after
it. Probing (G38) is not in a job yet.
