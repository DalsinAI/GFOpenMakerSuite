/* Written by core/library/gen-library.py from openmakemachine_lib.sfd: do not edit.
 * The library's side of each call: its registers to the C function. */
#include "omlib.h"
#include <om_machine.h>

static int LIB_OM_Parse(REG(a0, om_machine *m), REG(a1, const char *text), REG(a2, char *err), REG(d0, int errlen), REG(a6, struct Library *base))
{
    (void)base;
    return om_parse(m, text, err, errlen);
}

static int LIB_OM_Load(REG(a0, om_machine *m), REG(a1, const char *path), REG(a2, char *err), REG(d0, int errlen), REG(a6, struct Library *base))
{
    (void)base;
    return om_load(m, path, err, errlen);
}

static void LIB_OM_Clear(REG(a0, om_machine *m), REG(a6, struct Library *base))
{
    (void)base;
    om_clear(m);
}

static int LIB_OM_Has(REG(a0, const om_machine *m), REG(d0, unsigned feature), REG(a6, struct Library *base))
{
    (void)base;
    return om_has(m, feature);
}

static int LIB_OM_Check(REG(a0, const om_machine *m), REG(a1, const om_job *job), REG(a2, char *why), REG(d0, int whylen), REG(a6, struct Library *base))
{
    (void)base;
    return om_check(m, job, why, whylen);
}

static ULONG LIB_OM_ClampSpeed(REG(a0, const om_machine *m), REG(d0, int axis), REG(d1, ULONG v), REG(a1, int *held), REG(a6, struct Library *base))
{
    (void)base;
    return om_lib_bits(om_clamp_speed(m, axis, om_lib_float(v), held));
}

static int LIB_OM_Code(REG(a0, const char *tmpl), REG(d0, ULONG hotend), REG(d1, ULONG bed), REG(a1, char *out), REG(d2, int outlen), REG(a6, struct Library *base))
{
    (void)base;
    return om_code(tmpl, om_lib_float(hotend), om_lib_float(bed), out, outlen);
}

#define OPENMAKEMACHINE_LAST_BIAS 66
#define OPENMAKEMACHINE_VECTORS \
    (APTR)LIB_OM_Parse, \
    (APTR)LIB_OM_Load, \
    (APTR)LIB_OM_Clear, \
    (APTR)LIB_OM_Has, \
    (APTR)LIB_OM_Check, \
    (APTR)LIB_OM_ClampSpeed, \
    (APTR)LIB_OM_Code
