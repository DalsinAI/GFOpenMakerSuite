/* openmakegcode.library 1.2: OpenMakeCore's G-code for a printer
 * (core/src/gcode.c) as a shared library. Its calls are
 * openmakegcode_lib.sfd's. The machine's start and end code are filled in
 * through openmakemachine.library (om_code is OM_Code, by
 * openmakemachine_link.c), opened at init: without it this does not open.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#define LIB_NAME "openmakegcode.library"
#define LIB_VERSION 1
#define LIB_REVISION 2
#define LIB_DATE "17.11.2026"

#include "openmakegcode_vectors.h"
#define LIB_VECTORS OPENMAKEGCODE_VECTORS
#define LIB_LAST_BIAS OPENMAKEGCODE_LAST_BIAS

#include <exec/libraries.h>
#include <proto/exec.h>

struct Library *OpenMakeMachineBase;

static int machine_open(void)
{
    OpenMakeMachineBase = OpenLibrary((CONST_STRPTR) "openmakemachine.library", 1);
    return OpenMakeMachineBase != NULL;
}

static void machine_close(void)
{
    if (OpenMakeMachineBase) CloseLibrary(OpenMakeMachineBase);
    OpenMakeMachineBase = NULL;
}

#define LIB_OPEN_MORE machine_open
#define LIB_CLOSE_MORE machine_close

#include "libshell.c"
