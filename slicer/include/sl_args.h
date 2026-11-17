/* The print settings from a slicing command's arguments, the same for
 * gfopenslicer and openmake-slice:
 *   SETTINGS=file   a settings profile (settings/standard is the shipped
 *                   one; without it, the same built-in defaults)
 *   LAYER= INFILL= LINE= WALLS= SKINS= BRIM= SKIRT= FILAMENT= HOTEND= BED=
 *                   the older names, over the profile
 *   key=value       one setting by its profile key (layer-height=0.16,
 *                   pattern=grid, around=brim ...), over all of those
 * Words in capitals that are not settings (DRIVER, TASKS ...) are left
 * for the command; a lower-case word that is not a setting is an error.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef SL_ARGS_H
#define SL_ARGS_H

#include "gs_settings.h"

/* s from argv[first] on. 0, or -1 with why in err. */
int sl_args(gs_settings *s, int argc, char **argv, int first, char *err, int errlen);

#endif
