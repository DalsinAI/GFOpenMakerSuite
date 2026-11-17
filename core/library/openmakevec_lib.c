/* openmakevec.library 1.1: OpenMakeCore's 2D vector drawings, SVG
 * reader (core/src/vec.c) and ASCII DXF reader (core/src/dxf.c) as a shared library. Its calls are
 * openmakevec_lib.sfd's.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#define LIB_NAME "openmakevec.library"
#define LIB_VERSION 1
#define LIB_REVISION 1
#define LIB_DATE "17.11.2026"

#include "openmakevec_vectors.h"
#define LIB_VECTORS OPENMAKEVEC_VECTORS
#define LIB_LAST_BIAS OPENMAKEVEC_LAST_BIAS

#include "libshell.c"
