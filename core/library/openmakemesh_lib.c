/* openmakemesh.library 1.1: OpenMakeCore's meshes (core/src/model.c and
 * stl.c) as a shared library. Its calls are openmakemesh_lib.sfd's.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#define LIB_NAME "openmakemesh.library"
#define LIB_VERSION 1
#define LIB_REVISION 5
#define LIB_DATE "17.11.2026"

#include "openmakemesh_vectors.h"
#define LIB_VECTORS OPENMAKEMESH_VECTORS
#define LIB_LAST_BIAS OPENMAKEMESH_LAST_BIAS

#include "libshell.c"
