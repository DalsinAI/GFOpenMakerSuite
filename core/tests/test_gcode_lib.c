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
    expect(GC_EstimateHow(&p, &m, &ob, &sb) == gc_estimate_how(&p, &m, &oa, &sa) && !memcmp(&sa, &sb, sizeof sa) &&
               GC_EstimateHow(&p, NULL, &ob, &sb) == GC_AT_SPEEDS,
           "GC_EstimateHow: the estimate, and how its time was got");
    {
        /* 1.2: the more, a first-layer speed. */
        gc_more xa, xb;
        gc_more_defaults(&xa, &oa);
        memset(&xb, 0xff, sizeof xb);
        GC_MoreDefaults(&xb, &ob);
        expect(xb.size == xa.size && xb.o == &ob && xb.first == xa.first, "GC_MoreDefaults");
        xa.first = xb.first = 15.f;
        expect(GC_EstimateMore(&p, &m, &xb, &sb) == gc_estimate_more(&p, &m, &xa, &sa) && !memcmp(&sa, &sb, sizeof sa),
               "GC_EstimateMore");
        ka.n = kb.n = 0;
        expect(gc_write_more(into, &ka, &p, &m, &xa, &sa, err, (int)sizeof err) == 0 &&
                   GC_WriteMore(into, &kb, &p, &m, &xb, &sb, errb, (int)sizeof errb) == 0 && ka.n == kb.n &&
                   !strcmp(ka.buf, kb.buf) && strstr(kb.buf, " F900\n") && !memcmp(&sa, &sb, sizeof sa),
               "GC_WriteMore, eight arguments: the first layer at 15 mm/s");
    }
    m.family = OM_LASER;
    expect(GC_WriteTo(into, &kb, &p, &m, &ob, NULL, errb, (int)sizeof errb) == -1 && strstr(errb, "printer"),
           "GC_WriteTo refuses a laser, and says why");
    {
        /* 1.3: Grbl laser G-code, the same answers through the table. */
        static const char *drv =
            "family = laser\n"
            "dialect = grbl\n"
            "name = Test diode\n"
            "send = file\n"
            "features = laser-power\n"
            "area = 400 400 50\n"
            "speed = 200 200 20 0\n"
            "accel = 2000 2000 200 0\n"
            "laser = 100\n"
            "start = G21\n"
            "end = M5\n";
        om_machine laser;
        gc_laser La, Lb;
        gc_laser_more xa, xb;
        gc_stats sa2, sb2;
        om_job ja2, jb2;
        gt_path lp;
        int howa, howb;
        om_clear(&laser);
        expect(om_parse(&laser, drv, err, (int)sizeof err) == 0, "a test Grbl laser");
        gc_laser_defaults(&La);
        memset(&Lb, 0xff, sizeof Lb);
        GC_LaserDefaults(&Lb);
        expect(Lb.size == La.size && Lb.constant == La.constant && Lb.s100 == La.s100 &&
                   Lb.speed == La.speed && Lb.travel == La.travel,
               "GC_LaserDefaults");
        La.s100 = Lb.s100 = 1000;
        La.speed = Lb.speed = 20;
        gt_init(&lp);
        gt_add(&lp, GT_TRAVEL, 0, 0, 0, 0);
        gt_add(&lp, GT_CUT, 10, 0, 0, 50);
        gt_add(&lp, GT_TRAVEL, 10, 10, 0, 0);
        gt_add(&lp, GT_CUT, 20, 10, 0, 100);
        ka.n = kb.n = 0;
        expect(gc_laser_write_to(into, &ka, &lp, &laser, &La, &sa2, err, (int)sizeof err) == 0 &&
                   GC_LaserWriteTo(into, &kb, &lp, &laser, &Lb, &sb2, errb, (int)sizeof errb) == 0 &&
                   ka.n == kb.n && !strcmp(ka.buf, kb.buf) && strstr(kb.buf, "M4") && strstr(kb.buf, "S500") &&
                   !memcmp(&sa2, &sb2, sizeof sa2),
               "GC_LaserWriteTo matches the C function");
        howa = gc_laser_estimate_how(&lp, &laser, &La, &sa2);
        howb = GC_LaserEstimateHow(&lp, &laser, &Lb, &sb2);
        expect(howa == howb && !memcmp(&sa2, &sb2, sizeof sa2) && howb == GC_WITH_ACCEL, "GC_LaserEstimateHow");
        gc_laser_job(&lp, &laser, &La, &ja2);
        GC_LaserJob(&lp, &laser, &Lb, &jb2);
        expect(!memcmp(&ja2, &jb2, sizeof ja2) && jb2.laser == 100.f && jb2.needs == OM_LASER_POWER, "GC_LaserJob");
        gc_laser_more_defaults(&xa, &La);
        memset(&xb, 0xff, sizeof xb);
        GC_LaserMoreDefaults(&xb, &Lb);
        expect(xb.size == xa.size && xb.l == &Lb && xb.speed == xa.speed, "GC_LaserMoreDefaults");
        expect(GC_LaserEstimateMore(&lp, &laser, &xb, &sb2) == gc_laser_estimate_more(&lp, &laser, &xa, &sa2) &&
                   !memcmp(&sa2, &sb2, sizeof sa2),
               "GC_LaserEstimateMore");
        ka.n = kb.n = 0;
        expect(gc_laser_write_more(into, &ka, &lp, &laser, &xa, &sa2, err, (int)sizeof err) == 0 &&
                   GC_LaserWriteMore(into, &kb, &lp, &laser, &xb, &sb2, errb, (int)sizeof errb) == 0 &&
                   ka.n == kb.n && !strcmp(ka.buf, kb.buf),
               "GC_LaserWriteMore");
        m.family = OM_PRINTER;
        expect(GC_LaserWriteTo(into, &kb, &lp, &m, &Lb, NULL, errb, (int)sizeof errb) == -1 &&
                   strstr(errb, "laser"),
               "GC_LaserWriteTo refuses a printer");
        gt_free(&lp);
    }
    printf("through the library: %ld bytes of G-code, %d moves\n", ka.n, sb.moves);
    expect(calls == 18, "every GC_ call went through the table");

    gt_free(&p);
    if (fails) {
        printf("%d failed\n", fails);
        return 1;
    }
    printf("gcode library ok\n");
    return 0;
}
