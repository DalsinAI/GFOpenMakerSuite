/* openmaketask.library's calls, run on the host as a program makes them:
 * through the jump table's slot at each bias to the library's side and
 * back to the C function (par_posix.c here; par_exec.c in the library).
 * The pieces are the program's own code, called from the library's tasks.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include <proto/openmaketask.h>

#include "../library/openmaketask_vectors.h"

#include <stdio.h>
#include <string.h>

static int fails, calls;

static void expect(int ok, const char *what)
{
    if (!ok) {
        printf("fail: %s\n", what);
        fails++;
    }
}

static const APTR table[] = {NULL, NULL, NULL, NULL, OPENMAKETASK_VECTORS};
static struct Library *fake = (struct Library *)&fake;
struct Library *OpenMakeTaskBase;

APTR om_host_vector(struct Library *base, int bias)
{
    expect(base == fake && bias % 6 == 0 && bias >= 30 && bias <= OPENMAKETASK_LAST_BIAS, "a call in the table");
    calls++;
    return table[bias / 6 - 1];
}

/* Each piece fills its own slot: i squared, and how often it ran. */
typedef struct {
    long sq[64];
    int runs[64];
} work;

static void piece(void *arg, int i)
{
    work *w = arg;
    long k, s = 0;
    for (k = 0; k < 1000 + i; k++) s += (long)i * i;
    w->sq[i] = s / (1000 + i);
    w->runs[i]++;
}

int main(void)
{
    static work w;
    int i, ok = 1, tasks;
    OpenMakeTaskBase = fake;
    expect(sizeof table / sizeof table[0] == OPENMAKETASK_LAST_BIAS / 6, "every call has its slot");

    expect(GS_ParCores() == gs_par_cores() && GS_ParCores() >= 1, "GS_ParCores, 1 at least");
    tasks = GS_ParRun(64, piece, &w);
    for (i = 0; i < 64; i++) ok &= w.sq[i] == (long)i * i && w.runs[i] == 1;
    expect(ok, "GS_ParRun: every piece run once, its answer in its own slot");
    expect(tasks >= 1 && tasks <= 64, "on 1 to 64 tasks");
    memset(&w, 0, sizeof w);
    expect(GS_ParRun(1, piece, &w) == 1 && w.runs[0] == 1 && !w.runs[1], "one piece, on the caller");
    expect(GS_ParRun(0, piece, &w) == 1 && w.runs[0] == 1, "no pieces: nothing run");
    expect(calls == 5, "five calls, each through the table");
    printf(fails ? "task library: %d failed\n" : "task library ok\n", fails);
    return fails != 0;
}
