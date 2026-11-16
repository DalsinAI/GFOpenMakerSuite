/* openmakegcode.library's calls, run on the host as a program makes them:
 * through the jump table's slot at each bias to the library's side and
 * back to the C function. Every answer must be the C function's own.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#include <proto/openmakegcode.h>

#include "../library/openmakegcode_vectors.h"

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

static const APTR table[] = {NULL, NULL, NULL, NULL, OPENMAKEGCODE_VECTORS};
static struct Library *fake = (struct Library *)&fake;
struct Library *OpenMakeGcodeBase;

APTR om_host_vector(struct Library *base, int bias)
{
    expect(base == fake && bias % 6 == 0 && bias >= 30 && bias <= OPENMAKEGCODE_LAST_BIAS, "a call in the table");
    calls++;
    return table[bias / 6 - 1];
}

/* G-code into memory, as a program gives the library. */
typedef struct {
    char buf[8192];
    long n;
} sink;

static long into(const char *s, long n, void *ud)
{
    sink *k = ud;
    if (k->n + n > (long)sizeof k->buf - 1) return 0;
    memcpy(k->buf + k->n, s, (size_t)n);
    k->n += n;
    k->buf[k->n] = 0;
    return n;
}

int main(void)
{
    static sink ka, kb;
    om_machine m;
    om_job ja, jb;
    gc_opts oa, ob;
    gc_stats sa, sb;
    gt_path p;
    float sq[] = {0, 0, 10, 0, 10, 10, 0, 10};
    gl_loop square = {sq, 4, 1};
    char err[128], errb[128];
    OpenMakeGcodeBase = fake;
    expect(sizeof table / sizeof table[0] == OPENMAKEGCODE_LAST_BIAS / 6, "every call has its slot");

    om_clear(&m);
    expect(om_load(&m, "machines/printer/marlin/driver.om", err, (int)sizeof err) == 0 &&
               om_load(&m, "machines/printer/marlin/profiles/generic-220.om", err, (int)sizeof err) == 0,
           "the generic Marlin printer");
    gc_defaults(&oa);
    memset(&ob, 0xff, sizeof ob);
    GC_Defaults(&ob);
    expect(!memcmp(&oa, &ob, sizeof oa), "GC_Defaults");
    oa.retract = ob.retract = 1.f;

    gt_init(&p);
    gt_layer(&p);
    gt_loop(&p, GT_OUTER, &square, 0.2f, 0.4f);
    gt_add(&p, GT_TRAVEL, 50, 50, 0.2f, 0);
    gt_add(&p, GT_INFILL, 60, 50, 0.2f, 0.4f);

    gc_estimate(&p, &m, &oa, &sa);
    GC_Estimate(&p, &m, &ob, &sb);
    expect(!memcmp(&sa, &sb, sizeof sa) && sb.moves > 5, "GC_Estimate");
    gc_job(&p, &m, &oa, &ja);
    GC_Job(&p, &m, &ob, &jb);
    expect(!memcmp(&ja, &jb, sizeof ja) && jb.max[0] == 60.f, "GC_Job");

    expect(gc_write_to(into, &ka, &p, &m, &oa, &sa, err, (int)sizeof err) == 0 &&
               GC_WriteTo(into, &kb, &p, &m, &ob, &sb, errb, (int)sizeof errb) == 0 && ka.n == kb.n &&
               !strcmp(ka.buf, kb.buf) && strstr(kb.buf, "M104 S") && !memcmp(&sa, &sb, sizeof sa),
           "GC_WriteTo, eight arguments: the same G-code, start code and all");
    m.family = OM_LASER;
    expect(GC_WriteTo(into, &kb, &p, &m, &ob, NULL, errb, (int)sizeof errb) == -1 && strstr(errb, "printer"),
           "GC_WriteTo refuses a laser, and says why");
    printf("through the library: %ld bytes of G-code, %d moves\n", ka.n, sb.moves);
    expect(calls == 5, "every GC_ call went through the table");

    gt_free(&p);
    if (fails) {
        printf("%d failed\n", fails);
        return 1;
    }
    printf("gcode library ok\n");
    return 0;
}
