/* openmakeproject.library 1.0: OpenMakeCore's GFOpenMakerWorkshop
 * project (core/src/project.c) as a shared library. Its calls are
 * openmakeproject_lib.sfd's.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#define LIB_NAME "openmakeproject.library"
#define LIB_VERSION 1
#define LIB_REVISION 0
#define LIB_DATE "17.11.2026"

#include "openmakeproject_vectors.h"
#define LIB_VECTORS OPENMAKEPROJECT_VECTORS
#define LIB_LAST_BIAS OPENMAKEPROJECT_LAST_BIAS

#include "libshell.c"
