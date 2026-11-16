/* openmakepath.library 1.1: OpenMakeCore's paths (core/src/layer.c,
 * loops.c and inset.c) and toolpath (toolpath.c and fill.c) as a shared
 * library. Its calls are openmakepath_lib.sfd's.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#define LIB_NAME "openmakepath.library"
#define LIB_VERSION 1
#define LIB_REVISION 1
#define LIB_DATE "16.11.2026"

#include "openmakepath_vectors.h"
#define LIB_VECTORS OPENMAKEPATH_VECTORS
#define LIB_LAST_BIAS OPENMAKEPATH_LAST_BIAS

#include "libshell.c"
