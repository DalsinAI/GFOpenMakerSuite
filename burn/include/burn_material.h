/* A materials-library profile: power, speed, passes and mode for a
 * sheet, as a plain-text .om file like the machines/ ones.
 * MIT, Copyright (c) 2026 AmiCradle Team. */
#ifndef BURN_MATERIAL_H
#define BURN_MATERIAL_H

#include "burn_job.h"

typedef struct {
    int size;
    char name[64];
    char path[256];
    float power, speed, interval;
    int passes, mode, constant;
    unsigned set;
} burn_material;

#define BURN_SET_CONSTANT 64u

/* 0, or -1 with why in err. Unset fields stay 0 and are not in set. */
int burn_material_load(burn_material *m, const char *path, char *err, int errlen);

/* Copy the fields that were set onto o (job-wide defaults). */
void burn_material_apply(const burn_material *m, burn_opts *o);

typedef struct {
    burn_material *at;
    int n, cap, skipped;
    char why[160];
} burn_materials;

void burn_materials_init(burn_materials *l);
void burn_materials_free(burn_materials *l);

/* Every readable .om in dir, sorted by name. 0, or -1 out of memory. */
int burn_materials_scan(burn_materials *l, const char *dir);

#endif
