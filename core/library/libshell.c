/* The shell every OpenMake library is built in: its romtag, init, open,
 * close and expunge, the jump table, and the bases the runtime uses. A
 * library's own file sets LIB_NAME, LIB_VERSION, LIB_REVISION, LIB_DATE,
 * LIB_VECTORS and LIB_LAST_BIAS, includes its vectors.h, then this.
 * A library that uses another may also set LIB_OPEN_MORE(), which opens
 * it at init (0 when it cannot: the library then does not open either),
 * and LIB_CLOSE_MORE(), which closes whatever it opened.
 *
 * Built with the FPU (__HAVE_68881__), it will not open on a machine
 * without one: init fails and OpenLibrary() returns NULL, where the code
 * would crash at its first float. The build without the FPU opens anywhere.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include <exec/execbase.h>
#include <exec/libraries.h>
#include <exec/resident.h>
#include <dos/dos.h>
#include <proto/exec.h>

struct ExecBase *SysBase;
struct DosLibrary *DOSBase; /* runtime_dos.c's files */
#ifndef __HAVE_68881__
/* Without the FPU the float arithmetic is libnix's libm (-lm), through the
 * mathieee libraries; they use an FPU themselves when there is one. */
struct Library *MathIeeeSingBasBase, *MathIeeeDoubBasBase;
#endif

struct OMLibBase {
    struct Library lib;
    BPTR seglist;
};

#ifdef __HAVE_68881__
#define LIB_FPU " FPU"
#else
#define LIB_FPU ""
#endif
#define STR_(x) #x
#define STR(x) STR_(x)

static const char lib_name[] = LIB_NAME;
static const char lib_id[] = LIB_NAME " " STR(LIB_VERSION) "." STR(LIB_REVISION) " (" LIB_DATE ")" LIB_FPU " AmiCradle Team\r\n";
static const char lib_ver[] __attribute__((used)) =
    "$VER: " LIB_NAME " " STR(LIB_VERSION) "." STR(LIB_REVISION) " (" LIB_DATE ")" LIB_FPU " AmiCradle Team";

static BPTR lib_expunge(REG(a6, struct OMLibBase *base));

static void lib_free(struct OMLibBase *base)
{
#ifdef LIB_CLOSE_MORE
    LIB_CLOSE_MORE();
#endif
#ifndef __HAVE_68881__
    if (MathIeeeSingBasBase) CloseLibrary(MathIeeeSingBasBase);
    if (MathIeeeDoubBasBase) CloseLibrary(MathIeeeDoubBasBase);
    MathIeeeSingBasBase = MathIeeeDoubBasBase = NULL;
#endif
    if (DOSBase) CloseLibrary((struct Library *)DOSBase);
    DOSBase = NULL;
    FreeMem((UBYTE *)base - base->lib.lib_NegSize, base->lib.lib_NegSize + base->lib.lib_PosSize);
}

static struct Library *lib_init(REG(d0, struct OMLibBase *base), REG(a0, BPTR seglist), REG(a6, struct ExecBase *sys))
{
    SysBase = sys;
#ifdef __HAVE_68881__
    if (!(sys->AttnFlags & AFF_68881)) {
        lib_free(base);
        return NULL;
    }
#endif
    DOSBase = (struct DosLibrary *)OpenLibrary((CONST_STRPTR) "dos.library", 37);
#ifndef __HAVE_68881__
    MathIeeeSingBasBase = OpenLibrary((CONST_STRPTR) "mathieeesingbas.library", 37);
    MathIeeeDoubBasBase = OpenLibrary((CONST_STRPTR) "mathieeedoubbas.library", 37);
    if (!MathIeeeSingBasBase || !MathIeeeDoubBasBase) {
        lib_free(base);
        return NULL;
    }
#endif
    if (!DOSBase) {
        lib_free(base);
        return NULL;
    }
#ifdef LIB_OPEN_MORE
    if (!LIB_OPEN_MORE()) {
        lib_free(base);
        return NULL;
    }
#endif
    base->seglist = seglist;
    base->lib.lib_Revision = LIB_REVISION;
    return &base->lib;
}

static struct Library *lib_open(REG(a6, struct OMLibBase *base))
{
    base->lib.lib_OpenCnt++;
    base->lib.lib_Flags &= ~LIBF_DELEXP;
    return &base->lib;
}

static BPTR lib_close(REG(a6, struct OMLibBase *base))
{
    base->lib.lib_OpenCnt--;
    if (base->lib.lib_OpenCnt == 0 && (base->lib.lib_Flags & LIBF_DELEXP)) return lib_expunge(base);
    return 0;
}

static BPTR lib_expunge(REG(a6, struct OMLibBase *base))
{
    BPTR seglist;
    if (base->lib.lib_OpenCnt) {
        base->lib.lib_Flags |= LIBF_DELEXP;
        return 0;
    }
    seglist = base->seglist;
    Remove(&base->lib.lib_Node);
    lib_free(base);
    return seglist;
}

static ULONG lib_null(void) { return 0; }

static const APTR lib_vectors[] = {
    (APTR)lib_open, (APTR)lib_close, (APTR)lib_expunge, (APTR)lib_null,
    LIB_VECTORS,
    (APTR)-1
};

/* Each call where its bias says: one put in or taken out without its bias
 * stops the build. */
typedef char lib_vectors_match_biases[sizeof lib_vectors / sizeof lib_vectors[0] == LIB_LAST_BIAS / 6 + 1 ? 1 : -1];

static const struct {
    ULONG size;
    const APTR *vectors;
    APTR data;
    APTR init;
} lib_inittable = {sizeof(struct OMLibBase), lib_vectors, NULL, (APTR)lib_init};

const struct Resident lib_romtag __attribute__((used)) = {
    RTC_MATCHWORD, (struct Resident *)&lib_romtag, (APTR)(&lib_romtag + 1),
    RTF_AUTOINIT, LIB_VERSION, NT_LIBRARY, 0,
    (char *)lib_name, (char *)lib_id, (APTR)&lib_inittable
};
