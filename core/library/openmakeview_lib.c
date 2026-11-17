/* openmakeview.library 1.3: OpenMakeCore's view (core/src/solid.c,
 * view.c, layer_paint.c, toolpath_paint.c and gpu.c) as a shared library, for every app of the suite. Its calls are
 * openmakeview_lib.sfd's. MIT, Copyright (c) 2026 AmiCradle Team. */
#define LIB_NAME "openmakeview.library"
#define LIB_VERSION 1
#define LIB_REVISION 3
#define LIB_DATE "17.11.2026"

#include "openmakeview_vectors.h"
#define LIB_VECTORS OPENMAKEVIEW_VECTORS
#define LIB_LAST_BIAS OPENMAKEVIEW_LAST_BIAS

#include "libshell.c"
