/* openmakemachine.library 1.0: OpenMakeMachine (machines/src/machine.c) as
 * a shared library. Its calls are openmakemachine_lib.sfd's.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#define LIB_NAME "openmakemachine.library"
#define LIB_VERSION 1
#define LIB_REVISION 0
#define LIB_DATE "16.11.2026"

#include "openmakemachine_vectors.h"
#define LIB_VECTORS OPENMAKEMACHINE_VECTORS
#define LIB_LAST_BIAS OPENMAKEMACHINE_LAST_BIAS

#include "libshell.c"
