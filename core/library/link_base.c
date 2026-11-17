/* A link library's base: the library opened before main() and closed after
 * it. Without the library the program stops at once with RETURN_FAIL and
 * says why, where a call through no library would crash. Built with
 * -DLINK_BASE=OpenMakeMachineBase -DLINK_NAME='"openmakemachine.library"',
 * and for a program on two libraries LINK_BASE2 and LINK_NAME2 as well.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include <exec/types.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <inline/macros.h>

struct Library *LINK_BASE;
#ifdef LINK_BASE2
struct Library *LINK_BASE2;
#endif

extern void exit(int);

/* name opened into *base, or the program says so and stops. */
static void link_one(struct Library **base, const char *name, const char *why, LONG whylen)
{
    struct Library *dos;
    if (*base) return;
    *base = OpenLibrary((CONST_STRPTR)name, 1);
    if (*base) return;
    dos = OpenLibrary((CONST_STRPTR) "dos.library", 36);
    if (dos) {
        BPTR out = LP0(0x3c, BPTR, Output, , dos);
        if (out) LP3(0x30, LONG, Write, BPTR, out, d1, APTR, (APTR)why, d2, LONG, whylen, d3, , dos);
        CloseLibrary(dos);
    }
    exit(RETURN_FAIL);
}

static void __attribute__((constructor)) link_open(void)
{
    static const char why[] = "This needs " LINK_NAME " in LIBS:\n";
    link_one(&LINK_BASE, LINK_NAME, why, (LONG)sizeof why - 1);
#ifdef LINK_BASE2
    {
        static const char why2[] = "This needs " LINK_NAME2 " in LIBS:\n";
        link_one(&LINK_BASE2, LINK_NAME2, why2, (LONG)sizeof why2 - 1);
    }
#endif
}

static void __attribute__((destructor)) link_close(void)
{
    if (LINK_BASE) CloseLibrary(LINK_BASE);
    LINK_BASE = NULL;
#ifdef LINK_BASE2
    if (LINK_BASE2) CloseLibrary(LINK_BASE2);
    LINK_BASE2 = NULL;
#endif
}
