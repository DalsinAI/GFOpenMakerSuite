/* openmakerexx.library 1.0: a public ARexx port and a client
 * (core/src/rexx.c) as a shared library. Its calls are
 * openmakerexx_lib.sfd's. The Amiga port, rexxsyslib and timer.device
 * are in core/src/rexx_amiga.c; the host test uses the stub in rexx.c.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#define LIB_NAME "openmakerexx.library"
#define LIB_VERSION 1
#define LIB_REVISION 0
#define LIB_DATE "17.11.2026"

#include "openmakerexx_vectors.h"
#define LIB_VECTORS OPENMAKEREXX_VECTORS
#define LIB_LAST_BIAS OPENMAKEREXX_LAST_BIAS

#include "libshell.c"
