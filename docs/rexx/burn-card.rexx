/* Open a drawing in OpenMakeBurn, apply the card material, and write
 * Grbl laser G-code. Starts GFOpenBurn if the port is not already open.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
OPTIONS RESULTS

PARSE ARG drawing gcode material
IF STRIP(drawing) = '' THEN DO
  SAY 'usage: burn-card.rexx drawing.svg [out.gcode] [material.om]'
  EXIT 20
END
IF STRIP(gcode) = '' THEN gcode = outfile(drawing)
IF STRIP(material) = '' THEN material = 'materials/laser/card.om'

CALL ensure_port 'OPENMAKEBURN.1', 'GFOpenBurn'
ADDRESS 'OPENMAKEBURN.1'
'OPEN FILE' drawing
IF RC ~= 0 THEN DO
  SAY 'OPEN' drawing': RC='RC RESULT
  EXIT 10
END
'MATERIAL FILE' material
IF RC ~= 0 THEN DO
  SAY 'MATERIAL' material': RC='RC RESULT
  EXIT 10
END
'SAVE FILE' gcode
IF RC ~= 0 THEN DO
  SAY 'SAVE' gcode': RC='RC RESULT
  EXIT 10
END
SAY gcode': 'RESULT
EXIT 0

outfile: PROCEDURE
  PARSE ARG path
  n = LASTPOS('.', path)
  slash = LASTPOS('/', path)
  colon = LASTPOS(':', path)
  IF slash > colon THEN colon = slash
  IF n = 0 | n < colon THEN RETURN path || '.gcode'
  RETURN LEFT(path, n - 1) || '.gcode'

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
