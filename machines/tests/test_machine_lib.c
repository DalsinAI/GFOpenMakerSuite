/* openmakemachine.library's calls, run on the host the way a program makes
 * them: each inline call goes through the jump table's slot at its bias
 * (om_host_vector here, a jsr on the Amiga) to the library's side, which
 * takes the registers back to the C function. Every answer must be the
 * C function's own, floats included. MIT, Copyright (c) 2026 AmiCradle Team. */
#include <proto/openmakemachine.h>

#include "../library/openmakemachine_vectors.h"

#include <stdio.h>
#include <string.h>

static int fails;

static void expect(int ok, const char *what)
{
    if (!ok) {
        printf("fail: %s\n", what);
        fails++;
    }
}

/* The jump table as the library's lib_vectors has it: open, close, expunge
 * and the reserved slot, then the calls from bias 30. */
static const APTR table[] = {NULL, NULL, NULL, NULL, OPENMAKEMACHINE_VECTORS};
static struct Library *fake = (struct Library *)&fake;
struct Library *OpenMakeMachineBase;
static int calls;

APTR om_host_vector(struct Library *base, int bias)
{
    expect(base == fake, "the call goes through the library's base");
    expect(bias % 6 == 0 && bias >= 30 && bias <= OPENMAKEMACHINE_LAST_BIAS, "a bias in the table");
    calls++;
    return table[bias / 6 - 1];
}

int main(void)
{
    om_machine a, b;
    om_job job;
    char ea[160], eb[160];
    int held = 0, ra, rb;
    float v;
    OpenMakeMachineBase = fake;
    expect(sizeof table / sizeof table[0] == OPENMAKEMACHINE_LAST_BIAS / 6, "every call has its slot");

    om_clear(&a);
    OM_Clear(&b);
    expect(memcmp(&a, &b, sizeof a) == 0, "OM_Clear");
    ra = om_load(&a, "machines/printer/marlin/driver.om", ea, (int)sizeof ea);
    rb = OM_Load(&b, "machines/printer/marlin/driver.om", eb, (int)sizeof eb);
    expect(ra == 0 && rb == 0, "OM_Load the driver");
    ra = om_load(&a, "machines/printer/marlin/profiles/generic-220.om", ea, (int)sizeof ea);
    rb = OM_Load(&b, "machines/printer/marlin/profiles/generic-220.om", eb, (int)sizeof eb);
    expect(ra == 0 && rb == 0 && memcmp(&a, &b, sizeof a) == 0, "OM_Load the profile over it");
    ra = om_parse(&a, "speed = fast", ea, (int)sizeof ea);
    rb = OM_Parse(&b, "speed = fast", eb, (int)sizeof eb);
    expect(ra == rb && ra == -1 && strcmp(ea, eb) == 0, "OM_Parse says the same as om_parse");
    printf("through the library: %s\n", eb);
    expect(OM_Has(&b, OM_HEATED_BED) == om_has(&a, OM_HEATED_BED) && OM_Has(&b, OM_ROTARY) == 0, "OM_Has");

    memset(&job, 0, sizeof job);
    job.max[0] = 250.f;
    job.speed[0] = 400.f;
    ra = om_check(&a, &job, ea, (int)sizeof ea);
    rb = OM_Check(&b, &job, eb, (int)sizeof eb);
    expect(ra == rb && (rb & OM_TOO_BIG) && (rb & OM_TOO_FAST) && strcmp(ea, eb) == 0, "OM_Check");

    v = OM_ClampSpeed(&b, OM_X, 512.5f, &held);
    expect(v == 300.f && held, "OM_ClampSpeed holds 512.5 to 300, a float each way");
    held = 0;
    v = OM_ClampSpeed(&b, OM_Z, 2.75f, &held);
    expect(v == 2.75f && !held, "and leaves 2.75 alone");
    expect(OM_ClampSpeed(&b, OM_E, -0.0f, &held) == om_clamp_speed(&a, OM_E, -0.0f, &held), "and -0");

    ra = om_code("M104 S{hotend}\nM140 S{bed}", 215.f, 60.5f, ea, (int)sizeof ea);
    rb = OM_Code("M104 S{hotend}\nM140 S{bed}", 215.f, 60.5f, eb, (int)sizeof eb);
    expect(ra == 0 && rb == 0 && strcmp(ea, eb) == 0, "OM_Code takes two floats in d0 and d1");
    printf("%s\n", eb);
    expect(calls == 11, "every OM_ call went through the table");
    if (fails) {
        printf("%d failed\n", fails);
        return 1;
    }
    printf("machine library ok\n");
    return 0;
}
