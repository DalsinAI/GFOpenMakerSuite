/* openmaketask.library 1.0: OpenMakeCore's work on several tasks
 * (core/src/par_exec.c) as a shared library. Its calls are
 * openmaketask_lib.sfd's. The pieces are the calling program's code, run
 * on the tasks this makes: it must be built without -fbaserel (as the
 * OpenMake programs are), and its pieces must not use DOS (tasks, not
 * processes). They may allocate: libnix's malloc and the libraries'
 * AllocVec are safe from several tasks (par_exec.c says why).
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#define LIB_NAME "openmaketask.library"
#define LIB_VERSION 1
#define LIB_REVISION 0
#define LIB_DATE "17.11.2026"

#include "openmaketask_vectors.h"
#define LIB_VECTORS OPENMAKETASK_VECTORS
#define LIB_LAST_BIAS OPENMAKETASK_LAST_BIAS

#include "libshell.c"
