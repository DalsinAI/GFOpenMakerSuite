/* A link library's base: the library opened before main() and closed after
 * it. Without the library the program stops at once with RETURN_FAIL and
 * says why, where a call through no library would crash. Built with
 * -DLINK_BASE=OpenMakeMachineBase -DLINK_NAME='"openmakemachine.library"'.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include <exec/types.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <inline/macros.h>

struct Library *LINK_BASE;

extern void exit(int);

static void __attribute__((constructor)) link_open(void)
{
    struct Library *dos;
    static const char why[] = "This needs " LINK_NAME " in LIBS:\n";
    if (LINK_BASE) return;
    LINK_BASE = OpenLibrary((CONST_STRPTR)LINK_NAME, 1);
    if (LINK_BASE) return;
    dos = OpenLibrary((CONST_STRPTR) "dos.library", 36);
    if (dos) {
        BPTR out = LP0(0x3c, BPTR, Output, , dos);
        if (out) LP3(0x30, LONG, Write, BPTR, out, d1, APTR, (APTR)why, d2, LONG, (LONG)sizeof(why) - 1, d3, , dos);
        CloseLibrary(dos);
    }
    exit(RETURN_FAIL);
}

static void __attribute__((destructor)) link_close(void)
{
    if (LINK_BASE) CloseLibrary(LINK_BASE);
    LINK_BASE = NULL;
}
