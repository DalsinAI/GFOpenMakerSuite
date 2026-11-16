/* Written by core/library/gen-library.py from openmakemachine_lib.sfd: do not edit.
 * The C API through openmakemachine.library: a program links this instead of
 * the sources, and opens OpenMakeMachineBase itself. */
#include <proto/openmakemachine.h>

int om_parse(om_machine *m, const char *text, char *err, int errlen)
{
    return OM_Parse(m, text, err, errlen);
}

int om_load(om_machine *m, const char *path, char *err, int errlen)
{
    return OM_Load(m, path, err, errlen);
}

void om_clear(om_machine *m)
{
    OM_Clear(m);
}

int om_has(const om_machine *m, unsigned feature)
{
    return OM_Has(m, feature);
}

int om_check(const om_machine *m, const om_job *job, char *why, int whylen)
{
    return OM_Check(m, job, why, whylen);
}

float om_clamp_speed(const om_machine *m, int axis, float v, int *held)
{
    return OM_ClampSpeed(m, axis, v, held);
}

int om_code(const char *tmpl, float hotend, float bed, char *out, int outlen)
{
    return OM_Code(tmpl, hotend, bed, out, outlen);
}
