/* Ask GFOpenMake to add a box, export it as an STL, then ask the slicer
 * to load that part and write G-code. Workshop is the usual way to run
 * this as a project; this script talks to the two apps directly.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
OPTIONS RESULTS

PARSE ARG stl gcode
IF STRIP(stl) = '' THEN stl = 'RAM:part.stl'
IF STRIP(gcode) = '' THEN gcode = 'RAM:part.gcode'

CALL ensure_port 'GFOPENMAKE.1', 'GFOpenMake'
ADDRESS 'GFOPENMAKE.1'
'ADD box W 20 D 20 H 20'
IF RC ~= 0 THEN DO
  SAY 'ADD: RC='RC RESULT
  EXIT 10
END
'EXPORT FILE' stl
IF RC ~= 0 THEN DO
  SAY 'EXPORT' stl': RC='RC RESULT
  EXIT 10
END
SAY 'exported' stl

CALL ensure_port 'OPENMAKESLICER.1', 'GFOpenSlicer'
ADDRESS 'OPENMAKESLICER.1'
'LOAD FILE' stl
IF RC ~= 0 THEN DO
  SAY 'LOAD' stl': RC='RC RESULT
  EXIT 10
END
'SLICE'
IF RC ~= 0 THEN DO
  SAY 'SLICE: RC='RC RESULT
  EXIT 10
END
'SAVE FILE' gcode
IF RC ~= 0 THEN DO
  SAY 'SAVE' gcode': RC='RC RESULT
  EXIT 10
END
SAY gcode': 'RESULT
EXIT 0

ensure_port: PROCEDURE
  PARSE ARG port, prog
  IF SHOW('P', port) THEN RETURN 0
  ADDRESS COMMAND 'Run' prog
  DO i = 1 TO 30
    IF SHOW('P', port) THEN RETURN 0
    ADDRESS COMMAND 'Wait 1 SEC'
  END
  SAY 'Could not start' prog':' port 'is not open'
  EXIT 20
