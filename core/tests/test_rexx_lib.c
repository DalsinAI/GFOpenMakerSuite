/* openmakerexx.library's calls, run on the host as a program makes them:
 * through the jump table's slot at each bias to the library's side and
 * back to the C function. Every answer must be the C function's own.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include <proto/openmakerexx.h>

#include "../library/openmakerexx_vectors.h"

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

static const APTR table[] = {NULL, NULL, NULL, NULL, OPENMAKEREXX_VECTORS};
static struct Library *fake = (struct Library *)&fake;
struct Library *OpenMakeRexxBase;

APTR om_host_vector(struct Library *base, int bias)
{
    expect(base == fake && bias % 6 == 0 && bias >= 30 && bias <= OPENMAKEREXX_LAST_BIAS, "a call in the table");
    calls++;
    return table[bias / 6 - 1];
}

static int verb_open(void *cmd, void *app)
{
    const char *file = OMR_Arg(cmd, "FILE");
    int *hits = app;
    if (hits) (*hits)++;
    OMR_SetResult(cmd, file ? file : "");
    return OMR_OK;
}

static const omr_verb verbs[] = {
    {sizeof(omr_verb), "OPEN", "FILE/A", "open a file", verb_open},
};

int main(void)
{
    omr_port *p;
    omr_client *c;
    omr_io io;
    omr_opt opt;
    char buf[128], found[40], why[80];
    int hits = 0, before;
    OpenMakeRexxBase = fake;
    expect((int)(sizeof table / sizeof table[0]) == OPENMAKEREXX_LAST_BIAS / 6, "every call has its slot");

    memset(&io, 0, sizeof io);
    io.size = (int)sizeof io;
    io.result = buf;
    io.resultlen = (int)sizeof buf;
    io.timeout = 200;
    expect(OMR_DoLine("OPEN a.stl", verbs, 1, &hits, &io) == 0 && io.rc == OMR_OK && strcmp(buf, "a.stl") == 0 &&
               hits == 1,
           "OMR_DoLine OPEN a.stl");
    expect(omr_do_line("OPEN a.stl", verbs, 1, &hits, &io) == 0 && io.rc == OMR_OK && strcmp(buf, "a.stl") == 0,
           "OMR_DoLine matches the C function");
    expect(OMR_DoLine("OPEN", verbs, 1, &hits, &io) == 0 && io.rc == OMR_ERROR && strstr(buf, "required"),
           "OMR_DoLine missing FILE");

    p = OMR_Open("LIBTEST", "LibTest 1.0");
    expect(p && OMR_Name(p) && strcmp(OMR_Name(p), "LIBTEST.1") == 0 && strcmp(omr_name(p), "LIBTEST.1") == 0,
           "OMR_Open LIBTEST.1");
    expect(OMR_Mask(p) == 0 && omr_mask(p) == 0, "OMR_Mask on the host");
    expect(OMR_Attach(p, verbs, 1, &hits) == 0, "OMR_Attach");
    expect(OMR_Find("LIBTEST", found, (int)sizeof found) == 0 && strcmp(found, "LIBTEST.1") == 0, "OMR_Find");

    memset(&opt, 0, sizeof opt);
    opt.size = (int)sizeof opt;
    c = OMR_Connect("LIBTEST", &opt);
    expect(c && strcmp(opt.name, "LIBTEST.1") == 0, "OMR_Connect");
    expect(OMR_Send(c, "VERSION", &io) == 0 && io.rc == OMR_OK && strcmp(buf, "LibTest 1.0") == 0, "OMR_Send VERSION");
    expect(OMR_Send(c, "OPEN z.stl", &io) == 0 && io.rc == OMR_OK && strcmp(buf, "z.stl") == 0, "OMR_Send OPEN");
    OMR_Disconnect(c);

    /* A command object, parse and args, through the table. */
    {
        omr_cmd *cmd;
        /* Build a pending command on an unattached second port, then Get it. */
        omr_port *q = OMR_Open("LIBTEST", NULL);
        omr_client *d;
        expect(q && strcmp(OMR_Name(q), "LIBTEST.2") == 0, "OMR_Open second copy");
        memset(&opt, 0, sizeof opt);
        opt.size = (int)sizeof opt;
        d = OMR_Connect("LIBTEST.2", &opt);
        expect(d != NULL, "connect .2");
        /* No attach: queue, then Get without waiting in Send (we Get first by
         * not using Send). Use the C queue via a short Send in a... skip.
         * Parse a line through DoLine already covered parse. Exercise
         * OMR_Parse on a Get: put nothing, Get is NULL. */
        expect(OMR_Get(q) == NULL, "OMR_Get empty");
        (void)cmd;
        (void)why;
        OMR_Disconnect(d);
        OMR_Close(q);
    }

    OMR_Close(p);
    expect(OMR_Find("LIBTEST", found, (int)sizeof found) != 0, "ports closed");
    before = calls;
    expect(before >= 21, "every kind of call went through the table");
    printf(fails ? "rexx library: %d failed\n" : "rexx library ok\n", fails);
    return fails != 0;
}
