/* gs_par.h on exec tasks (GFOS, AmigaOS): a task a piece but the first,
 * which runs on the caller; a piece whose task cannot be made runs on the
 * caller too. Each task, done, counts itself off and signals the caller
 * inside Forbid(), then ends: exec removes it before anything else runs,
 * so the caller frees its memory only once it is gone.
 *
 * The pieces may allocate: libnix's malloc, free and realloc hold
 * __memsema (a SignalSemaphore) round the program's pool, and the OpenMake
 * libraries' allocate with AllocVec, so both are safe from several tasks.
 * They must not use DOS: these are tasks, not processes.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include "gs_par.h"

#include <exec/execbase.h>
#include <exec/memory.h>
#include <exec/tasks.h>
#include <proto/exec.h>

#define STACK 65536UL

extern struct ExecBase *SysBase;

typedef struct {
    struct Task task;           /* first: the task is the piece */
    gs_par_fn fn;
    void *arg;
    int i;
    struct Task *caller;
    ULONG sig;
    volatile LONG *left;
    UBYTE *stack;
} piece;

static char task_name[] = "OpenMake slice";

static void run_piece(void)
{
    piece *p = (piece *)FindTask(NULL);
    p->fn(p->arg, p->i);
    Forbid();
    --*p->left;
    Signal(p->caller, p->sig);
    /* Ends here still forbidden: exec's RemTask() ends the Forbid(). */
}

static void free_piece(piece *p)
{
    if (p->stack) FreeMem(p->stack, STACK);
    FreeMem(p, sizeof *p);
}

int gs_par_run(int n, gs_par_fn fn, void *arg)
{
    struct Task *me = FindTask(NULL);
    piece **ps;
    volatile LONG left = 0;
    BYTE bit;
    int i, tasks = 1;
    if (n <= 0) return 1;
    bit = AllocSignal(-1);
    ps = bit >= 0 ? AllocMem((ULONG)n * sizeof *ps, MEMF_ANY | MEMF_CLEAR) : NULL;
    if (!ps) {
        if (bit >= 0) FreeSignal(bit);
        for (i = 0; i < n; i++) fn(arg, i);
        return 1;
    }
    for (i = 1; i < n; i++) {
        piece *p = AllocMem(sizeof *p, MEMF_PUBLIC | MEMF_CLEAR);
        if (p) p->stack = AllocMem(STACK, MEMF_ANY);
        if (!p || !p->stack) {
            if (p) free_piece(p);
            continue;
        }
        p->fn = fn;
        p->arg = arg;
        p->i = i;
        p->caller = me;
        p->sig = 1UL << bit;
        p->left = &left;
        p->task.tc_Node.ln_Type = NT_TASK;
        p->task.tc_Node.ln_Pri = me->tc_Node.ln_Pri;
        p->task.tc_Node.ln_Name = task_name;
        /* Empty: RemTask() frees what is on it, and nothing is. */
        p->task.tc_MemEntry.lh_Head = (struct Node *)&p->task.tc_MemEntry.lh_Tail;
        p->task.tc_MemEntry.lh_Tail = NULL;
        p->task.tc_MemEntry.lh_TailPred = (struct Node *)&p->task.tc_MemEntry.lh_Head;
        p->task.tc_SPLower = p->stack;
        p->task.tc_SPUpper = p->stack + STACK;
        p->task.tc_SPReg = p->stack + STACK;
        Forbid();
        left++;
        Permit();
        if (!AddTask(&p->task, (APTR)run_piece, NULL)) {
            Forbid();
            left--;
            Permit();
            free_piece(p);
            continue;
        }
        ps[i] = p;
        tasks++;
    }
    fn(arg, 0);
    for (i = 1; i < n; i++)
        if (!ps[i]) fn(arg, i);
    for (;;) {
        LONG now;
        Forbid();
        now = left;
        Permit();
        if (!now) break;
        Wait(1UL << bit);
    }
    for (i = 1; i < n; i++)
        if (ps[i]) free_piece(ps[i]);
    FreeMem(ps, (ULONG)n * sizeof *ps);
    FreeSignal(bit);
    return tasks;
}

/* Whether exec spreads tasks over cores is GFOS's business, not ours to
 * guess: 1, and a program asks for more itself (openmake-slice TASKS=). */
int gs_par_cores(void) { return 1; }
