/* Load each STL named on the command, slice it in OpenMakeSlicer, and
 * write G-code beside it (name.gcode). Starts GFOpenSlicer if the port
 * is not already open. Workshop's C path uses OMR_Connect the same way.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
OPTIONS RESULTS

PARSE ARG rest
IF STRIP(rest) = '' THEN DO
  SAY 'usage: batch-slice.rexx file.stl ...'
  EXIT 20
END

CALL ensure_port 'OPENMAKESLICER.1', 'GFOpenSlicer'
ADDRESS 'OPENMAKESLICER.1'

DO WHILE STRIP(rest) ~= ''
  PARSE VAR rest file rest
  file = STRIP(file)
  IF file = '' THEN ITERATE
  'LOAD FILE' file
  IF RC ~= 0 THEN DO
    SAY 'LOAD' file': RC='RC RESULT
    EXIT 10
  END
  'SLICE'
  IF RC ~= 0 THEN DO
    SAY 'SLICE' file': RC='RC RESULT
    EXIT 10
  END
  out = outfile(file)
  'SAVE FILE' out
  IF RC ~= 0 THEN DO
    SAY 'SAVE' out': RC='RC RESULT
    EXIT 10
  END
  SAY out': 'RESULT
END
EXIT 0

outfile: PROCEDURE
  PARSE ARG path
  n = LASTPOS('.', path)
  slash = LASTPOS('/', path)
  colon = LASTPOS(':', path)
  IF slash > colon THEN colon = slash
  IF n = 0 | n < colon THEN RETURN path || '.gcode'
  RETURN LEFT(path, n - 1) || '.gcode'

/* Start prog if port is not public yet, and wait up to about 30 s. */
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
