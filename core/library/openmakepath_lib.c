/* openmakepath.library 1.4: OpenMakeCore's paths (core/src/layer.c,
 * loops.c and inset.c), toolpath (toolpath.c and fill.c) and a printer's
 * layers planned (plan.c) as a shared library. Its calls are
 * openmakepath_lib.sfd's.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#define LIB_NAME "openmakepath.library"
#define LIB_VERSION 1
#define LIB_REVISION 4
#define LIB_DATE "17.11.2026"

#include "openmakepath_vectors.h"
#define LIB_VECTORS OPENMAKEPATH_VECTORS
#define LIB_LAST_BIAS OPENMAKEPATH_LAST_BIAS

#include "libshell.c"
